#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>

struct CacheQuery {
    int page;
    QString source;
};

// SQLite 翻译缓存
// 键 = (docKey, langPair, page, src_hash)，段落粒度
// docKey 由 文件路径|大小|修改时间 哈希得到：PDF 变了缓存即失效
class TranslationCache
{
public:
    explicit TranslationCache(const QString &dbPath);

    // false = 驱动缺失等，调用方降级为无缓存运行
    bool init(QString *errorMessage);
    bool isValid() const;

    static QString docKey(const QString &filePath, qint64 fileSize,
                          const QDateTime &lastModified);
    static QString defaultDbPath();

    // 与 queries 对齐返回，空串 = 未命中
    QStringList lookup(const QString &docKey, const QString &langPair,
                       const QList<CacheQuery> &queries) const;
    bool store(const QString &docKey, const QString &langPair, int page,
               const QStringList &sources, const QStringList &translations);
    qint64 sizeBytes() const;
    bool clear();

private:
    static QString hashText(const QString &text);
    QString m_dbPath;
    bool m_valid = false;
};
