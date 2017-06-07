#include "setting.h"
#include <QDebug>
Dialog::Dialog(QDialog *parent)
    : QDialog(parent), ui(new Ui::Dialog)
{
    ui->setupUi(this); //使用ui类
    ui->user_List->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}
Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_Connect_exit_btn_clicked()
{
    this->hide();
}

void Dialog::on_List_fresh_btn_clicked()
{   
    emit List_fresh_command();
}

void Dialog::on_Connect_server_btn_toggled(bool checked)
{
    QString IP,port,ID;

    if(checked)
    {
        ui->Connect_server_btn->setText(QString::fromUtf8("关闭连接"));
        IP = ui->Host_IP->text().simplified().trimmed();
        if(IP.isEmpty())
        {
            IP = "control.big7lion.win";
        }
        port = ui->Host_port->text().simplified().trimmed();
        if(port.isEmpty())
        {
            port = "19910";
        }
        ID = ui->Host_ID->text().simplified().trimmed();
        if(ID.isEmpty())
        {
            QMessageBox::warning(this,
                                 QString::fromUtf8("please check"),
                                 QString::fromUtf8("ID is empty,please key in"),
                                 QMessageBox::Cancel);
            ui->Connect_server_btn->setChecked(false);
            return;
        }
        emit Connect_Setting(IP,port,ID);
        qDebug()<<IP<<endl<<port<<endl<<ID;
        ui->Host_IP->setDisabled(true);
        ui->Host_port->setDisabled(true);
        ui->Host_ID->setDisabled(true);
    }
    else
    {
        ui->Connect_server_btn->setText(QString::fromUtf8("重新连接"));
        emit Connect_Close();
        ui->Host_IP->setDisabled(false);
        ui->Host_port->setDisabled(false);
        ui->Host_ID->setDisabled(false);
    }
}


void Dialog::Show_List(QJsonObject data)
{
    ui->user_List->clear();
    QJsonArray list_arr = data.value("list_store").toArray();
    QJsonObject List_obj;
    QJsonArray platform_list;
    QJsonArray logintime_list;
    if(list_arr.isEmpty())
    {
        QMessageBox::warning(this,
                             QString::fromUtf8("Take care"),
                             QString::fromUtf8("There is no user online,please manually key in your ID"),
                             QMessageBox::Cancel);
        return;
    }
    int ID_cnt;
    int platform_cnt;
    int cnt = 0;
    QVector<QStringList> ID_root;
    QVector<QStringList> ID_child;
    QVector<QTreeWidgetItem*> tree_root;
    QVector<QTreeWidgetItem*> tree_child;
    for(ID_cnt=0;ID_cnt<list_arr.count();ID_cnt++)
    {
        List_obj = list_arr.at(ID_cnt).toObject();
        ID_root.append(QStringList("ID:" + List_obj.value("list_ID").toString())) ;
        tree_root.append( new QTreeWidgetItem(ui->user_List, ID_root[ID_cnt]));
        platform_list = List_obj.value("list_platform").toArray();
        logintime_list = List_obj.value("list_logintime").toArray();
        for(platform_cnt=0;platform_cnt<List_obj.value("list_num").toInt();platform_cnt++)
        {
              qDebug() << platform_list.at(platform_cnt).toString() << logintime_list.at(platform_cnt).toString();
              QStringList q;
              q << QString("platform: "+platform_list.at(platform_cnt).toString()) << QString(logintime_list.at(platform_cnt).toString());
              ID_child.append(q);
              tree_child.append(new QTreeWidgetItem(tree_root[ID_cnt], ID_child[cnt]));
              tree_root[ID_cnt]->addChild(tree_child[cnt]);
              cnt++;
        }
    }
}

void Dialog::socket_error()
{
    ui->Connect_server_btn->setChecked(false);
    QMessageBox::warning(this,
                         QString::fromUtf8("please check"),
                         QString::fromUtf8("Network is not working!"),
                         QMessageBox::Cancel);
}

void Dialog::on_user_List_clicked(const QModelIndex &index)
{
    QTreeWidgetItem *parent=ui->user_List->currentItem();
    QRegExp test("^ID:\\S*");
    if(!test.exactMatch(parent->text(0)))
    {
        parent = parent->parent();
    }
    if(ui->Host_ID->isEnabled())
    {
        ui->Host_ID->setText(parent->text(0).remove(QRegExp("\\s|(ID:)")));
    }
}

void Dialog::on_user_List_doubleClicked(const QModelIndex &index)
{
    QTreeWidgetItem *parent=ui->user_List->currentItem();
    QRegExp test("^ID:\\S*");
    if(!test.exactMatch(parent->text(0)))
    {
        parent = parent->parent();
    }
    QString ID_text;
    ID_text = parent->text(0).remove(QRegExp("\\s|(ID:)"));
    if(ui->Connect_server_btn->isChecked())
    {
        if(ui->Host_ID->text() == ID_text)
        {
            return;
        }
        else{
            ui->Connect_server_btn->setChecked(false);
        }
    }
    ui->Host_ID->setText(ID_text);
    ui->Connect_server_btn->setChecked(true);
}


void Dialog::on_expand_btn_toggled(bool checked)
{
    if(checked)
    {
        ui->user_List->expandAll();
        ui->expand_btn->setText(QString::fromUtf8("收缩用户列表"));
    }
    else
    {
        ui->user_List->collapseAll();
        ui->expand_btn->setText(QString::fromUtf8("展开用户列表"));
    }
}
