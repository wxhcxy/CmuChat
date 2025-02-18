#ifndef APPLYFRIENDPAGE_H
#define APPLYFRIENDPAGE_H

#include <QWidget>
#include "userdata.h"
#include <memory>
#include <QJsonArray>
#include <unordered_map>
#include "applyfrienditem.h"

namespace Ui {
class ApplyFriendPage;
}

class ApplyFriendPage : public QWidget
{
    Q_OBJECT

public:
    explicit ApplyFriendPage(QWidget *parent = nullptr);
    ~ApplyFriendPage();
    void AddNewApply(std::shared_ptr<AddFriendApply> apply);
protected:
    void paintEvent(QPaintEvent *event);
private:
    void loadApplyList();   //加载申请列表，后台传过来的
    Ui::ApplyFriendPage *ui;
    std::unordered_map<int, ApplyFriendItem*> _unauth_items;    //管理已经认证处理过的好友请求（就是已添加或者还未处理）
public slots:
    void slot_auth_rsp(std::shared_ptr<AuthRsp> );  //点完同意，出发的槽函数
signals:
    void sig_show_search(bool);
};

#endif // APPLYFRIENDPAGE_H
