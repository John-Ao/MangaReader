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
#include <QMimeDatabase>
#include <QMap>
#include <QList>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

const int prefetchNumber = 4; // 预取数量（双向）

class ImgMap {
public:
    QMap<int, QLabel*> map;
    int offset = 0;
    QLabel*& operator[](const int &key) {
        return map[key + offset];
    }
    QLabel* get(const int &key) {
        auto ptr = map.find(key + offset);
        if (ptr == map.end()) {
            return nullptr;
        } else {
            return *ptr;
        }
    }
    void shift(const int &v) {
        offset += v;
    }
    void remove(const int &key) {
        map.remove(key + offset);
    }
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void openPath(const QString&); // 加载路径

private slots:
    void on_read_r2l_triggered(bool checked);
    void on_read_l2r_triggered(bool checked);
    void on_animation_key_triggered(bool checked);

    void on_read_slide_triggered();

    void on_no_gap_triggered(bool checked);

private:
    Ui::MainWindow *ui;
    QString filePath;
    QList<QString> files;
    int gap = 5; // 图像间间距
    ImgMap imgs; // 用于显示图片的容器
    QList<QLabel*> imgsBank; // 备用库
    enum Position {none, left, right} pos = none; // 填补位置
    int focusId = 0; // 当前阅读页
    int offset = 0; // 滑动位移
    int lastMouseX = 0, lastMouseY = 0;
    bool mousePressed = false;
    bool reversed = true; // true-向右滑动翻页；false-向左滑动翻页
    bool sliding = false; // true-上下滑动模式
    bool animationKey = true; // 按方向键翻页时是否显示动画
    int imageHeight, imageWidth = 0, imageTop;
    double imageSlide = 0;
    QMimeDatabase db;
    QVariantAnimation *ani = nullptr;
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
    void keyPressEvent(QKeyEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void resizeEvent(QResizeEvent*);
    QLabel* newImg(); // 创建新的img对象
    void deleteImg(QLabel*); // "删除"img对象
    void loadImage(); // 从文件夹加载图片，将imgs填满
    void setOneImage(QLabel*, const int&); // 按id加载图片到指定QLabel
    void adjustImage(QLabel*); // 调整QLabel尺寸
    void arrangeImage(); // 排列可见图像并根据需要创建新图像
    void shiftImage(bool); // 加载新图像并修改offset，true-左侧图像，false-右侧图像
    void slideAnimation(); // 创建滑动动画
};
#endif // MAINWINDOW_H
