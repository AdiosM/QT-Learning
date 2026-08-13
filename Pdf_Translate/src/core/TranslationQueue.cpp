#include "TranslationQueue.h"

#include "Translator.h"

#include <QTimer>

namespace {
constexpr int kMaxItemsPerBatch = 30;      // Azure 上限 100，留余量
constexpr int kMaxCharsPerBatch = 20000;   // Azure 上限 50000，留余量
constexpr int kMaxRetries = 3;
constexpr int kRetryBaseDelayMs = 1000;    // 退避 1s / 2s / 4s
} // namespace

TranslationQueue::TranslationQueue(Translator *translator, QObject *parent)
    : QObject(parent)
{
    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout,
            this, &TranslationQueue::sendNextBatch);
    setTranslator(translator);
}

void TranslationQueue::setTranslator(Translator *translator)
{
    if (m_translator) {
        disconnect(m_translator, nullptr, this, nullptr);
        m_translator->cancelAll();
    }
    m_translator = translator;
    if (m_translator) {
        connect(m_translator, &Translator::batchFinished,
                this, &TranslationQueue::onBatchFinished);
        connect(m_translator, &Translator::batchFailed,
                this, &TranslationQueue::onBatchFailed);
    }
}

void TranslationQueue::start(const QList<Item> &items,
                             const QString &from, const QString &to)
{
    stop();

    m_from = from;
    m_to = to;
    m_done = 0;
    m_failed = 0;
    m_batchIndex = 0;
    m_retryCount = 0;
    m_jobId++;

    // 贪心打包：每批 ≤30 段且 ≤20000 字符
    QVector<Item> current;
    int currentChars = 0;
    for (const Item &item : items) {
        if (!current.isEmpty()
            && (current.size() >= kMaxItemsPerBatch
                || currentChars + item.source.size() > kMaxCharsPerBatch)) {
            m_batches.append(current);
            current.clear();
            currentChars = 0;
        }
        current.append(item);
        currentChars += item.source.size();
    }
    if (!current.isEmpty())
        m_batches.append(current);

    m_totalItems = items.size();
    if (m_batches.isEmpty()) {
        emit allFinished(0, 0);
        return;
    }

    m_running = true;
    emit progress(0, m_totalItems);
    sendNextBatch();
}

void TranslationQueue::stop()
{
    m_running = false;
    m_retryTimer->stop();
    if (m_translator)
        m_translator->cancelAll();
    m_batches.clear();
    m_batchIndex = 0;
    m_retryCount = 0;
}

bool TranslationQueue::isRunning() const
{
    return m_running;
}

void TranslationQueue::sendNextBatch()
{
    if (!m_running || m_batchIndex >= m_batches.size())
        return;

    const QVector<Item> &batch = m_batches.at(m_batchIndex);
    Translator::Texts texts;
    texts.reserve(batch.size());
    for (const Item &item : batch)
        texts.append(item.source);
    m_translator->translate(texts, m_from, m_to, m_jobId);
}

void TranslationQueue::onBatchFinished(quint64 jobId,
                                       const Translator::Texts &translations)
{
    if (!m_running || jobId != m_jobId)
        return;

    const QVector<Item> &batch = m_batches.at(m_batchIndex);
    if (translations.size() != batch.size()) {
        // 响应数量与请求不符：整批判失败（避免段落错位）
        onBatchFailed(jobId, QStringLiteral("翻译服务返回数量与请求不符"), true);
        return;
    }

    for (int i = 0; i < batch.size(); ++i)
        emit itemTranslated(batch.at(i).page, batch.at(i).paraIndex,
                            translations.at(i));

    m_done += batch.size();
    m_batchIndex++;
    m_retryCount = 0;
    emit progress(m_done, m_totalItems);

    if (m_batchIndex >= m_batches.size()) {
        m_running = false;
        emit allFinished(m_done, m_failed);
        return;
    }
    sendNextBatch();
}

void TranslationQueue::onBatchFailed(quint64 jobId, const QString &error,
                                     bool retryable)
{
    if (!m_running || jobId != m_jobId)
        return;

    if (retryable && m_retryCount < kMaxRetries) {
        m_retryCount++;
        const int delay = kRetryBaseDelayMs << (m_retryCount - 1);   // 1s/2s/4s
        m_retryTimer->start(delay);
        return;
    }

    // 重试耗尽：该批所有段落判失败
    const QVector<Item> &batch = m_batches.at(m_batchIndex);
    for (const Item &item : batch)
        emit itemFailed(item.page, item.paraIndex, error);

    m_failed += batch.size();
    m_batchIndex++;
    m_retryCount = 0;
    emit progress(m_done, m_totalItems);

    if (m_batchIndex >= m_batches.size()) {
        m_running = false;
        emit allFinished(m_done, m_failed);
        return;
    }
    sendNextBatch();
}
