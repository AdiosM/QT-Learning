#include "AzureTranslator.h"
#include "AzureJson.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace {

constexpr int kTimeoutMs = 30000;

// 网络层错误 -> 中文提示
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

// HTTP 错误状态 -> 中文提示（优先附带服务端 error.message）
QString httpErrorText(int status, const QByteArray &body)
{
    const QString serverMessage = AzureJson::extractErrorMessage(body);
    const QString suffix = serverMessage.isEmpty()
        ? QString()
        : QStringLiteral("（%1）").arg(serverMessage);

    switch (status) {
    case 401:
        return QStringLiteral("密钥无效或区域设置错误 (401)%1").arg(suffix);
    case 403:
        return QStringLiteral("密钥被禁用或配额用尽 (403)%1").arg(suffix);
    case 429:
        return QStringLiteral("请求过于频繁，已触发限流 (429)%1").arg(suffix);
    default:
        if (status >= 500)
            return QStringLiteral("翻译服务暂时不可用 (%1)%2").arg(status).arg(suffix);
        return QStringLiteral("请求失败 (%1)%2").arg(status).arg(suffix);
    }
}

} // namespace

AzureTranslator::AzureTranslator(const QString &endpoint, const QString &apiKey,
                                 const QString &region, QObject *parent)
    : Translator(parent)
    , m_endpoint(endpoint)
    , m_apiKey(apiKey)
    , m_region(region)
{
}

void AzureTranslator::translate(const Texts &texts, const QString &from,
                                const QString &to, quint64 jobId)
{
    QUrl url(m_endpoint + QStringLiteral("/translate?api-version=3.0&to=")
             + QUrl::toPercentEncoding(to));
    if (!from.isEmpty()) {
        QUrlQuery query(url);
        query.addQueryItem(QStringLiteral("from"), from);
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Ocp-Apim-Subscription-Key", m_apiKey.toUtf8());
    if (!m_region.isEmpty())
        request.setRawHeader("Ocp-Apim-Subscription-Region", m_region.toUtf8());
    request.setTransferTimeout(kTimeoutMs);

    QJsonArray body;
    for (const QString &text : texts)
        body.append(QJsonObject{{QStringLiteral("Text"), text}});

    QNetworkReply *reply = m_nam.post(request,
                                      QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_jobIds.insert(reply, jobId);
    m_expectedCounts.insert(jobId, texts.size());
    connect(reply, &QNetworkReply::finished, this, &AzureTranslator::onReplyFinished);
}

void AzureTranslator::cancelAll()
{
    const QList<QNetworkReply *> replies = m_jobIds.keys();
    m_jobIds.clear();
    for (QNetworkReply *reply : replies)
        reply->abort();   // abort 后 finished 仍会触发，jobId 已移除 -> 直接忽略
}

void AzureTranslator::onReplyFinished()
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
        emit batchFailed(jobId, networkErrorText(reply));
        return;
    }

    const int status = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    if (status / 100 != 2) {
        // 401/403 是密钥类错误，重试无意义
        const bool retryable = status != 401 && status != 403;
        emit batchFailed(jobId, httpErrorText(status, body), retryable);
        return;
    }

    QStringList translations;
    if (!AzureJson::parseTranslations(body, expectedCount, &translations, &error)) {
        emit batchFailed(jobId, error, true);
        return;
    }
    emit batchFinished(jobId, translations);
}
