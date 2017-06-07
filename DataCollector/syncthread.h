#ifndef SYNCTHREAD_H
#define SYNCTHREAD_H
#include <QWidget>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QDebug>
#include <QTime>
#include <QtWebSockets/QtWebSockets>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>

class SyncThread : public QThread
{
    Q_OBJECT

public:
    explicit SyncThread(QObject *parent = 0)
        : QThread(parent)
    {
        qDebug() << "websocket Thread : " << QThread::currentThreadId();
    }
    QSqlDatabase setting_db;
    QWebSocket *sync_socket;
    QWebSocket *anonymous_socket;
    bool connected_flag;
    void stop();
    int data_init_buff[7];
    QString data_init_string;

public slots:
//    void Setting_sync_Store(QString command,int data);
    void Connect_Login(QString IP,QString port,QString ID);
    void Connect_Close();
    void onConnected();
    void onError(QAbstractSocket::SocketError error);
    void onTextMessageReceived(QString message);
    void Setting_sync_Set(QString command,int data);
    void List_fresh_SLOT();
    void anonymous_Message(QString message);
    void anonymous_connect();

signals:
    void Connect_Failed();
    void Fresh_List(QJsonObject);
    void Setting_sync_Get(QString,int);
    void Connect_Success();
    void Connect_Closed();

protected:
    virtual void run() Q_DECL_OVERRIDE;

private:
    QUrl sync_url;
    QString current_ID;
    bool stop_flag;
    bool sync_action_flag;
    QSqlQuery sync_query;
    int current_data;
    QString command2db(QString command);
    int record_cnt;
    bool exec_ok;
    QString current_command;
    bool anonymous_flag;
};

#endif // SYNCTHREAD_H
