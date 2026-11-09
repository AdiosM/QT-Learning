#include "protocolparser.h"

ProtocolParser::ProtocolParser(QObject *parent)
    : QObject(parent)
{
}

void ProtocolParser::reset()
{
    m_buffer.clear();
}

quint8 ProtocolParser::checksum(const QByteArray &data)
{
    quint8 sum = 0;
    for (char c : data) {
        sum += static_cast<quint8>(c);
    }
    return sum;
}

// 组帧：帧头 + 长度 + 载荷 + 校验
QByteArray ProtocolParser::buildFrame(const QByteArray &payload)
{//QByteArray 是 Qt 框架中用于存储和操作字节数组的类。它可以存储原始字节数据，
    QByteArray frame;
    frame.append(char(0xAA));
    frame.append(char(0x55));
    frame.append(char(payload.size()));
    frame.append(payload);
    frame.append(char(checksum(frame)));   // 对"除校验外全部字节"求和
    return frame;
}

void ProtocolParser::appendData(const QByteArray &data)
{
    m_buffer.append(data);       // ① 先攒着 → 解决半包

    // 缓冲区保护：攒太多还凑不出帧，说明对面在发垃圾，清空防内存涨爆
    if (m_buffer.size() > 65536) {
        emit syncLost(m_buffer.size());
        m_buffer.clear();
        return;
    }

    while (tryParseOne()) {}            // ② 循环取帧 → 解决粘包
}

bool ProtocolParser::tryParseOne()
{//返回 true：表示 "我从缓冲区消费掉了一帧"（不管这帧校验过没过）
//返回 false：表示 "缓冲区现在还凑不出一帧"（半包，需要等下一次数据再来）
    // ---- 1. 找帧头 AA 55 ----
    int idx = -1;
    for (int i = 0; i + 1 < m_buffer.size(); ++i) {//扫描缓冲区，
        if (quint8(m_buffer.at(i)) == 0xAA && quint8(m_buffer.at(i + 1)) == 0x55) {
            idx = i;//找第一个 0xAA 0x55 连续出现的位置idx
            break;
        }
    }

    if (idx < 0) {//没找到
        // 没帧头：只保留最后 1 字节（它可能是下一帧帧头的前半 AA），其余丢弃
        if (m_buffer.size() > 1) {
            emit syncLost(m_buffer.size() - 1);
            m_buffer.remove(0, m_buffer.size() - 1);
        }
        return false;
    }

    if (idx > 0) {
        emit syncLost(idx);
        m_buffer.remove(0, idx);// 帧头前有垃圾字节 → 丢弃并对齐
    }

    // ---- 2. 长度检查：至少要有 帧头2 + 长度1 + 校验1 = 4 字节 ----
    if (m_buffer.size() < 4) {// 至少 AA+55+长度+校验=4字节
        return false;       // 半包：等下一次数据到来
    }

    const int payloadLen = quint8(m_buffer.at(2));//第3字节 = 载荷长度
    const int totalLen   = 3 + payloadLen + 1;//整帧 = 帧头2+长度1+载荷N+校验1

    if (m_buffer.size() < totalLen) {
        return false;          // 半包：载荷还没到齐，等
    }

    // ---- 3. 取帧 + 校验 ----
    const QByteArray frame = m_buffer.left(totalLen); //取出整帧
    m_buffer.remove(0, totalLen);       // 无论校验过不过，这帧都从缓冲区消费掉

    const quint8 expect = checksum(frame.left(totalLen - 1));// 期望校验 = 除校验外所有字节求和
    const quint8 actual = quint8(frame.at(totalLen - 1));// 实际校验 = 最后一个字节

    if (expect != actual) {
        emit syncLost(totalLen);        // 校验错：整帧丢弃
        return true;                    // 继续尝试解析缓冲区里的下一帧
    }

    emit frameReceived(frame.mid(3, payloadLen));// 校验过 → 把纯载荷发给 UI
    return true;
}