#include "findsuccessdlg.h"
#include "ui_findsuccessdlg.h"
#include <QDir>
#include "applyfriend.h"
#include <memory>

FindSuccessDlg::FindSuccessDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindSuccessDlg),
    _parent(parent)
{
    ui->setupUi(this);
    // 设置对话框标题
    setWindowTitle("添加");
    // 隐藏对话框标题栏
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("FindSuccessDlg");
    // 获取当前应用程序的路径
    QString app_path = QCoreApplication::applicationDirPath();
    //存储好友头像到本地指定路径。这里先做测试。(建议以后更改为先从服务器获取好友头像，再保存到本地)
    QString pix_path = QDir::toNativeSeparators(app_path +
                                                QDir::separator() + "static"+QDir::separator()+"head_1.jpg");
    QPixmap head_pix(pix_path);
    head_pix = head_pix.scaled(ui->head_lb->size(),
                               Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->head_lb->setPixmap(head_pix);
    ui->add_friend_btn->SetState("normal","hover","press");
    this->setModal(true);
}

FindSuccessDlg::~FindSuccessDlg()
{
    qDebug()<<"FindSuccessDlg destruct";
    delete ui;
}

void FindSuccessDlg::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    ui->name_lb->setText(si->_name);
    _si = si;
}

void FindSuccessDlg::on_add_friend_btn_clicked()
{
    this->hide();
    //弹出加好友界面
    auto applyFriend = new ApplyFriend(_parent);
    applyFriend->SetSearchInfo(_si);
    applyFriend->setModal(true);
    applyFriend->show();
}
