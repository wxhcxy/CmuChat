#ifndef CONTACTUSERLIST_H
#define CONTACTUSERLIST_H

//联系人列表，就我们点击导航栏好友，显示的好友那个列表

#include <QListWidget>
#include <QEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QDebug>
#include <memory>
#include "userdata.h"
class ConUserItem;

class ContactUserList : public QListWidget
{
    Q_OBJECT
public:
    ContactUserList(QWidget* parent = nullptr);
    void ShowRedPoint(bool bshow = true);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override ;
private:
    void addContactUserList();

public slots:
    void slot_item_clicked(QListWidgetItem *item);
    void slot_add_auth_firend(std::shared_ptr<AuthInfo>);   //收到tcp回复，可能对方同意我们添加好友，就触发这个槽函数，刷新好友列表
    void slot_auth_rsp(std::shared_ptr<AuthRsp>);   //我们收到别人的添加好友申请，我们同意，触发这个槽函数，刷新好友列表
signals:
    void sig_loading_contact_user();    //加载联系人
    void sig_switch_apply_friend_page();    //切换到申请好友的界面
    void sig_switch_friend_info_page(std::shared_ptr<UserInfo> user_info);  //切换到好友信息的界面
private:
    bool _load_pending;
    ConUserItem* _add_friend_item;
    QListWidgetItem * _groupitem;
};

#endif // CONTACTUSERLIST_H
