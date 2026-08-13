#pragma once

#include "Translator.h"

#include <QHash>
#include <QNetworkAccessManager>

class QNetworkReply;

// Azure Translator Text API v3.0 实现
// 请求: POST {endpoint}/translate?api-version=3.0&to=xx
// 头: Ocp-Apim-Subscription-Key (+ Ocp-Apim-Subscription-Region)
// 体: [{"Text": "..."}, ...]（上限 100 段 / 5 万字符，由 TranslationQueue 控制）
class AzureTranslator : public Translator
{
    Q_OBJECT

public:
    AzureTranslator(const QString &endpoint, const QString &apiKey,
                    const QString &region, QObject *parent = nullptr);

    void translate(const Texts &texts, const QString &from,
                   const QString &to, quint64 jobId) override;
    void cancelAll() override;

private slots:
    void onReplyFinished();

private:
    QNetworkAccessManager m_nam;
    QString m_endpoint;      // 基地址，不含尾部斜杠
    QString m_apiKey;
    QString m_region;
    QHash<QNetworkReply *, quint64> m_jobIds;      // reply -> jobId
    QHash<quint64, int> m_expectedCounts;          // jobId -> 请求段数
};
