//实现新的Button

#ifndef CLICKEDBTN_H
#define CLICKEDBTN_H
#include <QPushButton>
#include <QEnterEvent>

class ClickedBtn:public QPushButton
{
    Q_OBJECT
public:
    ClickedBtn(QWidget *parent = nullptr);
    ~ClickedBtn();
    void SetState(QString nomal, QString hover, QString press);//按钮的三种状态，普通、鼠标悬浮、按下
protected:
    virtual void enterEvent(QEnterEvent *event) override; // 鼠标进入
    virtual void leaveEvent(QEvent *event) override; // 鼠标离开
    virtual void mousePressEvent(QMouseEvent *event) override; // 鼠标按下
    virtual void mouseReleaseEvent(QMouseEvent *event) override; // 鼠标释放
private:
    QString _normal;
    QString _hover;
    QString _press;
};

#endif // CLICKEDBTN_H
