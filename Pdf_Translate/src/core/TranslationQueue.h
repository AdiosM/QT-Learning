#pragma once

#include "Translator.h"

#include <QObject>
#include <QVector>

class QTimer;

// 串行批量翻译队列：贪心打包 -> 逐批请求 -> 失败重试（指数退避）
class TranslationQueue : public QObject
{
    Q_OBJECT

public:
    struct Item {
        int page;
        int paraIndex;
        QString source;
    };

    explicit TranslationQueue(Translator *translator, QObject *parent = nullptr);

    void setTranslator(Translator *translator);
    void start(const QList<Item> &items, const QString &from, const QString &to);
    void stop();
    bool isRunning() const;

signals:
    void itemTranslated(int page, int paraIndex, const QString &text);
    void itemFailed(int page, int paraIndex, const QString &error);
    void progress(int done, int total);
    void allFinished(int ok, int failed);

private slots:
    void onBatchFinished(quint64 jobId, const Translator::Texts &translations);
    void onBatchFailed(quint64 jobId, const QString &error, bool retryable);

private:
    void sendNextBatch();
    void finishRunIfDone();

    Translator *m_translator = nullptr;
    QTimer *m_retryTimer = nullptr;
    QVector<QVector<Item>> m_batches;   // 打包后的批次
    int m_batchIndex = 0;
    int m_retryCount = 0;               // 当前批次已重试次数
    int m_done = 0;
    int m_failed = 0;
    int m_totalItems = 0;
    quint64 m_jobId = 0;
    bool m_running = false;
    QString m_from;
    QString m_to;
};
