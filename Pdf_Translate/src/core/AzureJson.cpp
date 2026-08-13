#include "AzureJson.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

QString AzureJson::extractErrorMessage(const QByteArray &json)
{
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject())
        return QString();
    return doc.object().value(QStringLiteral("error"))
        .toObject().value(QStringLiteral("message")).toString();
}

bool AzureJson::parseTranslations(const QByteArray &json, int expectedCount,
                                  QStringList *out, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error)
            *error = QStringLiteral("响应解析失败: ") + parseError.errorString();
        return false;
    }

    if (!doc.isArray()) {
        const QString serverMessage = extractErrorMessage(json);
        if (error)
            *error = serverMessage.isEmpty() ? QStringLiteral("响应格式错误") : serverMessage;
        return false;
    }

    const QJsonArray array = doc.array();
    if (array.size() != expectedCount) {
        if (error)
            *error = QStringLiteral("响应数量与请求不符（期望 %1，收到 %2）")
                         .arg(expectedCount).arg(array.size());
        return false;
    }

    QStringList result;
    result.reserve(array.size());
    for (const QJsonValue &value : array) {
        const QJsonArray translations =
            value.toObject().value(QStringLiteral("translations")).toArray();
        if (translations.isEmpty()) {
            if (error)
                *error = QStringLiteral("响应缺少译文");
            return false;
        }
        result.append(translations.first().toObject()
                          .value(QStringLiteral("text")).toString());
    }

    if (out)
        *out = result;
    return true;
}
