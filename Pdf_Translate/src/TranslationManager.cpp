#include "TranslationManager.h"

#include "PdfViewWidget.h"
#include "TranslationView.h"
#include "core/ParagraphSplitter.h"
#include "core/TranslationCache.h"
#include "core/TranslationQueue.h"
#include "core/Translator.h"
#include "settings/AppSettings.h"

#include <QFileInfo>
#include <QPdfDocument>
#include <QTimer>

TranslationManager::TranslationManager(PdfViewWidget *pdf, TranslationView *view,
                                       QObject *parent)
    : QObject(parent)
    , m_pdf(pdf)
    , m_view(view)
{
    // 由设置决定使用模拟翻译器还是 Azure
    m_translator = createTranslator(AppSettings::instance(), this);
    m_queue = new TranslationQueue(m_translator, this);

    connect(m_queue, &TranslationQueue::itemTranslated,
            this, &TranslationManager::onItemTranslated);
    connect(m_queue, &TranslationQueue::itemFailed,
            this, &TranslationManager::onItemFailed);
    connect(m_queue, &TranslationQueue::progress, this,
            [this](int done, int total) {
                // 计入缓存命中数，进度条从命中的部分开始
                emit progressChanged(done + m_cacheHits, total + m_cacheHits);
            });
    connect(m_queue, &TranslationQueue::allFinished, this,
            [this](int ok, int failed) { emit runFinished(ok + m_cacheHits, failed); });
}

TranslationManager::~TranslationManager()
{
    delete m_cache;
}

void TranslationManager::setDocument(const QString &path)
{
    stop();
    m_view->reset();
    m_view->setDocumentTitle(QFileInfo(path).fileName());
    m_originals.clear();
    m_translatedPages.clear();
    m_cacheHits = 0;

    // 初始化该文档的缓存
    delete m_cache;
    m_cache = nullptr;
    const QFileInfo info(path);
    m_docKey = TranslationCache::docKey(info.absoluteFilePath(), info.size(),
                                        info.lastModified());
    auto *cache = new TranslationCache(TranslationCache::defaultDbPath());
    QString error;
    if (cache->init(&error)) {
        m_cache = cache;
    } else {
        delete cache;
        emit statusMessage(QStringLiteral("缓存不可用：%1").arg(error));
    }
}

void TranslationManager::translateCurrentPage()
{
    if (isRunning())
        return;
    const int page = m_pdf->currentPage();
    if (m_translatedPages.contains(page)) {
        emit statusMessage(QStringLiteral("第 %1 页已翻译").arg(page + 1));
        return;
    }
    startJob({page});
}

void TranslationManager::translateWholeDocument()
{
    if (isRunning())
        return;

    m_wholePages.clear();
    const int count = m_pdf->pageCount();
    for (int i = 0; i < count; ++i)
        if (!m_translatedPages.contains(i))
            m_wholePages.append(i);

    if (m_wholePages.isEmpty()) {
        emit statusMessage(QStringLiteral("全部页面均已翻译"));
        return;
    }

    m_wholeIndex = 0;
    m_wholePending.clear();
    m_originals.clear();
    m_cacheHits = 0;
    m_langPair = AppSettings::instance().sourceLang()
        + QStringLiteral("->") + AppSettings::instance().targetLang();

    emit statusMessage(QStringLiteral("正在提取全文（%1 页）…").arg(m_wholePages.size()));
    QTimer::singleShot(0, this, &TranslationManager::collectWholeDocument);
}

void TranslationManager::collectWholeDocument()
{
    // 每次只处理一页，通过事件循环链式推进，保持界面响应
    if (m_wholeIndex < m_wholePages.size()) {
        const int page = m_wholePages.at(m_wholeIndex++);
        m_view->beginPage(page);
        m_translatedPages.insert(page);
        QList<TranslationQueue::Item> pageItems;
        processPage(page, &pageItems);
        m_wholePending.append(pageItems);
        QTimer::singleShot(0, this, &TranslationManager::collectWholeDocument);
        return;
    }

    if (m_wholePending.isEmpty()) {
        emit statusMessage(m_cacheHits > 0
                               ? QStringLiteral("全部命中缓存（%1 段）").arg(m_cacheHits)
                               : QStringLiteral("没有可翻译的内容"));
        emit runFinished(m_cacheHits, 0);
        return;
    }

    const AppSettings &settings = AppSettings::instance();
    emit statusMessage(QStringLiteral("开始翻译：缓存命中 %1 段，待翻译 %2 段")
                           .arg(m_cacheHits).arg(m_wholePending.size()));
    m_queue->start(m_wholePending, settings.sourceLang(), settings.targetLang());
}

void TranslationManager::startJob(const QList<int> &pages)
{
    m_originals.clear();
    m_cacheHits = 0;
    const AppSettings &settings = AppSettings::instance();
    m_langPair = settings.sourceLang() + QStringLiteral("->") + settings.targetLang();

    QList<TranslationQueue::Item> pendingItems;
    for (int page : pages) {
        m_view->beginPage(page);
        m_translatedPages.insert(page);
        processPage(page, &pendingItems);
    }

    if (pendingItems.isEmpty()) {
        emit statusMessage(m_cacheHits > 0
                               ? QStringLiteral("全部命中缓存（%1 段）").arg(m_cacheHits)
                               : QStringLiteral("没有可翻译的内容"));
        emit runFinished(m_cacheHits, 0);
        return;
    }

    emit statusMessage(QStringLiteral("开始翻译：缓存命中 %1 段，待翻译 %2 段")
                           .arg(m_cacheHits).arg(pendingItems.size()));
    m_queue->start(pendingItems, settings.sourceLang(), settings.targetLang());
}

void TranslationManager::processPage(int page, QList<TranslationQueue::Item> *pending)
{
    QString text;
    if (m_pdf->document()->status() == QPdfDocument::Status::Ready) {
        text = ParagraphSplitter::normalizePageText(
            m_pdf->document()->getAllText(page).text());
    }
    if (text.isEmpty()) {
        m_view->appendNotice(
            tr("（第 %1 页无可提取文本，可能为扫描件）").arg(page + 1));
        return;
    }

    const QList<QString> chunks = ParagraphSplitter::splitPage(text);

    // 批量查缓存
    QList<CacheQuery> queries;
    queries.reserve(chunks.size());
    for (int i = 0; i < chunks.size(); ++i)
        queries.append({page, chunks.at(i)});
    const QStringList cached = m_cache
        ? m_cache->lookup(m_docKey, m_langPair, queries)
        : QStringList(chunks.size(), QString());

    for (int i = 0; i < chunks.size(); ++i) {
        const QString &chunk = chunks.at(i);
        m_originals.insert({page, i}, chunk);
        if (!cached.at(i).isEmpty()) {
            m_cacheHits++;
            m_view->showTranslated(page, i, chunk, cached.at(i));
        } else {
            m_view->showPending(page, i, chunk);
            pending->append({page, i, chunk});
        }
    }
}

void TranslationManager::onItemTranslated(int page, int paraIndex, const QString &text)
{
    const QString original = m_originals.value({page, paraIndex});
    m_view->showTranslated(page, paraIndex, original, text);
    if (m_cache)
        m_cache->store(m_docKey, m_langPair, page, {original}, {text});
}

void TranslationManager::onItemFailed(int page, int paraIndex, const QString &error)
{
    m_view->showError(page, paraIndex,
                      m_originals.value({page, paraIndex}), error);
}

void TranslationManager::stop()
{
    if (!m_queue->isRunning())
        return;
    m_queue->stop();
    emit statusMessage(QStringLiteral("已停止翻译"));
    emit runFinished(0, 0);
}

bool TranslationManager::isRunning() const
{
    return m_queue->isRunning();
}

void TranslationManager::reloadSettings()
{
    setTranslator(createTranslator(AppSettings::instance(), this));
}

void TranslationManager::setTranslator(Translator *translator)
{
    stop();
    Translator *old = m_translator;
    m_translator = translator;
    m_translator->setParent(this);
    m_queue->setTranslator(m_translator);
    if (old)
        old->deleteLater();
}
