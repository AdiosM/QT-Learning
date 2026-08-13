#include "DeepSeekTranslator.h"
#include "DeepSeekJson.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace {

constexpr int kTimeoutMs = 60000;   // LLM 生成较慢，超时放宽

// 系统提示词：要求按同序返回 JSON 数组，且只输出 JSON
const char kSystemPrompt[] =
    "You are a professional academic translation engine. "
    "The user gives you a JSON array of text segments. "
    "Translate each segment into Simplified Chinese and reply with ONLY a JSON "
    "array of the same length in the same order. No markdown fences, no extra text.";

QString networkErrorText(QNetworkReply *reply)
{
    switch (reply->error()) {
    case QNetworkReply::TimeoutError:
        return QStringLiteral("网络请求超时");
    case QNetworkReply::HostNotFoundError:
        return QStringLiteral("无法解析服务器地址");
    case QNetworkReply::ConnectionRefusedError:
        return QStringLiteral("连接被拒绝");
    case QNetworkReply::RemoteHostClosedError:
        return QStringLiteral("服务器关闭了连接");
    case QNetworkReply::SslHandshakeFailedError:
        return QStringLiteral("TLS 握手失败");
    case QNetworkReply::OperationCanceledError:
        return QStringLiteral("请求已取消");
    default:
        return QStringLiteral("网络错误：%1").arg(reply->errorString());
    }
}

QString httpErrorText(int status, const QByteArray &body)
{
    const QString serverMessage = DeepSeekJson::extractErrorMessage(body);
    const QString suffix = serverMessage.isEmpty()
        ? QString()
        : QStringLiteral("（%1）").arg(serverMessage);

    switch (status) {
    case 401:
        return QStringLiteral("DeepSeek 密钥无效 (401)%1").arg(suffix);
    case 402:
        return QStringLiteral("DeepSeek 账户余额不足 (402)%1").arg(suffix);
    case 403:
        return QStringLiteral("DeepSeek 无权限或配额不足 (403)%1").arg(suffix);
    case 429:
        return QStringLiteral("DeepSeek 请求过于频繁，已触发限流 (429)%1").arg(suffix);
    default:
        if (status >= 500)
            return QStringLiteral("DeepSeek 服务暂时不可用 (%1)%2").arg(status).arg(suffix);
        return QStringLiteral("请求失败 (%1)%2").arg(status).arg(suffix);
    }
}

} // namespace

DeepSeekTranslator::DeepSeekTranslator(const QString &endpoint,
                                       const QString &apiKey,
                                       const QString &model, QObject *parent)
    : Translator(parent)
    , m_endpoint(endpoint)
    , m_apiKey(apiKey)
    , m_model(model)
{
}

void DeepSeekTranslator::translate(const Texts &texts, const QString &,
                                   const QString &, quint64 jobId)
{
    QNetworkRequest request(QUrl(m_endpoint + QStringLiteral("/chat/completions")));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Authorization",
                         QStringLiteral("Bearer ").toUtf8() + m_apiKey.toUtf8());
    request.setTransferTimeout(kTimeoutMs);

    QJsonArray inputArray;
    for (const QString &text : texts)
        inputArray.append(text);

    QJsonArray messages{
        QJsonObject{{QStringLiteral("role"), QStringLiteral("system")},
                    {QStringLiteral("content"), QString::fromLatin1(kSystemPrompt)}},
        QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                    {QStringLiteral("content"),
                     QString::fromUtf8(QJsonDocument(inputArray)
                                           .toJson(QJsonDocument::Compact))}},
    };
    QJsonObject body{
        {QStringLiteral("model"), m_model},
        {QStringLiteral("messages"), messages},
        {QStringLiteral("temperature"), 0.3},
        {QStringLiteral("stream"), false},
    };

    QNetworkReply *reply = m_nam.post(request,
                                      QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_jobIds.insert(reply, jobId);
    m_expectedCounts.insert(jobId, texts.size());
    connect(reply, &QNetworkReply::finished, this, &DeepSeekTranslator::onReplyFinished);
}

void DeepSeekTranslator::cancelAll()
{
    const QList<QNetworkReply *> replies = m_jobIds.keys();
    m_jobIds.clear();
    for (QNetworkReply *reply : replies)
        reply->abort();
}

void DeepSeekTranslator::onReplyFinished()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    const quint64 jobId = m_jobIds.take(reply);
    reply->deleteLater();

    if (!m_expectedCounts.contains(jobId))
        return;   // 已被取消的请求

    const int expectedCount = m_expectedCounts.take(jobId);
    QString error;

    if (reply->error() != QNetworkReply::NoError) {
        emit batchFailed(jobId, networkErrorText(reply), true);
        return;
    }

    const int status = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    if (status / 100 != 2) {
        // 401/402/403 是密钥/余额类错误，重试无意义
        const bool retryable = status == 429 || status >= 500;
        emit batchFailed(jobId, httpErrorText(status, body), retryable);
        return;
    }

    QString content;
    if (!DeepSeekJson::parseContent(body, &content, &error)) {
        emit batchFailed(jobId, error, true);
        return;
    }
    QStringList translations;
    if (!DeepSeekJson::parseTranslationsFromContent(content, expectedCount,
                                                    &translations, &error)) {
        emit batchFailed(jobId, error, true);
        return;
    }
    emit batchFinished(jobId, translations);
}
