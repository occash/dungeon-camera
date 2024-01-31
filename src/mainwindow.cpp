#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_videooverlay.h"
#include "virtualoutput.h"
#include "character.h"

#include <QtCore/QDebug>
#include <QtCore/QStandardPaths>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QMessageBox>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QVideoSink>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QMediaDevices>
#include <QtSvg/QSvgRenderer>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_ui{ new Ui::MainWindow() },
    m_overlayUi{ new Ui::VideoOverlay() },
    m_overlayWidget{ new QWidget() },
    m_captureSession{ new QMediaCaptureSession(this) },
    m_cameraDevice{ QMediaDevices::defaultVideoInput() },
    m_camera{ new QCamera(m_cameraDevice, this) },
    m_videoSink{ new QVideoSink(this) },
    m_output{ new VirtualOutput(this) },
    m_character{ new Character(this) }
{
    m_ui->setupUi(this);
    setupOverlay();

    for (const auto &device : QMediaDevices::videoInputs()) {
        m_ui->cameraComboBox->addItem(device.description());
    }

    for (const auto &format : m_cameraDevice.videoFormats()) {
        if (!format.isNull() && format.pixelFormat() != QVideoFrameFormat::Format_Invalid) {
            QString formatString = QString("%1 %2x%3 %4Hz")
                .arg(QVideoFrameFormat::pixelFormatToString(format.pixelFormat()))
                .arg(format.resolution().width())
                .arg(format.resolution().height())
                .arg(format.maxFrameRate());
            m_ui->formatComboBox->addItem(formatString);
        }
    }

    QCameraFormat cameraFormat = m_cameraDevice.videoFormats().first();
    resizeOverlay(cameraFormat.resolution());

    m_captureSession->setCamera(m_camera);
    m_captureSession->setVideoSink(m_videoSink);

    setupConnections();
    m_character->load();
    m_camera->start();
}

MainWindow::~MainWindow()
{
    m_camera->stop();
    delete m_overlayWidget;
    delete m_overlayUi;
    delete m_ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_overlayWidget->close();
    event->accept();
}

void MainWindow::processVideoFrame()
{
    QVideoFrame frame = m_videoSink->videoFrame();

    if (frame.map(QVideoFrame::ReadOnly)) {
        QImage image = frame.toImage();
        QPainter painter{ &image };
        painter.drawImage(0, 0, m_overlay);
        painter.end();
        frame.unmap();
        QVideoFrameFormat format{
            image.size(),
            QVideoFrameFormat::pixelFormatFromImageFormat(image.format())
        };
        qInfo() << "Image:" << image.format();
        qInfo() << "Frame:" << format;
        QVideoFrame outputFrame{ format };
        outputFrame.map(QVideoFrame::WriteOnly);
        memcpy(outputFrame.bits(0), image.bits(), image.sizeInBytes());
        outputFrame.unmap();

        if (image.format() != QImage::Format_RGB32)
            image = image.convertToFormat(QImage::Format_RGB32);

        m_ui->videoOutput->videoSink()->setVideoFrame(outputFrame);
        m_output->send(image.constBits());
    }
}

void MainWindow::toggleStreaming(bool checked)
{
    if (checked) {
        const QCameraFormat cameraFormat = m_cameraDevice.videoFormats().first();
        const QSize resolution = cameraFormat.resolution();
        m_output->start(resolution.width(), resolution.height(), cameraFormat.maxFrameRate());
    } else
        m_output->stop();
}

void MainWindow::setupOverlay()
{
    m_overlayUi->setupUi(m_overlayWidget);
    m_overlayUi->armorClassBadge->load(QString(":/assets/armor.svg"));
    m_overlayUi->hitPointsBadge->load(QString(":/assets/container.svg"));
    // Setup widget
    m_overlayWidget->setAttribute(Qt::WA_DontShowOnScreen);
    m_overlayWidget->resize(400, 300);
    m_overlayWidget->show();
}

void MainWindow::resizeOverlay(const QSize &size)
{
    const double aspect = double(size.width()) / double(size.height());
    const int height = 300;
    const int width = 300 * aspect;
    m_overlayWidget->resize(width, height);
    m_overlayWidget->updateGeometry();
    m_overlayWidget->repaint();
    m_overlay = QImage{ size.width(), size.height(), QImage::Format_ARGB32 };
    renderOverlay();
}

void MainWindow::renderOverlay()
{
    // Redraw widget
    const double scale = double(m_overlay.height()) / double(m_overlayWidget->height());
    m_overlay.fill(Qt::transparent);
    QPainter painter{ &m_overlay };
    painter.scale(scale, scale);
    m_overlayWidget->render(&painter, {}, {}, QWidget::DrawChildren);
    // No transparency in SvgRenderer
    painter.end();
}

void MainWindow::setupConnections()
{
    connect(m_character, &Character::portraitUpdated, [this]() {
        m_overlayUi->portraitLabel->setPixmap(QPixmap::fromImage(m_character->portrait()));
        renderOverlay();
    });
    connect(m_character, &Character::updated, [this]() {
        m_ui->characterIdLineEdit->setText(QString::number(m_character->id()));
        m_overlayUi->nameLabel->setText(m_character->name());
        m_overlayUi->levelLabel->setText(QString("Level %1").arg(m_character->level()));
        m_overlayUi->raceLabel->setText(m_character->race());
        m_overlayUi->classLabel->setText(m_character->playerClass());
        m_overlayUi->armorClassLabel->setText(QString::number(m_character->armorClass()));
        m_overlayUi->hitPointsLabel->setText(QString::number(m_character->currenthitPoints()));
        m_overlayUi->maxHitPointsLabel->setText(QString::number(m_character->maxHitPoints()));
        renderOverlay();
    });

    connect(m_ui->reloadButton, &QPushButton::clicked, [this]() {
        m_character->reload(m_ui->characterIdLineEdit->text().toInt());
    });
    connect(m_ui->actionQuit, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(m_ui->actionAbout, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About", "<h2>Dungeon Companion</h2><p>Version 0.1</p><p>Created by Artem Shal</p>");
    });
    connect(m_ui->actionAboutQt, &QAction::triggered, this, [this]() {
        QMessageBox::aboutQt(this);
    });
    connect(m_ui->actionPlay, &QAction::toggled, this, &MainWindow::toggleStreaming);
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &MainWindow::processVideoFrame);
}
