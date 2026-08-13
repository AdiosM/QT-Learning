#pragma once

#include <QTextBlock>
#include <QVector>
#include <QWidget>

class QTextBrowser;

// 右栏：译文显示
// - 段落级渐进刷新：段落占位块先行，翻译结果到达后原位替换
// - QTextBlock 句柄在后续编辑后仍有效，因此替换是 O(1) 定位
class TranslationView : public QWidget
{
    Q_OBJECT

public:
    explicit TranslationView(QWidget *parent = nullptr);

    void reset();
    void setDocumentTitle(const QString &title);
    void beginPage(int page);                       // 追加 "第 N 页" 页眉
    void showPending(int page, int paraIndex, const QString &original);
    void showTranslated(int page, int paraIndex,
                        const QString &original, const QString &translation);
    void showError(int page, int paraIndex,
                   const QString &original, const QString &error);
    void appendNotice(const QString &text);         // 提示信息（扫描件等）
    void jumpToPage(int page);                      // 滚动同步：右 -> 左
    int pageAtViewportTop() const;                  // 滚动同步：左 -> 右

signals:
    void pageRequested(int page);

private:
    struct Entry {
        QTextBlock block;      // 该条目占位块（替换时定位用）
        QString original;
    };
    struct PageData {
        QVector<Entry> entries;
    };

    void ensurePage(int page);
    void ensureEntry(int page, int paraIndex);
    void replaceBlock(const QTextBlock &block, const QString &html);
    void appendHtml(const QString &html);
    QTextCursor endCursor() const;
    QString escapedMultiline(const QString &text) const;
    QString entryHtml(const QString &original, const QString &dstHtml) const;

    QTextBrowser *m_browser = nullptr;
    QVector<PageData> m_pages;
    QVector<int> m_pageBlockPositions;   // 每页页眉块在文档中的位置
    int m_lastEmittedPage = -1;          // 避免重复发出同一页
};
