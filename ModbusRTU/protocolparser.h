#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

//定义通信协议，字节怎么组成有意义的 "消息 / 帧",自定义 AA55 帧。
//把原始字节流还原成一帧帧消息，负责 "自定义协议"。

#include <QObject>
#include <QByteArray>

// 协议层：把"字节流"还原成"一帧一帧的消息"
// 它不知道串口的存在，也不知道界面的存在 —— 只认字节
class ProtocolParser : public QObject
{
    Q_OBJECT
public:
    explicit ProtocolParser(QObject *parent = nullptr);

    void reset();      // 清空缓冲区（开/关串口时调用）
    static QByteArray buildFrame(const QByteArray &payload);  //组帧（AA 55 + 长度 + 载荷 + 校验和）
    //（校验 = 累加 & 0xFF）

public slots:
    void appendData(const QByteArray &data);  // 喂入原始字节流

signals:
    void frameReceived(const QByteArray &payload); // 解析出一帧完整数据
    void syncLost(int droppedBytes);               // 丢弃了垃圾/坏帧（调试用）

private:
    bool tryParseOne();                            // 尝试从缓冲区取一帧
    static quint8 checksum(const QByteArray &data);// 校验和 = 累加 & 0xFF

    QByteArray m_buffer;      // ★ 接收缓冲区（粘包/半包的核心）
};

#endif // PROTOCOLPARSER_H
