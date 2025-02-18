#ifndef PICTUREBUBBLE_H
#define PICTUREBUBBLE_H

//图片气泡

#include <QHBoxLayout>
#include <QPixmap>
#include "bubbleframe.h"
class PictureBubble : public BubbleFrame
{
    Q_OBJECT
public:
    PictureBubble(const QPixmap &picture, ChatRole role, QWidget *parent = nullptr);
};

#endif // PICTUREBUBBLE_H
