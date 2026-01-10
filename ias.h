#ifndef H_IAS_
  #define H_IAS_

#include <QWidget>
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

using namespace std;

class ias : public QWidget{

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

    void ensureAVPermissions();

    private:

    public slots:

    private slots:

};

#endif

