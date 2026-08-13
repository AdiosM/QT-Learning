#include "ParagraphSplitter.h"

#include <QStringList>
#include <utility>

namespace {

// 段落超过该长度即按句子切块（真实 PDF 提取的文字常缺少空行分段，
// 按句子切块能获得更好的显示粒度与缓存粒度）
constexpr int kSentenceSplitTarget = 400;

// 按句子结束符切分句子；跳过数字小数点（如 "1.35"）
QStringList splitSentences(const QString &paragraph)
{
    QStringList sentences;
    int start = 0;
    for (int i = 0; i < paragraph.size(); ++i) {
        const QChar c = paragraph.at(i);
        const bool terminator = c == u'.' || c == u'!' || c == u'?'
            || c == u'。' || c == u'！' || c == u'？' || c == u';' || c == u'；';
        if (!terminator)
            continue;
        if (c == u'.') {
            // "1.35" 之类的小数点不是句子边界
            const bool prevDigit = i > 0 && paragraph.at(i - 1).isDigit();
            const bool nextDigit = i + 1 < paragraph.size()
                                   && paragraph.at(i + 1).isDigit();
            if (prevDigit && nextDigit)
                continue;
        }
        sentences.append(paragraph.mid(start, i - start + 1).trimmed());
        start = i + 1;
    }
    const QString tail = paragraph.mid(start).trimmed();
    if (!tail.isEmpty())
        sentences.append(tail);
    return sentences;
}

} // namespace

QString ParagraphSplitter::normalizePageText(const QString &raw)
{
    QString text = raw;
    text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    text.remove(u'\r');
    text.replace(u'\f', QStringLiteral("\n\n"));
    text.replace(QStringLiteral("-\n"), QString());   // 断行连字符合并单词

    // 逐行处理：段内换行合并为空格，空行保留为段落分隔
    QStringList parts;
    bool prevBlank = true;
    const QStringList lines = text.split(u'\n');
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty()) {
            if (!prevBlank)
                parts.append(QString());
            prevBlank = true;
        } else {
            parts.append(line);
            prevBlank = false;
        }
    }

    QString result;
    prevBlank = true;
    for (const QString &part : std::as_const(parts)) {
        if (part.isEmpty()) {
            if (!prevBlank)
                result += QStringLiteral("\n\n");
            prevBlank = true;
        } else {
            if (!result.isEmpty() && !prevBlank)
                result += u' ';
            result += part;
            prevBlank = false;
        }
    }
    return result.trimmed();
}

QList<QString> ParagraphSplitter::splitPage(const QString &normalizedText,
                                            int maxChunkChars)
{
    QList<QString> result;
    const QStringList paragraphs =
        normalizedText.split(QStringLiteral("\n\n"), Qt::SkipEmptyParts);
    for (QString paragraph : paragraphs) {
        paragraph = paragraph.trimmed();
        if (paragraph.isEmpty())
            continue;

        const bool shortEnough = paragraph.size() <= maxChunkChars
            && paragraph.size() <= kSentenceSplitTarget;
        if (shortEnough) {
            result.append(paragraph);
            continue;
        }

        // 长段落：贪心按句子边界打包（目标块大小取较小者）
        const int target = qMin(maxChunkChars, kSentenceSplitTarget);
        const QStringList sentences = splitSentences(paragraph);
        QString chunk;
        for (const QString &sentence : sentences) {
            const int addLen = (chunk.isEmpty() ? 0 : 1) + sentence.size();
            if (chunk.size() + addLen > target) {
                if (!chunk.isEmpty()) {
                    result.append(chunk);
                    chunk.clear();
                }
                // 单句超限：硬切
                if (sentence.size() > target) {
                    for (int pos = 0; pos < sentence.size(); pos += target)
                        result.append(sentence.mid(pos, target));
                    continue;
                }
            }
            chunk += (chunk.isEmpty() ? QString() : QStringLiteral(" ")) + sentence;
        }
        if (!chunk.isEmpty())
            result.append(chunk);
    }
    return result;
}
