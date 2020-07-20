#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

#include <QMainWindow>
#include <QMimeData>
#include <QDir>
#include <QFileInfo>
#include <QDropEvent>
#include <QDebug>
#include <QImageReader>
#include <QLabel>
#include <QPixmap>
#include <QFrame>
#include <QLayout>
#include <QStyle>
#include <QVariantAnimation>
#include <QCollator>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

const QList<const char*> supportedTypes = {"jpg", "jpeg", "png", "bmp", "gif", "pbm", "pgm", "ppm", "xbm", "xpm", "svg"};
const int gap = 5; // 图像间间距
const int prefetchNumber = 5; // 预取数量（双向）

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_read_r2l_triggered(bool checked);
    void on_read_l2r_triggered(bool checked);
    void on_animation_key_triggered(bool checked);

private:
    Ui::MainWindow *ui;
    QString filePath;
    QList<QString> files;
    QLabel* imgs[4]; // imgs[3]用于填补在左边或者右边
    enum Position {none, left, right} pos = none; // 填补位置
    int focusId = -1; // 当前阅读页
    int offset = 0; // 滑动位移
    int lastMouseX = 0;
    bool mousePressed = false;
    bool reversed = true; // true-向右滑动翻页；false-向左滑动翻页
    bool animationKey = true; // 按方向键翻页时是否显示动画
    int imageHeight, imageTop;
    QVariantAnimation *ani = nullptr;
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
    void keyPressEvent(QKeyEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void resizeEvent(QResizeEvent*);
    void loadImage(); // 从文件夹加载图片
    void setOneImage(QLabel*, const int&); // 按id加载图片到指定QLabel
    void adjustImage(QLabel*); // 调整QLabel尺寸
    void arangeImage(); // 排列3个图像
    void shiftImage(bool); // 移动并加载新图像，true-下一张，false-上一张
    void slideAnimation(bool); // 创建滑动动画，true-向右滚动，false-向左滚动
};
#endif // MAINWINDOW_H
