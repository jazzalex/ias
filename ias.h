#ifndef H_IAS_
  #define H_IAS_

#include <QMainWindow>
#include <QPermissions>
#include <QTimer>

// std-includes
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <cstdio>
#include <math.h>

#ifndef WIN32
    #include <sys/time.h>
#else
   //#include <windows.h>
   #include <chrono>
   #include <winsock2.h>
#endif

#include <udp.h>

// AV
#include "./include/portaudio/include/portaudio.h"


#ifdef __MACOSX_CORE__
  #include "./include/portaudio/include/pa_mac_core.h"
#endif

#ifdef __LINUX_ALSA__
  #include "./include/portaudio/include/pa_linux_alsa.h"
#endif

#define bzero(b,len) (memset((b),'\0',(len)), (void)0)

/// VIDEO RELATED INCLUDES
#include "ui_camera.h"
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QBuffer>
#include <QCamera>
#include <QImageCapture>
#include <QFile>
#include <QScreen>
#include <QPermissions>

/// SENSOR RELATED
#include <QAccelerometer>
#include <QGyroscope>


using namespace std;

class ias : public QMainWindow{

Q_OBJECT

public :

    ias();
    ~ias();

    // AUDIO
    PaStream *stream;
    PaError paErr;
    const PaDeviceInfo *info;

    bool audiofault;
    bool soundIsRunning;

    // SOUND SPECIFIC VARIABLES
    int amountOfAudioDevices,
    inputDevice,
    outputDevice,
    format,
    audioChannels,
    sampleRate,
    frameSize;

    // UDP CLASS
    udp client;

    // CALLBACK
    struct callbackdata{

        ias *myIAS;

        double callbackInterval;
        double before;
        double after;

        bool even;
    };

    callbackdata *dFC;

    void ensureMicPermissions();
    void ensureCameraPermission();

    /// VIDEO RELATED
    void initVideoAndSensor();
    void startVideo(int index);

    unsigned short videoDisplayWidth  = 1080;
    unsigned short videoDisplayHeight = 1920;

    Ui::Camera *ui;
    
    QList<QCameraDevice> availableCameras;
    QCamera *camera = nullptr;
    QImageCapture imageCapture;
    QMediaCaptureSession captureSession;
    QScreen *screen = QGuiApplication::primaryScreen();
    QTimer *captureTimer;

    bool videoThreadRunning  = true;

    unsigned char *videoBuffer;

    unsigned short videoImageCounter;
    bool videoReady;
    int BW, JPEG, interleaved;
    bool interleavedReady;
    unsigned short packetFragments;
    char videoResolution;
    int saveVideoIndex;
    struct timeval tVideoCallbackBegin, tVideoCallbackEnd;

    /*
    boost::thread videoThread;
    boost::mutex videoMutex;
    boost::condition_variable videoLock;
    
    void videoGo();
    */

    /// SENSOR RELATED
    QAccelerometer *sensor = nullptr;
    void getSensorData();

    private:

    public slots:

        void captureImage();
        void processImage(int requestId, const QImage &img);

        void videoBWSlot(int index);
        void videoCompressionRatioSlot(int index);
        void videoResolutionSlot(int index);
        void videoInterleaverSlot(int index);

    private slots:

};

#endif

