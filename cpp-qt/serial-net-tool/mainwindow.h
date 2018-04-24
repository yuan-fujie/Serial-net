#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QMessageBox>
#include <QDebug>
//FOR serialport
#include <QString>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

enum my_skin
{
    black = 0,
    white,
};

class QTcpSocket;
class TcpServer;
class QUdpSocket;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void ui_style_init();
    void config_init(void);
    void tcp_serial_init(void);
    void tcp_init();
    void serial_init();

    void writeConfig();

    void data_processing(const QByteArray receive_tmp);
    void data_processing(const QByteArray receive_tmp, const QString ip);

    void data_show(QByteArray &receive_tmp);
    void data_save(QByteArray receive_tmp);

    qint64 serialport_write(const char *data, qint64 len);

private slots:
    /* custom system btn */
    void onMin( bool );
    void onMaxOrNormal( bool );
    void onClose( bool );
private:
    bool maxOrNormal;//true表示当前窗口状态为normal，图标应显示为max

    /* custom title bar */
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    QPoint dragPosition;

public slots:
    void readComDataSlot(void);
    void read_serial_timer();

    void readSendData();        //读取发送配置文件数据
    void readDeviceData();      //读取设备转发文件数据

    void sendDataTcpServer();
    void sendDataTcpServer(QString data);
    void sendDataTcpServer(QString ip, int port, QString data);

    void sendDataTcpClient();
    void sendDataTcpClient(QString data);

    void sendDataUdpServer();
    void sendDataUdpServer(QString data);
    void sendDataUdpServer(QString ip, int port, QString data);

    void appendTcpClient(quint8 type, QString msg);
    void appendTcpServer(quint8 type, QString msg);
    void appendUdpServer(quint8 type, QString msg);

private slots:
    void tcpClientReadData();
    void tcpClientReadError();

    void clientReadData(int, QString ip, int port, QByteArray data);
    void clientConnect(int, QString ip, int port);
    void clientDisConnect(int, QString ip, int port);

    void udpServerReadData();
    void udpServerReadError();

    void on_connectPushButton_clicked();
    void on_sendPushButton_clicked();

private slots:

    void on_dataSavePushButton_clicked();

    void on_configTabWidget_currentChanged(int index);

    void on_tcpTypeComboBox_currentIndexChanged(int index);

    void on_clear_pushButton_clicked();

    void on_quit_pushButton_clicked();

    void on_timerCheckBox_clicked(bool checked);

    void on_receiveCheckBox_toggled(bool checked);

    void on_serialPortCBox_currentIndexChanged(const QString &arg1);

    void on_skinButton_clicked();


public:
    int skin_mode;

private:
    Ui::MainWindow *ui;

    QSerialPort *my_serialport;
    QTimer *myTimer_serial;

    int rfid_clear_times;
    uint8_t sn_num;

    int msgMaxCount;
    int countTcpServer;
    int countTcpClient;
    int countUdpServer;

    QString current_time;


    QStringList keys;           //模拟设备回复数据key
    QStringList values;         //模拟设备回复数据value

    QString server_ip;
    int server_port;

    QTcpSocket *tcpClient;
    TcpServer *tcpServer;
    QUdpSocket *udpServer;

    QTimer *timerTcpClient;
    QTimer *timerTcpServer;
    QTimer *timerUdpServer;

    QTimer *timerDevUpdate;
    int timer_dev_update_n;
#define TCP_IP_TYPE 0
#define SERIAL_TYPE 1

    int communication_type = TCP_IP_TYPE;    //通讯方式
};

#endif // MAINWINDOW_H
