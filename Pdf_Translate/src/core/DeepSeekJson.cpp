#include "DeepSeekJson.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

QString DeepSeekJson::extractErrorMessage(const QByteArray &json)
{
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject())
        return QString();
    return doc.object().value(QStringLiteral("error"))
        .toObject().value(QStringLiteral("message")).toString();
}

bool DeepSeekJson::parseContent(const QByteArray &json, QString *content, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error)
            *error = QStringLiteral("响应解析失败: ") + parseError.errorString();
        return false;
    }

    const QJsonArray choices = doc.object().value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        const QString serverMessage = extractErrorMessage(json);
        if (error)
            *error = serverMessage.isEmpty() ? QStringLiteral("响应缺少 choices") : serverMessage;
        return false;
    }
    const QString text = choices.first().toObject()
        .value(QStringLiteral("message")).toObject()
        .value(QStringLiteral("content")).toString();
    if (text.isEmpty()) {
        if (error)
            *error = QStringLiteral("响应内容为空");
        return false;
    }
    if (content)
        *content = text;
    return true;
}

bool DeepSeekJson::parseTranslationsFromContent(const QString &content,
                                                int expectedCount,
                                                QStringList *out, QString *error)
{
    QString text = content.trimmed();

    // 去除可能的 markdown 代码围栏
    if (text.startsWith(QLatin1String("```"))) {
        const int firstNewline = text.indexOf(u'\n');
        const int lastFence = text.lastIndexOf(QLatin1String("```"));
        if (firstNewline >= 0 && lastFence > firstNewline) {
            text = text.mid(firstNewline + 1, lastFence - firstNewline - 1).trimmed();
        }
    }

    // 定位首个 '[' 与最后一个 ']'，容忍模型输出多余说明文字
    const int start = text.indexOf(u'[');
    const int end = text.lastIndexOf(u']');
    if (start < 0 || end <= start) {
        if (error)
            *error = QStringLiteral("DeepSeek 返回内容中未找到 JSON 数组");
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(
        text.mid(start, end - start + 1).toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error)
            *error = QStringLiteral("DeepSeek 返回解析失败: ") + parseError.errorString();
        return false;
    }
    if (!doc.isArray()) {
        if (error)
            *error = QStringLiteral("DeepSeek 返回内容不是 JSON 数组");
        return false;
    }

    const QJsonArray array = doc.array();
    if (array.size() != expectedCount) {
        if (error)
            *error = QStringLiteral("DeepSeek 返回数量与请求不符（期望 %1，收到 %2）")
                         .arg(expectedCount).arg(array.size());
        return false;
    }

    QStringList result;
    result.reserve(array.size());
    for (const QJsonValue &value : array) {
        if (!value.isString()) {
            if (error)
                *error = QStringLiteral("DeepSeek 返回数组含非字符串元素");
            return false;
        }
        result.append(value.toString());
    }

    if (out)
        *out = result;
    return true;
}
