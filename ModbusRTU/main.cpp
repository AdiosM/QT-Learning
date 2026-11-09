#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;//w是main()里面的栈对象，直到程序结束才销毁
    w.show();
    return QApplication::exec();
}



//该程序有两条独立的通信路径通向串口设备
/*通道A：自定义串口调试（AA55 帧）
 * SerialPortManager（物理层）
    封装 QSerialPort：开关/收发/枚举/错误；管波特率/数据位/校验等物理参数
                                        ↓收到原始字节 / 发出组帧数据
 *ProtocolParser（协议层·自定义帧）
    把字节流拆成 AA55+长度+载荷+校验 的帧；负责组帧/半包/粘包/校验
    用于通用串口调试 / 自定义设备
 */

/*通道B：标准 Modbus RTU
 * ModbusManager（协议层·标准 Modbus）
   封装 Qt 的 QModbusRtuSerialClient：连接/断开、读保持寄存器(03)、写单寄存器(06)、异步应答
 *              ↓ 自带串口管理 + RTU 帧(地址+功能码+CRC)
 * QModbusRtuSerialClient（Qt 内置）
    自己开串口、自己组 RTU 帧、自己算 CRC16
    用于真正的 Modbus 从站设备
 */