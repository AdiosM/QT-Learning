#include "TranslationView.h"

#include <QScrollBar>
#include <QTextBrowser>
#include <QTextCursor>
#include <QVBoxLayout>

TranslationView::TranslationView(QWidget *parent)
    : QWidget(parent)
{
    m_browser = new QTextBrowser(this);
    m_browser->setOpenLinks(false);
    m_browser->setOpenExternalLinks(false);
    QFont baseFont(QStringLiteral("Microsoft YaHei"));
    baseFont.setPointSizeF(10.5);
    m_browser->document()->setDefaultFont(baseFont);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_browser);

    // 用户滚动右栏时，报告视口顶部所在页（供滚动同步）
    connect(m_browser->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this](int) {
                const int page = pageAtViewportTop();
                if (page >= 0 && page != m_lastEmittedPage) {
                    m_lastEmittedPage = page;
                    emit pageRequested(page);
                }
            });

    reset();
}

void TranslationView::reset()
{
    m_browser->clear();
    m_pages.clear();
    m_pageBlockPositions.clear();
    m_lastEmittedPage = -1;
}

void TranslationView::setDocumentTitle(const QString &title)
{
    appendHtml(QStringLiteral(
        "<h2 style=\"color:#2c3e50; border-bottom:1px solid #ccc; "
        "padding-bottom:4px;\">%1</h2>").arg(title.toHtmlEscaped()));
}

void TranslationView::beginPage(int page)
{
    ensurePage(page);
    appendHtml(QStringLiteral(
        "<h3 style=\"color:#2c3e50; background:#eef2f7; margin-top:10px;\">"
        "—— 第 %1 页 ——</h3>").arg(page + 1));

    QTextCursor c = endCursor();
    c.movePosition(QTextCursor::StartOfBlock);
    if (m_pageBlockPositions.size() <= page)
        m_pageBlockPositions.resize(page + 1);
    m_pageBlockPositions[page] = c.position();
}

void TranslationView::showPending(int page, int paraIndex, const QString &original)
{
    ensurePage(page);
    ensureEntry(page, paraIndex);
    PageData &pd = m_pages[page];
    pd.entries[paraIndex].original = original;
    replaceBlock(pd.entries[paraIndex].block,
                 entryHtml(original,
                           QStringLiteral("<span style=\"color:#999;\">翻译中…</span>")));
}

void TranslationView::showTranslated(int page, int paraIndex,
                                     const QString &original,
                                     const QString &translation)
{
    ensurePage(page);
    ensureEntry(page, paraIndex);
    PageData &pd = m_pages[page];
    pd.entries[paraIndex].original = original;
    replaceBlock(pd.entries[paraIndex].block,
                 entryHtml(original, escapedMultiline(translation)));
}

void TranslationView::showError(int page, int paraIndex,
                                const QString &original, const QString &error)
{
    ensurePage(page);
    ensureEntry(page, paraIndex);
    PageData &pd = m_pages[page];
    pd.entries[paraIndex].original = original;
    const QString dst = QStringLiteral(
        "<span style=\"color:#c0392b;\">[翻译失败] %1</span>").arg(error.toHtmlEscaped());
    replaceBlock(pd.entries[paraIndex].block, entryHtml(original, dst));
}

void TranslationView::appendNotice(const QString &text)
{
    appendHtml(QStringLiteral(
        "<div style=\"color:#b58900; margin:6px 0;\">%1</div>").arg(text.toHtmlEscaped()));
}

void TranslationView::jumpToPage(int page)
{
    if (page < 0 || page >= m_pageBlockPositions.size())
        return;
    QTextCursor c(m_browser->document());
    c.setPosition(m_pageBlockPositions.at(page));
    m_browser->setTextCursor(c);
}

int TranslationView::pageAtViewportTop() const
{
    const QTextCursor c = m_browser->cursorForPosition(QPoint(4, 4));
    const int pos = c.position();
    int page = -1;
    for (int i = 0; i < m_pageBlockPositions.size(); ++i) {
        if (m_pageBlockPositions.at(i) <= pos)
            page = i;
        else
            break;
    }
    return page;
}

void TranslationView::ensurePage(int page)
{
    while (m_pages.size() <= page)
        m_pages.append(PageData());
}

void TranslationView::ensureEntry(int page, int paraIndex)
{
    PageData &pd = m_pages[page];
    while (pd.entries.size() <= paraIndex) {
        QTextCursor c = endCursor();
        c.insertHtml(QStringLiteral("<div style=\"color:#ccc;\">…</div>"));
        pd.entries.append({c.block(), QString()});
    }
}

void TranslationView::replaceBlock(const QTextBlock &block, const QString &html)
{
    QTextCursor c(block);
    c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    c.insertHtml(html);
}

void TranslationView::appendHtml(const QString &html)
{
    endCursor().insertHtml(html);
}

QTextCursor TranslationView::endCursor() const
{
    QTextCursor c(m_browser->document());
    c.movePosition(QTextCursor::End);
    return c;
}

QString TranslationView::escapedMultiline(const QString &text) const
{
    return text.toHtmlEscaped().replace(u'\n', QStringLiteral("<br>"));
}

// 原文(灰色小字) + 译文(黑色正文)；dstHtml 为已转义的 HTML 片段
QString TranslationView::entryHtml(const QString &original, const QString &dstHtml) const
{
    return QStringLiteral(
        "<div style=\"color:#888; font-size:9pt; margin-bottom:2px;\">%1</div>"
        "<div style=\"color:#111; margin-bottom:12px; line-height:1.5;\">%2</div>")
        .arg(escapedMultiline(original), dstHtml);
}
