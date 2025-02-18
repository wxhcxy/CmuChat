#ifndef CHATITEMBASE_H
#define CHATITEMBASE_H

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include "global.h"
class BubbleFrame;//气泡框的一个基类，消息内容

class ChatItemBase : public QWidget
{
    Q_OBJECT
public:
    explicit ChatItemBase(ChatRole role, QWidget *parent = nullptr);
    void setUserName(const QString &name);
    void setUserIcon(const QPixmap &icon);
    void setWidget(QWidget *w);//这个就是消息的那里

private:
    ChatRole m_role;
    QLabel *m_pNameLabel;
    QLabel *m_pIconLabel;
    QWidget *m_pBubble;
};

#endif // CHATITEMBASE_H
