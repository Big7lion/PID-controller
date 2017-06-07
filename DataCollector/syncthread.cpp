#include "syncthread.h"


void SyncThread::run()
{
    connected_flag = 0;
    stop_flag = 0;
    anonymous_flag = 1;
    sync_action_flag = 0;
    data_init_buff[0] = 0;
    data_init_buff[1] = 0;
    data_init_buff[2] = 0;
    data_init_buff[3] = 600;
    data_init_buff[4] = 500;
    data_init_buff[5] = 200;
    data_init_buff[6] = 0;
    data_init_string = "option-PWM";
    qDebug() << "Worker Run Thread : " << QThread::currentThreadId();
    connect(sync_socket, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(sync_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
    connect(anonymous_socket, SIGNAL(connected()), this, SLOT(anonymous_connect()));

    setting_db = QSqlDatabase::addDatabase("QMYSQL","B_DB");
    setting_db.setHostName("35.164.254.201");
    setting_db.setDatabaseName("Temp_Controller_ID");
    setting_db.setUserName("root");
    setting_db.setPassword("cjy24283");
    bool ok = setting_db.open();
    if(ok)
    {
        qDebug() << endl<<"connect to root mysql admin"<<endl;
        sync_query = QSqlQuery(QSqlDatabase::database("B_DB"));
    }
    else
    {
        qDebug() <<endl<< "Connect failed!!!"<<endl;
        emit Connect_Failed();
        return;
    }
    while(!stop_flag)
    {
        while(!connected_flag)
        {
            msleep(5);
        }
        qDebug()<< "hhh1";
        sync_query.clear();
        sync_query.prepare("select count(*) from `parameter_data` where `user_ID`=?");
        sync_query.addBindValue(current_ID);
        sync_query.exec();
        qDebug()<< "hhh2";
        qDebug()<< current_ID;
        while(sync_query.next())
        {
            if(sync_query.isActive())
            {
                record_cnt = sync_query.value(0).toInt();
            }
            else
            {
                record_cnt = 0;
            }
        }
        qDebug()<<record_cnt;
        if(record_cnt == 0)
        {
            sync_query.clear();
            qDebug()<< current_ID;
            sync_query.prepare("insert into `parameter_data`(`user_ID`,`P_set`,`I_set`,`D_set`,`Aim_temp_set`,`PWM_cyc_set`,`read_cyc_set`,`DB_set`,`Mode_set`) values(?,?,?,?,?,?,?,?,?)");
            sync_query.bindValue(0,current_ID);
            sync_query.bindValue(1,data_init_buff[0]);
            sync_query.bindValue(2,data_init_buff[1]);
            sync_query.bindValue(3,data_init_buff[2]);
            sync_query.bindValue(4,data_init_buff[3]);
            sync_query.bindValue(5,data_init_buff[4]);
            sync_query.bindValue(6,data_init_buff[5]);
            sync_query.bindValue(7,data_init_buff[6]);
            sync_query.bindValue(8,data_init_string);
            if(!sync_query.exec()){
                qDebug() << "insert failed";
            }
            else{
                qDebug() << "insert success";
            }
        }
        else
        {
            qDebug()<<"dowmload setting";
            sync_query.clear();
            sync_query.prepare("select * from `parameter_data` where `user_ID`=?");
            sync_query.addBindValue(current_ID);
            exec_ok = sync_query.exec();
            if(!exec_ok){
                qDebug() << "download failed";
            }
            else{
                qDebug() << "dowmload success";
            }
            while(sync_query.next())
            {
                qDebug()<<sync_query.value(0).toString();
                emit Setting_sync_Get("P-sync",sync_query.value(1).toInt());
                emit Setting_sync_Get("I-sync",sync_query.value(2).toInt());
                emit Setting_sync_Get("D-sync",sync_query.value(3).toInt());
                emit Setting_sync_Get("Aim-temp-sync",sync_query.value(4).toInt());
                emit Setting_sync_Get("PWM-cyc-sync",sync_query.value(5).toInt());
                emit Setting_sync_Get("read-cyc-sync",sync_query.value(6).toInt());
                emit Setting_sync_Get("DB-sync",sync_query.value(7).toInt());
                if(sync_query.value(8).toString() == "option-close")
                {
                    emit Setting_sync_Get("option-sync",1);
                }
                else if(sync_query.value(8).toString() == "option-PWM")
                {
                    emit Setting_sync_Get("option-sync",2);
                }
                else if(sync_query.value(8).toString() == "option-heat")
                {
                    emit Setting_sync_Get("option-sync",3);
                }
            }
            qDebug()<<"dowmload finished!";
        }
        while(connected_flag)
        {
            msleep(5);
            if(sync_action_flag)
            {
                sync_action_flag = 0;
                sync_query.clear();
                QString query_string = QString("UPDATE `parameter_data` SET `%1` = :d2 WHERE `user_ID`=:d3").arg(command2db(current_command));
                sync_query.prepare(query_string);
                if(current_command == "option-sync")
                {
                    switch (current_data)
                    {
                        case 1:{sync_query.bindValue(":d2","option-close");break;}
                        case 2:{sync_query.bindValue(":d2","option-PWM");break;}
                        case 3:{sync_query.bindValue(":d2","option-heat");break;}
                    }
                }
                else
                {
                    sync_query.bindValue(":d2",current_data);
                }
                sync_query.bindValue(":d3",current_ID);
                exec_ok = sync_query.exec();
                if(!exec_ok)
                {
                    qDebug()<<"update error!";
                    qDebug()<<sync_query.lastQuery();
                    qDebug()<<sync_query.lastError();
                }
                else
                {
                    qDebug()<<"update success!";
                }
            }
        }
        qDebug()<<"DB connect closed!"<<endl;
    }
    setting_db.close();
    qDebug()<<"Websocket&DB thread stoped!"<<endl;
}

void SyncThread::Connect_Login(QString IP,QString port,QString ID)
{
    disconnect(anonymous_socket, SIGNAL(textMessageReceived(const QString)),
                    this, SLOT(anonymous_Message(QString)));
    anonymous_socket->close();
    QString sUrl = "ws://"+IP+":"+port;
    sync_url = QUrl(sUrl);
    current_ID = ID;
    qRegisterMetaType<QAbstractSocket::SocketState>();
    sync_socket->open(sync_url);
    qDebug()<<"ws://"+IP+":"+port;
}

void SyncThread::Connect_Close()
{
    sync_socket->close();
    anonymous_socket->open(QUrl(QString("ws://big7lion.win:19910")));
    qDebug() << "Close";
    emit Connect_Closed();
    connected_flag = 0;
//    anonymous_flag = 1;
//    qRegisterMetaType<QAbstractSocket::SocketState>();
//    sync_socket->open(QUrl(QString("ws://big7lion.win")));
}

void SyncThread::onError(QAbstractSocket::SocketError error)
{
//    qDebug() << error;
    qDebug() << "error";
    emit Connect_Failed();
    connected_flag = 0;
}

void SyncThread::onConnected()
{
    connect(sync_socket, SIGNAL(textMessageReceived(const QString)),
                    this, SLOT(onTextMessageReceived(QString)));
    qDebug() << "Open!";
    QJsonObject json;
    json.insert("user_ID", current_ID);
    json.insert("user_command", QString("Login In"));
    json.insert("user_platform", QString("QT Client"));
    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString json_str(byte_array);
    qDebug() << json_str;
    sync_socket->sendTextMessage(json_str);
    emit Connect_Success();
    connected_flag = 1;
}

void SyncThread::onTextMessageReceived(QString message)
{
    //qDebug()<<message;
    QByteArray message_buf = message.toUtf8();
    QJsonDocument message_data = QJsonDocument::fromJson(message_buf);
    int code_flag;
    if(message_data.isObject())
    {
        QJsonObject message_obj = message_data.object();
        if(message_obj.value("user_command").toString() == "List fresh")
        {
            emit Fresh_List(message_obj);
            qDebug()<<"received list info";
            return;
        }
        else if(message_obj.value("user_command").toString() == "setting sync")
        {
            if(message_obj.value("user_ID").toString() == current_ID)
            {
                if(message_obj.value("command_code").toString() == "option-sync")
                {
                    if(message_obj.value("user_data").toString() == "option-close")
                    {
                        code_flag = 1;
                    }
                    else if(message_obj.value("user_data").toString() == "option-PWM")
                    {
                        code_flag = 2;
                    }
                    else if(message_obj.value("user_data").toString() == "option-heat")
                    {
                        code_flag = 3;
                    }
                }
                else if(message_obj.value("command_code").toString() == "Aim-temp-sync")
                {
                    code_flag = message_obj.value("user_data").toDouble()*10;
                }
                else
                {
                    code_flag = message_obj.value("user_data").toInt();
                }
                emit Setting_sync_Get(message_obj.value("command_code").toString(),code_flag);
                qDebug()<<message_obj.value("command_code").toString()<<code_flag;
                return;
            }
        }
    }
}


void SyncThread::Setting_sync_Set(QString command,int data)
{
    QJsonObject json;
    json.insert("user_ID", current_ID);
    json.insert("user_command", "setting sync");
    json.insert("command_code", command);
    if(command == "option-sync")
    {
        switch (data)
        {
            case 1:{json.insert("user_data", "option-close");break;}
            case 2:{json.insert("user_data", "option-PWM");break;}
            case 3:{json.insert("user_data", "option-heat");break;}
        }
    }
    else if(command == "Aim-temp-sync")
    {
        json.insert("user_data", (float)data/10);
    }
    else
    {
        json.insert("user_data", data);
    }
    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString json_str(byte_array);
    qDebug() << json_str;
    sync_socket->sendTextMessage(json_str);
    current_command = command;
    current_data = data;
    if(command == "alive-temp")
    {
        sync_action_flag = 0;
    }
    else{
        sync_action_flag = 1;
    }
}

void SyncThread::stop()
{
    stop_flag = 1;
}

QString SyncThread::command2db(QString command)
{
    if(command == "P-sync")
    {
        return "P_set";
    }
    else if(command == "I-sync")
    {
        return "I_set";
    }
    else if(command == "D-sync")
    {
        return "D_set";
    }
    else if(command == "Aim-temp-sync")
    {
        return "Aim_temp_set";
    }
    else if(command == "PWM-cyc-sync")
    {
        return "PWM_cyc_set";
    }
    else if(command == "DB-sync")
    {
        return "DB_set";
    }
    else if(command == "option-sync")
    {
        return "Mode_set";
    }
    else if(command == "read-cyc-sync")
    {
        return "read_cyc_set";
    }
    qDebug()<<"update data ready";
}

void SyncThread::List_fresh_SLOT()
{
    QJsonObject json;
    json.insert("user_command", "List fresh");
    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString json_str(byte_array);
    if(sync_socket->isValid())
    {
        qDebug()<<"sync_socket";
        sync_socket->sendTextMessage(json_str);
    }
    else if(anonymous_socket->isValid())
    {
        qDebug()<<"anonymous_socket";
        anonymous_socket->sendTextMessage(json_str);
    }
    qDebug()<<"List_fresh_SLOT";
}

void SyncThread::anonymous_Message(QString message)
{
    qDebug()<<message;
    QByteArray message_buf = message.toUtf8();
    QJsonDocument message_data = QJsonDocument::fromJson(message_buf);
    if(message_data.isObject())
    {
        QJsonObject message_obj = message_data.object();
        if(message_obj.value("user_command").toString() == "List fresh")
        {
            emit Fresh_List(message_obj);
            return;
        }
    }
}

void SyncThread::anonymous_connect()
{
    connect(anonymous_socket, SIGNAL(textMessageReceived(const QString)),
                    this, SLOT(anonymous_Message(QString)));
    qDebug()<< "anonymous_connect online";
}
