#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QLabel>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <QTimer>
#include <QImage>
#include <QMessageBox>
using namespace cv;


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //图片工具函数
    QImage  MatToQImage(const cv::Mat& mat);//Mat转换为QImage
    QImage ImageCenter(QImage  qimage,QLabel *qLabel);//调整图片比例
    QImage setRGB(QImage image,int value_r, int value_g, int value_b);//调整rgb函数调用

    //图像处理函数
    QImage gray(QImage image);//图片灰度化
    QImage junzhi(QImage image);//图片均值滤波
    QImage bianyuan(QImage image);//图片边缘检测
    QImage fuhe(QImage images);//图片+边缘检测符合调用
    QImage gamma(QImage image);//伽马变换
    QImage AdjustContrast(QImage image, int value);//对比度
    QImage AdjustSaturation(QImage Img, int iSaturateValue);//饱和度


    //视频工具函数
    QString stom(int s);//秒转分函数
    //视频处理函数
    Mat masaike(Mat image);//马赛克


private slots:

    //图片
    void on_action_Dock_triggered();//菜单栏工具箱显示
    void on_action_L_triggered();//菜单栏文字翻译
    void on_action_About_triggered();//菜单栏帮助

    void on_action_Open_triggered();//菜单栏打开文件
    void on_action_Save_triggered();//菜单栏保存处理后的图片
    void on_action_H_triggered();//工具栏灰度化
    void on_action_J_triggered();//工具栏均值滤波
    void on_action_B_triggered();//工具栏边缘检测
    void on_action_Y_triggered();//工具栏边缘原图复合
    void on_action_G_triggered();//工具栏伽马变换


    void on_pushButton_open_clicked();//打开文件按钮
    void on_pushButton_origin_clicked();//显示原图按钮
    void on_pushButton_gray_clicked();//图片灰度化按钮
    void on_pushButton_junzhi_clicked();//图片均值滤波按钮
    void on_pushButton_bianyuan_clicked();//图片边缘检测按钮
    void on_pushButton_bianyuan_2_clicked();//图片边缘检测+原图复合按钮 0.5：0.5
    void on_horizontalSlider_2_valueChanged(int value1);//图片边缘检测+原图复合滑动条 更改比例
    void on_pushButton_gamma_clicked();//图片伽马变换按钮

    void on_pushButton_turnleft_clicked();//图片左转按钮
    void on_pushButton_turnright_clicked();//图片右转按钮
    void on_pushButton_turn_left_right_clicked();//左右翻转
    void on_pushButton_turn_up_down_clicked();//上下翻转

    void on_pushButton_3_clicked();//上一张图片按钮
    void on_pushButton_4_clicked();//下一张图片按钮
    void on_pushButton_save_clicked();//保存图片按钮

    void on_horizontalSlider_valueChanged(int value);//图片亮度调节滑动条
    void on_horizontalSlider_erzhi_valueChanged(int value);//二值化调节滑动条
    void on_horizontalSlider_duibi_valueChanged(int value);//对比度调节滑动条
    void on_horizontalSlider_baohe_valueChanged(int value);//饱和度调节滑动条


    void on_horizontalSlider_R_valueChanged(int value);//改变R值滑动条
    void on_horizontalSlider_G_valueChanged(int value);//改变G值滑动条
    void on_horizontalSlider_B_valueChanged(int value);//改变B值滑动条

    //视频
    void onTimeout();//timer触发函数
    void updatePosition();//进度条随视频移动
    void on_action_V_triggered();//菜单栏打开视频
    void on_pushButton_6_clicked();//暂停/播放
    void on_VideohorizontalSlider_2_valueChanged(int value);//进度条

    void on_pushButton_8_clicked();//原画
    void on_pushButton_7_clicked();//灰度
    void on_pushButton_9_clicked();//均值滤波
    void on_pushButton_10_clicked();//平滑
    void on_pushButton_11_clicked();//二值化
    void on_pushButton_2_clicked();//马赛克
    void on_horizontalSlider_suofang_valueChanged(int value);//缩放





private:
    int type=0;//视频操作类型
    int index =0;//图片索引

    bool language=true;//语言
    bool isstart=false;//视频播放标志位
    QString origin_path;//目前处理的图片的原图
    QString videoSrcDir;//视频路径
    cv::VideoCapture capture; //用来读取视频结构
    QTimer timer;//视频播放的定时器
    int beishu;//调节播放速率
    int delay;//帧延迟时间
    QMessageBox customMsgBox;
    QStringList srcDirPathList;//图片list



private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
