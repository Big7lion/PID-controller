#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QWidget>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QThread>
#include <QDebug>
#include <QTime>
#include <QStringList>

class WorkerThread : public QThread
{
    Q_OBJECT

public:
    explicit WorkerThread(QObject *parent = 0)
        : QThread(parent)
    {
        qDebug() << "Worker Thread : " << QThread::currentThreadId();
    }

public:
    QSqlDatabase db;
    bool Thread_stop = 0;
    bool data_write_falg = 0;
    void stop();

protected:
    virtual void run() Q_DECL_OVERRIDE {
        qDebug() << "Worker Run Thread : " << QThread::currentThreadId();
        Thread_stop = 0;
        db = QSqlDatabase::addDatabase("QMYSQL","A_DB");
        db.setHostName("35.164.254.201");
        db.setDatabaseName("Temp_Controller_Data");
        db.setUserName("root");
        db.setPassword("cjy24283");
        bool ok = db.open();
        if(ok)
        {
            qDebug() << endl<<"connect to root mysql admin"<<endl;
            query = QSqlQuery(QSqlDatabase::database("A_DB"));
            emit on_Connect_DB_success_signal();
        }
        else
        {
            qDebug() <<endl<< "Connect failed!!!"<<endl;
        }
        while(!connect_flag)
        {
            msleep(5);
        }
        QString query_string;
        QStringList strTables = db.tables();
        if(!strTables.contains(current_ID))
        {
            qDebug()<<"no ID table exist";
            query_string = QString("create table IF NOT EXISTS %1(ID INT NOT NULL,TEMP VARCHAR(20) NOT NULL,P INT NOT NULL,I INT NOT NULL,D INT NOT NULL,TIME VARCHAR(30),PRIMARY KEY (ID))").arg(current_ID);
            qDebug()<<query_string;
            if(query.exec(query_string))
            {
                qDebug()<<"create ID table";
            }
            else
            {
                qDebug()<<query.lastError();
            }
            query.clear();
            query_string = QString("alter table %1 modify ID INT auto_increment").arg(current_ID);
            qDebug()<<query_string;
            if(query.exec(query_string))
            {
                qDebug()<<"set ID index";
            }
            else
            {
                qDebug()<<query.lastError();
            }
            query.clear();
        }
        else{
            qDebug()<<"ID table exist!!!!";
        }
        while(!Thread_stop)
        {
            msleep(5);
            if(data_write_falg)
            {
                query.prepare(QString("insert into %1 (`TEMP`, `P`, `I`, `D`, `TIME`) values(?,?,?,?,?)").arg(current_ID));
                query.bindValue(1,P_data);
                query.bindValue(2,I_data);
                query.bindValue(3,D_data);
                query.bindValue(0,Temp_data);
                query.bindValue(4,QTime::currentTime().toString(" hh:mm:ss"));
                success = query.exec();
                if(!success){
                    qDebug() << "²åÈëÊ§°Ü£º" <<endl;
                }
                data_write_falg = 0;
            }
        }
        db.close();
        qDebug()<<"Databases closed!"<<endl;
    }

signals:
    void on_Connect_DB_success_signal();
    void initRead_Database(int P,int I,int D);
public slots:
    void Write_Database(int P,int I,int D,float temp);
    void slot_connect(QString ip,QString port,QString ID);
    void A_DB_connect_SLOT(bool flag);
private:
    int P_data;
    int I_data;
    int D_data;
    float Temp_data;
    QSqlQuery query;
    QTime CurrentTime;
    bool success;
    QString current_ID;
    bool connect_flag=0;
};

#endif // MYTHREAD_H
