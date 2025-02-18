#ifndef CONUSERITEM_H
#define CONUSERITEM_H

//这个就是联系人列表里的一个好友的那个界面样式，一个好友左侧是头像和可能有红点，右侧是一句话

#include <QWidget>
#include "listitembase.h"
#include "userdata.h"

namespace Ui {
class ConUserItem;
}

class ConUserItem : public ListItemBase
{
    Q_OBJECT

public:
    explicit ConUserItem(QWidget *parent = nullptr);
    ~ConUserItem();
    QSize sizeHint() const override;
    void SetInfo(std::shared_ptr<AuthInfo> auth_info);
    void SetInfo(std::shared_ptr<AuthRsp> auth_rsp);
    void SetInfo(int uid, QString name, QString icon);
    void ShowRedPoint(bool show = false);
    std::shared_ptr<UserInfo> GetInfo();
private:
    Ui::ConUserItem *ui;
    std::shared_ptr<UserInfo> _info;
};

#endif // CONUSERITEM_H
