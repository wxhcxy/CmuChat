#ifndef CHATUSERWID_H
#define CHATUSERWID_H

//显示聊天联系人记录列表的一项，即listwidget里的一项
//ChatUserWid继承ListItemBase，ListItemBase继承QWidget

#include <QWidget>
#include "listitembase.h"
#include "userdata.h"

namespace Ui {
class ChatUserWid;
}

class ChatUserWid : public ListItemBase
{
    Q_OBJECT

public:
    explicit ChatUserWid(QWidget *parent = nullptr);
    ~ChatUserWid();

    QSize sizeHint() const override {
        return QSize(250, 70); // 返回自定义的尺寸
    }

    void SetInfo(std::shared_ptr<UserInfo> user_info);
    void SetInfo(std::shared_ptr<FriendInfo> friend_info);

    std::shared_ptr<UserInfo> GetUserInfo();//获取用户信息

    void updateLastMsg(std::vector<std::shared_ptr<TextChatData>> msgs);    //更新消息

private:
    Ui::ChatUserWid *ui;
    std::shared_ptr<UserInfo> _user_info;
};


#endif // CHATUSERWID_H
