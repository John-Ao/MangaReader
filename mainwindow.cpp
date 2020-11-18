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
    imgs[1]->setText("将图片或者文件夹拖入窗口以开始\n\n"
                     "滑动、左右方向键、点击页面的左右侧均可翻页\n\n"
                     "在[选项]中可以更改阅读方向");
    imgs[1]->setStyleSheet("font-size:20px;");
}

MainWindow::~MainWindow() {
    delete ui;
    for (auto& img : imgs) {
        delete img;
    }
}

bool checkFile(QString file) {
    for (auto i : QImageReader::supportedImageFormats()) {
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
    openPath(path);
    QApplication::restoreOverrideCursor();
}

void MainWindow::openPath(const QString& path) {
    QFileInfo file(path);
    files.clear();
    focusId = 0;
    if (file.isFile()) {
        filePath = file.path() + "/";
        auto fn = file.fileName();
        for (auto i : file.dir().entryInfoList()) {
            if (checkFile(i.suffix())) {
                auto n = i.fileName();
                files.append(n);
            }
            QCollator co;
            co.setNumericMode(true);
            std::sort(files.begin(), files.end(), [&co](const QString & s1, const QString & s2) {
                return co.compare(s1, s2) < 0;
            });
            for (int i = 0, j = files.size(); i < j; ++i) {
                if (files[i] == fn) {
                    focusId = i;
                }
            }
        }
    } else if (file.isDir()) {
        filePath = path + "/";
        for (auto i : QDir(path).entryInfoList()) {
            if (checkFile(i.suffix())) {
                files.append(i.fileName());
            }
        }
        QCollator co;
        co.setNumericMode(true);
        std::sort(files.begin(), files.end(), [&co](const QString & s1, const QString & s2) {
            return co.compare(s1, s2) < 0;
        });
    }
    loadImage();
}

void MainWindow::resizeEvent(QResizeEvent*) {
    auto &h = imageHeight = this->height() - ui->statusbar->height() - (imageTop = ui->menuBar->height()), w = this->width();
    int ih, iw;
    for (auto& img : imgs) {
        auto p = img->pixmap();
        if (p == nullptr) {
            ih = iw = 1;
        } else {
            ih = p->height();
            iw = p->width();
        }
        auto ww = iw * h / ih;
        if (ww > w) {
            img->resize(w, ih * w / iw);
        } else {
            img->resize(ww, h);
        }
    }
    arangeImage();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    auto key = event->key();
    bool left;
    if ((left = key == Qt::Key_Left) || key == Qt::Key_Right) {
        offset = 0;
        if (ani != nullptr) {
            ani->stop();
            ani->deleteLater();
            ani = nullptr;
        }
        if (animationKey) {
            slideAnimation(left ^ reversed);
        } else {
            if (left && focusId > 0) {
                shiftImage(false);
            } else if (!left && focusId < files.size() - 1) {
                shiftImage(true);
            }
            arangeImage();
        }
    }
}

void MainWindow::shiftImage(bool next) {
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
            tmp->clear();
            tmp->setVisible(false);
        }
    } else {
        --focusId;
        if (focusId > 0) {
            setOneImage(tmp, focusId - 1);
            tmp->setVisible(true);
        } else {
            tmp->clear();
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
    if (ani != nullptr) {
        ani->stop();
        offset = 0;
        arangeImage();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    mousePressed = false;
    bool left;
    if (abs(offset) > 30) {
        left = offset > 0;
    } else {
        auto w = this->width(), x = event->x();
        if (x < w / 3) {
            left = false;
        } else if (x > 2 * w / 3) {
            left = true;
        } else {
            return;
        }
        offset = 0;
    }
    slideAnimation(left);
}

void MainWindow::slideAnimation(bool left) {
    bool prev, legal;
    if ((prev = left ^ reversed)) {
        legal = focusId > 0;
    } else {
        legal = focusId < files.size() - 1;
    }
    arangeImage();
    if (legal) {
        if (left) {
            offset -= (imgs[1]->width() + imgs[0]->width()) / 2 + gap;
            auto p = imgs[2]->pixmap();
            if (p != nullptr) {
                imgs[3]->setPixmap(*p);
                adjustImage(imgs[3]);
                pos = Position::right;
            }
        } else {
            offset += (imgs[1]->width() + imgs[2]->width()) / 2 + gap;
            auto p = imgs[0]->pixmap();
            if (p != nullptr) {
                imgs[3]->setPixmap(*p);
                adjustImage(imgs[3]);
                pos = Position::left;
            }
        }
        shiftImage(!prev);
    }
    arangeImage();
    if (offset != 0) {
        if (ani != nullptr) {
            ani->stop();
            ani->deleteLater();
        }
        ani = new QVariantAnimation();
        ani->setDuration(min(300, abs(offset) * 3 / 4));
        ani->setStartValue(offset);
        ani->setEndValue(0);
        ani->setEasingCurve(QEasingCurve::InOutCubic);
        connect(ani, &QVariantAnimation::valueChanged, [this](const QVariant & value) {
            offset = value.toInt();
            arangeImage();
            QApplication::processEvents();
        });
        connect(ani, &QVariantAnimation::finished, [this]() {
            this->pos = none;
            arangeImage();
        });
        ani->start();
    }
}

void MainWindow::loadImage() {
    if (focusId < 0) {
        return;
    }
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
    auto mime = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    auto img = QPixmap::fromImage(QImageReader(path, mime.preferredSuffix().toUtf8()).read());
    if (img.isNull()) {
        label->setText(tr("Cannot open this file\n") + path);
        label->setStyleSheet("background-color:white; font-size:20px; color:red;");
    } else {
        label->setPixmap(img);
        label->setStyleSheet("color:black;");
        adjustImage(label);
    }
}

void MainWindow::adjustImage(QLabel *label) {
    auto &h = imageHeight, w = this->width();
    auto img = label->pixmap();
    auto ih = img->height(), iw = img->width();
    auto ww = iw * h / ih;
    if (ww > w) {
        label->resize(w, ih * w / iw);
    } else {
        label->resize(ww, h);
    }
}

void MainWindow::arangeImage() {
    auto &h = imageHeight, w = this->width();
    auto left = (w - imgs[1]->width()) / 2 + offset;
    imgs[1]->move(left, imageTop + (h - imgs[1]->height()) / 2);
    imgs[0]->move(left - imgs[0]->width() - gap, imageTop + (h - imgs[0]->height()) / 2);
    imgs[2]->move(left + imgs[1]->width() + gap, imageTop + (h - imgs[2]->height()) / 2);
    if (pos == Position::left) {
        imgs[3]->move(left - imgs[0]->width() - gap * 2 - imgs[3]->width(), imageTop + (h - imgs[3]->height()) / 2);
        imgs[3]->setVisible(true);
    } else if (pos == Position::right) {
        imgs[3]->move(left + imgs[1]->width() + gap * 2 + imgs[2]->width(), imageTop + (h - imgs[3]->height()) / 2);
        imgs[3]->setVisible(true);
    } else {
        imgs[3]->setVisible(false);
    }
}

void MainWindow::on_read_r2l_triggered(bool checked) {
    ui->read_l2r->setChecked(!(reversed = checked));
    loadImage();
}

void MainWindow::on_read_l2r_triggered(bool checked) {
    ui->read_r2l->setChecked((reversed = !checked));
    loadImage();
}

void MainWindow::on_animation_key_triggered(bool checked) {
    animationKey = checked;
}
