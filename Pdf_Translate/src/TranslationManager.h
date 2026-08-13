#pragma once

#include "core/TranslationQueue.h"   // Item 结构在此定义

#include <QHash>
#include <QObject>
#include <QSet>

class PdfViewWidget;
class TranslationCache;
class TranslationView;
class Translator;

// 编排翻译管线：提取文字 -> 切段 -> 查缓存 -> 排队 -> 写视图与缓存
class TranslationManager : public QObject
{
    Q_OBJECT

public:
    TranslationManager(PdfViewWidget *pdf, TranslationView *view,
                       QObject *parent = nullptr);
    ~TranslationManager() override;

    void setDocument(const QString &path);
    void translateCurrentPage();
    void translateWholeDocument();
    void stop();
    bool isRunning() const;
    void reloadSettings();                        // 设置变更后重建翻译服务

signals:
    void progressChanged(int done, int total);
    void runFinished(int ok, int failed);
    void statusMessage(const QString &text);

private:
    void startJob(const QList<int> &pages);
    void processPage(int page, QList<TranslationQueue::Item> *pending);
    void collectWholeDocument();                  // 逐页提取（经事件循环）
    void setTranslator(Translator *translator);   // 替换翻译服务实例
    void onItemTranslated(int page, int paraIndex, const QString &text);
    void onItemFailed(int page, int paraIndex, const QString &error);

    PdfViewWidget *m_pdf = nullptr;
    TranslationView *m_view = nullptr;
    TranslationQueue *m_queue = nullptr;
    Translator *m_translator = nullptr;
    TranslationCache *m_cache = nullptr;
    QString m_docKey;
    QString m_langPair;                            // "源->目标"
    QHash<QPair<int, int>, QString> m_originals;   // (page, paraIndex) -> 原文
    QSet<int> m_translatedPages;                   // 本次会话已翻译的页
    int m_cacheHits = 0;

    QList<int> m_wholePages;                       // 全文翻译待处理页
    int m_wholeIndex = 0;
    QList<TranslationQueue::Item> m_wholePending;  // 全文翻译累积的待译条目
};
