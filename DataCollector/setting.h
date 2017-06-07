#ifndef SETTING_H
#define SETTING_H

#include <QtGui/QtGui>
#include "ui_setting.h"
#include "qdialog.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QDialog *parent = 0);
    ~Dialog();

public slots:
    void Show_List(QJsonObject data);
    void socket_error();
signals:
    void Connect_Close();
    void Connect_Setting(QString,QString,QString);
    void List_fresh_command();

private slots:
    void on_Connect_exit_btn_clicked();

    void on_List_fresh_btn_clicked();

    void on_Connect_server_btn_toggled(bool checked);

    void on_user_List_clicked(const QModelIndex &index);

    void on_user_List_doubleClicked(const QModelIndex &index);

    void on_expand_btn_toggled(bool checked);

private:
    Ui::Dialog *ui;
};


#endif // SETTING_H
