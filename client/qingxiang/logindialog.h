#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <global.h>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

private:
    void initHttpHandlers();
    void initHead();
    Ui::LoginDialog *ui;
    bool checkUserValid();
    bool checkPwdValid();
    void AddTipErr(TipErr te,QString tips);
    void DelTipErr(TipErr te);
    void showTip(QString str, bool b_ok);
    QMap<TipErr, QString> _tip_errs;    //存储错误提示
    bool enableBtn(bool);
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers; //回调函数处理回复
    int _uid;
    QString _token;

public slots:
    void slot_forget_pwd();

signals:
    void switchRegister();//切换到注册窗口的信号
    void switchReset();//切换到重置密码界面的信号
    void sig_connect_tcp(ServerInfo);

private slots:
    void on_login_btn_clicked();
    void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err);
    void slot_tcp_con_finish(bool bsuccess);
    void slot_login_failed(int);
};

#endif // LOGINDIALOG_H
