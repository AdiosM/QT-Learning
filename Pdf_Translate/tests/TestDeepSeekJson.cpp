#include "TestDeepSeekJson.h"

#include <QtTest>

#include "DeepSeekJson.h"

void TestDeepSeekJson::parseContent_success()
{
    const QByteArray json =
        R"({"choices":[{"message":{"role":"assistant",
            "content":"[\"你好\",\"世界\"]"}}]})";
    QString content;
    QString error;
    QVERIFY(DeepSeekJson::parseContent(json, &content, &error));
    QCOMPARE(content, QStringLiteral("[\"你好\",\"世界\"]"));
    QVERIFY(error.isEmpty());
}

void TestDeepSeekJson::parseContent_emptyChoices()
{
    const QByteArray json =
        R"({"error":{"message":"Authentication Fails","type":"authentication_error"},"choices":[]})";
    QString content;
    QString error;
    QVERIFY(!DeepSeekJson::parseContent(json, &content, &error));
    QCOMPARE(error, QStringLiteral("Authentication Fails"));
}

void TestDeepSeekJson::parseTranslations_cleanArray()
{
    QStringList out;
    QString error;
    QVERIFY(DeepSeekJson::parseTranslationsFromContent(
        QStringLiteral("[\"你好\",\"世界\"]"), 2, &out, &error));
    QCOMPARE(out.at(0), QStringLiteral("你好"));
    QCOMPARE(out.at(1), QStringLiteral("世界"));
}

void TestDeepSeekJson::parseTranslations_withMarkdownFences()
{
    QStringList out;
    QString error;
    QVERIFY(DeepSeekJson::parseTranslationsFromContent(
        QStringLiteral("```json\n[\"你好\",\"世界\"]\n```"), 2, &out, &error));
    QCOMPARE(out.size(), 2);
    QCOMPARE(out.at(1), QStringLiteral("世界"));
}

void TestDeepSeekJson::parseTranslations_withStrayText()
{
    // 模型可能在数组前后输出多余说明文字
    QStringList out;
    QString error;
    QVERIFY(DeepSeekJson::parseTranslationsFromContent(
        QStringLiteral("Here are the translations: [\"你好\",\"世界\"] done."),
        2, &out, &error));
    QCOMPARE(out.size(), 2);
    QCOMPARE(out.at(0), QStringLiteral("你好"));
}

void TestDeepSeekJson::parseTranslations_countMismatch()
{
    QStringList out;
    QString error;
    QVERIFY(!DeepSeekJson::parseTranslationsFromContent(
        QStringLiteral("[\"只有一个\"]"), 2, &out, &error));
    QVERIFY(error.contains(QStringLiteral("数量")));
}

void TestDeepSeekJson::parseTranslations_notFound()
{
    QStringList out;
    QString error;
    QVERIFY(!DeepSeekJson::parseTranslationsFromContent(
        QStringLiteral("我没有收到任何文本"), 1, &out, &error));
    QVERIFY(error.contains(QStringLiteral("JSON 数组")));
}

void TestDeepSeekJson::parseErrorMessageExtraction()
{
    const QByteArray json =
        R"({"error":{"message":"Insufficient Balance","type":"insufficient_balance"}})";
    QCOMPARE(DeepSeekJson::extractErrorMessage(json),
             QStringLiteral("Insufficient Balance"));
}
