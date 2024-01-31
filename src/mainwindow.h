#pragma once

#include <QtWidgets/QMainWindow>
#include <QtMultimedia/QCameraDevice>

class QMediaCaptureSession;
class QCamera;
class QVideoSink;
class QVideoFrame;
class VirtualOutput;
class Character;

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
    class VideoOverlay;
};
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void processVideoFrame();
    void toggleStreaming(bool checked);

private:
    void setupOverlay();
    void resizeOverlay(const QSize &size);
    void renderOverlay();
    void setupConnections();

private:
    Ui::MainWindow *m_ui;
    Ui::VideoOverlay *m_overlayUi;
    QWidget *m_overlayWidget;
    QMediaCaptureSession *m_captureSession;
    QCameraDevice m_cameraDevice;
    QCamera *m_camera;
    QVideoSink *m_videoSink;
    VirtualOutput *m_output;
    QImage m_overlay;
    Character *m_character;

};
