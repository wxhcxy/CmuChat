#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "registerdialog.h"
#include "resetdialog.h"
#include "chatdialog.h"
/******************************************************************************
 *
 * @file       mainwindow.h
 * @brief      主界面功能 Function
 *
 * @author     新辉
 * @date       2024/12/21
 *****************************************************************************/

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

enum UIStatus { LOGIN_UI, REGISTER_UI, RESET_UI, CHAT_UI };

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void SlotSwitchReg();
    void SlotSwitchLogin();
    void SlotSwitchReset();//重置密码
    void SlotSwitchLogin2();

    void SlotSwitchChat();//登陆成功后，跳转聊天界面的槽函数

    void SlotOffline();

    void SlotExcepConOffline();

private:
    void offlineLogin(); //其他地方登陆同账号，这边下线
    Ui::MainWindow *ui;

    LoginDialog *_login_dlg;
    RegisterDialog *_reg_dlg;
    ResetDialog *_reset_dlg;
    ChatDialog *_chat_dlg;

    UIStatus _ui_status;
};
#endif // MAINWINDOW_H
