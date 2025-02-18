#ifndef HTTPMGR_H
#define HTTPMGR_H

#include "singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>

class HttpMgr: public QObject, public Singleton<HttpMgr>, public std::enable_shared_from_this<HttpMgr>//奇异递归模板，CRTP技术
{
    Q_OBJECT
public:
    ~HttpMgr();
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);

private:
    friend class Singleton<HttpMgr>;//把基类声明为它的友元, 这样基类就能够访问子类的构造函数
    HttpMgr();

    QNetworkAccessManager _manager;

private slots:
    void slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);

signals:
    void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);//当http发送完成之后，会通知其他模块
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_reset_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_login_mod_finish(ReqId id, QString res, ErrorCodes err);
};

#endif // HTTPMGR_H
