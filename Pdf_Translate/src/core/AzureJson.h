#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

// Azure Translator v3.0 响应解析（独立于网络层，便于单元测试）
namespace AzureJson {

// 解析 translate 响应体：数组长度必须等于 expectedCount
// 成功返回 true 并把每段译文写入 out（与请求同序）
bool parseTranslations(const QByteArray &json, int expectedCount,
                       QStringList *out, QString *error);

// 提取错误响应体中的 error.message（无则返回空串）
QString extractErrorMessage(const QByteArray &json);

} // namespace AzureJson
