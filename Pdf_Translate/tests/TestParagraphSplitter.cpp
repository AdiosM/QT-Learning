#include "TestParagraphSplitter.h"

#include <QtTest>

#include "ParagraphSplitter.h"

void TestParagraphSplitter::normalize_joinsLines()
{
    const QString out = ParagraphSplitter::normalizePageText(
        QStringLiteral("line one\nline two\n\nnext paragraph"));
    QCOMPARE(out, QStringLiteral("line one line two\n\nnext paragraph"));
}

void TestParagraphSplitter::normalize_mergesHyphenation()
{
    const QString out = ParagraphSplitter::normalizePageText(
        QStringLiteral("water manage-\nment strategies"));
    QCOMPARE(out, QStringLiteral("water management strategies"));
}

void TestParagraphSplitter::normalize_handlesFormFeedAndCr()
{
    const QString out = ParagraphSplitter::normalizePageText(
        QStringLiteral("first line\r\nsecond\fpage two"));
    QCOMPARE(out, QStringLiteral("first line second\n\npage two"));
}

void TestParagraphSplitter::split_byBlankLines()
{
    const QList<QString> parts = ParagraphSplitter::splitPage(
        QStringLiteral("para one\n\npara two\n\npara three"));
    QCOMPARE(parts.size(), 3);
    QCOMPARE(parts.at(0), QStringLiteral("para one"));
    QCOMPARE(parts.at(2), QStringLiteral("para three"));
}

void TestParagraphSplitter::split_keepsShortParagraphWhole()
{
    const QList<QString> parts = ParagraphSplitter::splitPage(
        QStringLiteral("A short paragraph."), 100);
    QCOMPARE(parts.size(), 1);
    QCOMPARE(parts.at(0), QStringLiteral("A short paragraph."));
}

void TestParagraphSplitter::split_longParagraphAtSentences()
{
    // 每句 4 字符 + 1 空格，上限 10 字符 => 每块装两句
    const QList<QString> parts = ParagraphSplitter::splitPage(
        QStringLiteral("aaa. bbb. ccc."), 10);
    QCOMPARE(parts.size(), 2);
    QCOMPARE(parts.at(0), QStringLiteral("aaa. bbb."));
    QCOMPARE(parts.at(1), QStringLiteral("ccc."));
}

void TestParagraphSplitter::split_hardCutWhenSentenceTooLong()
{
    const QList<QString> parts = ParagraphSplitter::splitPage(
        QStringLiteral("abcdefghijklmnopqrstuvwxyz"), 10);
    QCOMPARE(parts.size(), 3);
    QCOMPARE(parts.at(0), QStringLiteral("abcdefghij"));
    QCOMPARE(parts.at(2), QStringLiteral("uvwxyz"));
}

void TestParagraphSplitter::split_skipsDecimalPoints()
{
    // "1.35" 的数字小数点不应切分；小上限触发硬切，块长必须 ≤ 10
    const QList<QString> parts = ParagraphSplitter::splitPage(
        QStringLiteral("density of 1.35 g cm-3. Next sentence."), 10);
    for (const QString &p : parts)
        QVERIFY2(p.size() <= 10, qPrintable(p));
    QCOMPARE(parts.join(QLatin1String("|")),
             QStringLiteral("density of| 1.35 g cm|-3.|Next sente|nce."));
}

void TestParagraphSplitter::split_sentencesForLongParagraphUnderLimit()
{
    // 段落超过 400 字符但低于 4500 上限：仍按句子切块，每块 ≤ 400
    QString paragraph;
    for (int i = 0; i < 30; ++i)
        paragraph += QStringLiteral("Sentence number %1 is here. ").arg(i);
    const QList<QString> parts = ParagraphSplitter::splitPage(paragraph);
    QVERIFY(parts.size() > 1);
    for (const QString &p : parts)
        QVERIFY2(p.size() <= 400, qPrintable(QString::number(p.size())));
    QVERIFY(parts.first().startsWith(QStringLiteral("Sentence number 0")));
}
