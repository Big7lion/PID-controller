#ifndef WIDGET_H
#define WIDGET_H

#include "ui_widget.h"
#include "setting.h"
#include <QWidget>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>
#include <QDebug>
#include <QTime>
#include <QStatusBar>
#include <QFile>
#include <QFileDialog>
#include <QDataStream>
#include <QStatusBar>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "mythread.h"
#include "syncthread.h"
#include <QCloseEvent>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

protected:
     void closeEvent(QCloseEvent *event);

signals:
    void Write_Database_signal(int P,int I,int D,float temp);
    void emit_sync_setting(QString,int);
    void A_DB_connect(bool);

public slots:
    void Slider_val_set(int P,int I,int D);
    void socket_open();
    void socket_close();
    void socket_error();

private slots:
    void on_PushButtonSendData_clicked();

    void readMyCom();

    void drawplot();

    void on_PushButtonClearData_clicked();

    void on_pushButtonSearchSerial_clicked();

    void checkAvailablePorts();

    void checkPort();

    void on_pushButtonOpenSerial_toggled(bool checked);

    void setBaudRate();

    void setParity();

    void setDataBits();

    void setStopBits();

    void EnableCombox();

    void DisableCombox();

    void writeHex(QTextEdit *textEdit);

    void writeChr(QTextEdit *textEdit);

    void StringToHex(QString str, QByteArray &senddata);

    char ConvertHexChar(char ch);

    void on_SaveData_clicked();

    bool saveFile(const QString &fileName);

    bool writeFile(const QString &fileName);

    void open();

    bool loadFile(const QString &fileName);

    bool readFile(const QString &fileName);

    void on_Control_Mode_dial_valueChanged(int value);

    void on_Aim_Temp_Slider_valueChanged(int value);

    void SendData_Init(char * mydata,quint8 funcflag,quint8 datalength);

    void on_PID_Read_Button_clicked();

    void on_PID_Send_Button_clicked();

    void on_Temp_Set_Button_clicked();

    void on_PWMcyc_Set_Button_clicked();

    void on_Tempcyc_Set_Button_clicked();

    void on_Local_Stream_Button_toggled(bool checked);

    void Local_Stream_Write(QString currentTemp);

    void on_Repaint_button_clicked();

    void on_pushButton_clicked();

    void Aim_Line_Update(int value);

    bool Check_Serial_Open();

    void on_Connect_DB_Button_toggled(bool checked);

    void on_Connect_DB_success();

    void on_Connect_DB_close();

    void on_AutoSend_checkbox_toggled(bool checked);

    void on_pushButtonOpenSetting_clicked();

    void on_P_Slider_sliderReleased();

    void Sync_Set_func(QString command,int code);

    void on_I_Slider_sliderReleased();

    void on_D_Slider_sliderReleased();

    void on_Aim_Temp_Slider_sliderReleased();

    void on_PWMcyc_Slider_sliderReleased();

    void on_Readcyc_Slider_sliderReleased();

    void on_P_Slider_valueChanged(int value);

    void on_I_Slider_valueChanged(int value);

    void on_D_Slider_valueChanged(int value);

    void on_PWMcyc_Slider_valueChanged(int value);

    void on_Readcyc_Slider_valueChanged(int value);

private:
    Ui::Widget *ui;
    QSerialPort *my_serialPort = NULL;//(实例化一个指向串口的指针，可以用于访问串�
    QByteArray requestData;//（用于存储从串口那读取的数据�
    QTimer *timer;//（用于计时）
    QTimer *Time_CP;
    QTimer *Timer_UPDATE;
    QTimer *Timer_AutoSend;
    QSet<QString> portSet;
    QVector<int> iVec;
    QTime CurrentTime;
    QString fileName;
    QString csvPathName;
    QFile csvfile;
    QStatusBar *myStatusbar;
    int Curve_cnt = 0;
    double key;
    QTime *time = NULL;
    bool Time_clear = 1;
    WorkerThread *workerThread;
    SyncThread *syncThread;
    Dialog s;
};

#endif // WIDGET_H
