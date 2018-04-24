#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHostInfo>
#include <QNetworkInterface>
#include <QFileDialog>
#include <QDataStream>
#include <QDate>

#include "app.h"
#include "tcpserver.h"
#include "myhelper.h"
#include "commonstyle.h"

#define QDEBUG_EN           1
#define QSHOWTEXTEDIT_EN    1

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("JY TOOL"));

    current_time = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    msgMaxCount = 1000;

    config_init();

    myHelper::formInCenter(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::config_init()
{
    App::ReadConfig();

    tcp_serial_init();
}

void MainWindow::tcp_serial_init()
{
    ui->sendCheckBox->setChecked(true);
    ui->receiveCheckBox->setChecked(true);

    myTimer_serial = new QTimer(this);
    connect(myTimer_serial,SIGNAL(timeout()),this,SLOT(read_serial_timer()));

    if(ui->configTabWidget->currentIndex() == 0)
    {
        if(communication_type == SERIAL_TYPE)
        {
            if(ui->connectPushButton->text() == "断开")
            {
                if(my_serialport->isOpen())
                {
                    ui->connectPushButton->setText("连接");
                    my_serialport->close();
                }
                delete my_serialport;
#if QDEBUG_EN
                qDebug() << "串口 已关闭！" << "\n";
#endif
            }
        }
        communication_type = TCP_IP_TYPE;
    }
    else if (ui->configTabWidget->currentIndex() == 1)
    {
        if(communication_type == TCP_IP_TYPE)
        {
            if(ui->connectPushButton->text() == "断开")
            {
                ui->connectPushButton->setText("连接");
#if QDEBUG_EN
                qDebug() << "tcp server 关闭!\n";
#endif
            }
        }
        communication_type = SERIAL_TYPE;
    }
    else
    {
        communication_type = -1;
    }

    tcp_init();
    serial_init();

    ui->sendPushButton->setEnabled(false);

    if(communication_type == TCP_IP_TYPE)
    {
    }
    else if(communication_type == SERIAL_TYPE)
    {
    }
}


void MainWindow::tcp_init()
{

    QString localHostName = QHostInfo::localHostName();
    QList<QHostAddress> addrs = QHostInfo::fromName(localHostName).addresses();
    server_port = 6666;

    foreach (QHostAddress addr, addrs)
    {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol)
        {
            server_ip = addr.toString();
            this->setWindowTitle(QString("Net-Tool 本机IP[%1]").arg(server_ip));
#if QDEBUG_EN
            qDebug() << server_ip;
#endif
            break;
        }
    }

    tcpClient = new QTcpSocket(this);
    tcpClient->abort();
    connect(tcpClient, SIGNAL(readyRead()), this, SLOT(tcpClientReadData()));
    connect(tcpClient, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(tcpClientReadError()));

    tcpServer = new TcpServer(this);
    connect(tcpServer, SIGNAL(clientConnect(int, QString, int)),
            this, SLOT(clientConnect(int, QString, int)));
    connect(tcpServer, SIGNAL(clientDisConnect(int, QString, int)),
            this, SLOT(clientDisConnect(int, QString, int)));
    connect(tcpServer, SIGNAL(clientReadData(int, QString, int, QByteArray)),
            this, SLOT(clientReadData(int, QString, int, QByteArray)));

    udpServer = new QUdpSocket(this);
    udpServer->abort();
    connect(udpServer, SIGNAL(readyRead()), this, SLOT(udpServerReadData()));
    connect(udpServer, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(udpServerReadError()));

    timerTcpClient = new QTimer(this);
    timerTcpClient->setInterval(App::IntervalTcpClient);
    connect(timerTcpClient, SIGNAL(timeout()), this, SLOT(sendDataTcpClient()));

    timerTcpServer = new QTimer(this);
    timerTcpServer->setInterval(App::IntervalTcpServer);
    connect(timerTcpServer, SIGNAL(timeout()), this, SLOT(sendDataTcpServer()));

    timerUdpServer = new QTimer(this);
    timerUdpServer->setInterval(App::IntervalUdpServer);
    connect(timerUdpServer, SIGNAL(timeout()), this, SLOT(sendDataUdpServer()));

    if (ui->timerCheckBox->isChecked() == true)
    {
        if(ui->sendPushButton->text() == "断开")
        {
            if(ui->tcpTypeComboBox->currentIndex() == 0)
                timerTcpServer->start();
            if(ui->tcpTypeComboBox->currentIndex() == 1)
                timerTcpClient->start();
            if(ui->tcpTypeComboBox->currentIndex() == 2)
                timerUdpServer->start();
        }
    }
    else
    {
        if(ui->tcpTypeComboBox->currentIndex() == 0)
            timerTcpServer->stop();
        if(ui->tcpTypeComboBox->currentIndex() == 1)
            timerTcpClient->stop();
        if(ui->tcpTypeComboBox->currentIndex() == 2)
            timerUdpServer->stop();
    }
    QStringList tcpTypeList;       //TCP/IP协议类型
    tcpTypeList << "TCP server" << "TCP client" << "UDP" ;
    ui->tcpTypeComboBox->clear();
    ui->tcpTypeComboBox->addItems(tcpTypeList);
    ui->tcpTypeComboBox->setCurrentIndex(0);
    ui->tcpTypeComboBox->setEnabled(true);

    ui->tcpIPLineEdit->setText(server_ip);
    ui->tcpPortLineEdit->setText(QString::number(server_port));

#if QDEBUG_EN
    qDebug() << "ip:" << server_ip << " port:" << server_port << "\n";
#endif

}

void MainWindow::serial_init()
{
    ui->serialPortCBox->clear();
    ui->serialPortCBox->setEnabled(true);
    // Example use QSerialPortInfo
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
#if QDEBUG_EN
        qDebug() << "Name        : " << info.portName();
        qDebug() << "Description : " << info.description();
        //qDebug() << "Manufacturer: " << info.manufacturer();
#endif
        ui->serialPortCBox->addItem(QString(info.portName()));
    }

    QStringList baudList;       //波特率
    QStringList parityList;     //校验位
    QStringList dataBitsList;   //数据位
    QStringList stopBitsList;   //停止位
    QStringList folwCtrlList;   //流控

    baudList<< "50" << "75" << "100" << "134" << "150" << "200" << "300"
            << "600" << "1200" << "1800" << "2400" << "4800" << "9600"
            << "14400" << "19200" << "38400" << "56000" << "57600"
            << "76800" << "115200" << "128000" << "256000";
    ui->baudRateCBox->clear();
    ui->baudRateCBox->addItems(baudList);
    ui->baudRateCBox->setCurrentIndex(19);//12:9600, 19:115200

    dataBitsList << "5" << "6" << "7" << "8";
    ui->dataBitCBox->clear();
    ui->dataBitCBox->addItems(dataBitsList);
    ui->dataBitCBox->setCurrentIndex(3);

    stopBitsList << "1";    //OneStop = 1,
    stopBitsList << "1.5";  //OneAndHalfStop = 3,
    stopBitsList << "2";    //TwoStop = 2,
    ui->stopBitBox->clear();
    ui->stopBitBox->addItems(stopBitsList);
    ui->stopBitBox->setCurrentIndex(0);

    parityList << "None" << "Odd" << "Even";
    parityList << "Mark";
    parityList << "Space";
    ui->parityBitCBox->clear();
    ui->parityBitCBox->addItems(parityList);
    ui->parityBitCBox->setCurrentIndex(0);

    folwCtrlList << "None" << "Hardware" << "Software" << "Custom";
    ui->flowControlCBox->clear();
    ui->flowControlCBox->addItems(folwCtrlList);
    ui->flowControlCBox->setCurrentIndex(0);
}

void MainWindow::writeConfig()
{

}

void MainWindow::on_receiveCheckBox_toggled(bool checked)
{
   App::HexReceiveTcpServer = checked;
}

void MainWindow::appendTcpClient(quint8 type, QString msg)
{
    if (countTcpClient > msgMaxCount) {
        ui->showTextEdit->clear();
        countTcpClient = 0;
    }

    QString str;

    if (type == 0) {
        str = "发送:";
        ui->showTextEdit->setTextColor(QColor("dodgerblue"));
    } else if (type == 1) {
        str = "接收:";
        ui->showTextEdit->setTextColor(QColor("red"));
    }

    ui->showTextEdit->append(QString("时间[%1] %2 %3").arg(TIMEMS).arg(str).arg(msg));
    countTcpClient++;
}

void MainWindow::appendTcpServer(quint8 type, QString msg)
{
    if (countTcpServer > msgMaxCount) {
        ui->showTextEdit->clear();
        countTcpServer = 0;
    }

    QString str;

    if (type == 0) {
        str = ">> 发送 :";
        ui->showTextEdit->setTextColor(QColor("dodgerblue"));
    } else if (type == 1) {
        str = "<< 接收 :";
        ui->showTextEdit->setTextColor(QColor("red"));
    }

    ui->showTextEdit->append(QString("时间[%1] %2 %3").arg(TIMEMS).arg(str).arg(msg));
    countTcpServer++;
}

void MainWindow::sendDataUdpServer()
{
    QString data = ui->sendComboBox->currentText();
    sendDataUdpServer(data);
}

void MainWindow::sendDataUdpServer(QString data)
{
    QString ip = ui->tcpIPLineEdit->text();
    int port = ui->tcpPortLineEdit->text().toInt();
    sendDataUdpServer(ip, port, data);
}

void MainWindow::sendDataUdpServer(QString ip, int port, QString data)
{
    if (!data.isEmpty()) {
        QByteArray buffer;

        if (App::HexSendUdpServer) {
            buffer = myHelper::hexStrToByteArray(data);
        } else {
            buffer = myHelper::asciiStrToByteArray(data);
        }

        udpServer->writeDatagram(buffer, QHostAddress(ip), port);

#if QSHOWTEXTEDIT_EN
        QString str = QString("目标:%1[%2] ").arg(ip).arg(port);
        appendUdpServer(0, str + data);
#endif
    }
}

void MainWindow::appendUdpServer(quint8 type, QString msg)
{
    if (countUdpServer > msgMaxCount) {
        ui->showTextEdit->clear();
        countUdpServer = 0;
    }

    QString str;

    if (type == 0) {
        str = ">> 发送 :";
        ui->showTextEdit->setTextColor(QColor("dodgerblue"));
    } else if (type == 1) {
        str = "<< 接收 :";
        ui->showTextEdit->setTextColor(QColor("red"));
    }

    ui->showTextEdit->append(QString("时间[%1] %2 %3").arg(TIMEMS).arg(str).arg(msg));
    countUdpServer++;
}

void MainWindow::tcpClientReadData()
{
    if (tcpClient->bytesAvailable() <= 0) {
        return;
    }

    QByteArray data = tcpClient->readAll();
    QString buffer;

    if (App::HexReceiveTcpClient) {
        buffer = myHelper::byteArrayToHexStr(data);
    } else {
        buffer = myHelper::byteArrayToAsciiStr(data);
    }

#if QSHOWTEXTEDIT_EN
    appendTcpClient(1, buffer);
#endif

    if (App::DebugTcpClient) {
        int count = keys.count();

        for (int i = 0; i < count; i++) {
            if (keys.at(i) == buffer) {
                sendDataTcpClient(values.at(i));
                break;
            }
        }
    }
}

void MainWindow::tcpClientReadError()
{
    tcpClient->disconnectFromHost();
    ui->connectPushButton->setText("连接");
    ui->sendPushButton->setEnabled(false);

    appendTcpClient(1, QString("连接服务器失败,原因 : %1").arg(tcpClient->errorString()));
}

void MainWindow::clientReadData(int , QString ip, int port, QByteArray data)
{
    QString buffer;

    data_processing(data, QString("%1:%2").arg(ip).arg(port));

#if QSHOWTEXTEDIT_EN
    if (App::HexReceiveTcpServer)
    {
        buffer = myHelper::byteArrayToHexStr(data);
    }
    else
    {
        buffer = myHelper::byteArrayToAsciiStr(data);
    }

    appendTcpServer(1, QString("%1 [%2:%3]").arg(buffer).arg(ip).arg(port));

    if (App::DebugTcpServer)
    {
        int count = keys.count();

        for (int i = 0; i < count; i++)
        {
            if (keys.at(i) == buffer)
            {
                sendDataTcpServer(ip, port, values.at(i));
                break;
            }
        }
    }
#endif
}

void MainWindow::clientConnect(int , QString ip, int port)
{
    appendTcpServer(1, QString("客户端[%1:%2] 上线").arg(ip).arg(port));

    ui->listTcpClient->clear();


    QStringList ip_list = tcpServer->getClientInfo();

    ui->listTcpClient->addItems(ip_list);
    ui->labTcpClientCount->setText(QString("已连接客户端共 %1 个").arg(tcpServer->getClientCount()));

    int count = ui->listTcpClient->count();

    if (count > 0) {
        ui->listTcpClient->setCurrentRow(count - 1);
    }
}

void MainWindow::clientDisConnect(int , QString ip, int port)
{
    appendTcpServer(1, QString("客户端[%1:%2] 下线").arg(ip).arg(port));

    ui->listTcpClient->clear();
    QStringList ip_list = tcpServer->getClientInfo();

    ui->listTcpClient->addItems(ip_list);
    ui->labTcpClientCount->setText(QString("已连接客户端共 %1 个").arg(tcpServer->getClientCount()));

    int count = ui->listTcpClient->count();

    if (count > 0) {
        ui->listTcpClient->setCurrentRow(count - 1);
    }
}

void MainWindow::udpServerReadData()
{
    QHostAddress senderServerIP;
    quint16 senderServerPort;
    QByteArray data;
    QString buffer;

    do {
        data.resize(udpServer->pendingDatagramSize());
        udpServer->readDatagram(data.data(), data.size(), &senderServerIP, &senderServerPort);

        if (App::HexReceiveUdpServer) {
            buffer = myHelper::byteArrayToHexStr(data);
        } else {
            buffer = myHelper::byteArrayToAsciiStr(data);
        }

#if QSHOWTEXTEDIT_EN
        QString str = QString("来自:%1[%2] ").arg(senderServerIP.toString()).arg(senderServerPort);
        appendUdpServer(1, str + buffer);
#endif

        if (App::DebugUdpServer) {
            int count = keys.count();

            for (int i = 0; i < count; i++) {
                if (keys.at(i) == buffer) {
                    sendDataUdpServer(values.at(i));
                    break;
                }
            }
        }
    } while (udpServer->hasPendingDatagrams());
}

void MainWindow::udpServerReadError()
{
    appendUdpServer(1, QString("发生错误,原因 : %1").arg(udpServer->errorString()));
}

void MainWindow::readSendData()
{
    QString fileName = QString("%1/%2").arg(AppPath).arg(App::SendFileName);
    QFile file(fileName);

    if (!file.exists()) {
        return;
    }

    ui->sendComboBox->clear();
    ui->sendComboBox->clear();
    ui->sendComboBox->clear();

    file.open(QFile::ReadOnly | QIODevice::Text);
    QTextStream in(&file);
    QString line;

    do {
        line = in.readLine();

        if (line != "") {
            ui->sendComboBox->addItem(line);
            ui->sendComboBox->addItem(line);
            ui->sendComboBox->addItem(line);
        }
    } while (!line.isNull());

    file.close();
}

void MainWindow::readDeviceData()
{
    QString fileName = QString("%1/%2").arg(AppPath).arg(App::DeviceFileName);
    QFile file(fileName);

    if (!file.exists()) {
        return;
    }

    file.open(QFile::ReadOnly | QIODevice::Text);
    QTextStream in(&file);
    QString line;

    do {
        line = in.readLine();

        if (line != "") {
            QStringList list = line.split(";");
            QString key = list.at(0);
            QString value;

            for (int i = 1; i < list.count(); i++) {
                value += QString("%1;").arg(list.at(i));
            }

            //去掉末尾分号
            value = value.mid(0, value.length() - 1);

            keys.append(key);
            values.append(value);
        }
    } while (!line.isNull());

    file.close();
}

void MainWindow::sendDataTcpServer()
{
    QString data = ui->sendComboBox->currentText();
    sendDataTcpServer(data);
}

void MainWindow::sendDataTcpServer(QString data)
{
    if (!tcpServer->isListening()) {
        return;
    }

    if (data.isEmpty()) {
        return;
    }

//    bool all = ui->ckAllTcpServer->isChecked();
    bool all = false;
    {
        QString str = ui->listTcpClient->currentIndex().data().toString();

        //没有一个连接则不用处理
        if (str.isEmpty()) {
            return;
        }

        QStringList list = str.split(":");
        QString ip = list.at(3);
        int port = list.at(4).toInt();

        QByteArray buffer;

        if (App::HexSendTcpServer) {
            buffer = myHelper::hexStrToByteArray(data);
        } else {
            buffer = myHelper::asciiStrToByteArray(data);
        }

        if (!all) {
            tcpServer->sendData(ip, port, buffer);
        } else {
            tcpServer->sendData(buffer);
        }
    }
#if QSHOWTEXTEDIT_EN
    appendTcpServer(0, data);
#endif
}

void MainWindow::sendDataTcpServer(QString ip, int port, QString data)
{
    if (!tcpServer->isListening()) {
        return;
    }

    QByteArray buffer;

    if (App::HexSendTcpServer) {
        buffer = myHelper::hexStrToByteArray(data);
    } else {
        buffer = myHelper::asciiStrToByteArray(data);
    }

    tcpServer->sendData(ip, port, buffer);
#if QSHOWTEXTEDIT_EN
    appendTcpServer(0, data);
#endif
}

void MainWindow::sendDataTcpClient()
{
    QString data = ui->sendComboBox->currentText();
    sendDataTcpClient(data);
}

void MainWindow::sendDataTcpClient(QString data)
{
    if (!tcpClient->isWritable()) {
        return;
    }

    if (data.isEmpty()) {
        return;
    }

    if (!ui->sendPushButton->isEnabled()) {
        return;
    }

    QByteArray buffer;

    if (App::HexSendTcpClient) {
        buffer = myHelper::hexStrToByteArray(data);
    } else {
        buffer = myHelper::asciiStrToByteArray(data);
    }

    tcpClient->write(buffer);

#if QSHOWTEXTEDIT_EN
    appendTcpClient(0, data);
#endif
}

//index = 0 -> tcp/ip;
//index = 1 -> serial;
void MainWindow::on_configTabWidget_currentChanged(int index)
{
    if(ui->connectPushButton->text() == "断开")
        on_connectPushButton_clicked();
    //tcp_serial_init();

    if(index == 1)//serier
    {
//        ui->sendProgressBar->setEnabled(true);
//        ui->firmwareFileUpgrade->setEnabled(true);
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        {
#if QDEBUG_EN
            qDebug() << "Name        : " << info.portName();
            qDebug() << "Description : " << info.description();
            qDebug() << "Manufacturer: " << info.manufacturer();
#endif
            if(ui->serialPortCBox->currentText() == info.portName())
            {
                this->setWindowTitle(QString("Serial-Tool 连接信息[%1]").arg(info.portName() + " " + info.description()));
                break;
            }
        }
    }
    else//tcp
    {
        QString localHostName = QHostInfo::localHostName();
        QList<QHostAddress> addrs = QHostInfo::fromName(localHostName).addresses();
        QString ip;
        foreach (QHostAddress addr, addrs)
        {
            if (addr.protocol() == QAbstractSocket::IPv4Protocol)
            {
                ip = addr.toString();
                this->setWindowTitle(QString("Net-Tool 本机IP[%1]").arg(ip));
#if QDEBUG_EN
                qDebug() << ip;
#endif
                break;
            }
        }
//        ui->sendProgressBar->setEnabled(false);
//        ui->firmwareFileUpgrade->setEnabled(false);
    }
}

void MainWindow::on_connectPushButton_clicked()
{
    if(ui->connectPushButton->text() == "连接") //非连接状态
    {
        ui->sendPushButton->setEnabled(true);

        if(communication_type == SERIAL_TYPE)
        {
            my_serialport = new QSerialPort(this);
            my_serialport->setPortName(ui->serialPortCBox->currentText());          //设置串口号

            //以读写方式打开串口
            if(my_serialport->open(QIODevice::ReadWrite))
            {
                //my_serialport->write("open serial\n", 12);
                my_serialport->setBaudRate(ui->baudRateCBox->currentText().toInt());    //设置波特率
                my_serialport->setDataBits(QSerialPort::Data8);                         //设置数据位
                my_serialport->setParity(QSerialPort::NoParity);                        //设置校验位
                my_serialport->setFlowControl(QSerialPort::NoFlowControl);              //设置流控制
                my_serialport->setStopBits(QSerialPort::OneStop);                       //设置停止位

                my_serialport->clearError();
                my_serialport->clear();

                connect(my_serialport, SIGNAL(readyRead()), this, SLOT(readComDataSlot()));
#if QDEBUG_EN
                qDebug() << ui->serialPortCBox->currentText() <<  " 串口打开！";
#endif
                ui->connectPushButton->setText("断开");
                ui->serialPortCBox->setEnabled(false);
            }
            else
            {
                QMessageBox::about(NULL, "提示", "串口没有打开！可能已被占用！");
#if QDEBUG_EN
                qDebug() << "串口没有打开！";
#endif
                return;
            }
        }
        else if(communication_type == TCP_IP_TYPE)
        {
            if (ui->tcpTypeComboBox->currentIndex() == 0)
            {
                if (ui->connectPushButton->text() == "连接")
                {
                    bool ok = tcpServer->listen(QHostAddress::Any, ui->tcpPortLineEdit->text().toInt());

                    if (ok)
                    {
                        ui->connectPushButton->setText("断开");
                        ui->sendPushButton->setEnabled(true);
                        appendTcpServer(0, "监听成功");
                    }
                    else
                    {
                        appendTcpServer(1, "监听失败,请检查端口是否被占用");
                    }
                }
            }
            if (ui->tcpTypeComboBox->currentIndex() == 1)
            {
                if (ui->connectPushButton->text() == "连接")
                {
                    tcpClient->connectToHost(ui->tcpIPLineEdit->text(), ui->tcpPortLineEdit->text().toInt());

                    if (tcpClient->waitForConnected(1000))
                    {
                        ui->connectPushButton->setText("断开");
                        ui->connectPushButton->setEnabled(true);
                        appendTcpClient(0, "连接服务器成功");
                    } else
                    {
                        appendTcpClient(1, "连接服务器失败");
                    }
                }
            }
            if (ui->tcpTypeComboBox->currentIndex() == 2)
            {
                bool ok = udpServer->bind(App::UdpListenPort);

                if (ok)
                {
                    ui->connectPushButton->setText("断开");
                    ui->sendPushButton->setEnabled(true);
                    appendUdpServer(0, "监听成功");
                }
                else
                {
                    appendUdpServer(1, "监听失败,请检查端口是否被占用");
                }
            }
            if(ui->connectPushButton->text() == "断开")
                ui->tcpTypeComboBox->setEnabled(false);
#if QDEBUG_EN
            qDebug() << "tcp connect!\n";
#endif
        }
    }
    else if(ui->connectPushButton->text() == "断开")    //正在连接状态,->>断开
    {
        ui->sendPushButton->setEnabled(false);

        if(communication_type == SERIAL_TYPE)
        {
            if(my_serialport->isOpen())
            {
                ui->connectPushButton->setText("连接");
                my_serialport->close();
            }
            delete my_serialport;
            ui->serialPortCBox->setEnabled(true);
#if QDEBUG_EN
            qDebug() << "串口 已关闭！" << "\n";
#endif
        }
        else if(communication_type == TCP_IP_TYPE)
        {
            if (ui->tcpTypeComboBox->currentIndex() == 0)
            {
                ui->listTcpClient->clear();
                tcpServer->closeAll();
                ui->connectPushButton->setText("连接");
                ui->sendPushButton->setEnabled(false);
                appendTcpServer(0, "停止监听成功");
            }
            if (ui->tcpTypeComboBox->currentIndex() == 1)
            {
                tcpClient->disconnectFromHost();

                if (tcpClient->state() == QAbstractSocket::UnconnectedState || tcpClient->waitForDisconnected(1000))
                {
                    ui->connectPushButton->setText("连接");
                    ui->sendPushButton->setEnabled(false);
                    appendTcpClient(0, "断开连接成功");
                }
            }
            if (ui->tcpTypeComboBox->currentIndex() == 2)
            {
                udpServer->abort();
                ui->connectPushButton->setText("连接");
                ui->sendPushButton->setEnabled(false);
                appendUdpServer(0, "停止监听成功");
            }
            if(ui->connectPushButton->text() == "连接")
                ui->tcpTypeComboBox->setEnabled(true);
#if QDEBUG_EN
            qDebug() << "tcp close!\n";
#endif
        }
    }
}

/****************************数据传输******************************/
void MainWindow::readComDataSlot()
{
    myTimer_serial->start(5);
}
void MainWindow::read_serial_timer()
{
    myTimer_serial->stop();
#if QDEBUG_EN
    qDebug() << "read serial data timer";
#endif

    //读取串口数据
    QByteArray readComData = my_serialport->readAll();
    data_processing(readComData);

#if QSHOWTEXTEDIT_EN
    data_show(readComData);
#endif

    //清除缓冲区
    readComData.clear();
}
void MainWindow::data_show(QByteArray &readComData)
{
    //将读到的数据显示到数据接收区
    if(ui->showTextEdit->toPlainText().count() > 1000)
    {
        ui->showTextEdit->clear();
    }

    if(readComData != NULL)
    {
        if(ui->receiveCheckBox->isChecked() == false)   //字符显示
        {
            ui->showTextEdit->textCursor().insertText(readComData+"\n");
            ui->showTextEdit->moveCursor(QTextCursor::End);
        }
        else        //HEX显示
        {
            QDataStream out(&readComData,QIODevice::ReadWrite);    //将字节数组读入

            while(!out.atEnd())
            {
               qint8 outChar = 0;
               out>>outChar;   //每字节填充一次，直到结束
               //十六进制的转换
               QString str = QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0'));
               ui->showTextEdit->textCursor().insertText(str);
            }
        }
        ui->showTextEdit->moveCursor(QTextCursor::End);
        //qDebug() << readComData << "\n";
    }
}


void MainWindow::data_processing(const QByteArray receive_tmp, const QString ip)
{
    data_processing(receive_tmp);
}
void MainWindow::data_processing(const QByteArray receive_tmp)
{

}


void MainWindow::data_save(QByteArray receive_tmp)
{
    QString file_name("syslog_" + current_time + ".log");

    QFile file(file_name);
#if QDEBUG_EN
    qDebug() << file_name;
#endif
    if(!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append))
    {
#if QDEBUG_EN
        qDebug()<<"Can't open the file!"<<endl;
#endif
        return ;
    }

    QTextStream stream(&file);

    stream.seek(file.size());//将当前读取文件指针移动到文件末尾

    QDataStream out(&file);
    for(int i=0; i<receive_tmp.length(); i++)
        out << (unsigned char)receive_tmp[i];
    file.close();
}

qint64 MainWindow::serialport_write(const char *data, qint64 len)
{
    if(my_serialport != nullptr)
        if(true == my_serialport->isOpen())
            return my_serialport->write(data, len);
    return 0;
}

void MainWindow::on_sendPushButton_clicked()
{
    QString str = ui->sendComboBox->currentText();
    QByteArray ba = QByteArray(str.toLatin1());

    if(ui->connectPushButton->text() == "断开")
    {
        if(communication_type == SERIAL_TYPE)
        {
            if(ui->sendCheckBox->isChecked() == false)
            {
                if(my_serialport != nullptr)
                    if(true == my_serialport->isOpen())
                        my_serialport->write(ba.data(), ba.length());
            }
            else
            {
                QByteArray senddata;
                myHelper::String2Hex(str,senddata);

                //发送数据
                if(my_serialport != nullptr)
                    if(true == my_serialport->isOpen())
                        my_serialport->write(senddata, senddata.length());
            }
        }
        else if(communication_type == TCP_IP_TYPE)
        {
            if(ui->sendCheckBox->isChecked() == false)
            {
                if(ui->tcpTypeComboBox->currentIndex() == 0)
                    sendDataTcpServer(ba.data());
                if(ui->tcpTypeComboBox->currentIndex() == 1)
                    sendDataTcpClient(ba.data());
                if(ui->tcpTypeComboBox->currentIndex() == 2)
                    sendDataUdpServer(ba.data());
            }
            else
            {
                QByteArray senddata;
                myHelper::String2Hex(str,senddata);

                if(ui->tcpTypeComboBox->currentIndex() == 0)
                    sendDataTcpServer(senddata);
                if(ui->tcpTypeComboBox->currentIndex() == 1)
                    sendDataTcpClient(senddata);
                if(ui->tcpTypeComboBox->currentIndex() == 2)
                    sendDataUdpServer(senddata);
            }
        }
    }
}

void MainWindow::on_dataSavePushButton_clicked()
{

}

void MainWindow::on_tcpTypeComboBox_currentIndexChanged(int index)
{
    if(index == 0)
    {
        ui->tcpIpLabel->setText("本地IP");
        ui->tcpPortLabel->setText("本地端口号");
    }
    else if(index == 1)
    {
        ui->tcpIpLabel->setText("远端IP");
        ui->tcpPortLabel->setText("远端端口号");
    }
    else if(index == 2)
    {
        ui->tcpIpLabel->setText("IP");
        ui->tcpPortLabel->setText("端口号");
    }
    else
    {
        ui->tcpIpLabel->setText("本地IP");
        ui->tcpPortLabel->setText("本地端口号");
    }
}

void MainWindow::on_timerCheckBox_clicked(bool checked)
{
    if(communication_type == SERIAL_TYPE)
    {

    }
    if(communication_type == TCP_IP_TYPE)
    {
        if (checked)
        {
            if(ui->connectPushButton->text() == "断开")
            {
                if(ui->tcpTypeComboBox->currentIndex() == 0)
                    timerTcpServer->start();
                if(ui->tcpTypeComboBox->currentIndex() == 1)
                    timerTcpClient->start();
                if(ui->tcpTypeComboBox->currentIndex() == 2)
                    timerUdpServer->start();
#if QDEBUG_EN
                qDebug() << "tcp timer start! \n";
#endif
            }
        }
        else
        {
            if(ui->tcpTypeComboBox->currentIndex() == 0)
                timerTcpServer->stop();
            if(ui->tcpTypeComboBox->currentIndex() == 1)
                timerTcpClient->stop();
            if(ui->tcpTypeComboBox->currentIndex() == 2)
                timerUdpServer->stop();
#if QDEBUG_EN
            qDebug() << "tcp timer stop! \n";
#endif
        }
    }
}

void MainWindow::on_clear_pushButton_clicked()
{
    ui->showTextEdit->clear();
}

void MainWindow::on_quit_pushButton_clicked()
{
    close();
}

void MainWindow::on_serialPortCBox_currentIndexChanged(const QString &arg1)
{
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
#if QDEBUG_EN
        qDebug() << "Name        : " << info.portName();
        qDebug() << "Description : " << info.description();
        //qDebug() << "Manufacturer: " << info.manufacturer();
#endif
        if(arg1 == info.portName())
        {
            this->setWindowTitle(QString("Serial-Tool 连接信息[%1]").arg(info.portName() + " " + info.description()));
            break;
        }
    }
}

void MainWindow::on_skinButton_clicked()
{
    if(skin_mode == my_skin::white)
    {
        CommonHelper::setStyle(":/style/White/white.qss");
        skin_mode = my_skin::black;
    }
    else if(skin_mode == my_skin::black)
    {
        CommonHelper::setStyle(":/style/Black/black.qss");
        skin_mode = my_skin::white;
    }
}
