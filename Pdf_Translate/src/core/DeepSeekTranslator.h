#pragma once

#include "Translator.h"

#include <QHash>
#include <QNetworkAccessManager>

class QNetworkReply;

// DeepSeek chat/completions 接口实现
// 请求: POST {endpoint}/chat/completions
// 鉴权: Authorization: Bearer <api key>
// 批量翻译: 多段文本打包进一条 user 消息（JSON 数组），
//           提示词要求模型按同序返回 JSON 数组
class DeepSeekTranslator : public Translator
{
    Q_OBJECT

public:
    DeepSeekTranslator(const QString &endpoint, const QString &apiKey,
                       const QString &model, QObject *parent = nullptr);

    void translate(const Texts &texts, const QString &from,
                   const QString &to, quint64 jobId) override;
    void cancelAll() override;

private slots:
    void onReplyFinished();

private:
    QNetworkAccessManager m_nam;
    QString m_endpoint;      // 基地址，不含尾部斜杠
    QString m_apiKey;
    QString m_model;
    QHash<QNetworkReply *, quint64> m_jobIds;      // reply -> jobId
    QHash<quint64, int> m_expectedCounts;          // jobId -> 请求段数
};
