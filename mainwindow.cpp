#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    auto layout = this->layout();
    for (auto& img : imgs) {
        img = new QLabel();
        img->setBackgroundRole(QPalette::Base);
        img->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        img->setScaledContents(true);
        img->setAlignment(Qt::AlignmentFlag::AlignCenter | Qt::AlignmentFlag::AlignHCenter);
        img->setFrameShape(QFrame::Shape::Box);
        layout->addWidget(img);
    }
}

MainWindow::~MainWindow() {
    delete ui;
    for (auto& img : imgs) {
        delete img;
    }
}

bool checkFile(QString file) {
    for (auto i : supportedTypes) {
        if (file.compare(i) == 0) {
            return true;
        }
    }
    return false;
}

template<class T>
inline T min(T a, T b) {
    return a <= b ? a : b;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    bool accept = false;
    if (event->mimeData()->hasUrls()) {
        auto urls = event->mimeData()->urls();
        if (!urls.empty()) {
            auto path = QDir::cleanPath(urls.at(0).toLocalFile());
            QFileInfo file(path);
            if (file.isFile()) {
                auto fn = file.fileName();
                accept = checkFile(file.suffix());
            } else if (file.isDir()) {
                accept = true;
            }
            if (accept) {
                event->acceptProposedAction();
            }
        }
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    // this is not triggered if not accepted in dropEnterEvent()
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
    auto path = QDir::cleanPath(event->mimeData()->urls().at(0).toLocalFile());
    QFileInfo file(path);
    filePath = file.path() + "/";
    files.clear();
    focusId = 0;
    if (file.isFile()) {
        auto fn = file.fileName();
        for (auto i : file.dir().entryInfoList()) {
            if (checkFile(i.suffix())) {
                auto n = i.fileName();
                files.append(n);
                if (n == fn) {
                    focusId = files.size() - 1;
                }
            }
        }
    } else if (file.isDir()) {
        for (auto i : file.dir().entryInfoList()) {
            if (checkFile(i.suffix())) {
                files.append(i.fileName());
            }
        }
    }
    loadImage();
    QApplication::restoreOverrideCursor();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    auto h = this->height() - ui->statusbar->height(), w = this->width();
    for (auto& img : imgs) {
        auto pixmap = img->pixmap();
        if (pixmap != nullptr) {
            auto ih = pixmap->height(), iw = pixmap->width();
            auto ww = iw * h / ih;
            if (ww > w) {
                img->resize(w, ih * w / iw);
            } else {
                img->resize(ww, h);
            }
        }
    }
    arangeImage();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    processKey(event->key());
    arangeImage();
}

void MainWindow::processKey(int key) {
    bool next;
    if (key == Qt::Key_Left && focusId > 0) {
        next = false;
    } else if (key == Qt::Key_Right && focusId < files.size() - 1) {
        next = true;
    } else {
        return;
    }
    QLabel* tmp;
    if (next ^ reversed) {
        tmp = imgs[0];
        imgs[0] = imgs[1];
        imgs[1] = imgs[2];
        imgs[2] = tmp;
    } else {
        tmp = imgs[2];
        imgs[2] = imgs[1];
        imgs[1] = imgs[0];
        imgs[0] = tmp;
    }
    if (next) {
        ++focusId;
        if (focusId < files.size() - 1) {
            setOneImage(tmp, focusId + 1);
            tmp->setVisible(true);
        } else {
            tmp->setVisible(false);
        }
    } else {
        --focusId;
        if (focusId > 0) {
            setOneImage(tmp, focusId - 1);
            tmp->setVisible(true);
        } else {
            tmp->setVisible(false);
        }
    }
    ui->statusbar->showMessage(QString("%1    %2/%3").arg(files[focusId]).arg(focusId + 1).arg(files.size()));
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (mousePressed) {
        offset = event->x() - lastMouseX;
        arangeImage();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    mousePressed = true;
    lastMouseX = event->x();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    mousePressed = false;
    bool left;
    bool isClick = false;
    if (abs(offset) > 30) {
        left = offset > 0;
    } else {
        auto w = this->width(), x = event->x();
        if (x < w / 3) {
            left = false;
            isClick = true;
        } else if (x > 2 * w / 3) {
            left = true;
            isClick = true;
        } else {
            return;
        }
        offset = 0;
    }

    int key;
    bool legal;
    if (left ^ reversed) {
        key = Qt::Key_Left;
        legal = focusId > 0;
    } else {
        key = Qt::Key_Right;
        legal = focusId < files.size() - 1;
    }
    arangeImage();
    processKey(key);
    if (legal) {
        if (left) {
            offset -= (imgs[1]->width() + imgs[0]->width()) / 2 + gap;
        } else {
            offset += (imgs[1]->width() + imgs[2]->width()) / 2 + gap;
        }
    }
    arangeImage();
    if (offset != 0) {
        if (ani != nullptr) {
            delete ani;
        }
        ani = new QVariantAnimation();
        ani->setDuration(300);
        ani->setStartValue(offset);
        ani->setEndValue(0);
        ani->setEasingCurve(QEasingCurve::InOutCubic);
        connect(ani, &QVariantAnimation::valueChanged, [this](const QVariant & value) {
            offset = value.toInt();
            arangeImage();
            QApplication::processEvents();
        });
        ani->start();
    }
}

void MainWindow::loadImage() {
    for (int i = 0, t; i < 3; ++i) {
        if (reversed) {
            t = focusId + 1 - i;
        } else {
            t = focusId - 1 + i;
        }
        if (t >= 0 && t < files.size()) {
            setOneImage(imgs[i], t);
            imgs[i]->setVisible(true);
        } else {
            imgs[i]->setVisible(false);
        }
    }
    arangeImage();
    ui->statusbar->showMessage(QString("%1    %2/%3").arg(files[focusId]).arg(focusId + 1).arg(files.size()));
}

void MainWindow::setOneImage(QLabel* label, const int& id) {
    auto path = filePath + files[id];
    auto img = QPixmap::fromImage(QImageReader(path).read());
    auto h = this->height() - ui->statusbar->height(), w = this->width();
    if (img.isNull()) {
        label->setText(tr("Cannot open this file\n") + path);
        label->setStyleSheet("background-color:white; font-size:20px; color:red;");
    } else {
        label->setPixmap(img);
        auto ih = img.height(), iw = img.width();
        auto ww = iw * h / ih;
        if (ww > w) {
            label->resize(w, ih * w / iw);
        } else {
            label->resize(ww, h);
        }
    }
}

void MainWindow::arangeImage() {
    auto h = this->height() - ui->statusbar->height(), w = this->width();
    auto left = (w - imgs[1]->width()) / 2 + offset;
    imgs[1]->move(left, (h - imgs[1]->height()) / 2);
    imgs[0]->move(left - imgs[0]->width() - gap, (h - imgs[0]->height()) / 2);
    imgs[2]->move(left + imgs[1]->width() + gap, (h - imgs[2]->height()) / 2);
}
