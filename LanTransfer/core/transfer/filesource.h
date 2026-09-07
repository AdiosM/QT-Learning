#pragma once

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QtGlobal>

// ============================================================================
// FileSource：文件流式读取抽象（设计文档 §8.2/§16）
// Windows 与 Android 普通路径使用 QFileFileSource；
// Android SAF Content URI 由平台层提供实现。
// 禁止一次性把整个文件装入内存。
// ============================================================================
namespace lantransfer {

class FileSource
{
public:
    virtual ~FileSource() = default;

    virtual bool open() = 0;                                  // 打开失败原因见 errorString()
    virtual void close() = 0;
    virtual qint64 size() const = 0;
    virtual qint64 read(char* buffer, qint64 maxSize) = 0;    // >0 字节数；0 EOF；<0 错误
    virtual QString errorString() const = 0;
};

// Windows 路径 / Android 普通文件路径
class QFileFileSource : public FileSource
{
public:
    explicit QFileFileSource(const QString& filePath)
        : m_path(filePath)
    {
    }

    bool open() override
    {
        m_file.setFileName(m_path);
        return m_file.open(QIODevice::ReadOnly);
    }

    void close() override { m_file.close(); }

    qint64 size() const override
    {
        return QFileInfo(m_path).size();
    }

    qint64 read(char* buffer, qint64 maxSize) override
    {
        return m_file.read(buffer, maxSize);
    }

    QString errorString() const override { return m_file.errorString(); }

    QString path() const { return m_path; }

private:
    QString m_path;
    QFile m_file;
};

} // namespace lantransfer
