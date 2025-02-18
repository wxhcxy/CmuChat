#ifndef CHATUSERLIST_H
#define CHATUSERLIST_H
#include <QListWidget>
#include <QWheelEvent>
#include <QEvent>
#include <QScrollBar>
#include <QDebug>

//重写聊天用户列表，chatdialog.ui里的chat_user_list会提升为这个类

class ChatUserList: public QListWidget
{
    Q_OBJECT
public:
    ChatUserList(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool _load_pending;

signals:
    void sig_loading_chat_user();
};

#endif // CHATUSERLIST_H
