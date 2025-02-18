#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include "global.h"
#include "statewidget.h"
#include "userdata.h"
#include <QListWidgetItem>

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();
    void addChatUserList();//这个函数做测试用，后期都是从服务器发来数据再加载
    void ClearLabelState(StateWidget* lb);//点击导航项后，清除原来的状态
protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    void handleGlobalMousePress(QMouseEvent *event);

private:
    void ShowSearch(bool bsearch = false);
    void AddLBGroup(StateWidget* lb);//将导航项加到_lb_list里管理
    QList<StateWidget*> _lb_list;
    Ui::ChatDialog *ui;
    ChatUIMode _mode;   //模式，控制side_bar
    ChatUIMode _state;  //状态，如搜索状态、联系人状态；在不同的模式下，也可有搜索状态
    bool _b_loading;    //负责加载
    QMap<int, QListWidgetItem*> _chat_items_added;

    void SetSelectChatItem(int uid = 0);
    void SetSelectChatPage(int uid = 0);
    int _cur_chat_uid;

    void loadMoreChatUser();    //加载聊天记录联系人
    void loadMoreConUser(); //加载好友列表联系人

    QWidget* _last_widget;

    void UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgdata);

private slots:
    void slot_loading_chat_user();
    void slot_side_chat();
    void slot_side_contact();
    void slot_text_changed(const QString & str);
    void slot_loading_contact_user();

public slots:
    void slot_apply_friend(std::shared_ptr<AddFriendApply> apply);
    void slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info);//别人通知到客户端，客户端的槽函数
    void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);//自己收到服务器的一个回包
    void slot_jump_chat_item(std::shared_ptr<SearchInfo> si);
    void slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> ui);
    void slot_friend_info_page(std::shared_ptr<UserInfo> user_info);
    void slot_switch_apply_friend_page();
    void slot_item_clicked(QListWidgetItem *item);
    void slot_append_send_chat_msg(std::shared_ptr<TextChatData> msgdata);
    void slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
};

#endif // CHATDIALOG_H
