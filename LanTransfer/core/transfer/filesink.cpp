#include "filesink.h"

#include <QDir>
#include <QFileInfo>

namespace lantransfer {

FileNameCheckResult validateFileName(const QString& name)
{
    FileNameCheckResult r;
    if (name.isEmpty()) {
        r.error = QStringLiteral("文件名为空");
        return r;
    }
    if (name.size() > 200) {
        r.error = QStringLiteral("文件名过长（超过 200 字符）");
        return r;
    }
    // 仅允许 basename：禁止路径分隔符与 ".." 片段
    if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\'))) {
        r.error = QStringLiteral("文件名不允许包含路径");
        return r;
    }
    if (name == QStringLiteral(".") || name == QStringLiteral("..") || name.contains(QStringLiteral(".."))) {
        r.error = QStringLiteral("文件名不允许包含 \"..\"");
        return r;
    }
    // Windows 保留设备名（无论大小写、是否带扩展名）
    static const QStringList reserved = {
        QStringLiteral("CON"), QStringLiteral("PRN"), QStringLiteral("AUX"), QStringLiteral("NUL"),
        QStringLiteral("COM1"), QStringLiteral("COM2"), QStringLiteral("COM3"), QStringLiteral("COM4"),
        QStringLiteral("COM5"), QStringLiteral("COM6"), QStringLiteral("COM7"), QStringLiteral("COM8"),
        QStringLiteral("COM9"), QStringLiteral("LPT1"), QStringLiteral("LPT2"), QStringLiteral("LPT3"),
        QStringLiteral("LPT4"), QStringLiteral("LPT5"), QStringLiteral("LPT6"), QStringLiteral("LPT7"),
        QStringLiteral("LPT8"), QStringLiteral("LPT9"),
    };
    const QString base = name.section(QLatin1Char('.'), 0, 0);
    if (reserved.contains(base, Qt::CaseInsensitive)) {
        r.error = QStringLiteral("文件名是系统保留名");
        return r;
    }
    // Windows 非法字符
    const QString illegal = QStringLiteral("<>:\"/\\|?*");
    for (const QChar c : name) {
        if (illegal.contains(c)) {
            r.error = QStringLiteral("文件名包含非法字符 \"%1\"").arg(c);
            return r;
        }
        if (c.unicode() < 0x20) {
            r.error = QStringLiteral("文件名包含控制字符");
            return r;
        }
    }
    // 首尾不允许为空格或点
    if (name.startsWith(QLatin1Char(' ')) || name.endsWith(QLatin1Char(' '))
        || name.startsWith(QLatin1Char('.')) || name.endsWith(QLatin1Char('.'))) {
        r.error = QStringLiteral("文件名首尾不允许为空格或点");
        return r;
    }
    r.ok = true;
    return r;
}

QString uniqueFileName(const QString& dir, const QString& fileName)
{
    const QFileInfo info(fileName);
    const QString base = info.completeBaseName();
    const QString ext = info.suffix();
    QString candidate = fileName;
    for (int i = 1; QFileInfo::exists(QDir(dir).filePath(candidate)) && i < 10000; ++i) {
        candidate = ext.isEmpty()
            ? QStringLiteral("%1(%2)").arg(base).arg(i)
            : QStringLiteral("%1(%2).%3").arg(base).arg(i).arg(ext);
    }
    return candidate;
}

bool FileSink::open(const QString& destDir, const QString& fileName, ConflictPolicy policy)
{
    m_error.clear();
    m_policy = policy;

    const FileNameCheckResult check = validateFileName(fileName);
    if (!check.ok) {
        m_error = check.error;
        return false;
    }

    const QDir dir(destDir);
    if (!dir.exists()) {
        m_error = QStringLiteral("目标目录不存在：%1").arg(destDir);
        return false;
    }

    // 预计算最终路径（AutoRename 时避开现有文件）
    QString finalName = fileName;
    if (policy == ConflictPolicy::AutoRename)
        finalName = uniqueFileName(destDir, fileName);
    m_finalPath = dir.filePath(finalName);

    // 临时文件：最终名 + ".part"
    m_tempPath = dir.filePath(finalName + QStringLiteral(".part"));
    m_file.setFileName(m_tempPath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_error = QStringLiteral("无法创建文件：%1（%2）")
                      .arg(m_tempPath, m_file.errorString());
        return false;
    }
    return true;
}

bool FileSink::write(const char* data, qint64 len)
{
    if (!m_file.isOpen()) {
        m_error = QStringLiteral("文件未打开");
        return false;
    }
    const qint64 written = m_file.write(data, len);
    if (written != len) {
        m_error = QStringLiteral("写入失败（可能磁盘空间不足）：%1").arg(m_file.errorString());
        return false;
    }
    return true;
}

bool FileSink::commit()
{
    if (!m_file.isOpen()) {
        m_error = QStringLiteral("文件未打开");
        return false;
    }
    m_file.close();

    // 提交时再次处理冲突（open 之后可能有新文件出现）
    if (m_policy == ConflictPolicy::Overwrite) {
        if (QFileInfo::exists(m_finalPath) && !QFile::remove(m_finalPath)) {
            m_error = QStringLiteral("无法覆盖已有文件：%1").arg(m_finalPath);
            return false;
        }
    } else {
        QFileInfo fi(m_finalPath);
        if (fi.exists()) {
            const QString dir = fi.absolutePath();
            const QString newName = uniqueFileName(dir, fi.fileName());
            m_finalPath = QDir(dir).filePath(newName);
        }
    }

    if (!m_file.rename(m_finalPath)) {
        m_error = QStringLiteral("重命名失败：%1 -> %2（%3）")
                      .arg(m_tempPath, m_finalPath, m_file.errorString());
        return false;
    }
    return true;
}

void FileSink::abort()
{
    if (m_file.isOpen())
        m_file.close();
    if (!m_tempPath.isEmpty() && QFileInfo::exists(m_tempPath))
        QFile::remove(m_tempPath);
}

} // namespace lantransfer
