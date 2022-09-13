#include "imageencode.h"
#include <QImage>
#include <QBuffer>
#include <QTextCodec>

imageEncode::imageEncode(QObject *parent) : QObject(parent)
{

}

//对图片进行base64编码
QByteArray imageEncode::imageToBase64(QString filename)
{
    //生成一个图片对象
    QImage img(filename);

    //用字节数组进行编码
    QByteArray ba;

    //缓冲区
    //用QByteArray构造QBuffer
    //QBuffer是缓冲区，一段连续的存储空间
    //这里可以充当IO设备
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);

    //把img写入到QBuffer
    img.save(&buf,"jpg");

    //对图片进行base64编码
    QByteArray imgbase64 = ba.toBase64();

    //对图片进行urlencode
    QTextCodec * codec = QTextCodec::codecForName("UTF-8");
    QByteArray imgData = codec->fromUnicode(imgbase64).toPercentEncoding();

    return  imgData;

}














