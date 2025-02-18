#ifndef LOADINGDLG_H
#define LOADINGDLG_H

//鼠标滚动到底部，加载更多联系人记录的动画

#include <QDialog>

namespace Ui {
class LoadingDlg;
}

class LoadingDlg : public QDialog
{
    Q_OBJECT

public:
    explicit LoadingDlg(QWidget *parent = nullptr);
    ~LoadingDlg();

private:
    Ui::LoadingDlg *ui;
};

#endif // LOADINGDLG_H
