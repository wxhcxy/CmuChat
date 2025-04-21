#ifndef TCPMGR_H
#define TCPMGR_H

//这是服务器传回(RSP)的一层

#include <QTcpSocket>
#include "singleton.h"
#include "global.h"
#include <functional>
#include <QObject>
#include "userdata.h"

class TcpMgr:public QObject, public Singleton<TcpMgr>,
               public std::enable_shared_from_this<TcpMgr>
{
    Q_OBJECT
public:
    ~ TcpMgr();
    void CloseConnection(); //踢人、下线、退出账号

private:
    friend class Singleton<TcpMgr>;
    TcpMgr();
    void initHandlers();    //注册一些回调的处理，将请求id和回调函数绑定到一起
    void handleMsg(ReqId id, int len, QByteArray data);
    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    QByteArray _buffer;
    bool _b_recv_pending;   //判断数据有没有收全,true表示未收全
    quint16 _message_id;
    quint16 _message_len;
    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers; //将请求id和回调函数绑定到一起
public slots:
    void slot_tcp_connect(ServerInfo);
    void slot_send_data(ReqId reqId, QByteArray data);
signals:
    void sig_con_success(bool bsuccess);    //tcp连接成功的信号
    void sig_send_data(ReqId reqId, QByteArray data);
    void sig_swich_chatdlg();   //登录成功，切换到聊天的信号 //登录成功后发送信号，跳转到聊天界面的槽函数
    void sig_login_failed(int);
    void sig_user_search(std::shared_ptr<SearchInfo>);

    void sig_friend_apply(std::shared_ptr<AddFriendApply>);
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);

    void sig_text_chat_msg(std::shared_ptr<TextChatMsg> msg);

    void sig_notify_offline();
};

#endif // TCPMGR_H
