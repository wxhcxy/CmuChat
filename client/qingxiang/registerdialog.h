#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

private slots:
    void on_get_code_clicked();
    void slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err);//注册模块收到http发过来的注册信号后，开始这个注册槽函数

    void on_sure_btn_clicked();

    void on_return_btn_clicked();

    void on_cancel_btn_clicked();

private:
    void initHttpHandlers();
    void showTip(QString str, bool b_ok);
    bool checkUserValid();
    bool checkEmailValid();
    bool checkPassValid();
    bool checkVarifyValid();
    bool checkConfirmValid();
    void AddTipErr(TipErr te,QString tips);
    void DelTipErr(TipErr te);
    void ChangeTipPage();   //注册成功后切换到提示页
    QMap<TipErr, QString> _tip_errs;    //存储错误提示

    Ui::RegisterDialog *ui;
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;

    QTimer * _countdown_timer;  //注册成功之后的计时器
    int _countdown; //倒计时显示的数字

signals:
    void sigSwitchLogin();

};

#endif // REGISTERDIALOG_H
