#pragma once

#include <QByteArray>
#include <QCryptographicHash>
#include <QString>

// ============================================================================
// HashCalculator：流式 SHA-256（设计文档 §8.3）
// 发送端边读边算、接收端边写边算，禁止为校验额外完整扫描大文件。
// ============================================================================
namespace lantransfer {

class HashCalculator
{
public:
    void reset() { m_hash.reset(); }
    void addData(const char* data, qint64 len) { m_hash.addData(QByteArrayView(data, len)); }
    void addData(const QByteArray& data) { m_hash.addData(QByteArrayView(data)); }

    // 原始 32 字节二进制结果
    QByteArray result() const { return m_hash.result(); }

    // 小写 hex 字符串（协议中传输格式）
    QString resultHex() const { return QString::fromLatin1(m_hash.result().toHex()); }

    static QString toHex(const QByteArray& raw) { return QString::fromLatin1(raw.toHex()); }

    // hex -> 原始字节；输入非法（长度非 64 或含非 hex 字符）返回空
    static QByteArray fromHex(const QString& hex)
    {
        if (hex.size() != 64)
            return {};
        return QByteArray::fromHex(hex.toLatin1());
    }

private:
    QCryptographicHash m_hash{QCryptographicHash::Sha256};
};

} // namespace lantransfer
