#ifndef USERMGR_H
#define USERMGR_H
#include <QObject>
#include <memory>
#include <singleton.h>
#include "userdata.h"
#include <vector>
#include <QJsonArray>

class UserMgr:public QObject,public Singleton<UserMgr>,
        public std::enable_shared_from_this<UserMgr>
{
    Q_OBJECT
public:
    friend class Singleton<UserMgr>;
    ~ UserMgr();
    void SetToken(QString token);
    int GetUid();
    QString GetName();
    std::vector<std::shared_ptr<ApplyInfo>> GetApplyList();
    bool AlreadyApply(int uid);
    void AddApplyList(std::shared_ptr<ApplyInfo> app);
    void SetUserInfo(std::shared_ptr<UserInfo> user_info);
    void AppendApplyList(QJsonArray array);
    void AppendFriendList(QJsonArray array);
    bool CheckFriendById(int uid);
    void AddFriend(std::shared_ptr<AuthRsp> auth_rsp);//我们自己认证别人返回的回包
    void AddFriend(std::shared_ptr<AuthInfo> auth_info);//对方收到了我们的通知，把消息加到好友里
    std::shared_ptr<FriendInfo> GetFriendById(int uid);//通过id查到好友具体的信息

    std::vector<std::shared_ptr<FriendInfo>> GetChatListPerPage();
    bool IsLoadChatFin();//判断是否已经加载所有聊天联系人,滚动到底部
    void UpdateChatLoadedCount();
    std::vector<std::shared_ptr<FriendInfo>> GetConListPerPage();
    void UpdateContactLoadedCount();
    bool IsLoadConFin();//判断好友列表联系人是否加载完成
    std::shared_ptr<UserInfo> GetUserInfo();    //获取用户信息

    void AppendFriendChatMsg(int friend_id, std::vector<std::shared_ptr<TextChatData>>);

private:
    UserMgr();
    QString _token;

    std::vector<std::shared_ptr<ApplyInfo>> _apply_list;    //申请列表（应该是好友申请列表吧？）//可以改成map管理更好
    std::shared_ptr<UserInfo> _user_info;
    QMap<int, std::shared_ptr<FriendInfo>> _friend_map;
    std::vector<std::shared_ptr<FriendInfo>> _friend_list;

    int _chat_loaded;
    int _contact_loaded;

};

#endif // USERMGR_H
