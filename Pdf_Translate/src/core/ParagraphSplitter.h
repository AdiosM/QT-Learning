#pragma once

#include <QList>
#include <QString>

// 纯函数集合：PDF 页面文字的规范化与切段
// 输出必须是确定性的（缓存键依赖稳定结果）
class ParagraphSplitter
{
public:
    // 规范化页面原文:
    // - "\r\n" / "\r" 归一为 "\n"
    // - "\f"(换页符) 视为段落分隔
    // - "-\n" 断行连字符合并单词
    // - 段落内部单换行合并为空格，空行保留为段落分隔
    static QString normalizePageText(const QString &raw);

    // 按段落切分:
    // - 空行分段
    // - 超长段落优先按句子边界切分，单句仍超限则硬切
    static QList<QString> splitPage(const QString &normalizedText,
                                    int maxChunkChars = 4500);
};
