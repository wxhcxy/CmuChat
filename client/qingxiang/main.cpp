#include "mainwindow.h"

#include <QApplication>

#include <QFile>
#include "global.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qss(":/style/stylesheet.qss");
    if(qss.open(QFile::ReadOnly)){
        qDebug("Open success");
        //qss.readAll()返回QByteArray，QLatin1String将其转换为QString
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }else{
        qDebug("Open failed");
    }

    QString fileName = "config.ini";
    QString app_path = QCoreApplication::applicationDirPath();
    QString config_path
        = "../../../config.ini"; //QDir::toNativeSeparators(app_path + QDir::separator() + fileName);//拼接处config.ini在本地的文件路径
    qDebug() << "config_path: " << config_path;
    QSettings settings(config_path,
                       QSettings::IniFormat); //读取config.ini配置,因为是.ini文件，所以指定用模式去读
    QString gate_host = settings.value("GateServer/host")
                            .toString(); //将配置文件里的GateServer里的host取出来，并转成string类型
    QString gate_port = settings.value("GateServer/port").toString();
    gate_url_prefix = "http://" + gate_host + ":" + gate_port;
    qDebug() << "gate_host: " << gate_host;
    qDebug() << "gate_port: " << gate_port;

    MainWindow w;
    w.show();
    return a.exec();
}
