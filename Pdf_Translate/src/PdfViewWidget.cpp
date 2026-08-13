#include "PdfViewWidget.h"

#include <QInputDialog>
#include <QLabel>
#include <QPixmap>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

namespace {
constexpr qreal kMinZoom = 0.5;
constexpr qreal kMaxZoom = 4.0;
constexpr qreal kZoomStep = 1.25;   // 每次缩放 25%
constexpr int kPageGap = 8;         // 页面间距（像素）
constexpr int kMargin = 10;         // 页面距容器边缘
constexpr int kPrefetchPages = 2;   // 可见范围前后各预取页数
constexpr int kMaxCacheImages = 40;
constexpr qint64 kMaxCacheBytes = 120LL * 1024 * 1024;
} // namespace

PdfViewWidget::PdfViewWidget(QWidget *parent)
    : QWidget(parent)
    , m_renderer(this)
{
    m_container = new QWidget;
    m_container->setStyleSheet(QStringLiteral("background: #4a4a4a;"));

    m_scroll = new QScrollArea(this);
    m_scroll->setWidget(m_container);
    m_scroll->setWidgetResizable(false);
    m_scroll->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_scroll->setBackgroundRole(QPalette::Dark);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_scroll);

    // 滚动停止约 100ms 后再补齐可见页，避免滚动过程中频繁创建/销毁
    m_settleTimer = new QTimer(this);
    m_settleTimer->setSingleShot(true);
    m_settleTimer->setInterval(100);
    connect(m_settleTimer, &QTimer::timeout,
            this, &PdfViewWidget::updateVisiblePages);

    connect(m_scroll->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &PdfViewWidget::onScrollChanged);
    connect(&m_doc, &QPdfDocument::statusChanged,
            this, &PdfViewWidget::onDocumentStatusChanged);
    connect(&m_renderer, &QPdfPageRenderer::pageRendered,
            this, &PdfViewWidget::onPageRendered);

    m_renderer.setDocument(&m_doc);
}

bool PdfViewWidget::openDocument(const QString &path, QString *error)
{
    m_doc.close();
    m_pendingPath = path;
    m_passwordPrompted = false;
    m_doc.load(path);   // 异步：状态变化在 onDocumentStatusChanged 处理

    if (m_doc.status() == QPdfDocument::Status::Error) {
        if (error)
            *error = m_doc.error() == QPdfDocument::Error::FileNotFound
                ? tr("文件不存在或无法读取")
                : tr("无效的 PDF 文件");
        return false;
    }
    return true;
}

void PdfViewWidget::onDocumentStatusChanged(QPdfDocument::Status status)
{
    if (status == QPdfDocument::Status::Ready) {
        m_generation++;
        m_zoom = 1.0;
        m_currentPage = -1;
        m_requested.clear();
        m_pending.clear();
        relayout();
        onScrollChanged();   // 触发 pageChanged(0)
        emit documentReady(m_doc.pageCount());
    } else if (status == QPdfDocument::Status::Error) {
        // 密码保护：弹一次输入框，重试加载
        if (m_doc.error() == QPdfDocument::Error::IncorrectPassword
            && !m_passwordPrompted) {
            m_passwordPrompted = true;
            bool ok = false;
            const QString pwd = QInputDialog::getText(
                this, tr("需要密码"),
                tr("该 PDF 受密码保护，请输入密码："),
                QLineEdit::Password, QString(), &ok);
            if (ok) {
                m_doc.setPassword(pwd);
                m_doc.load(m_pendingPath);
                return;
            }
        }

        QString message;
        switch (m_doc.error()) {
        case QPdfDocument::Error::FileNotFound:
            message = tr("文件不存在或无法读取");
            break;
        case QPdfDocument::Error::IncorrectPassword:
            message = tr("密码错误");
            break;
        case QPdfDocument::Error::UnsupportedSecurityScheme:
            message = tr("不支持的安全加密方式");
            break;
        case QPdfDocument::Error::InvalidFileFormat:
            message = tr("无效的 PDF 文件");
            break;
        default:
            message = tr("打开 PDF 失败");
            break;
        }
        emit documentFailed(message);
    }
}

int PdfViewWidget::pageCount() const
{
    return m_doc.pageCount();
}

int PdfViewWidget::currentPage() const
{
    return qMax(0, m_currentPage);
}

double PdfViewWidget::zoomFactor() const
{
    return m_zoom;
}

void PdfViewWidget::setZoom(double factor)
{
    factor = qBound(kMinZoom, factor, kMaxZoom);
    if (qFuzzyCompare(factor, m_zoom))
        return;

    // 记录缩放前位置，缩放后回到同一页面的大致同一位置
    const int anchorPage = currentPage();
    const int oldTop = m_scroll->verticalScrollBar()->value();
    qreal frac = 0.5;
    if (anchorPage >= 0 && anchorPage < m_pageTops.size()) {
        const int ph = m_pageHeights.at(anchorPage);
        if (ph > 0)
            frac = qBound(0.0, qreal(oldTop - m_pageTops.at(anchorPage)) / ph, 1.0);
    }

    m_zoom = factor;
    m_generation++;          // 旧尺寸的渲染回调一律作废
    m_requested.clear();
    m_pending.clear();
    relayout();

    if (anchorPage >= 0 && anchorPage < m_pageTops.size()) {
        m_scroll->verticalScrollBar()->setValue(
            m_pageTops.at(anchorPage) + qRound(frac * m_pageHeights.at(anchorPage)));
    }
}

void PdfViewWidget::zoomIn()
{
    setZoom(m_zoom * kZoomStep);
}

void PdfViewWidget::zoomOut()
{
    setZoom(m_zoom / kZoomStep);
}

void PdfViewWidget::fitWidth()
{
    if (m_maxPageWidth <= 0)
        return;
    const QScrollBar *vbar = m_scroll->verticalScrollBar();
    const int avail = m_scroll->viewport()->width()
        - (vbar->isVisible() ? vbar->width() : 0)
        - 2 * kMargin - 4;
    if (avail <= 0)
        return;
    setZoom(m_zoom * qreal(avail) / qreal(m_maxPageWidth));
}

void PdfViewWidget::goToPage(int page)
{
    if (m_pageTops.isEmpty())
        return;
    page = qBound(0, page, m_pageTops.size() - 1);
    m_scroll->verticalScrollBar()->setValue(m_pageTops.at(page));
}

QPdfDocument *PdfViewWidget::document()
{
    return &m_doc;
}

QSize PdfViewWidget::displaySizeFor(int page) const
{
    return (m_doc.pagePointSize(page) * m_zoom).toSize();
}

QSize PdfViewWidget::renderSizeFor(int page) const
{
    const QSizeF pt = m_doc.pagePointSize(page);
    const qreal dpr = devicePixelRatioF();
    return QSize(qRound(pt.width() * m_zoom * dpr),
                 qRound(pt.height() * m_zoom * dpr));
}

void PdfViewWidget::relayout()
{
    qDeleteAll(m_pageWidgets);
    m_pageWidgets.clear();
    m_imageCache.clear();
    m_cacheOrder.clear();
    m_cacheBytes = 0;

    m_pageTops.clear();
    m_pageWidths.clear();
    m_pageHeights.clear();
    m_maxPageWidth = 0;

    int y = kMargin;
    const int n = m_doc.pageCount();
    for (int i = 0; i < n; ++i) {
        const QSize display = displaySizeFor(i);
        m_pageTops.append(y);
        m_pageWidths.append(display.width());
        m_pageHeights.append(display.height());
        m_maxPageWidth = qMax(m_maxPageWidth, display.width());
        y += display.height() + kPageGap;
    }
    m_container->setFixedSize(m_maxPageWidth + 2 * kMargin,
                              qMax(1, y - kPageGap + kMargin));
    updateVisiblePages();
}

void PdfViewWidget::onScrollChanged()
{
    const int n = m_doc.pageCount();
    if (n == 0)
        return;

    const int viewTop = m_scroll->verticalScrollBar()->value();
    int page = n - 1;
    for (int i = 0; i < n; ++i) {
        if (m_pageTops.at(i) + m_pageHeights.at(i) > viewTop + 40) {
            page = i;
            break;
        }
    }
    if (page != m_currentPage) {
        m_currentPage = page;
        emit pageChanged(page);
    }
    m_settleTimer->start();
}

void PdfViewWidget::updateVisiblePages()
{
    const int n = m_doc.pageCount();
    if (n == 0)
        return;

    const int viewTop = m_scroll->verticalScrollBar()->value();
    const int viewBottom = viewTop + m_scroll->viewport()->height();

    // 计算可见页范围
    int first = n - 1;
    for (int i = 0; i < n; ++i) {
        if (m_pageTops.at(i) + m_pageHeights.at(i) > viewTop) {
            first = i;
            break;
        }
    }
    int last = 0;
    for (int i = n - 1; i >= 0; --i) {
        if (m_pageTops.at(i) < viewBottom) {
            last = i;
            break;
        }
    }
    first = qMax(0, first - kPrefetchPages);
    last = qMin(n - 1, last + kPrefetchPages);

    // 销毁范围外的页面控件
    const QList<int> livePages = m_pageWidgets.keys();
    for (int page : livePages) {
        if (page < first || page > last)
            delete m_pageWidgets.take(page);
    }

    // 创建/填充范围内的页面控件
    for (int page = first; page <= last; ++page) {
        if (m_pageWidgets.contains(page))
            continue;

        auto *label = new QLabel(m_container);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(QStringLiteral(
            "background: white; border: 1px solid #b0b0b0;"));
        const QSize display = displaySizeFor(page);
        label->setFixedSize(display);
        label->move((m_maxPageWidth - display.width()) / 2 + kMargin,
                    m_pageTops.at(page));
        label->show();
        m_pageWidgets.insert(page, label);

        if (m_imageCache.contains(page)) {
            setLabelImage(label, page);
        } else if (!m_requested.contains(page)) {
            label->setText(tr("渲染中…"));
            requestRender(page);
        }
    }
}

void PdfViewWidget::requestRender(int page)
{
    const QSize target = renderSizeFor(page);
    m_requested.insert(page, target);

    const quint64 requestId = m_renderer.requestPage(page, target);
    m_pending.insert(requestId, {page, m_generation});
}

void PdfViewWidget::onPageRendered(int pageNumber, QSize, const QImage &image,
                                   QPdfDocumentRenderOptions, quint64 requestId)
{
    const auto it = m_pending.constFind(requestId);
    if (it == m_pending.constEnd())
        return;                    // 已作废的请求
    const int generation = it->second;
    m_pending.erase(it);
    m_requested.remove(pageNumber);

    // 过期渲染（期间发生过缩放）或空图：丢弃
    if (generation != m_generation || image.isNull())
        return;

    // 存入 LRU 缓存
    m_imageCache.insert(pageNumber, image);
    m_cacheOrder.removeAll(pageNumber);
    m_cacheOrder.prepend(pageNumber);
    m_cacheBytes += qint64(image.sizeInBytes());
    evictCacheToLimits();

    if (QLabel *label = m_pageWidgets.value(pageNumber, nullptr))
        setLabelImage(label, pageNumber);
}

void PdfViewWidget::setLabelImage(QLabel *label, int page)
{
    const QImage &image = m_imageCache.value(page);
    if (image.isNull())
        return;
    QPixmap pixmap = QPixmap::fromImage(image);
    pixmap.setDevicePixelRatio(devicePixelRatioF());
    label->setPixmap(pixmap);
}

void PdfViewWidget::evictCacheToLimits()
{
    while ((m_cacheOrder.size() > kMaxCacheImages
            || m_cacheBytes > kMaxCacheBytes)
           && !m_cacheOrder.isEmpty()) {
        const int page = m_cacheOrder.takeLast();
        if (!m_imageCache.contains(page))
            continue;
        m_cacheBytes -= qint64(m_imageCache.value(page).sizeInBytes());
        m_imageCache.remove(page);
    }
}
