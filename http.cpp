#include "http.h"

Http::Http(QObject *parent) : QObject(parent)
{

}

bool Http::post_sync(QString url, QMap<QString, QString> header, QByteArray &requestData, QByteArray &replyData)
{
    //创建一个执行发送请求的对象
    QNetworkAccessManager manage;

    //请求的内容 （包括url和header）
    QNetworkRequest request;
    request.setUrl(url);

    QMapIterator<QString,QString> iter(header);
    while(iter.hasNext()){
        iter.next();
        request.setRawHeader(iter.key().toLatin1(),iter.value().toLatin1());
    }

    //回复
    //当reply收到数据的时候会产生结束信号，继而循环结束
    QNetworkReply* reply = manage.post(request,requestData);
    QEventLoop loop;
    connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);

    //模拟阻塞状态，直到收到数据再继续进行
    loop.exec();

    if(reply!=nullptr&&reply->error()==QNetworkReply::NoError){
        replyData = reply->readAll();
        return true;
    }
    else{
        return false;
    }

}
