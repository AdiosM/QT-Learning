#pragma once

#include <QFile>
#include <QString>

// ============================================================================
// FileSink：接收端文件落盘（设计文档 §12）
//   临时文件 "<name>.part" -> SHA-256 校验成功 -> 原子重命名
//   文件名安全：仅 basename，禁 ".."/保留名/非法字符
//   冲突策略：AutoRename（生成 "(1)" 等）或 Overwrite
// ============================================================================
namespace lantransfer {

struct FileNameCheckResult {
    bool ok = false;
    QString error; // 不合法时的原因（中文）
};

// 校验接收文件名（发送方 basename）
FileNameCheckResult validateFileName(const QString& name);

// 在 dir 内为 fileName 生成不冲突的最终名（"name(1).ext"）
QString uniqueFileName(const QString& dir, const QString& fileName);

// 接收文件写入策略
enum class ConflictPolicy {
    AutoRename, // 已存在时自动生成 "(1)" 等新名
    Overwrite,  // 覆盖已有文件
};

class FileSink
{
public:
    // 打开临时文件准备写入。destDir 必须已存在。
    // 冲突按 policy 在提交（commit）时处理；open 阶段预计算最终路径。
    bool open(const QString& destDir, const QString& fileName, ConflictPolicy policy);

    // 追加写入；失败返回 false（如磁盘满），errorString() 给出原因
    bool write(const char* data, qint64 len);

    // 校验成功后调用：关闭临时文件并原子重命名为最终路径
    bool commit();

    // 取消/失败：删除临时文件
    void abort();

    bool isOpen() const { return m_file.isOpen(); }

    QString finalPath() const { return m_finalPath; }
    QString tempPath() const { return m_tempPath; }
    QString errorString() const { return m_error; }

private:
    QFile m_file;
    QString m_tempPath;
    QString m_finalPath;
    ConflictPolicy m_policy = ConflictPolicy::AutoRename;
    QString m_error;
};

} // namespace lantransfer
