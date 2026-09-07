#pragma once

#include <QObject>
#include <QRandomGenerator>
#include <QString>

// ============================================================================
// PairingManager：用户确认与 6 位 PIN 配对（设计文档 §13）
// V1.0 安全基线：设备ID + 人工确认 + 可选 6 位 PIN。
// 明确：PIN 解决的是"用户确认/配对"，本身不是数据加密。
// ============================================================================
namespace lantransfer {

class PairingManager : public QObject
{
    Q_OBJECT
public:
    explicit PairingManager(QObject* parent = nullptr) : QObject(parent) {}

    // 生成 6 位数字 PIN（000000-999999）
    static QString generatePin()
    {
        const quint32 v = QRandomGenerator::global()->bounded(1000000u);
        return QStringLiteral("%1").arg(v, 6, 10, QLatin1Char('0'));
    }

    // 校验用户输入的 PIN 是否与生成值一致（长度/格式/内容）
    static bool verifyPin(const QString& expected, const QString& provided)
    {
        if (expected.size() != 6 || provided.size() != 6)
            return false;
        for (const QChar c : expected + provided) {
            if (!c.isDigit())
                return false;
        }
        return expected == provided;
    }
};

} // namespace lantransfer
