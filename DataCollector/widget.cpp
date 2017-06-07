#include "widget.h"
#include "setting.h"
#include "qcustomplot.h"
#include "mythread.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>
#include <QTime>
#include <QDebug>
#include <QBitArray>
#include <QMessageBox>
#include <QByteArray>
# pragma warning (disable:4819)

Widget::Widget(QWidget *parent) : QWidget(parent),
                                  ui(new Ui::Widget)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()& ~Qt::WindowMaximizeButtonHint);
    setFixedSize(this->width(), this->height());
    myStatusbar = new QStatusBar(this);
    ui->verticalLayout ->addWidget(myStatusbar);
    myStatusbar->showMessage("Hello Guys,Thanks for casually controlling",3000);
    drawplot();
    Time_CP = new QTimer(this);
    Timer_UPDATE = new QTimer(this);
    Timer_AutoSend = new QTimer(this);
    connect(Timer_UPDATE, SIGNAL(timeout()), this, SLOT(repaint()));
    Timer_UPDATE->start(1000);

    QString text = QString::number(float(600)/10,'f',1).append("C") ;
    ui->Aim_Temp_num->display(text);
    text = "0C";
    ui->current_Temp_num->display(text);
    ui->Controller_Mode_Status->setStyleSheet("color:Blue;");
    ui->DrawWindows->setBackground(QColor(240, 240, 240));
    Aim_Line_Update(ui->Aim_Temp_Slider->value());
    ui->DrawWindows->graph(1)->rescaleAxes();
    ui->DrawWindows->yAxis->setRange(ui->Aim_Temp_Slider->value()/10-50,50+30,Qt::AlignLeft);
    ui->DrawWindows->replot();

    time = new QTime(QTime::currentTime());
    workerThread = new WorkerThread(this);
    syncThread = new SyncThread(this);
    syncThread->sync_socket = new QWebSocket();
    syncThread->anonymous_socket = new QWebSocket();
    syncThread->anonymous_socket->open(QUrl(QString("ws://big7lion.win:19910")));
    syncThread->start();
//    syncThread->sync_socket->open(QUrl(QString("ws://big7lion.win:19910")));

    connect(&s,SIGNAL(Connect_Setting(QString,QString,QString)),syncThread,SLOT(Connect_Login(QString,QString,QString)));
    connect(&s,SIGNAL(Connect_Setting(QString,QString,QString)),workerThread,SLOT(slot_connect(QString,QString,QString)));
    connect(&s,SIGNAL(Connect_Close()),syncThread,SLOT(Connect_Close()));
    connect(&s,SIGNAL(List_fresh_command()),syncThread,SLOT(List_fresh_SLOT()));
    connect(syncThread,SIGNAL(Setting_sync_Get(QString,int)),this,SLOT(Sync_Set_func(QString,int)));
    connect(this,SIGNAL(emit_sync_setting(QString,int)),syncThread,SLOT(Setting_sync_Set(QString,int)));
    connect(syncThread,SIGNAL(Connect_Failed()),this,SLOT(socket_error()));
    connect(syncThread,SIGNAL(Connect_Failed()),&s,SLOT(socket_error()));
    connect(syncThread,SIGNAL(Connect_Success()),this,SLOT(socket_open()));
    connect(syncThread,SIGNAL(Connect_Closed()),this,SLOT(socket_close()));
    connect(syncThread,SIGNAL(Fresh_List(QJsonObject)),&s,SLOT(Show_List(QJsonObject)));
    connect(this,SIGNAL(A_DB_connect(bool)),workerThread,SLOT(A_DB_connect_SLOT(bool)));
}

Widget::~Widget()
{
    if(my_serialPort->isOpen())
    {
        my_serialPort->close();
    }
    qDebug()<<"close socket";
    qDebug()<<"~widget";
    s.reject();
    delete ui;
    qDebug()<<"~widget";
}

void Widget::closeEvent(QCloseEvent *event)
{
    //|窗口关闭之前需要的操作~
    syncThread->sync_socket->close();
    syncThread->anonymous_socket->close();
    qDebug()<<"close event";
}

void Widget::on_PushButtonSendData_clicked()
{
    if(Check_Serial_Open())
    {
        if (ui->SendData_HEX->isChecked())
            writeHex(ui->SendData);
        else if (!(ui->SendData_HEX->isChecked()))
            writeChr(ui->SendData);
    }
}

void Widget::writeHex(QTextEdit *textEdit)
{
    QString dataStr = textEdit->toPlainText().simplified().remove(QRegExp("\\s|(0x)|,"));
    if (dataStr.length() % 2)
    {
        dataStr.insert(dataStr.length() - 1, '0');
    }
    QByteArray sendData;
    StringToHex(dataStr, sendData);
    if (ui->SendData_NEW_LINE->isChecked())
    {
        sendData.append("\r\n");
    }
    my_serialPort->write(sendData);
}

void Widget::writeChr(QTextEdit *textEdit)
{
    QByteArray sendData = textEdit->toPlainText().toUtf8();
    if (ui->SendData_NEW_LINE->isChecked())
    {
        sendData.append("\r\n");
    }
    if (!sendData.isEmpty() && !sendData.isNull())
    {
        my_serialPort->write(sendData);
    }
}

void Widget::StringToHex(QString str, QByteArray &senddata)
{
    int hexdata, lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len / 2);
    char lstr, hstr;
    for (int i = 0; i < len;)
    {
        hstr = str[i].toLatin1();
        if (hstr == ' ')
        {
            ++i;
            continue;
        }
        ++i;
        if (i >= len)
            break;
        lstr = str[i].toLatin1();
        hexdata = ConvertHexChar(hstr);
        lowhexdata = ConvertHexChar(lstr);
        if ((hexdata == 16) || (lowhexdata == 16))
            break;
        else
            hexdata = hexdata * 16 + lowhexdata;
        ++i;
        senddata[hexdatalen] = (char)hexdata;
        ++hexdatalen;
    }
    senddata.resize(hexdatalen);
}

char Widget::ConvertHexChar(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
        return ch - 0x30;
    else if ((ch >= 'A') && (ch <= 'F'))
        return ch - 'A' + 10;
    else if ((ch >= 'a') && (ch <= 'f'))
        return ch - 'a' + 10;
    else
        return ch - ch;
}


void Widget::readMyCom()
{
    requestData = my_serialPort->readAll();
    while (my_serialPort->waitForReadyRead(100))
                      requestData += my_serialPort->readAll();
    QString buf;
    if (!requestData.isEmpty())
    {
        if ((unsigned char)requestData.at(0) == 0xAA)
        {
            qDebug() << "Yeah!";
            if(((unsigned char)requestData.at(1)== 'P')\
                    &&((unsigned char)requestData.at(2)== 'R')\
                    &&(requestData.length()>10))
            {
                ui->P_Slider->setValue((int)requestData.at(3)<<8|requestData.at(4));
                ui->I_Slider->setValue((int)requestData.at(5)<<8|requestData.at(6));
                ui->D_Slider->setValue((int)requestData.at(7)<<8|requestData.at(8));
                myStatusbar->showMessage("Note: PID Setting received!",3000);
            }
            else if(((unsigned char)requestData.at(1)== 'T'))
            {
                int currentTemp_data = (int)requestData.at(2)<<8 |(unsigned char)requestData.at(3);
                QString currentTemp_string;
                currentTemp_string = QString::number(float(currentTemp_data)/10,'f',1);
                if(ui->Local_Stream_Button->isChecked())
                {
                    Local_Stream_Write(currentTemp_string);
                }
                ui->current_Temp_num->display(currentTemp_string.append('C').insert(0,"   "));
                ui->current_Temp_Box->insertPlainText(currentTemp_string.append("  "));
                ui->current_Temp_Box->insertPlainText(QTime::currentTime().toString(" hh:mm:ss").append("\r\n"));
                ui->current_Temp_Box->moveCursor(QTextCursor::End);
                // calculate two new data points:
                if(Time_clear)
                {
                    time->restart();
                    Time_clear = 0;
                }
                key = time->elapsed()/1000.0; // time elapsed since start of demo, in seconds
                ui->DrawWindows->graph(0)->addData(key,(float)(currentTemp_data)/10);
                Curve_cnt++;
                ui->DrawWindows->graph(1)->addData(key+20,(float)ui->Aim_Temp_Slider->value()/10);
                if(ui->DrawWindows->xAxis->range().upper - key<(ui->DrawWindows->xAxis->range().upper-ui->DrawWindows->xAxis->range().lower)*0.2)
                {
                    ui->DrawWindows->xAxis->setRange(ui->DrawWindows->xAxis->range().lower+\
                                                     ui->DrawWindows->xAxis->range().size()*0.2,\
                                                     ui->DrawWindows->xAxis->range().size(),Qt::AlignLeft);
                }
                if(ui->DrawWindows->xAxis->range().upper - key<0)
                {
                    ui->DrawWindows->xAxis->setRange(key,\
                                                     ui->DrawWindows->xAxis->range().size(),Qt::AlignCenter);
                }
                ui->DrawWindows->replot();
                emit Write_Database_signal(ui->P_Slider->value(),ui->I_Slider->value(),ui->D_Slider->value(),float(currentTemp_data)/10);
                emit emit_sync_setting("alive-temp",currentTemp_data);
            }
        }
        else
        {
            qDebug() << "Wrong Data Received!!" << endl;
            myStatusbar->showMessage("Warning: NOT A readable combination of statistics",2000);
        }

        for (int i = 0; i < requestData.count(); i++)
        {
            QString s;
            s.sprintf("0x%02X, ", (unsigned char)requestData.at(i));
            buf += s;
        }
        if (ui->Read_Data_HEX->isChecked())
        {
            ui->ReadData->insertPlainText(buf);
            ui->ReadData->moveCursor(QTextCursor::End);
        }
        else if (!ui->Read_Data_HEX->isChecked())
        {
            ui->ReadData->insertPlainText(requestData);
            ui->ReadData->moveCursor(QTextCursor::End);
        }
        if(ui->ReadData->textCursor().position()>3000)
        {
            ui->ReadData->clear();
        }
    }
    requestData.clear();
}

void Widget::on_PushButtonClearData_clicked()
{
    requestData.clear();
    ui->ReadData->clear();
}

void Widget::on_pushButtonOpenSerial_toggled(bool checked)
{
    if (checked == true)
    {
        qDebug() << "Open the Serial " << ui->comboBoxChooseSerial->currentText();
        myStatusbar->showMessage(QString("Open the Serial %1").arg(ui->comboBoxChooseSerial->currentText()),3000);
        ui->pushButtonOpenSerial->setText(QString::fromUtf8("Open"));
        my_serialPort = new QSerialPort(this);
        my_serialPort->setPortName(ui->comboBoxChooseSerial->currentText());
        if (my_serialPort->open(QIODevice::ReadWrite))
        {
            my_serialPort->setFlowControl(QSerialPort::NoFlowControl);
            emit setBaudRate();
            emit setParity();
            emit setDataBits();
            emit setStopBits();
            DisableCombox();
            connect(Time_CP, SIGNAL(timeout()), this, SLOT(checkPort()));
            connect(my_serialPort, SIGNAL(readyRead()), this, SLOT(readMyCom()));
            Time_CP->start(1000);
        }
        else
        {
            QMessageBox::warning(this,
                                 QString::fromUtf8("please check"),
                                 QString::fromUtf8("No Serial exist,you may try again"),
                                 QMessageBox::Cancel);
            ui->pushButtonOpenSerial->setChecked(false);
            return;
        }
    }
    else if (checked == false)
    {
        qDebug() << "Close the Serial";
        myStatusbar->showMessage("Close the Serial",3000);
        if (Time_CP->isActive())
            Time_CP->stop();
        Time_CP->disconnect();
        if (my_serialPort->isOpen())
            my_serialPort->close();
        ui->pushButtonOpenSerial->setText(QString::fromUtf8("Close"));
        EnableCombox();
    }
}

void Widget::checkAvailablePorts()
{
    int index;
    if(QSerialPortInfo::availablePorts().isEmpty())
    {
        myStatusbar->showMessage("ERROR: No available Ports",3000);
    }
    foreach (const QSerialPortInfo &Info, QSerialPortInfo::availablePorts())
    {
        index = ui->comboBoxChooseSerial->findText(Info.portName());
        ui->comboBoxChooseSerial->setItemText(index, Info.portName());
        ui->comboBoxChooseSerial->setCurrentIndex(index);
        iVec.push_back(index);
    }
}

void Widget::on_pushButtonSearchSerial_clicked()
{
//    if (!iVec.isEmpty())
//    {
//        for (int i = 0; i != iVec.size(); ++i)
//        {
//            QString tempStr;
//            ui->comboBoxChooseSerial->setItemText(iVec[i], QString("COM") + tempStr.setNum(iVec[i] + 1));
//        }
//        ui->comboBoxChooseSerial->setCurrentIndex(0);
//        iVec.clear();
//    }
    emit checkAvailablePorts();
}

void Widget::checkPort()
{
    QSet<QString> portSet;
    foreach (const QSerialPortInfo &Info, QSerialPortInfo::availablePorts())
    {
        portSet.insert(Info.portName());
    }
    if (portSet.find(my_serialPort->portName()) == portSet.end())
    {
        QMessageBox::warning(this,
                             QString::fromUtf8("Application error"),
                             QString::fromUtf8("Fail with the following error : \nThe port is not available\nPort:%1")
                                 .arg(my_serialPort->portName()),
                             QMessageBox::Close);
        connect(this, SIGNAL(toggled(bool)), this, SLOT(on_pushButtonOpenSerial_toggled(bool)));
        emit on_pushButtonOpenSerial_toggled(false);
    }
}

void Widget::setBaudRate()
{
    if (ui->comboBoxBaudRate->currentText() == "115200")
        my_serialPort->setBaudRate(QSerialPort::Baud115200);
    else if (ui->comboBoxBaudRate->currentText() == "9600")
        my_serialPort->setBaudRate(QSerialPort::Baud9600);
    else if (ui->comboBoxBaudRate->currentText() == "1200")
        my_serialPort->setBaudRate(QSerialPort::Baud1200);
    else if (ui->comboBoxBaudRate->currentText() == "2400")
        my_serialPort->setBaudRate(QSerialPort::Baud2400);
    else if (ui->comboBoxBaudRate->currentText() == "4800")
        my_serialPort->setBaudRate(QSerialPort::Baud4800);
    else if (ui->comboBoxBaudRate->currentText() == "19200")
        my_serialPort->setBaudRate(QSerialPort::Baud19200);
    else if (ui->comboBoxBaudRate->currentText() == "38400")
        my_serialPort->setBaudRate(QSerialPort::Baud38400);
    else if (ui->comboBoxBaudRate->currentText() == "57600")
        my_serialPort->setBaudRate(QSerialPort::Baud57600);
}

void Widget::setParity()
{
    if (ui->comboBoxCheckBit->currentText() == "None")
        my_serialPort->setParity(QSerialPort::NoParity);
    else if (ui->comboBoxCheckBit->currentText() == "Odd")
        my_serialPort->setParity(QSerialPort::OddParity);
    else if (ui->comboBoxCheckBit->currentText() == "Even")
        my_serialPort->setParity(QSerialPort::EvenParity);
}

void Widget::setDataBits()
{
    if (ui->comboBoxDataBit->currentText() == "8")
        my_serialPort->setDataBits(QSerialPort::Data8);
    else if (ui->comboBoxDataBit->currentText() == "7")
        my_serialPort->setDataBits(QSerialPort::Data7);
    else if (ui->comboBoxDataBit->currentText() == "6")
        my_serialPort->setDataBits(QSerialPort::Data6);
    else if (ui->comboBoxDataBit->currentText() == "5")
        my_serialPort->setDataBits(QSerialPort::Data5);
}

void Widget::setStopBits()
{
    if (ui->comboBoxStopBit->currentText() == "1")
        my_serialPort->setStopBits(QSerialPort::OneStop);
    else if (ui->comboBoxStopBit->currentText() == "1.5")
        my_serialPort->setStopBits(QSerialPort::OneAndHalfStop);
    else if (ui->comboBoxStopBit->currentText() == "2")
        my_serialPort->setStopBits(QSerialPort::TwoStop);
}

void Widget::EnableCombox()
{
    ui->comboBoxChooseSerial->setEnabled(true);
    ui->comboBoxBaudRate->setEnabled(true);
    ui->comboBoxStopBit->setEnabled(true);
    ui->comboBoxDataBit->setEnabled(true);
    ui->comboBoxCheckBit->setEnabled(true);
}

void Widget::DisableCombox()
{
    ui->comboBoxChooseSerial->setEnabled(false);
    ui->comboBoxBaudRate->setEnabled(false);
    ui->comboBoxStopBit->setEnabled(false);
    ui->comboBoxDataBit->setEnabled(false);
    ui->comboBoxCheckBit->setEnabled(false);
}

void Widget::on_SaveData_clicked()
{
    if(fileName.isEmpty())
    {fileName = QFileDialog::getSaveFileName(this, tr("Save Data"), ".", tr("Text File (*.txt)"));}
    saveFile(fileName);
    myStatusbar->showMessage(QString::fromUtf8("Note: Serial received data has stored in %1").arg(fileName),5000);
}

bool Widget::saveFile(const QString &fileName)
{
    if (!writeFile(fileName))
    {
        return false;
    }
    return true;
}

bool Widget::writeFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::Append | QFile::Text))
    {
        QMessageBox::warning(this, tr("Save Data"),
                             tr("Cannot write file %1 : \n%2")
                                 .arg(file.fileName())
                                 .arg(file.errorString()));
        return false;
    }
    QTextStream out(&file);
    out << endl;
    out << QDate::currentDate().toString("yyyy-MM-dd") << QTime::currentTime().toString(" hh:mm:ss") << endl;
    out << ui->ReadData->toPlainText() << endl
        << endl;
    return true;
}

void Widget::open()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Choose Text File"), ".",
                                                    tr("Text File (*.txt)"));
    if (!fileName.isEmpty())
    {
        loadFile(fileName);
    }
}

bool Widget::loadFile(const QString &fileName)
{
    if (!readFile(fileName))
    {
        return false;
    }
    return true;
}

bool Widget::readFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Read failed"),
                             tr("Cannot read file %1 : \n%2.")
                                 .arg(file.fileName())
                                 .arg(file.errorString()));
        return false;
    }
    QTextStream in(&file);
    ui->ReadData->setText(in.readAll());
    return true;
}

void Widget::drawplot()
{
    // add two new graphs and set their look:
    ui->DrawWindows->addGraph();
    ui->DrawWindows->graph(0)->setPen(QPen(Qt::blue));                  // line color blue for first graph
    ui->DrawWindows->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20))); // first graph will be filled with translucent blue
    ui->DrawWindows->graph(0)->setLineStyle(QCPGraph::lsLine);
    ui->DrawWindows->graph(0)->setName("Temp Curve");
    ui->DrawWindows->addGraph();
    QPen Linepen;
    Linepen.setWidth(3);
    Linepen.setColor(Qt::red);
    Linepen.setStyle(Qt::DotLine);
    QPen legendpen;
    legendpen.setColor(QColor(20, 20, 20, 150));
    ui->DrawWindows->graph(1)->setPen(Linepen);
    ui->DrawWindows->graph(1)->setLineStyle(QCPGraph::lsLine);
    ui->DrawWindows->graph(1)->setName("Aim Temp Line");
    ui->DrawWindows->legend->setVisible(true);
    ui->DrawWindows->legend->setBrush(QColor(240, 240, 240, 150));
    ui->DrawWindows->legend->setBorderPen(legendpen);

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    ui->DrawWindows->xAxis->setTicker(timeTicker);
    ui->DrawWindows->axisRect()->setupFullAxesBox();

    // configure right and top axis to show ticks but no labels:
    // (see QCPAxisRect::setupFullAxesBox for a quicker method to do this)
    ui->DrawWindows->xAxis2->setVisible(true);
    ui->DrawWindows->xAxis2->setTickLabels(true);
    ui->DrawWindows->yAxis2->setVisible(true);
    ui->DrawWindows->yAxis2->setTickLabels(true);
    // make left and bottom axes always transfer their ranges to right and top axes:
    connect(ui->DrawWindows->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->DrawWindows->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->DrawWindows->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->DrawWindows->yAxis2, SLOT(setRange(QCPRange)));

    // let the ranges scale themselves so graph 0 fits perfectly in the visible area:
    ui->DrawWindows->graph(0)->rescaleAxes();
    // same thing for graph 1, but only enlarge ranges (in case graph 1 is smaller than graph 0):
    // Note: we could have also just called ui->DrawWindows->rescaleAxes(); instead
    // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
    ui->DrawWindows->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
}

void Widget::on_Control_Mode_dial_valueChanged(int value)
{
    quint8 Control_Mode_read = value;
    char Control_Mode_data[6];
    SendData_Init(Control_Mode_data,'K',sizeof(Control_Mode_data)/sizeof(quint8));
    if(2==Control_Mode_read)
    {
        ui->Controller_Mode_Status->setText("<html><head/><body><p align=\"center\">Auto</p></body></html>");
        ui->Controller_Mode_Status->setStyleSheet("color:Blue;");
        ui->Control_Mode_icon->setText("<html><head/><body><p><img src=\":/media_src_pack/media_src_pack/led-bulb.png\"/></p></body></html>");
        Control_Mode_data[2] = 'N';
        myStatusbar->showMessage("Note: Control Mode switch to Auto( PWM heating )",3000);
        syncThread->data_init_string = "option-PWM";
    }
    else if(3==Control_Mode_read)
    {
        ui->Controller_Mode_Status->setText("<html><head/><body><p align=\"center\">ON</p></body></html>");
        ui->Controller_Mode_Status->setStyleSheet("color:Red;");
        ui->Control_Mode_icon->setText("<html><head/><body><p><img src=\":/media_src_pack/media_src_pack/led-bulb (2).png\"/></p></body></html>");
        Control_Mode_data[2] = 'O';
        myStatusbar->showMessage("Note: Control Mode switch to ON( Always heating )",3000);
        syncThread->data_init_string = "option-heat";
    }
    else if(1==Control_Mode_read)
    {
        ui->Controller_Mode_Status->setText("<html><head/><body><p align=\"center\">OFF</p></body></html>");
        ui->Controller_Mode_Status->setStyleSheet("color:Black;");
        ui->Control_Mode_icon->setText("<html><head/><body><p><img src=\":/media_src_pack/media_src_pack/led-bulb (1).png\"/></p></body></html>");
        Control_Mode_data[2] = 'F';
        myStatusbar->showMessage("Note: Control Mode switch to OFF( No heating )",3000);
        syncThread->data_init_string = "option-close";
    }
    emit emit_sync_setting("option-sync",Control_Mode_read);
    if(Check_Serial_Open())
    {
        my_serialPort->write(Control_Mode_data,6);
    }
}

void Widget::on_Aim_Temp_Slider_valueChanged(int value)
{
    QString text = QString::number(float(value)/10,'f',1).append("C") ;
    ui->Aim_Temp_num->display(text);
    syncThread->data_init_string[3] = value;
    emit Aim_Line_Update(value);
}

void Widget::SendData_Init(char * mydata,quint8 funcflag,quint8 datalength)
{
    mydata[0] = (char)0xAA;
    mydata[1] = funcflag;
    mydata[datalength-2] = 13;
    mydata[datalength-1] = 10;
}

void Widget::on_PID_Read_Button_clicked()
{
    char PID_Read_data[14];
    if(Check_Serial_Open())
    {
        SendData_Init(PID_Read_data,'P',sizeof(PID_Read_data)/sizeof(char));
        PID_Read_data[2] = 'R';
        PID_Read_data[3] = 0x00;
        my_serialPort->write(PID_Read_data,sizeof(PID_Read_data)/sizeof(char));
        myStatusbar->showMessage("Note: Waiting for MCU callback",500);
    }
}

void Widget::on_PID_Send_Button_clicked()
{
    char PID_Send_data[14];
    if(Check_Serial_Open())
    {
        SendData_Init(PID_Send_data,'P',sizeof(PID_Send_data)/sizeof(char));
        PID_Send_data[2] = 'S';
        PID_Send_data[3] = (char)(ui->P_Slider->value()>>8);
        PID_Send_data[4] = (char)ui->P_Slider->value();
        PID_Send_data[5] = (char)(ui->I_Slider->value()>>8);
        PID_Send_data[6] = (char)ui->I_Slider->value();
        PID_Send_data[7] = (char)(ui->D_Slider->value()>>8);
        PID_Send_data[8] = (char)ui->D_Slider->value();
        PID_Send_data[9] = (char)5>>8;
        PID_Send_data[10] = (char)5;
        my_serialPort->write(PID_Send_data,sizeof(PID_Send_data)/sizeof(char));
        myStatusbar->showMessage("Note: PID Setting has delivered~",2000);
    }
}

void Widget::on_Temp_Set_Button_clicked()
{
    char Temp_Set_data[14];
    if(Check_Serial_Open())
    {
        SendData_Init(Temp_Set_data,'S',sizeof(Temp_Set_data)/sizeof(char));
        Temp_Set_data[2] = (char)(ui->Aim_Temp_Slider->value()>>8);
        Temp_Set_data[3] = (char)ui->Aim_Temp_Slider->value();
        my_serialPort->write(Temp_Set_data,sizeof(Temp_Set_data)/sizeof(char));
        myStatusbar->showMessage("Note: Aim Temperature Setting has delivered~",2000);
    }
}

void Widget::on_PWMcyc_Set_Button_clicked()
{
    char PWMcyc_Set_data[14];
    if(Check_Serial_Open())
    {
        SendData_Init(PWMcyc_Set_data,'L',sizeof(PWMcyc_Set_data)/sizeof(char));
        PWMcyc_Set_data[2] = (char)(ui->PWMcyc_Slider->value()>>8);
        PWMcyc_Set_data[3] = (char)ui->PWMcyc_Slider->value();
        my_serialPort->write(PWMcyc_Set_data,sizeof(PWMcyc_Set_data)/sizeof(char));
        myStatusbar->showMessage("Note: PWM Period Setting has delivered~",2000);
    }
}

void Widget::on_Tempcyc_Set_Button_clicked()
{
    char Tempcyc_Set_data[14];
    if(Check_Serial_Open())
    {
        SendData_Init(Tempcyc_Set_data,'T',sizeof(Tempcyc_Set_data)/sizeof(char));
        Tempcyc_Set_data[2] = (char)(ui->Readcyc_Slider->value()>>8);
        Tempcyc_Set_data[3] = (char)ui->Readcyc_Slider->value();
        my_serialPort->write(Tempcyc_Set_data,sizeof(Tempcyc_Set_data)/sizeof(char));
        myStatusbar->showMessage("Note: Temperature Reading Period Setting has delivered~",2000);
    }
}

void Widget::on_Local_Stream_Button_toggled(bool checked)
{
    if(true == checked)
    {
        if(csvPathName.isEmpty())
        {
            csvPathName = QFileDialog::getSaveFileName(this, tr("csv Stream store"), ".", tr("CSV File (*.csv)"));
        }
        ui->Local_Stream_Button->setText(QString::fromUtf8("Stream ON"));
        csvfile.setFileName(csvPathName);
        if (!csvfile.open(QIODevice::Append | QFile::Text))
        {
            QMessageBox::warning(this, tr("Save Data"),
                                 tr("Cannot write file %1 : \n%2")
                                     .arg(csvfile.fileName())
                                     .arg(csvfile.errorString()));
        }
        QTextStream out(&csvfile);
        out << "Current Temperature,parameter P,parameter I,parameter D, Current Time"<<endl;
        myStatusbar->showMessage(QString::fromUtf8("Note: PID and Temp data will stored in %1").arg(csvPathName),5000);
    }
    else
    {
        ui->Local_Stream_Button->setText(QString::fromUtf8("Stream OFF"));
        csvfile.close();
        myStatusbar->showMessage(QString::fromUtf8("Warning: Data Stream has closed"),5000);
    }
}

void Widget::Local_Stream_Write(QString currentTemp)
{
    QTextStream out(&csvfile);
    out << currentTemp <<','<< ui->P_Slider->value()<<','<<ui->I_Slider->value()\
        <<','<<ui->D_Slider->value()<<','<< QTime::currentTime().toString(" hh:mm:ss")\
        <<endl;
}

void Widget::on_Repaint_button_clicked()
{
    Time_clear = 1;
    ui->DrawWindows->graph(0)->data()->clear();
    ui->DrawWindows->graph(1)->data()->clear();
    ui->DrawWindows->graph(1)->addData(0,(float)ui->Aim_Temp_Slider->value()/10);
    ui->DrawWindows->graph(1)->addData(time->elapsed()/1000.0+200,(float)ui->Aim_Temp_Slider->value()/10);
    ui->DrawWindows->replot();
}

void Widget::on_pushButton_clicked()
{
    ui->DrawWindows->yAxis->setRange(ui->Aim_Temp_Slider->value()/10-50,50+30,Qt::AlignLeft);
    if(key>20)
    {
        ui->DrawWindows->xAxis->setRange(key-20,60,Qt::AlignLeft);
    }
    else
    {
        ui->DrawWindows->xAxis->setRange(key-10,key+40,Qt::AlignLeft);
    }
    ui->DrawWindows->replot();
}

void Widget::Aim_Line_Update(int value)
{
    ui->DrawWindows->graph(1)->data()->clear();
    ui->DrawWindows->graph(1)->addData(0,float(value)/10);
    ui->DrawWindows->graph(1)->addData(ui->DrawWindows->xAxis->range().center()+50,(float)value/10);
    ui->DrawWindows->replot();
}

bool Widget::Check_Serial_Open()
{
    if(my_serialPort!=NULL)
    {
        if(my_serialPort->isOpen())
        {
            return true;
        }
        else
        {
            myStatusbar->showMessage("ERROR: Please Open a Serialport",3000);
            return false;
        }
    }
    else
    {
        myStatusbar->showMessage("ERROR: Please Open a Serialport",3000);
        return false;
    }
}

void Widget::on_Connect_DB_success()
{
    ui->Connect_DB_Button->setText("Database Run");
    myStatusbar->showMessage("Note: Databases Connect success",3000);
}

void Widget::on_Connect_DB_Button_toggled(bool checked)
{
    if(true == checked)
    {
        if(!syncThread->connected_flag)
        {
            QMessageBox::warning(this,
                                 QString::fromUtf8("please login in first"),
                                 QString::fromUtf8("you may try again"),
                                 QMessageBox::Cancel);
            ui->Connect_DB_Button->setChecked(false);
            return;
        }
        //connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));
        connect(workerThread,SIGNAL(on_Connect_DB_success_signal()),this,SLOT(on_Connect_DB_success()));
        connect(workerThread,SIGNAL(finished()),this,SLOT(on_Connect_DB_close()));
        connect(this,SIGNAL(Write_Database_signal(int,int,int,float)),workerThread,SLOT(Write_Database(int,int,int,float)));
        connect(workerThread,SIGNAL(initRead_Database(int,int,int)),this,SLOT(Slider_val_set(int,int,int)));
        syncThread->data_init_string[6] = 1;
        workerThread->start();
        emit A_DB_connect(true);
        emit emit_sync_setting("DB-sync",(int)checked);
    }
    else
    {
        workerThread->stop();
        disconnect(this,SIGNAL(Write_Database_signal(int,int,int,float)),workerThread,SLOT(Write_Database(int,int,int,float)));
        disconnect(workerThread,SIGNAL(initRead_Database(int,int,int)),this,SLOT(Slider_val_set(int,int,int)));
        syncThread->data_init_string[6] = 0;
        emit emit_sync_setting("DB-sync",(int)checked);
    }
}

void Widget::on_Connect_DB_close()
{
    ui->Connect_DB_Button->setText("Database close");
    myStatusbar->showMessage("Note: Databases closed",3000);
}

void Widget::on_AutoSend_checkbox_toggled(bool checked)
{
    if(checked)
    {
        if(Check_Serial_Open())
        {
            myStatusbar->showMessage("Note: Serial AutoSend running~",3000);
            connect(Timer_AutoSend, SIGNAL(timeout()), this, SLOT(on_PushButtonSendData_clicked()));
            Timer_AutoSend->start(1000);
        }
        else
        {
            ui->AutoSend_checkbox->setChecked(false);
        }
    }
    else
    {
        if(Check_Serial_Open())
        {
            myStatusbar->showMessage("Note: Serial AutoSend stopped~",3000);
            disconnect(Timer_AutoSend, SIGNAL(timeout()), this, SLOT(on_PushButtonSendData_clicked()));
            Timer_AutoSend->stop();
        }
    }
}

void Widget::Slider_val_set(int P,int I,int D)
{
    ui->P_Slider->setValue(P);
    ui->I_Slider->setValue(I);
    ui->D_Slider->setValue(D);
}

void Widget::on_pushButtonOpenSetting_clicked()
{
    qDebug()<<workerThread->Thread_stop<<endl;
    s.show();
    s.exec();
}

void Widget::on_P_Slider_sliderReleased()
{
    qDebug()<<ui->P_Slider->value();
    emit emit_sync_setting("P-sync",ui->P_Slider->value());
}

void Widget::socket_close()
{
    myStatusbar->showMessage("Server connect close",3000);
}

void Widget::socket_error()
{
    myStatusbar->showMessage("Cannot connect Server!Please check you network!",3000);
}

void Widget::socket_open()
{
    myStatusbar->showMessage("Server connect success!Syncing~",3000);
}

void Widget::Sync_Set_func(QString command,int code)
{

    if(command == "P-sync")
    {
        ui->P_Slider->setValue(code);
    }
    else if(command == "I-sync")
    {
        ui->I_Slider->setValue(code);
    }
    else if(command == "D-sync")
    {
        ui->D_Slider->setValue(code);
    }
    else if(command == "Aim-temp-sync")
    {
        ui->Aim_Temp_Slider->setValue(code);
    }
    else if(command == "PWM-cyc-sync")
    {
        ui->PWMcyc_Slider->setValue(code);
    }
    else if(command == "read-cyc-sync")
    {
        ui->Readcyc_Slider->setValue(code);
    }
    else if(command == "option-sync")
    {
        ui->Control_Mode_dial->setValue(code);
    }
    else if(command == "DB-sync")
    {
        ui->Connect_DB_Button->setChecked(code);
    }
    else if(command == "Set-AimTemp")
    {
        ui->Temp_Set_Button->click();
    }
    else if(command == "Set-PID")
    {
        ui->PID_Send_Button->click();
    }
    else if(command == "Set-PIDcyc")
    {
        ui->PWMcyc_Set_Button->click();
    }
    else if(command == "Set-Readcyc")
    {
        ui->Tempcyc_Set_Button->click();
    }
}

void WorkerThread::stop()
{
    Thread_stop = 1;
}

void WorkerThread::Write_Database(int P,int I,int D,float temp)
{
    qDebug()<< P << I << D << temp <<endl;
    P_data = P;
    I_data = I;
    D_data = D;
    Temp_data = temp;
    data_write_falg = 1;
}


void Widget::on_I_Slider_sliderReleased()
{
    qDebug()<<ui->I_Slider->value();
    emit emit_sync_setting("I-sync",ui->I_Slider->value());
}

void Widget::on_D_Slider_sliderReleased()
{
    qDebug()<<ui->D_Slider->value();
    emit emit_sync_setting("D-sync",ui->D_Slider->value());
}

void Widget::on_Aim_Temp_Slider_sliderReleased()
{
    qDebug()<<ui->Aim_Temp_Slider->value();
    emit emit_sync_setting("Aim-temp-sync",ui->Aim_Temp_Slider->value());
}

void Widget::on_PWMcyc_Slider_sliderReleased()
{
    qDebug()<<ui->PWMcyc_Slider->value();
    emit emit_sync_setting("PWM-cyc-sync",ui->PWMcyc_Slider->value());
}

void Widget::on_Readcyc_Slider_sliderReleased()
{
    qDebug()<<ui->Readcyc_Slider->value();
    emit emit_sync_setting("read-cyc-sync",ui->Readcyc_Slider->value());
}

void WorkerThread::slot_connect(QString ip,QString port,QString ID)
{
    current_ID = ID;
}

void WorkerThread::A_DB_connect_SLOT(bool flag)
{
    connect_flag = flag;
}

void Widget::on_P_Slider_valueChanged(int value)
{
    syncThread->data_init_string[0] = value;
}

void Widget::on_I_Slider_valueChanged(int value)
{
    syncThread->data_init_string[1] = value;
}

void Widget::on_D_Slider_valueChanged(int value)
{
    syncThread->data_init_string[2] = value;
}

void Widget::on_PWMcyc_Slider_valueChanged(int value)
{
    syncThread->data_init_string[4] = value;
}

void Widget::on_Readcyc_Slider_valueChanged(int value)
{
    syncThread->data_init_string[5] = value;
}
