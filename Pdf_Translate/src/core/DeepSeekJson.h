#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

// DeepSeek chat/completions 响应解析（独立于网络层，便于单元测试）
namespace DeepSeekJson {

// 从响应体提取 choices[0].message.content 文本
bool parseContent(const QByteArray &json, QString *content, QString *error);

// 从内容文本解析译文 JSON 数组：
// - 容忍 markdown 代码围栏（```json ... ```）
// - 在内容中定位首个 '[' 与最后一个 ']'（容忍模型输出多余说明文字）
// - 数组长度必须等于 expectedCount，元素必须为字符串
bool parseTranslationsFromContent(const QString &content, int expectedCount,
                                  QStringList *out, QString *error);

// 提取错误响应体中的 error.message（无则返回空串）
QString extractErrorMessage(const QByteArray &json);

} // namespace DeepSeekJson
