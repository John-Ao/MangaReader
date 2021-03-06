﻿#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setMinimumSize(450, 250);
    panel = new QLabel();
    auto layout = this->layout();
    panel->setAttribute(Qt::WA_TranslucentBackground);
    layout->addWidget(panel);
    imgContainer = new QWidget();
    layout->addWidget(imgContainer);
    imgContainer->stackUnder(panel);
    imgs[0] = newImg();
    imgs[0]->setText("将图片或者文件夹拖入窗口以开始\n\n"
                     "滑动、左右方向键、点击页面的左右侧均可翻页\n\n"
                     "在[选项]中可以更改阅读方向");
    imgs[0]->setStyleSheet("font-size:20px;");
}

MainWindow::~MainWindow() {
    delete ui;
}

QLabel* MainWindow::newImg() {
    QLabel*img;
    if (imgsBank.empty()) {
        img = new QLabel();
        img->setBackgroundRole(QPalette::Base);
        img->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        img->setScaledContents(true);
        img->setAlignment(Qt::AlignmentFlag::AlignCenter | Qt::AlignmentFlag::AlignHCenter);
        img->setFrameShape(QFrame::Shape::Box);
        img->setParent(imgContainer);
    } else {
        img = imgsBank.back();
        imgsBank.pop_back();
    }
    img->setFrameStyle(noGap ? QFrame::NoFrame : QFrame::Panel);
    return img;
}

void MainWindow::deleteImg(QLabel* img) {
    imgsBank.append(img);
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
    auto isFile = file.isFile();
    if (isFile) {
        filePath = file.path() + "/";
    } else {
        filePath = path + "/";
    }
    for (auto i : QDir(filePath).entryInfoList()) {
        if (checkFile(i.suffix())) {
            files.append(i.fileName());
        }
    }
    QCollator co;
    co.setNumericMode(true);
    std::sort(files.begin(), files.end(), [&co](const QString & s1, const QString & s2) {
        return co.compare(s1, s2) < 0;
    });
    if (isFile) {
        auto fn = file.fileName();
        for (int i = 0, j = files.size(); i < j; ++i) {
            if (files[i] == fn) {
                focusId = i;
                break;
            }
        }
    }
    loadImage();
    arrangeImage();
}

void MainWindow::resizeEvent(QResizeEvent*) {
    auto h = imageHeight = this->height() - ui->statusBar->height() - 3 - (imageTop = ui->menuBar->height());
    auto w = this->width();
    if (sliding && imageWidth != 0) {
        imageSlide = imageSlide * w / imageWidth;
    }
    imageWidth = w;
    if (files.empty()) {
        imgs[0]->resize(w, h);
    } else {
        for (auto img : imgs.map.values()) {
            img->setVisible(false);
            adjustImage(img);
        }
    }
    panel->resize(w, h);
    panel->move(0, imageTop);
    imgContainer->resize(w, h);
    imgContainer->move(0, imageTop);
    arrangeImage();
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
        if (animationKey && !sliding) {
            shiftImage(left ^ reversed);
            slideAnimation();
        } else {
            shiftImage(left);
            offset = 0;
            arrangeImage();
        }
    }
}

void MainWindow::shiftImage(bool left) {
    if (left) {
        auto id = focusId + (!sliding && reversed ? 1 : -1);
        if (id >= 0 && id < files.size()) {
            focusId = id;
            imgs.offset -= 1;
            auto img = imgs.get(0);
            if (img == nullptr) {
                img = imgs[0] = newImg();
                setOneImage(img, id);
            }
            if (sliding) {
                auto tmp = imgs[0]->height() + gap;
                offset -= tmp;
                lastMouseY += tmp;
            } else {
                auto tmp = (imgs[0]->width() + imgs[1]->width()) / 2 + gap;
                offset -= tmp;
                lastMouseX += tmp;
            }
        }
    } else {
        auto id = focusId + (!sliding && reversed ? -1 : 1);
        if (id >= 0 && id < files.size()) {
            focusId = id;
            imgs.offset += 1;
            auto img = imgs.get(0);
            if (img == nullptr) {
                img = imgs[0] = newImg();
                setOneImage(img, id);
            }
            if (sliding) {
                auto tmp = imgs[-1]->height() + gap;
                offset += tmp;
                lastMouseY -= tmp;
            } else {
                auto tmp = (imgs[0]->width() + imgs[-1]->width()) / 2 + gap;
                offset += tmp;
                lastMouseX -= tmp;
            }
        }
    }
    if (0 <= focusId && focusId < files.size()) {
        ui->statusBar->showMessage(QString("%1    %2/%3").arg(files[focusId]).arg(focusId + 1).arg(files.size()));
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (mousePressed) {
        if (sliding) {
            offset = event->y() - lastMouseY;
            if (imageSlide + offset > imageHeight / 2 + 20) {
                shiftImage(true);
            } else if (imageSlide + offset + imgs[0]->height() < imageHeight / 2 - 20) {
                shiftImage(false);
            }
        } else {
            offset = event->x() - lastMouseX;
            if (offset > imgs[0]->width() / 2 + 20) {
                shiftImage(true);
            } else if (offset < -imgs[0]->width() / 2 - 20) {
                shiftImage(false);
            }
        }
        arrangeImage();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    mousePressed = true;
    lastMouseX = event->x();
    lastMouseY = event->y();
    if (ani != nullptr) {
        ani->stop();
        ani->deleteLater();
        ani = nullptr;
        offset = 0;
        arrangeImage();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    mousePressed = false;
    if (sliding) {
        int tmp;
        if ((focusId == 0 || files.empty()) && (tmp = imageSlide + offset + imgs[0]->height() - imageHeight) > 0 && imageSlide + offset > 0) {
            offset = tmp;
            imageSlide = imageHeight - imgs[0]->height();
            if (imageSlide < 0) {
                offset += imageSlide;
                imageSlide = 0;
            }
            slideAnimation();
        } else if ((focusId == files.size() - 1 || files.empty()) && (tmp = imageSlide + offset) < 0 && imageSlide + offset + imgs[0]->height() < imageHeight) {
            offset = tmp;
            imageSlide = 0;
            if ((tmp = imgs[0]->height() - imageHeight) > 0) {
                imageSlide = -tmp;
                offset += tmp;
            }
            slideAnimation();
        } else {
            imageSlide += offset;
            offset = 0;
        }
    } else {
        if (abs(offset) > 30) {
            slideAnimation();
        } else {
            auto w = this->width(), x = event->x();
            if (x < w / 3) {
                shiftImage(false);
            } else if (x > 2 * w / 3) {
                shiftImage(true);
            } else {
                return;
            }
            if (!animationKey) {
                offset = 0;
                arrangeImage();
            } else {
                slideAnimation();
            }
        }
    }
}

void MainWindow::slideAnimation() {
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
            arrangeImage();
            QApplication::processEvents();
        });
//        connect(ani, &QVariantAnimation::finished, [this]() {
//            this->pos = none;
//            arrangeImage();
//        });
        ani->start();
    }
}

void MainWindow::loadImage() {
    if (focusId < 0 || focusId >= files.size()) {
        return;
    }
    for (auto key : imgs.map.keys()) {
        auto t = key - imgs.offset;
        t = focusId + (!sliding && reversed ? -t : t);
        if (t >= 0 && t < files.size()) {
            setOneImage(imgs.map[key], t);
        }
    }
}

void MainWindow::setOneImage(QLabel * label, const int& id) {
    auto path = filePath + files[id];
    auto mime = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    auto img = QPixmap::fromImage(QImageReader(path, mime.preferredSuffix().toUtf8()).read());
    if (img.isNull()) {
        auto m = min(imageWidth, imageHeight);
        label->resize(m, m);
        label->setText(tr("Cannot open this file\n") + path);
        label->setStyleSheet("background-color:white; font-size:20px; color:red;");
    } else {
        label->setPixmap(img);
        label->setStyleSheet("color:black;");
        adjustImage(label);
    }
}

void MainWindow::adjustImage(QLabel * label) {
    auto &h = imageHeight, &w = imageWidth;
    auto img = label->pixmap();
    auto ih = img->height(), iw = img->width();
    if (sliding) {
        label->resize(w, ih * w / iw);
    } else {
        auto ww = iw * h / ih;
        if (ww > w) {
            label->resize(w, ih * w / iw);
        } else {
            label->resize(ww, h);
        }
    }
}

void MainWindow::arrangeImage() {
    auto &h = imageHeight, &w = imageWidth;
    bool prevDone = false, nextDone = false;
    int prevPos, nextPos, prefetch = prefetchNumber;

    for (auto img : imgs.map.values()) {
        img->setVisible(false);
    }
    auto img = imgs[0];
    if (sliding) {
        prevPos = imageSlide + offset;
        img->move(0, prevPos);
        nextPos = prevPos + img->height() + gap;
        prevPos -= gap;
    } else {
        prevPos = (w - img->width()) / 2 + offset;
        img->move(prevPos, (h - img->height()) / 2);
        nextPos = prevPos + img->width() + gap;
        prevPos -= gap;
    }
    img->setVisible(true);
    for (int i = 2, j; !(prevDone && nextDone); ++i) {
        j = i % 2 ? -i / 2 : i / 2; // 1,-1,2,-2,...
        img = imgs.get(j);
        if (j < 0) {
            if (prevDone) {
                continue;
            } else if (((sliding && prevPos < 0) || (!sliding && prevPos < 0)) && --prefetch < 0) {
                if (img != nullptr) {
                    img->setVisible(false);
                    imgs.remove(j);
                    deleteImg(img);
                }
                prevDone = true;
                continue;
            }
        } else {
            if (nextDone) {
                continue;
            } else if (((sliding && nextPos > h) || (!sliding && nextPos > imageWidth)) && --prefetch < 0) {
                if (img != nullptr) {
                    img->setVisible(false);
                    imgs.remove(j);
                    deleteImg(img);
                }
                nextDone = true;
                continue;
            }
        }
        auto id = focusId + (!sliding && reversed ? -j : j);
        if (id >= 0 && id < files.size()) {
            if (img == nullptr) {
                img = imgs[j] = newImg();
                setOneImage(img, id);
            }
            img->setVisible(true);
        } else {
            if (j < 0) {
                prevDone = true;
            } else {
                nextDone = true;
            }
            continue;
        }
        if (sliding) {
            if (j < 0) {
                prevPos -= img->height();
                img->move(0, prevPos);
                prevPos -= gap;
            } else {
                img->move(0, nextPos);
                nextPos += img->height() + gap;
            }
        } else {
            if (j < 0) {
                prevPos -= img->width();
                img->move(prevPos, (h - img->height()) / 2);
                prevPos -= gap;
            } else {
                img->move(nextPos, (h - img->height()) / 2);
                nextPos += img->width() + gap;
            }
        }
    }
    panel->raise();
    ui->menuBar->raise();
    ui->statusBar->raise();
}

void MainWindow::on_read_r2l_triggered(bool) {
    ui->read_r2l->setChecked(true);
    ui->read_l2r->setChecked(false);
    ui->read_slide->setChecked(false);
    ui->animation_key->setEnabled(true);
    reversed = true;
    if (sliding) {
        sliding = false;
        resizeEvent(nullptr);
    } else {
        loadImage();
        arrangeImage();
    }
}

void MainWindow::on_read_l2r_triggered(bool) {
    ui->read_r2l->setChecked(false);
    ui->read_l2r->setChecked(true);
    ui->read_slide->setChecked(false);
    ui->animation_key->setEnabled(true);
    reversed = false;
    if (sliding) {
        sliding = false;
        resizeEvent(nullptr);
    } else {
        loadImage();
        arrangeImage();
    }
}

void MainWindow::on_animation_key_triggered(bool checked) {
    animationKey = checked;
}

void MainWindow::on_read_slide_triggered() {
    ui->read_r2l->setChecked(false);
    ui->read_l2r->setChecked(false);
    ui->read_slide->setChecked(true);
    ui->animation_key->setEnabled(false);
    if (!sliding) {
        sliding = true;
        resizeEvent(nullptr);
    }
}

void MainWindow::on_no_gap_triggered(bool checked) {
    if ((noGap = checked)) {
        gap = 0;
        for (auto img : imgs.map.values()) {
            img->setFrameStyle(QFrame::NoFrame);
        }
    } else {
        gap = 5;
        for (auto img : imgs.map.values()) {
            img->setFrameStyle(QFrame::Panel);
        }
    }
    arrangeImage();
}
