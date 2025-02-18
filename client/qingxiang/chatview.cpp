#include "chatview.h"
#include <QDebug>
#include <QEvent>
#include <QScrollBar>
#include <QVBoxLayout>

#include <QTimer>
#include <QStyleOption>
#include <QPainter>

ChatView::ChatView(QWidget *parent)
    : QWidget(parent)
    , isAppended(false)//默认允许它添加
{
    QVBoxLayout *pMainLayout = new QVBoxLayout();
    this->setLayout(pMainLayout);
    pMainLayout->setContentsMargins(0, 0, 0, 0);//setContentsMargins(0, 0, 0, 0);//setMargin(0);


    m_pScrollArea = new QScrollArea();
    m_pScrollArea->setObjectName("chat_area");
    pMainLayout->addWidget(m_pScrollArea);

    QWidget *w = new QWidget(this);
    w->setObjectName("chat_bg");
    w->setAutoFillBackground(true);
    QVBoxLayout *pHLayout_1 = new QVBoxLayout();
    pHLayout_1->addWidget(new QWidget(), 100000);//100000是伸缩因子，表示占据很大的高度
    w->setLayout(pHLayout_1);
    m_pScrollArea->setWidget(w);    //应该时在QSCrollArea构造后执行 才对

    m_pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar();
    connect(pVScrollBar, &QScrollBar::rangeChanged,this, &ChatView::onVScrollBarMoved);//当滚动条的范围变化，就会触发槽函数

    //把垂直ScrollBar放到上边 而不是原来的并排
    QHBoxLayout *pHLayout_2 = new QHBoxLayout();
    pHLayout_2->addWidget(pVScrollBar, 0, Qt::AlignRight);//伸缩因子设为0，不伸缩，固定大小
    pHLayout_2->setContentsMargins(0, 0, 0, 0);//setMargin(0);
    m_pScrollArea->setLayout(pHLayout_2);
    pVScrollBar->setHidden(true);

    m_pScrollArea->setWidgetResizable(true);
    m_pScrollArea->installEventFilter(this);//安装事件过滤器
    initStyleSheet();

    qDebug()<<"chatview .................................";
}

void ChatView::appendChatItem(QWidget *item)
{
    QVBoxLayout *vl = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());
    vl->insertWidget(vl->count()-1, item);
    isAppended = true;

}

void ChatView::prependChatItem(QWidget *item)
{

}

void ChatView::insertChatItem(QWidget *before, QWidget *item)
{

}



void ChatView::removeAllItem()
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());

    int count = layout->count();

    for (int i = 0; i < count - 1; ++i) {
        QLayoutItem *item = layout->takeAt(0); // 始终从第一个控件开始删除
        if (item) {
            if (QWidget *widget = item->widget()) {
                delete widget;
            }
            delete item;
        }
    }

}




bool ChatView::eventFilter(QObject *o, QEvent *e)
{
    /*if(e->type() == QEvent::Resize && o == )
    {

    }
    else */if(e->type() == QEvent::Enter && o == m_pScrollArea)
    {
        m_pScrollArea->verticalScrollBar()->setHidden(m_pScrollArea->verticalScrollBar()->maximum() == 0);
    }
    else if(e->type() == QEvent::Leave && o == m_pScrollArea)
    {
        m_pScrollArea->verticalScrollBar()->setHidden(true);
    }
    return QWidget::eventFilter(o, e);
}

void ChatView::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}


void ChatView::onVScrollBarMoved(int min, int max)
{
    if(isAppended) //添加item可能调用多次
    {
        QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar();
        pVScrollBar->setSliderPosition(pVScrollBar->maximum());
        //500毫秒内可能调用多次，防止频繁刷
        QTimer::singleShot(500, [this]()
                           {
                               isAppended = false;
                           });
    }
}

void ChatView::initStyleSheet()
{
    //    QScrollBar *scrollBar = m_pScrollArea->verticalScrollBar();
    //    scrollBar->setStyleSheet("QScrollBar{background:transparent;}"
    //                             "QScrollBar:vertical{background:transparent;width:8px;}"
    //                             "QScrollBar::handle:vertical{background:red; border-radius:4px;min-height:20px;}"
    //                             "QScrollBar::add-line:vertical{height:0px}"
    //                             "QScrollBar::sub-line:vertical{height:0px}"
    //                             "QScrollBar::add-page:vertical {background:transparent;}"
    //                             "QScrollBar::sub-page:vertical {background:transparent;}");
}
