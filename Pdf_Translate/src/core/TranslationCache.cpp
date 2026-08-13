#include "TranslationCache.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

namespace {
const char kConnectionName[] = "pdf_translation_cache";
} // namespace

TranslationCache::TranslationCache(const QString &dbPath)
    : m_dbPath(dbPath)
{
}

bool TranslationCache::init(QString *errorMessage)
{
    QSqlDatabase db;
    if (QSqlDatabase::contains(QLatin1String(kConnectionName))) {
        db = QSqlDatabase::database(QLatin1String(kConnectionName));
    } else {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                       QLatin1String(kConnectionName));
    }
    if (!db.isValid()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("SQLite 驱动不可用");
        return false;
    }

    // 确保父目录存在（SQLite 不会自动创建目录）
    if (!QDir().mkpath(QFileInfo(m_dbPath).absolutePath())) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法创建缓存目录");
        return false;
    }
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        if (errorMessage)
            *errorMessage = db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    const QStringList statements = {
        QStringLiteral("PRAGMA journal_mode=WAL"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS translations ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " doc_key TEXT NOT NULL, lang_pair TEXT NOT NULL, page INTEGER NOT NULL,"
            " src_hash TEXT NOT NULL, source TEXT NOT NULL, target TEXT NOT NULL,"
            " created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')))"),
        QStringLiteral(
            "CREATE UNIQUE INDEX IF NOT EXISTS idx_lookup"
            " ON translations(doc_key, lang_pair, page, src_hash)"),
    };
    for (const QString &statement : statements) {
        if (!query.exec(statement)) {
            if (errorMessage)
                *errorMessage = query.lastError().text();
            return false;
        }
    }
    m_valid = true;
    return true;
}

bool TranslationCache::isValid() const
{
    return m_valid;
}

QString TranslationCache::docKey(const QString &filePath, qint64 fileSize,
                                 const QDateTime &lastModified)
{
    const QString raw = QStringLiteral("%1|%2|%3")
                            .arg(filePath)
                            .arg(fileSize)
                            .arg(lastModified.toMSecsSinceEpoch());
    return hashText(raw);
}

QString TranslationCache::defaultDbPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/translations.db");
}

QStringList TranslationCache::lookup(const QString &docKey, const QString &langPair,
                                     const QList<CacheQuery> &queries) const
{
    QStringList results(queries.size());
    if (!m_valid)
        return results;

    QSqlDatabase db = QSqlDatabase::database(QLatin1String(kConnectionName));
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT target FROM translations"
        " WHERE doc_key=? AND lang_pair=? AND page=? AND src_hash=?"));
    for (int i = 0; i < queries.size(); ++i) {
        query.bindValue(0, docKey);
        query.bindValue(1, langPair);
        query.bindValue(2, queries.at(i).page);
        query.bindValue(3, hashText(queries.at(i).source));
        if (query.exec() && query.next())
            results[i] = query.value(0).toString();
    }
    return results;
}

bool TranslationCache::store(const QString &docKey, const QString &langPair,
                             int page, const QStringList &sources,
                             const QStringList &translations)
{
    if (!m_valid || sources.size() != translations.size())
        return false;

    QSqlDatabase db = QSqlDatabase::database(QLatin1String(kConnectionName));
    if (!db.transaction())
        return false;

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO translations"
        "(doc_key, lang_pair, page, src_hash, source, target)"
        " VALUES(?,?,?,?,?,?)"));
    for (int i = 0; i < sources.size(); ++i) {
        query.bindValue(0, docKey);
        query.bindValue(1, langPair);
        query.bindValue(2, page);
        query.bindValue(3, hashText(sources.at(i)));
        query.bindValue(4, sources.at(i));
        query.bindValue(5, translations.at(i));
        if (!query.exec()) {
            db.rollback();
            return false;
        }
    }
    return db.commit();
}

qint64 TranslationCache::sizeBytes() const
{
    return QFile(m_dbPath).size();
}

bool TranslationCache::clear()
{
    if (!m_valid)
        return false;
    QSqlDatabase db = QSqlDatabase::database(QLatin1String(kConnectionName));
    QSqlQuery query(db);
    return query.exec(QStringLiteral("DELETE FROM translations"));
}

QString TranslationCache::hashText(const QString &text)
{
    return QString::fromLatin1(QCryptographicHash::hash(
        text.toUtf8(), QCryptographicHash::Sha256).toHex());
}
