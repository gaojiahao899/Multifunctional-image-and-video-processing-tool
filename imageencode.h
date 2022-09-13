#ifndef IMAGEENCODE_H
#define IMAGEENCODE_H

#include <QObject>
#include <QString>
#include <QByteArray>

class imageEncode : public QObject
{
    Q_OBJECT
public:
    explicit imageEncode(QObject *parent = nullptr);

    //对图片进行base64编码
    static QByteArray imageToBase64(QString filename);

signals:

};

#endif // IMAGEENCODE_H
