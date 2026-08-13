#include "MockTranslator.h"

#include <QTimer>

MockTranslator::MockTranslator(int delayMs, int failEveryNth, QObject *parent)
    : Translator(parent)
    , m_delayMs(delayMs)
    , m_failEveryNth(failEveryNth)
{
}

void MockTranslator::translate(const Texts &texts, const QString &,
                               const QString &, quint64 jobId)
{
    const int batchIndex = ++m_batchCount;

    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this,
            [this, timer, jobId, batchIndex, texts] {
        m_pending.removeAll(timer);
        timer->deleteLater();

        if (m_failEveryNth > 0 && batchIndex % m_failEveryNth == 0) {
            emit batchFailed(jobId,
                             QStringLiteral("模拟网络错误（第 %1 批）").arg(batchIndex));
            return;
        }

        Texts out;
        out.reserve(texts.size());
        for (const QString &text : texts)
            out.append(QStringLiteral("【模拟译文】") + text);
        emit batchFinished(jobId, out);
    });
    m_pending.append(timer);
    timer->start(m_delayMs);
}

void MockTranslator::cancelAll()
{
    const QList<QTimer *> pending = m_pending;
    m_pending.clear();
    for (QTimer *timer : pending) {
        timer->stop();
        timer->deleteLater();
    }
}
