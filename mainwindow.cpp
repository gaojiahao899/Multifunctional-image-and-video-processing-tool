#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QToolButton>
#include <QApplication>
#include <QSpinBox>
#include <QTextEdit>
#include <QMdiSubWindow>
#include <QLabel>
#include <string>

using namespace std;
#include <iostream>
#include <sstream>
#include <QFontDialog>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QTranslator>
#include <QDebug>
#include <QPaintDevice>
#include <QPainter>
#include <QImage>
#include <QtCore/qmath.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include "opencv2/imgproc/imgproc_c.h"///for cvSmooth



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , beishu(1)//貌似没用
    ,delay(0)
{
    ui->setupUi(this);   

    setWindowFlags(windowFlags()&~Qt::WindowMaximizeButtonHint);// 禁止最大化按钮
    setFixedSize(this->width(),this->height());// 禁止拖动窗口大小

    this->setWindowTitle(QObject::tr("多功能图像视频处理工具--西安建筑科技大学人工智能与机器人研究所@Gao Jiahao"));//软件title
    this->setWindowIcon(QIcon(":/myImage/image/AItubiao.png"));//设置图标

    //关于本软件弹窗初始化
    customMsgBox.setWindowTitle(tr("关于本软件"));
    customMsgBox.addButton(tr("好的"),QMessageBox::ActionRole);
    customMsgBox.setIconPixmap(QPixmap(":/myImage/image/about.png"));
    customMsgBox.setText(tr("欢迎使用《多功能图像视频处理》软件！本软件具有简单的图像和视频处理功能。\n"
                            "图像功能包括多选打开、旋转、镜像、灰度化、均值滤波、"
                            "边缘检测、原图复合、伽马检测、二值化、色彩调整、亮度调整、对比度调整、饱和度调整等功能。\n"
                            "视频功能包括暂停、播放、进度条、灰度化、边缘检测、平滑、二值化、局部马赛克、缩放等功能。\n"
                            "——By GaoJiahao"));

    //状态栏消息
    ui->statusBar->showMessage(tr("欢迎使用多功能图像视频处理软件"),3000);
    QLabel *permanent = new QLabel(this);
    permanent->setObjectName("status");
    permanent->setFrameStyle(QFrame::Box|QFrame::Sunken);
    permanent->setText("欢迎使用！");
    ui->statusBar->addPermanentWidget(permanent);
    ui->tabWidget->setStyleSheet("QTabWidget:pane {border-top:0px;background:  transparent; }");

    //图片
    ui->pushButton_3->setDisabled(true);//图片左转不使能
    ui->pushButton_4->setDisabled(true);//图片右转不使能

    //视频
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimeout()));//视频绑定
    connect(&timer, SIGNAL(timeout()), this, SLOT(updatePosition()));//进度条绑定视频进度

}

MainWindow::~MainWindow()
{
    delete ui;
    capture.release();
}

//图片
////////////////////////////////////////////////////////////////////////////
//图片工具函数
//Mat转图像
QImage MainWindow::MatToQImage(const cv::Mat& mat)
{
    // 8-bits unsigned, NO. OF CHANNELS = 1
    if (mat.type() == CV_8UC1)
    {
        QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
        // Set the color table (used to translate colour indexes to qRgb values)
        image.setColorCount(256);
        for (int i = 0; i < 256; i++)
        {
            image.setColor(i, qRgb(i, i, i));
        }
        // Copy input Mat
        uchar *pSrc = mat.data;
        for (int row = 0; row < mat.rows; row++)
        {
            uchar *pDest = image.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return image;
    }
    // 8-bits unsigned, NO. OF CHANNELS = 3
    else if (mat.type() == CV_8UC3)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    else if (mat.type() == CV_8UC4)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
        return QImage();
    }
}

//图片居中显示,图片大小与label大小相适应
QImage MainWindow::ImageCenter(QImage  qimage,QLabel *qLabel)
{
    QImage image;
    QSize imageSize = qimage.size();
    QSize labelSize = qLabel->size();

    double dWidthRatio = 1.0*imageSize.width() / labelSize.width();
    double dHeightRatio = 1.0*imageSize.height() / labelSize.height();
    if (dWidthRatio>dHeightRatio)
    {
        image = qimage.scaledToWidth(labelSize.width());
    }
    else
    {
        image = qimage.scaledToHeight(labelSize.height());
    }
    return image;

    }

//调整rgb函数调用
QImage MainWindow::setRGB(QImage image,int value_r, int value_g, int value_b){
    int r,g,b;
    QColor oldColor;
    int height=image.height();
    int width=image.width();
    for (int i = 0; i < height; ++i)
    {
        for(int j=0;j<width;++j){
            oldColor = QColor(image.pixel(j,i));
            r=oldColor.red()+value_r;
            if(r>255)
                r=255;
            g=oldColor.green()+value_g;
            if(g>255)
                g=255;
            b=oldColor.blue()+value_b;
            if(b>255)
                b=255;
            image.setPixel(j,i, qRgb(r, g, b));
        }
    }
    return image;
}


////////////////////////////////////////////////////////////////////////////
//图像处理函数
//灰度化
QImage MainWindow::gray(QImage image){
    QImage newImage =image.convertToFormat(QImage::Format_ARGB32);
    QColor oldColor;

    for(int y = 0; y < newImage.height(); y++)
    {
        for(int x = 0; x < newImage.width(); x++)
        {
            oldColor = QColor(image.pixel(x,y));
            //灰度化
            int average = (oldColor.red() + oldColor.green() + oldColor.blue()) / 3;
            newImage.setPixel(x, y, qRgb(average, average, average));
        }
    }
    return newImage;
}
//均值滤波
QImage MainWindow::junzhi(QImage image){
    int kernel [3][3] = {   {1,1,1},
                            {1,1,1},
                            {1,1,1}   };
    int sizeKernel = 3;
    int sumKernel = 9;
    QColor color;
    for(int x = sizeKernel/2;x<image.width() - sizeKernel/2;x++)
    {
        for(int y= sizeKernel/2;y<image.height() - sizeKernel/2;y++)
        {
            int r = 0;
            int g = 0;
            int b = 0;

            for(int i = -sizeKernel/2;i<=sizeKernel/2;i++)
            {
                for(int j = -sizeKernel/2;j<=sizeKernel/2;j++)
                {
                    color = QColor(image.pixel(x+i,y+j));
                    r+=color.red()*kernel[sizeKernel/2+i][sizeKernel/2+j];
                    g+=color.green()*kernel[sizeKernel/2+i][sizeKernel/2+j];
                    b+=color.blue()*kernel[sizeKernel/2+i][sizeKernel/2+j];
                }
            }

            r = qBound(0,r/sumKernel,255);
            g = qBound(0,g/sumKernel,255);
            b = qBound(0,b/sumKernel,255);
            image.setPixel(x,y,qRgb( r,g,b));
        }
    }
    return image;
}
//边缘检测
QImage MainWindow::bianyuan(QImage image){
    QImage newImage =image.convertToFormat(QImage::Format_ARGB32);
    QColor color0;
    QColor color1;
    QColor color2;
    QColor color3;

    int  r = 0;
    int g = 0;
    int b = 0;
    int rgb = 0;

    int r1 = 0;
    int g1 = 0;
    int b1 = 0;
    int rgb1 = 0;

    int a = 0;

    for( int y = 0; y < image.height() - 1; y++)
    {
        for(int x = 0; x < image.width() - 1; x++)
        {
            color0 =   QColor ( image.pixel(x,y));
            color1 =   QColor ( image.pixel(x + 1,y));
            color2 =   QColor ( image.pixel(x,y + 1));
            color3 =   QColor ( image.pixel(x + 1,y + 1));
            //abs取绝对值
            r = abs(color0.red() - color3.red());
            g = abs(color0.green() - color3.green());
            b = abs(color0.blue() - color3.blue());
            rgb = r + g + b;

            r1 = abs(color1.red() - color2.red());
            g1= abs(color1.green() - color2.green());
            b1 = abs(color1.blue() - color2.blue());
            rgb1 = r1 + g1 + b1;

            a = rgb + rgb1;
            a = a>255?255:a;

            newImage.setPixel(x,y,qRgb(a,a,a));
        }
    }
    return newImage;
}

//原图+边缘滤波复合调用函数
QImage MainWindow::fuhe(QImage images){
    QImage image2 =images.convertToFormat(QImage::Format_ARGB32);
    QColor color0;
    QColor color1;
    QColor color2;
    QColor color3;

    int  r = 0;
    int g = 0;
    int b = 0;
    int rgb = 0;

    int r1 = 0;
    int g1 = 0;
    int b1 = 0;
    int rgb1 = 0;

    int a = 0;

    for( int y = 0; y < images.height() - 1; y++)
    {
        for(int x = 0; x < images.width() - 1; x++)
        {
            color0 =   QColor ( images.pixel(x,y));
            color1 =   QColor ( images.pixel(x + 1,y));
            color2 =   QColor ( images.pixel(x,y + 1));
            color3 =   QColor ( images.pixel(x + 1,y + 1));
            r = abs(color0.red() - color3.red());
            g = abs(color0.green() - color3.green());
            b = abs(color0.blue() - color3.blue());
            rgb = r + g + b;

            r1 = abs(color1.red() - color2.red());
            g1= abs(color1.green() - color2.green());
            b1 = abs(color1.blue() - color2.blue());
            rgb1 = r1 + g1 + b1;

            a = rgb + rgb1;
            a = a>255?255:a;

            image2.setPixel(x,y,qRgb(a,a,a));
        }
    }
    QImage image(origin_path);
    int red, green, blue;
    int red2,green2,blue2;
    int pixels = image.width() * image.height();

    unsigned int *data = (unsigned int *)image.bits();
    unsigned int *data2 = (unsigned int *)image2.bits();

    for (int i = 0; i < pixels; ++i)
    {
        red= qRed(data[i]);
        red2=qRed(data2[i])*0.5+red*0.5;
        red2 = (red2 < 0x00) ? 0x00 : (red2 > 0xff) ? 0xff : red2;

        green= qGreen(data[i]);
        green2= qGreen(data2[i])*0.5+green*0.5;
        green2 = (green2 < 0x00) ? 0x00 : (green2 > 0xff) ? 0xff : green2;

        blue= qBlue(data[i]);
        blue2= qBlue(data2[i])*0.5+blue*0.5;
        blue2 =  (blue2  < 0x00) ? 0x00 : (blue2  > 0xff) ? 0xff : blue2 ;

        data2[i] = qRgba(red2, green2, blue2, qAlpha(data2[i]));
    }
    return image2;
}

//伽马变换
QImage MainWindow::gamma(QImage image){
    double d=1.2;
    QColor color;
    int height = image.height();
    int width = image.width();
    for (int i=0;i<width;i++)
    {
        for(int j=0;j<height;j++)
        {
            color = QColor(image.pixel(i,j));
            double r = color.red();
            double g = color.green();
            double b = color.blue();
            int R = qBound(0,(int)qPow(r,d),255);
            int G = qBound(0,(int)qPow(g,d),255);
            int B = qBound(0,(int)qPow(b,d),255);
            image.setPixel(i,j,qRgb(R,G,B));
        }
    }
    return image;
}

//对比度
QImage MainWindow::AdjustContrast(QImage image, int value)
{
    int pixels = image.width() * image.height();//像素点个数
    unsigned int *data = (unsigned int *)image.bits();//无符号整数

    int red, green, blue;
    int nRed, nGreen, nBlue;

    if (value > 0 && value < 256)
    {
        float param = 1 / (1 - value / 256.0) - 1;

        for (int i = 0; i < pixels; ++i)
        {
            nRed = qRed(data[i]);
            nGreen = qGreen(data[i]);
            nBlue = qBlue(data[i]);

            red = nRed + (nRed - 127) * param;
            red = (red < 0x00) ? 0x00 : (red > 0xff) ? 0xff : red;
            green = nGreen + (nGreen - 127) * param;
            green = (green < 0x00) ? 0x00 : (green > 0xff) ? 0xff : green;
            blue = nBlue + (nBlue - 127) * param;
            blue = (blue < 0x00) ? 0x00 : (blue > 0xff) ? 0xff : blue;

            data[i] = qRgba(red, green, blue, qAlpha(data[i]));
        }
    }
    else
    {
        for (int i = 0; i < pixels; ++i)
        {
            nRed = qRed(data[i]);
            nGreen = qGreen(data[i]);
            nBlue = qBlue(data[i]);

            red = nRed + (nRed - 127) * value / 100.0;
            red = (red < 0x00) ? 0x00 : (red > 0xff) ? 0xff : red;
            green = nGreen + (nGreen - 127) * value / 100.0;
            green = (green < 0x00) ? 0x00 : (green > 0xff) ? 0xff : green;
            blue = nBlue + (nBlue - 127) * value / 100.0;
            blue = (blue < 0x00) ? 0x00 : (blue > 0xff) ? 0xff : blue;

            data[i] = qRgba(red, green, blue, qAlpha(data[i]));
        }
    }

    return image;
}

//饱和度
QImage MainWindow::AdjustSaturation(QImage Img, int iSaturateValue)
{
    int red, green, blue;
    int nRed, nGreen, nBlue;
    int pixels = Img.width() * Img.height();
    unsigned int *data = (unsigned int *)Img.bits();

    float Increment = iSaturateValue/100.0;

    float delta = 0;
    float minVal, maxVal;
    float L, S;
    float alpha;

    for (int i = 0; i < pixels; ++i)
    {
        nRed = qRed(data[i]);
        nGreen = qGreen(data[i]);
        nBlue = qBlue(data[i]);

        minVal = std::min(std::min(nRed, nGreen), nBlue);
        maxVal = std::max(std::max(nRed, nGreen), nBlue);
        delta = (maxVal - minVal) / 255.0;
        L = 0.5*(maxVal + minVal) / 255.0;
        S = std::max(0.5*delta / L, 0.5*delta / (1 - L));

        if (Increment > 0)
        {
            alpha = std::max(S, 1 - Increment);
            alpha = 1.0 / alpha - 1;
            red = nRed + (nRed - L*255.0)*alpha;
            red = (red < 0x00) ? 0x00 : (red > 0xff) ? 0xff : red;
            green = nGreen + (nGreen - L*255.0)*alpha;
            green = (green < 0x00) ? 0x00 : (green > 0xff) ? 0xff : green;
            blue = nBlue + (nBlue - L*255.0)*alpha;
            blue = (blue < 0x00) ? 0x00 : (blue > 0xff) ? 0xff : blue;
        }
        else
        {
            alpha = Increment;
            red = L*255.0 + (nRed - L * 255.0)*(1+alpha);
            red = (red < 0x00) ? 0x00 : (red > 0xff) ? 0xff : red;
            green = L*255.0 + (nGreen - L * 255.0)*(1+alpha);
            green = (green < 0x00) ? 0x00 : (green > 0xff) ? 0xff : green;
            blue = L*255.0 + (nBlue - L * 255.0)*(1+alpha);
            blue = (blue < 0x00) ? 0x00 : (blue > 0xff) ? 0xff : blue;
        }

        data[i] = qRgba(red, green, blue, qAlpha(data[i]));
    }

    return Img;
}

/////////////////////////////////////////////////////////////////////////////
//按钮相关
//菜单栏
//工具箱显示
void MainWindow::on_action_Dock_triggered()
{
    ui->dockWidget->show();
}
//菜单栏翻译
void MainWindow::on_action_L_triggered()
{
      QTranslator translator;
      if(language)
      {
          translator.load(":/myLanguage/language/zh_tr.qm");
      }
      else
      {
          translator.load(":/myLanguage/language/en_tr.qm");
      }
      QApplication *qapp;
      qapp->installTranslator(&translator);
      language=!language;
      ui->retranslateUi(this);//重新翻译刷新界面
}

//菜单栏帮助
void MainWindow::on_action_About_triggered()
{
      customMsgBox.show();

      customMsgBox.exec();
}

//菜单栏打开文件
void MainWindow::on_action_Open_triggered()
{
    QStringList srcDirPathListS = QFileDialog::getOpenFileNames(this, tr("选择图片"), "E:/QT/ImageProcess03/MainWindow/imagesprocess", tr("图像文件(*.jpg *.png *.bmp)"));
    //存在选择了图片时
    if(srcDirPathListS.size()>0)
    {
        ui->tabWidget->setCurrentIndex(0);
    }
    //选取三张及以上图片时
    if(srcDirPathListS.size()>=3)
    {
        srcDirPathList =srcDirPathListS;
        srcDirPathListS.clear();
        index =0;
        QString srcDirPath = srcDirPathList.at(index);
        QImage image(srcDirPath);
        QImage Image=ImageCenter(image,ui->label_show);//调整图片大小
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);//居中对齐
        origin_path=srcDirPath;//目前处理的图片的原图

        //待处理区域第一张图片
        QImage images=ImageCenter(image,ui->label_other);
        ui->label_other->setPixmap(QPixmap::fromImage(images));
        ui->label_other->setAlignment(Qt::AlignCenter);
        //状态栏显示图片路径
        QLabel *label=ui->statusBar->findChild<QLabel *>("status");
        label->setText(srcDirPath);

        //待处理区域第二张图片
        QString src1 = srcDirPathList.at((index+1)%srcDirPathList.size());
        QImage image1(src1);
        QImage Image1 = ImageCenter(image1,ui->label_other_2);
        ui->label_other_2->setPixmap(QPixmap::fromImage(Image1));
        ui->label_other_2->setAlignment(Qt::AlignCenter);

        //待处理区域第三张图片
        QString src2 = srcDirPathList.at((index+2)%srcDirPathList.size());
        QImage image2(src2);
        QImage Image2 = ImageCenter(image2,ui->label_other_3);
        ui->label_other_3->setPixmap(QPixmap::fromImage(Image2));
        ui->label_other_3->setAlignment(Qt::AlignCenter);

        //使能左右切换图片按键
        ui->pushButton_3->setDisabled(false);
        ui->pushButton_4->setDisabled(false);

    }
    //选取一张图片时
    else if(srcDirPathListS.size()==1)
    {
        srcDirPathList =srcDirPathListS;
        srcDirPathListS.clear();
        index =0;
        QString srcDirPath = srcDirPathList.at(index);
        QImage image(srcDirPath);
        QImage Image=ImageCenter(image,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
        origin_path=srcDirPath;//目前处理的图片的原图
        //待处理区域这一张图片
        QImage images=ImageCenter(image,ui->label_other);//调整图片大小
        ui->label_other->setPixmap(QPixmap::fromImage(images));
        ui->label_other->setAlignment(Qt::AlignCenter);
        //状态栏显示图片路径
        QLabel *label=ui->statusBar->findChild<QLabel *>("status");
        label->setText(srcDirPath);

        //不使能左右切换图片按键
        ui->pushButton_3->setDisabled(true);
        ui->pushButton_4->setDisabled(true);
        //不显示剩余两个待处理区域
        ui->label_other_3->setVisible(false);
        ui->label_other_2->setVisible(false);
     }
    //选取两张图片时
    else if(srcDirPathListS.size()==2)
    {
            srcDirPathList =srcDirPathListS;
            srcDirPathListS.clear();
            index =0;
            QString srcDirPath = srcDirPathList.at(index);
            QImage image(srcDirPath);
            QImage Image=ImageCenter(image,ui->label_show);
            ui->label_show->setPixmap(QPixmap::fromImage(Image));
            ui->label_show->setAlignment(Qt::AlignCenter);
            origin_path=srcDirPath;
            //待处理区域第一张图片
            QImage images=ImageCenter(image,ui->label_other);
            ui->label_other->setPixmap(QPixmap::fromImage(images));
            ui->label_other->setAlignment(Qt::AlignCenter);
            //状态栏显示图片路径
            QLabel *label=ui->statusBar->findChild<QLabel *>("status");
            label->setText(srcDirPath);

            //待处理区域第二张图片
            QString src1 = srcDirPathList.at((index+1)%srcDirPathList.size());
            QImage image1(src1);
            QImage Image1 = ImageCenter(image1,ui->label_other_2);
            ui->label_other_2->setPixmap(QPixmap::fromImage(Image1));
            ui->label_other_2->setAlignment(Qt::AlignCenter);

            //使能左右切换图片按键
            ui->pushButton_3->setDisabled(false);
            ui->pushButton_4->setDisabled(false);
            //不显示剩余一个待处理区域
            ui->label_other_3->setVisible(false);
           }
}

//菜单栏保存处理后的图片
void MainWindow::on_action_Save_triggered()
{
    //要加水印
    if(ui->checkBox->isChecked())
    {
        //如果打开图片了
        if(ui->label_show->pixmap()!=nullptr)
        {
            QImage image2(ui->label_show->pixmap()->toImage());
            //水印图片
            QImage simage("E:/QT/ImageProcess03/MainWindow/imagesprocess/watermark.png");

            int swidth = simage.width();//水印 宽
            int sheight = simage.height();//水印 高
            int r,b,g;

            for(int i=0; i<sheight; ++i)
            {
                for(int j=0; j<swidth; ++j)
                {
                    //拿到水印的rgb像素值
                    QColor oldcolor2=QColor(simage.pixel(j,i));
                    r=oldcolor2.red();
                    b=oldcolor2.blue();
                    g=oldcolor2.green();

                    //将处理图像的对应像素值设置为0
                    if(r==0&&b==0&&g==0)
                    {
                        image2.setPixelColor(j,i,qRgb(0,0,0));
                    }
                    else
                    {
                        //image.setPixelColor(j,i,qRgb(red,blue,green));
                    }
                }

            }


            QString filename = QFileDialog::getSaveFileName(this,
                    tr("保存图片"),
                    "E:/QT/ImageProcess03/MainWindow/imagesprocess/signed_images.png",
                    tr("*.png;; *.jpg;; *.bmp;; *.tif;; *.GIF")); //选择路径

            if (filename.isEmpty())
            {
                return;
            }
            else
            {
                if (!(image2.save(filename))) //保存图像
                {
                    QMessageBox::information(this,tr("图片保存成功！"),tr("图片保存失败！"));
                    return;
                }
                else
                {
                    ui->statusBar->showMessage("图片保存成功！");
                }
            }

        }
        //如果没有打开图片
        else
        {
            QMessageBox::warning(nullptr, "提示", "请先打开图片！", QMessageBox::Yes |  QMessageBox::Yes);
        }

    }

    //不加水印
    else
    {
        //如果打开了图片
        if(ui->label_show->pixmap()!=nullptr)
        {
            QString filename = QFileDialog::getSaveFileName(this,
                tr("保存图片"),
                "E:/QT/ImageProcess03/MainWindow/imagesprocess/images.png",
                tr("*.png;; *.jpg;; *.bmp;; *.tif;; *.GIF")); //选择路径
            if (filename.isEmpty())
            {
                return;
            }
            else
            {
                if (!(ui->label_show->pixmap()->toImage().save(filename))) //保存图像
                {
                    QMessageBox::information(this,
                        tr("图片保存成功！"),
                        tr("图片保存失败！"));
                    return;
                }
                ui->statusBar->showMessage("图片保存成功！");
            }

        }
        else
        {
            QMessageBox::warning(nullptr, "提示", "请先打开图片！", QMessageBox::Yes |  QMessageBox::Yes);
        }

   }

}

//工具栏灰度化
void MainWindow::on_action_H_triggered()
{
    if(origin_path!=nullptr){
    QImage image(origin_path);

    QImage images=gray(image);

    QImage Image=ImageCenter(images,ui->label_show);
    ui->label_show->setPixmap(QPixmap::fromImage(Image));
    ui->label_show->setAlignment(Qt::AlignCenter);
    }
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//工具栏均值滤波
void MainWindow::on_action_J_triggered()
{
    if(origin_path!=nullptr){
    QImage image(origin_path);
        image=junzhi(image);
             QImage Image=ImageCenter(image,ui->label_show);
             ui->label_show->setPixmap(QPixmap::fromImage(Image));
             ui->label_show->setAlignment(Qt::AlignCenter);
}
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//工具栏边缘检测
void MainWindow::on_action_B_triggered()
{
    if(origin_path!=nullptr){
    QImage image(origin_path);
    QImage newImage =bianyuan(image);
    QImage Image=ImageCenter(newImage,ui->label_show);
    ui->label_show->setPixmap(QPixmap::fromImage(Image));
    ui->label_show->setAlignment(Qt::AlignCenter);
}
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//工具栏边缘原图复合
void MainWindow::on_action_Y_triggered()
{
    if(origin_path!=nullptr){
        QImage images(origin_path);
        QImage image2 =fuhe(images);
        QImage Image=ImageCenter(image2,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//工具栏伽马变换
void MainWindow::on_action_G_triggered()
{
    if(origin_path!=nullptr){
        QImage image(origin_path);
        image=gamma(image);
        QImage Image=ImageCenter(image,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }else{
        QMessageBox::warning(nullptr, "提示", "请先打开图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}


////////////////////////////////////////////////////////////////////////////////////
//页面按钮
//按钮打开图片
void MainWindow::on_pushButton_open_clicked()
{
    QStringList srcDirPathListS = QFileDialog::getOpenFileNames(this, tr("选择图片"), "E:/QT/ImageProcess03/MainWindow/imagesprocess", tr("图像文件(*.jpg *.png *.bmp)"));
    if(srcDirPathListS.size()>0)
    {
        ui->tabWidget->setCurrentIndex(0);
    }
    if(srcDirPathListS.size()>=3){
        srcDirPathList =srcDirPathListS;
        srcDirPathListS.clear();
        index =0;
        QString srcDirPath = srcDirPathList.at(index);
        QImage image(srcDirPath);
        QImage Image=ImageCenter(image,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
        origin_path=srcDirPath;
        QImage images=ImageCenter(image,ui->label_other);
        ui->label_other->setPixmap(QPixmap::fromImage(images));
        ui->label_other->setAlignment(Qt::AlignCenter);
        //状态栏显示图片路径
        QLabel *label=ui->statusBar->findChild<QLabel *>("status");
        label->setText(srcDirPath);

        QString src1 = srcDirPathList.at((index+1)%srcDirPathList.size());
        QImage image1(src1);
        QImage Image1 = ImageCenter(image1,ui->label_other_2);
        ui->label_other_2->setPixmap(QPixmap::fromImage(Image1));
        ui->label_other_2->setAlignment(Qt::AlignCenter);

        QString src2 = srcDirPathList.at((index+2)%srcDirPathList.size());
        QImage image2(src2);
        QImage Image2 = ImageCenter(image2,ui->label_other_3);
        ui->label_other_3->setPixmap(QPixmap::fromImage(Image2));
        ui->label_other_3->setAlignment(Qt::AlignCenter);
        ui->pushButton_3->setDisabled(false);
        ui->pushButton_4->setDisabled(false);
    }
    else if(srcDirPathListS.size()==1)
    {
        srcDirPathList =srcDirPathListS;
        srcDirPathListS.clear();
        index =0;
        QString srcDirPath = srcDirPathList.at(index);
        QImage image(srcDirPath);
        QImage Image=ImageCenter(image,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
        origin_path=srcDirPath;
        QImage images=ImageCenter(image,ui->label_other);
        ui->label_other->setPixmap(QPixmap::fromImage(images));
        ui->label_other->setAlignment(Qt::AlignCenter);
        //状态栏显示图片路径
        QLabel *label=ui->statusBar->findChild<QLabel *>("status");
        label->setText(srcDirPath);

        ui->pushButton_3->setDisabled(true);
        ui->pushButton_4->setDisabled(true);
        ui->label_other_3->setVisible(false);
        ui->label_other_2->setVisible(false);
     }
    else if(srcDirPathListS.size()==2)
    {
            srcDirPathList =srcDirPathListS;
            srcDirPathListS.clear();
            index =0;
            QString srcDirPath = srcDirPathList.at(index);
            QImage image(srcDirPath);
            QImage Image=ImageCenter(image,ui->label_show);
            ui->label_show->setPixmap(QPixmap::fromImage(Image));
            ui->label_show->setAlignment(Qt::AlignCenter);
            origin_path=srcDirPath;
            QImage images=ImageCenter(image,ui->label_other);
            ui->label_other->setPixmap(QPixmap::fromImage(images));
            ui->label_other->setAlignment(Qt::AlignCenter);
            //状态栏显示图片路径
            QLabel *label=ui->statusBar->findChild<QLabel *>("status");
            label->setText(srcDirPath);

            QString src1 = srcDirPathList.at((index+1)%srcDirPathList.size());
            QImage image1(src1);
            QImage Image1 = ImageCenter(image1,ui->label_other_2);
            ui->label_other_2->setPixmap(QPixmap::fromImage(Image1));
            ui->label_other_2->setAlignment(Qt::AlignCenter);

            ui->pushButton_3->setDisabled(false);
            ui->pushButton_4->setDisabled(false);
            ui->label_other_3->setVisible(false);
           }
}

//显示原图按钮
void MainWindow::on_pushButton_origin_clicked()
{
    if(origin_path!=nullptr){
        QImage image(origin_path);
        QImage Image=ImageCenter(image,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }else{
        QMessageBox::warning(nullptr, "提示", "请先打开图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//灰度化
void MainWindow::on_pushButton_gray_clicked()
{
    //如果打开图片
    if(origin_path!=nullptr){
        QImage image(origin_path);
        QImage images=gray(image);//调用灰度化处理函数
        QImage Image=ImageCenter(images,ui->label_show);//调整显示大小
        ui->label_show->setPixmap(QPixmap::fromImage(Image));//设置显示图片
        ui->label_show->setAlignment(Qt::AlignCenter);//显示在区域中心
    }
    //如果没有打开图片
    else
    {
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//均值滤波
void MainWindow::on_pushButton_junzhi_clicked()
{
    if(origin_path!=nullptr)
    {
        QImage image(origin_path);
        image=junzhi(image);//调用均值滤波处理函数
        QImage Image=ImageCenter(image,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }
    else
    {
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//边缘检测
void MainWindow::on_pushButton_bianyuan_clicked()
{
    if(origin_path!=nullptr){
    QImage image(origin_path);
    QImage newImage =bianyuan(image);
    QImage Image=ImageCenter(newImage,ui->label_show);
    ui->label_show->setPixmap(QPixmap::fromImage(Image));
    ui->label_show->setAlignment(Qt::AlignCenter);
}
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//边缘检测+原图复合 0.5：0.5
void MainWindow::on_pushButton_bianyuan_2_clicked()
{

    if(origin_path!=nullptr){
        QImage images(origin_path);
        QImage image2 =fuhe(images);
        QImage Image=ImageCenter(image2,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//边缘检测+原图复合 更改比例
void MainWindow::on_horizontalSlider_2_valueChanged(int value1)
{
    float value=(float)value1/100;
    //边缘检测
    if(origin_path!=nullptr){
        QImage images(origin_path);
        QImage image2 =images.convertToFormat(QImage::Format_ARGB32);
        QColor color0;
        QColor color1;
        QColor color2;
        QColor color3;
        int  r = 0;
        int g = 0;
        int b = 0;
        int rgb = 0;
        int r1 = 0;
        int g1 = 0;
        int b1 = 0;
        int rgb1 = 0;
        int a = 0;
        for( int y = 0; y < images.height() - 1; y++)
        {
            for(int x = 0; x < images.width() - 1; x++)
            {
                color0 =   QColor ( images.pixel(x,y));
                color1 =   QColor ( images.pixel(x + 1,y));
                color2 =   QColor ( images.pixel(x,y + 1));
                color3 =   QColor ( images.pixel(x + 1,y + 1));
                r = abs(color0.red() - color3.red());
                g = abs(color0.green() - color3.green());
                b = abs(color0.blue() - color3.blue());
                rgb = r + g + b;

                r1 = abs(color1.red() - color2.red());
                g1= abs(color1.green() - color2.green());
                b1 = abs(color1.blue() - color2.blue());
                rgb1 = r1 + g1 + b1;

                a = rgb + rgb1;
                a = a>255?255:a;

                image2.setPixel(x,y,qRgb(a,a,a));
            }
        }

        //复合原图
        QImage image(origin_path);
        int red, green, blue;
        int red2,green2,blue2;
        int pixels = image.width() * image.height();

        unsigned int *data = (unsigned int *)image.bits();//原图
        unsigned int *data2 = (unsigned int *)image2.bits();//边缘检测结果

        for (int i = 0; i < pixels; ++i)
        {
            red= qRed(data[i]);
            red2=qRed(data2[i])*value+red*(1-value);
            red2 = (red2 < 0x00) ? 0x00 : (red2 > 0xff) ? 0xff : red2;

            green= qGreen(data[i]);
            green2= qGreen(data2[i])*value+green*(1-value);
            green2 = (green2 < 0x00) ? 0x00 : (green2 > 0xff) ? 0xff : green2;

            blue= qBlue(data[i]);
            blue2= qBlue(data2[i])*value+blue*(1-value);
            blue2 =  (blue2  < 0x00) ? 0x00 : (blue2  > 0xff) ? 0xff : blue2 ;

            data2[i] = qRgba(red2, green2, blue2, qAlpha(data2[i]));
        }

        QImage Image=ImageCenter(image2,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
        ui->label_fuhe->setText(QString::number(value).append(":").append(QString::number(1-value)));//显示比列
    }
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//伽马变换
void MainWindow::on_pushButton_gamma_clicked()
{
    if(origin_path!=nullptr){
        QImage image(origin_path);
        image=gamma(image);
        QImage Image=ImageCenter(image,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }else{
        QMessageBox::warning(nullptr, "提示", "请先打开图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }

}

//左转
void MainWindow::on_pushButton_turnleft_clicked()
{
    if(ui->label_show->pixmap()!=nullptr)
    {
        QImage images(ui->label_show->pixmap()->toImage());
        QMatrix matrix;
        matrix.rotate(-90.0);//逆时针旋转90度
        images= images.transformed(matrix,Qt::FastTransformation);
        //QImage Image=ImageCenter(images,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(images));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//右转
void MainWindow::on_pushButton_turnright_clicked()
{
    if(ui->label_show->pixmap()!=nullptr){
        QImage images(ui->label_show->pixmap()->toImage());
        QMatrix matrix;
        matrix.rotate(90.0);//逆时针旋转90度
        images= images.transformed(matrix,Qt::FastTransformation);
        //QImage Image=ImageCenter(images,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(images));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//左右翻转
void MainWindow::on_pushButton_turn_left_right_clicked()
{
    if(ui->label_show->pixmap()!=nullptr)
    {
        QImage images(ui->label_show->pixmap()->toImage());
        images = images.mirrored(true, false);
        //QImage Image=ImageCenter(images,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(images));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//上下翻转
void MainWindow::on_pushButton_turn_up_down_clicked()
{
    if(ui->label_show->pixmap()!=nullptr){
        QImage images(ui->label_show->pixmap()->toImage());
        images = images.mirrored(false, true);
        //QImage Image=ImageCenter(images,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(images));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }

}

//处理图像切换上一张
void MainWindow::on_pushButton_3_clicked()
{
    //选取的图片大于等于三张时
    if(srcDirPathList.size()>=3)
    {
    //初始index =0; qAbs 返回绝对值
    //如果选取三张图片，srcDirPathList.size()=3，index =2
    index=qAbs(index+srcDirPathList.size()-1);
    int i = index%srcDirPathList.size();//取余

    //获得上一张图片
    QString srcDirPath = srcDirPathList.at(i);
    QImage image(srcDirPath);
    //显示在处理区域
    QImage Image=ImageCenter(image,ui->label_show);//调整图片大小
    ui->label_show->setPixmap(QPixmap::fromImage(Image));
    ui->label_show->setAlignment(Qt::AlignCenter);
    origin_path=srcDirPath;//目前处理的图片的原图
    //显示在待处理第一区域
    QImage images3=ImageCenter(image,ui->label_other);
    ui->label_other->setPixmap(QPixmap::fromImage(images3));
    ui->label_other->setAlignment(Qt::AlignCenter);
    //状态栏显示图片路径
    QLabel *label=ui->statusBar->findChild<QLabel *>("status");
    label->setText(srcDirPath);

    //将初始第二张图片显示在待处理第二区域 这块源码有点问题（无伤大雅）
    QString src1 = srcDirPathList.at(qAbs(index+srcDirPathList.size()-1)%srcDirPathList.size());
    QImage image1(src1);
    QImage Image1 = ImageCenter(image1,ui->label_other_2);
    ui->label_other_2->setPixmap(QPixmap::fromImage(Image1));
    ui->label_other_2->setAlignment(Qt::AlignCenter);

    //将初始第一张图片显示在待处理第三区域
    QString src2 = srcDirPathList.at(qAbs(index+srcDirPathList.size()-2)%srcDirPathList.size());
    QImage image2(src2);
    QImage Image2 = ImageCenter(image2,ui->label_other_3);
    ui->label_other_3->setPixmap(QPixmap::fromImage(Image2));
    ui->label_other_3->setAlignment(Qt::AlignCenter);
    }

    //选取的图片大于等于二张时
    else if(srcDirPathList.size()==2)
    {
        //初始index =0; qAbs 返回绝对值
        //如果选取二张图片，srcDirPathList.size()=2，index =1
        index=qAbs(index+srcDirPathList.size()-1);
        int i = index%srcDirPathList.size();

        QString srcDirPath = srcDirPathList.at(i);
        QImage image(srcDirPath);
        QImage Image=ImageCenter(image,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
        origin_path=srcDirPath;

        QImage images3=ImageCenter(image,ui->label_other);
        ui->label_other->setPixmap(QPixmap::fromImage(images3));
        ui->label_other->setAlignment(Qt::AlignCenter);
        //状态栏显示图片路径
        QLabel *label=ui->statusBar->findChild<QLabel *>("status");
        label->setText(srcDirPath);

        //将初始第一张图片显示在待处理第二区域
        QString src1 = srcDirPathList.at(qAbs(index+srcDirPathList.size()-1)%srcDirPathList.size());
        QImage image1(src1);
        QImage Image1 = ImageCenter(image1,ui->label_other_2);
        ui->label_other_2->setPixmap(QPixmap::fromImage(Image1));
        ui->label_other_2->setAlignment(Qt::AlignCenter);
    }
}

//处理图像切换下一张
void MainWindow::on_pushButton_4_clicked()
{

    if(srcDirPathList.size()>=3)
    {
        //初始index =0; qAbs 返回绝对值
        //如果选取三张图片，srcDirPathList.size()=3，index =1
        index=qAbs(index+1);
        int i = index%srcDirPathList.size();

        QString srcDirPath = srcDirPathList.at(i);
        QImage image(srcDirPath);
        QImage Image=ImageCenter(image,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
        origin_path=srcDirPath;

        QImage images1=ImageCenter(image,ui->label_other);
        ui->label_other->setPixmap(QPixmap::fromImage(images1));
        ui->label_other->setAlignment(Qt::AlignCenter);
        //状态栏显示图片路径
        QLabel *label=ui->statusBar->findChild<QLabel *>("status");
        label->setText(srcDirPath);

        //第三张放在第二
        QString src1 = srcDirPathList.at((index+1)%srcDirPathList.size());
        QImage image1(src1);
        QImage Image1 = ImageCenter(image1,ui->label_other_2);
        ui->label_other_2->setPixmap(QPixmap::fromImage(Image1));
        ui->label_other_2->setAlignment(Qt::AlignCenter);

        //第一张放在最后
        QString src2 = srcDirPathList.at((index+2)%srcDirPathList.size());
        QImage image2(src2);
        QImage Image2 = ImageCenter(image2,ui->label_other_3);
        ui->label_other_3->setPixmap(QPixmap::fromImage(Image2));
        ui->label_other_3->setAlignment(Qt::AlignCenter);
    }
    else if(srcDirPathList.size()==2)
    {
        //初始index =0; qAbs 返回绝对值
        //如果选取二张图片，srcDirPathList.size()=2，index =1
        index=qAbs(index+1);
        int i = index%srcDirPathList.size();
        QString srcDirPath = srcDirPathList.at(i);
        QImage image(srcDirPath);
        QImage Image=ImageCenter(image,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
        origin_path=srcDirPath;

        QImage images1=ImageCenter(image,ui->label_other);
        ui->label_other->setPixmap(QPixmap::fromImage(images1));
        ui->label_other->setAlignment(Qt::AlignCenter);
        //状态栏显示图片路径
        QLabel *label=ui->statusBar->findChild<QLabel *>("status");
        label->setText(srcDirPath);

        //把第一张显示在第二
        QString src1 = srcDirPathList.at((index+1)%srcDirPathList.size());
        QImage image1(src1);
        QImage Image1 = ImageCenter(image1,ui->label_other_2);
        ui->label_other_2->setPixmap(QPixmap::fromImage(Image1));
        ui->label_other_2->setAlignment(Qt::AlignCenter);
    }
}

//保存
void MainWindow::on_pushButton_save_clicked()
{
    //要加水印
    if(ui->checkBox->isChecked())
    {
        //打开了图片
        if(ui->label_show->pixmap()!=nullptr)
        {
            QImage image2(ui->label_show->pixmap()->toImage());
            //加载水印图片路径
            QImage simage("E:/QT/ImageProcess03/MainWindow/imagesprocess/watermark.png");
            int swidth = simage.width();
            int sheight = simage.height();
            int r,b,g;

            for(int i=0; i<sheight; ++i)
            {
                for(int j=0; j<swidth; ++j)
                {
                    QColor oldcolor2=QColor(simage.pixel(j,i));
                    r=oldcolor2.red();
                    b=oldcolor2.blue();
                    g=oldcolor2.green();

                    if(r==0&&b==0&&g==0)
                    {
                        image2.setPixelColor(j,i,qRgb(0,0,0));//加水印
                    }
                    else
                    {
                        //image.setPixelColor(j,i,qRgb(red,blue,green));
                    }
                }

             }


             QString filename = QFileDialog::getSaveFileName(this,
                    tr("保存图片"),
                    "E:/QT/ImageProcess03/MainWindow/imagesprocess/signed_images.png",//默认保存路径
                    tr("*.png;; *.jpg;; *.bmp;; *.tif;; *.GIF")); //选择路径和格式
             if (filename.isEmpty())
             {
                 return;
             }
             else
             {
                if (!(image2.save(filename))) //保存图像
                {
                    QMessageBox::information(this,
                            tr("图片保存成功！"),
                            tr("图片保存失败！"));
                        return;
                }

                ui->statusBar->showMessage("图片保存成功！");
                }

        }
        //没有打开图片
        else
        {
            QMessageBox::warning(nullptr, "提示", "请先打开图片！", QMessageBox::Yes |  QMessageBox::Yes);
        }

        }

    //不加水印
    else
    {
        //打开了图片
        if(ui->label_show->pixmap()!=nullptr)
        {
            QString filename = QFileDialog::getSaveFileName(this,
                                                            tr("保存图片"),
                                                            "E:/QT/ImageProcess03/MainWindow/imagesprocess/images.png",
                                                            tr("*.png;; *.jpg;; *.bmp;; *.tif;; *.GIF")); //选择路径
            if (filename.isEmpty())
            {
                return;
            }
            else
            {
                if (!(ui->label_show->pixmap()->toImage().save(filename))) //保存图像
                {

                    QMessageBox::information(this,
                                             tr("图片保存成功！"),
                                             tr("图片保存失败！"));
                    return;
                }
                ui->statusBar->showMessage("图片保存成功！");
            }

        }
        //没有打开图片
        else
        {
            QMessageBox::warning(nullptr, "提示", "请先打开图片！", QMessageBox::Yes |  QMessageBox::Yes);
        }
    }
}

//亮度调节
void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    if(origin_path!=nullptr)
    {
        QImage image(origin_path);
        int red, green, blue;
        int pixels = image.width() * image.height();//像素点个数
        unsigned int *data = (unsigned int *)image.bits();//无符号整型

        for (int i = 0; i < pixels; ++i)
        {
            red= qRed(data[i])+ value;
            red = (red < 0x00) ? 0x00 : (red > 0xff) ? 0xff : red;
            green= qGreen(data[i]) + value;
            green = (green < 0x00) ? 0x00 : (green > 0xff) ? 0xff : green;
            blue= qBlue(data[i]) + value;
            blue =  (blue  < 0x00) ? 0x00 : (blue  > 0xff) ? 0xff : blue ;
            data[i] = qRgba(red, green, blue, qAlpha(data[i]));
        }
        QImage Image=ImageCenter(image,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
        ui->label_light->setText(QString::number(value));//显示当前亮度值
    }
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }

}

//二值化滑动条
void MainWindow::on_horizontalSlider_erzhi_valueChanged(int value)
{
    if(origin_path!=nullptr)
    {
        QImage image(origin_path);
        QImage images=gray(image);//灰度化
        int height=images.height();
        int width=images.width();
        int bt;
        QColor oldColor;
        for (int i = 0; i < height; ++i)
        {
            for(int j=0;j<width;++j)
            {
                oldColor = QColor(images.pixel(j,i));
                bt = oldColor.red();
                if(bt<value)
                {
                    bt=0;
                }
                else
                {
                    bt=255;
                }
                images.setPixel(j,i, qRgb(bt, bt, bt));
            }
        }
    QImage Image=ImageCenter(images,ui->label_show);
    ui->label_show->setPixmap(QPixmap::fromImage(Image));
    ui->label_show->setAlignment(Qt::AlignCenter);
    ui->label_yuzhi->setText(QString::number(value));
    }
    else
    {
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//对比度滑动条
void MainWindow::on_horizontalSlider_duibi_valueChanged(int value)
{
    if(origin_path!=nullptr){
    QImage image(origin_path);
    QImage images=AdjustContrast(image,value);//调用对比度函数
    QImage Image=ImageCenter(images,ui->label_show);
    ui->label_show->setPixmap(QPixmap::fromImage(Image));
    ui->label_show->setAlignment(Qt::AlignCenter);
    }
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//饱和度滑动条
void MainWindow::on_horizontalSlider_baohe_valueChanged(int value)
{
    if(origin_path!=nullptr){
    QImage image(origin_path);
    QImage images=AdjustSaturation(image,value);//调用饱和度函数
    QImage Image=ImageCenter(images,ui->label_show);
    ui->label_show->setPixmap(QPixmap::fromImage(Image));
    ui->label_show->setAlignment(Qt::AlignCenter);
    }
    else{
        QMessageBox::warning(nullptr, "提示", "请先选择一张图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}


//改变R值滑动条
void MainWindow::on_horizontalSlider_R_valueChanged(int value)
{
    if(ui->label_show->pixmap()!=nullptr)
    {
        int value_r=value;
        int value_g=ui->horizontalSlider_G->value();
        int value_b=ui->horizontalSlider_B->value();
        QImage image(origin_path);
        QImage images=setRGB(image,value_r,value_g,value_b);
        QImage Image=ImageCenter(images,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }
    else
    {
        QMessageBox::warning(nullptr, "提示", "请先打开图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }

}

//改变G值滑动条
void MainWindow::on_horizontalSlider_G_valueChanged(int value)
{
    if(ui->label_show->pixmap()!=nullptr)
    {
        int value_r=ui->horizontalSlider_R->value();
        int value_g=value;
        int value_b=ui->horizontalSlider_B->value();
        QImage image(origin_path);
        QImage images=setRGB(image,value_r,value_g,value_b);
        QImage Image=ImageCenter(images,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }
    else
    {
        QMessageBox::warning(nullptr, "提示", "请先打开图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}

//改变B值滑动条
void MainWindow::on_horizontalSlider_B_valueChanged(int value)
{
    if(ui->label_show->pixmap()!=nullptr){
        int value_r=ui->horizontalSlider_R->value();
        int value_g=ui->horizontalSlider_G->value();
        int value_b=value;
        QImage image(origin_path);
        QImage images=setRGB(image,value_r,value_g,value_b);
        QImage Image=ImageCenter(images,ui->label_show);
        ui->label_show->setPixmap(QPixmap::fromImage(Image));
        ui->label_show->setAlignment(Qt::AlignCenter);
    }else{
        QMessageBox::warning(nullptr, "提示", "请先打开图片！", QMessageBox::Yes |  QMessageBox::Yes);
    }
}


//视频
//////////////////////////////////////////////////////////////////////////////////////
//视频处理用到的工具方法
//timer触发函数
void MainWindow::onTimeout()
{
    Mat frame;
    //读取下一帧
    double rate = capture.get(CAP_PROP_FPS);
    double nowframe=capture.get(CAP_PROP_POS_FRAMES );
    int nows=nowframe/rate;
    cout<<"nows:"<<nows<<endl;
    long totalFrameNumber = capture.get(CAP_PROP_FRAME_COUNT);
    int totals=totalFrameNumber/rate;
    cout<<"totals:"<<totals<<endl;
    ui->label_12->setText(stom(nows)+"/"+stom(totals));

    //是否读到视频
    if (!capture.read(frame))
    {
        //ui->textEdit->append(QString::fromLocal8Bit("fail to load video"));
        return;
    }

    //灰度
    if(type==1)
    {
        cvtColor(frame,frame,CV_BGR2GRAY);
    }
    //边缘检测
    else if(type==2)
    {
        cvtColor(frame, frame, CV_BGR2GRAY);

        GaussianBlur(frame, frame, Size(3, 3),0, 0, BORDER_DEFAULT);//高斯滤波
        //Canny检测
        int edgeThresh =100;
        Mat Canny_result;
        Canny(frame, frame, edgeThresh, edgeThresh * 3, 3);
    }
    //平滑
    else if(type==3)
    {
         //Smooth(frame, frame,Size(3, 3), 0, 0);
         GaussianBlur(frame, frame, Size(3, 3), 0, 0);//高斯滤波
    }
    //二值化
    else if(type==4)
    {
        cvtColor(frame,frame,CV_BGR2GRAY);
        threshold(frame, frame, 96, 255, THRESH_BINARY);
    }
    //马赛克
    else if (type==5)
    {
        frame=masaike(frame);
    }


    QImage image=MatToQImage(frame);
    //缩放
    double scale=ui->horizontalSlider_suofang->value()/100.0;


    QSize qs = ui->label_11->rect().size()*scale;
    ui->label_11->setPixmap(QPixmap::fromImage(image).scaled(qs));
    ui->label_11->setAlignment(Qt::AlignCenter);
    ui->label_11->repaint();
    //这里加滤波程序

    //long totalFrameNumber = capture.get(CAP_PROP_POS_FRAMES);

   // ui->textEdit->append(QString::fromLocal8Bit("正在读取第：第 %1 帧").arg(totalFrameNumber));
}

//进度条随视频移动
void MainWindow::updatePosition(){
    long totalFrameNumber = capture.get(CAP_PROP_FRAME_COUNT);
    ui->VideohorizontalSlider_2->setMaximum(totalFrameNumber);
    long frame=capture.get(CAP_PROP_POS_FRAMES );
    ui->VideohorizontalSlider_2->setValue(frame);
}

//秒转分函数
QString MainWindow::stom(int s){
    QString m;
    if(s/60==0){
        m=QString::number (s%60);
    }else{
        m=QString::number (s/60)+":"+QString::number (s%60);
    }
    return m;
}


////////////////////////////////////////////////////////////////////////////////////
//视频处理算法
//马赛克
Mat MainWindow::masaike(Mat src){
        int width = src.rows;	//图片的长度
        int height = src.cols;	//图片的宽度

        //10*10的像素点进行填充
        int arr = 10;

        //i和j代表了矩形区域的左上角的像素坐标
        for (int i = width/2.5; i < width/1.5; i+=arr) {
            for (int j = height/2.5; j < height/1.5; j+=arr) {
            //对矩形区域内的每一个像素值进行遍历
                for (int k = i; k < arr + i && k < width; k++) {
                    for (int m = j; m < arr + j && m < height; m++) {
                        //在这里进行颜色的修改
                        src.at<Vec3b>(k, m)[0] = src.at<Vec3b>(i, j)[0];
                        src.at<Vec3b>(k, m)[1] = src.at<Vec3b>(i, j)[1];
                        src.at<Vec3b>(k, m)[2] = src.at<Vec3b>(i, j)[2];
                    }
                }
            }
        }
        return src;
}



///////////////////////////////////////////////////////////////////////////////
//打开视频
void MainWindow::on_action_V_triggered()
{
    QString video_path = QFileDialog::getOpenFileName(this,tr("选择视频"),"E:/QT/ImageProcess03/MainWindow/imagesprocess",tr("Video (*.WMV *.mp4 *.rmvb *.flv)"));
    if(video_path!=nullptr)
    {
        //打开视频文件：其实就是建立一个VideoCapture结构
        capture.open(video_path.toStdString());
        //检测是否正常打开:成功打开时，isOpened返回ture
        if (!capture.isOpened())
        {
            QMessageBox::warning(nullptr, "提示", "打开视频失败！", QMessageBox::Yes |  QMessageBox::Yes);
        }
        //显示视频处理页面
        ui->tabWidget->setCurrentIndex(1);
        //播放键使能
        ui->pushButton_6->setEnabled(true);
        //获取整个帧数
        long totalFrameNumber = capture.get(CAP_PROP_FRAME_COUNT);
        //ui->textEdit->append(QString::fromLocal8Bit("totally %1 frames").arg(totalFrameNumber));
        //ui->label_11->resize(QSize(capture.get(CAP_PROP_FRAME_WIDTH), capture.get(CAP_PROP_FRAME_HEIGHT)));

        //设置开始帧()
        long frameToStart = 0;
        capture.set(CAP_PROP_POS_FRAMES, frameToStart);
        //ui->textEdit->append(QString::fromLocal8Bit("from %1 frame").arg(frameToStart));

        //获取帧率
        double rate = capture.get(CAP_PROP_FPS);
        //ui->textEdit->append(QString::fromLocal8Bit("Frame rate: %1 ").arg(rate));


        delay = 1000 / rate;
        timer.start(delay);
        type=0;
        //timer.start();
        isstart=!isstart;
        ui->pushButton_6->setStyleSheet("background-image: url(:/myImage/image/stop.png);");
    }
}

//暂停/播放
void MainWindow::on_pushButton_6_clicked()
{
    if(isstart)
    {
        timer.stop();
        isstart=false;
        ui->pushButton_6->setStyleSheet("background-image: url(:/myImage/image/start.png);");
    }else {
        timer.start(delay);
        isstart=true;
        ui->pushButton_6->setStyleSheet("background-image: url(:/myImage/image/stop.png);");
    }
}

//进度条
void MainWindow::on_VideohorizontalSlider_2_valueChanged(int value)
{
    capture.set(CAP_PROP_POS_FRAMES, value);
}

//原画
void MainWindow::on_pushButton_8_clicked()
{
    type=0;
}

//灰度
void MainWindow::on_pushButton_7_clicked()
{
    type=1;
}

//均值滤波
void MainWindow::on_pushButton_9_clicked()
{
    type=2;
}

//平滑
void MainWindow::on_pushButton_10_clicked()
{
    type=3;
}

//二值化
void MainWindow::on_pushButton_11_clicked()
{
    type=4;
}

//马赛克
void MainWindow::on_pushButton_2_clicked()
{
    type=5;
}

//缩放
void MainWindow::on_horizontalSlider_suofang_valueChanged(int value)
{

    ui->label_suofangvalue->setText(QString::number(value/100.0));
    cout<<value<<endl;
}





