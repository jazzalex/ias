#include <ias.h>
#include <QCoreApplication>
#include <udp.h>

using namespace std::chrono;

/// PREPARATIONS
auto start = high_resolution_clock::now();
auto over  = high_resolution_clock::now();


/// CALLBACK FUNCTION
static int portAudioCallback( const void *inputBuffer, void *outputBuffer,
                              unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void *data ){

    /// PREPARATIONS
    (void) framesPerBuffer;
    (void) timeInfo;
    (void) statusFlags;
    ias::callbackdata *my;
    my = (ias::callbackdata *) data;
    char *input  = (char*) inputBuffer;
    char *output = (char*) outputBuffer;


    /// END TIME MEASUREMENT
    timeval result;
    #ifdef WIN32
        over = high_resolution_clock::now();
        duration_cast<microseconds>(over - start).count();
    #else
        gettimeofday(&result, NULL);
        my->after = (double) ((result.tv_sec*1000) + (((double) result.tv_usec) / 1000));
        my->callbackInterval = my->after - my->before;
    #endif

    /// CHOOSE IAS OPTION
    int option = 2; /// 0: PLOT CALLBACK INTERVAL 1: RECTANGLE GENERATOR, 2: INPUT=OUTPUT, 3: NETWORK MIRROR

    if (option == 0){
        #ifndef WIN32
            cout << "CALLBACK-INTERVAL: " << my->callbackInterval << " ms" << endl;
        #else
            cout << "CALLBACK-INTERVAL: " << duration_cast<microseconds>(over - start).count() << " us" << endl;
        #endif
    }


    if (option == 1){
        /// RECTANGLE GENERATOR
        for (int i=0; i<(int)framesPerBuffer*2;i++){
            if (my->even == true) output[i] = (char) 128; else output[i] = (char) -127;
        }

        /// CHANGE EVEN TO ODD AND VICE VERSA
        if (my->even == true) my->even = false; else my->even = true;
    }


    if (option == 2){
        /// OPTION 1: WRITE INPUTBUFFER TO OUTPUTBUFFER
        for (unsigned short i=0; i<framesPerBuffer*2;i++) {
            output[i] = input[i];
        }
    }


    if (option == 3){
        //cout << "SEND" << endl;

        /// FILL INPUT BUFFER
        for (unsigned short i=0; i<framesPerBuffer*2;i++) {
            my->myIAS->client.inputPacket[i] = input[i];
        }

        /// SEND AUDIO PACKET
        my->myIAS->client.sendInput();


        /// OPTION 2: READ FROM FIFO IF THRESHOLD IS REACHED
        unsigned int threshold = 500;

        if (my->myIAS->client.fifo.size() > (threshold * framesPerBuffer * 2) ){
            for (unsigned int i=0; i < framesPerBuffer*2;i++) output[i] = my->myIAS->client.fifo.dequeue();
        }
    }



    /// START TIME MEASUREMENT
    #ifdef WIN32
        start = high_resolution_clock::now();
    #else
        gettimeofday(&result, NULL);
    #endif
    my->before = (double) ((result.tv_sec*1000) + (((double) result.tv_usec) / 1000));


    return 0;
}



/// CONSTRUCTOR
ias::ias(){

    /// PREPARATIONS
    this->ensureMicPermissions();

    dFC = new callbackdata();
    dFC->even = false;
    dFC->myIAS = this;


    /// PORTAUDIO INIT
    paErr = Pa_Initialize();

    if (paErr == paNoError){
        amountOfAudioDevices = -1;
        amountOfAudioDevices = Pa_GetDeviceCount();

        if( amountOfAudioDevices < 0 ){
            cout << "ERROR: CountDevices returned:" << amountOfAudioDevices << endl;
        }
    }
 
    /// PLOT INFO WITH REGARD TO DETECTED DEVICES
    for (int i=0;i<=amountOfAudioDevices-1;i++) {
        info = Pa_GetDeviceInfo(i);

        cout << "device = " << i << endl;
        cout << "name                    = " << info->name << endl;
        cout << "maximum output channels = " << info->maxOutputChannels << endl;
        cout << "maximum input  channels = " << info->maxInputChannels << endl;
        cout << "default sample rate     = " << info->defaultSampleRate << endl;
        cout << endl;
    }

    /// SET DESIRED PARAMETERS
    audiofault = false;
    soundIsRunning = false;

    frameSize  = 512;
    sampleRate = 48000;
    audioChannels = 1;

    /// CONFIGURE INPUT AUDIODEVICE
    PaStreamParameters outputParameters;
    PaStreamParameters inputParameters;

    bzero( &inputParameters, sizeof( inputParameters ) );
    inputParameters.channelCount = audioChannels;
    inputParameters.device = 1;//2;
    inputParameters.sampleFormat = paInt16;

    inputParameters.hostApiSpecificStreamInfo = NULL;

    #ifdef __MACOSX_CORE__
        static PaMacCoreStreamInfo coreAudioInputInfo;
        PaMacCore_SetupStreamInfo(&coreAudioInputInfo, paMacCorePro);
        inputParameters.hostApiSpecificStreamInfo = &coreAudioInputInfo;
    #endif


    /// CONFIGURE OUTPUT AUDIODEVICE
    bzero( &outputParameters, sizeof( outputParameters ) );
    outputParameters.channelCount = audioChannels;
    outputParameters.device = 1;//3;
    outputParameters.sampleFormat = paInt16;

    outputParameters.hostApiSpecificStreamInfo = NULL;

    #ifdef __MACOSX_CORE__
        static PaMacCoreStreamInfo coreAudioOutputInfo;
        PaMacCore_SetupStreamInfo(&coreAudioOutputInfo, paMacCorePro);
        outputParameters.hostApiSpecificStreamInfo = &coreAudioOutputInfo;
    #endif

    inputParameters.suggestedLatency = 0.001;
    outputParameters.suggestedLatency = 0.001;


    /// OPEN THE AUDIO STREAM
    paErr = Pa_OpenStream( &stream,
                          &inputParameters,
                          &outputParameters,
                          sampleRate,
                          frameSize,
                          paClipOff,
                          portAudioCallback,
                          dFC);

    if (paErr==paNoError) audiofault = false; else audiofault = true;


    /// LAUNCH THE AUDIO PROCESS
    if (!audiofault){

        #ifdef __LINUX_ALSA__
            PaAlsa_EnableRealtimeScheduling(stream,true);
        #endif

        paErr = Pa_StartStream(stream);
        if (paErr==paNoError){
            soundIsRunning = true;
            cout << "SOUND IS RUNNING" << endl;
        }
        else{
            soundIsRunning = false;
            cout << "SOUND IS NOT RUNNING" << endl;
        }
    } else{
        soundIsRunning = false;
        cout << "SOUND IS NOT RUNNING" << endl;
    }


    /// VIDEO RELATED
    /// Camera permission is resolved asynchronously; initVideoAndSensor()
    /// runs from the callback once permission is granted.
    this->ensureCameraPermission();
}

void ias::initVideoAndSensor(){

    availableCameras = QMediaDevices::videoInputs();
    cout << "Available cameras: " << availableCameras.size() << endl;

    this->startVideo(0);


    /// SENSOR RELATED
    sensor = new QAccelerometer();
    sensor->setAccelerationMode(QAccelerometer::User);
    connect(sensor, &QAccelerometer::readingChanged, this, &ias::getSensorData);
    sensor->start();
}

/// DESTRUCTOR
ias::~ias(){
    cout << "Program is destructed now ..." << endl;

    if (soundIsRunning){
        paErr = Pa_CloseStream( stream );
        if (paErr == 0){ 
            soundIsRunning = false;
        } 
    } 
}

/// ENSURE PERMISSIONS
void ias::ensureMicPermissions(){

    QMicrophonePermission microphonePermission;
       
    switch (qApp->checkPermission(microphonePermission)) {

    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(microphonePermission, this, [](const QPermission &permission){
            if (permission.status() == Qt::PermissionStatus::Granted)
                cout << "Mic permission is granted !" << endl;
            else
                qWarning("Microphone permission is not granted!");
        });
        return;

    case Qt::PermissionStatus::Denied:
        qWarning("Microphone permission is not granted!");
        return;

    case Qt::PermissionStatus::Granted:
	cout << "Mic permission is granted !" << endl;
        break;
    }   
}

void ias::ensureCameraPermission(){

    QCameraPermission camPerm;

    switch (qApp->checkPermission(camPerm)) {

    case Qt::PermissionStatus::Granted:
        cout << "Camera permission is granted !" << endl;
        this->initVideoAndSensor();
        return;

    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(camPerm, this, [this](const QPermission &permission){
            if (permission.status() == Qt::PermissionStatus::Granted) {
                cout << "Camera permission is granted !" << endl;
                this->initVideoAndSensor();
            } else {
                cout << "Access to camera not granted" << endl;
            }
        });
        return;

    case Qt::PermissionStatus::Denied:
        cout << "Access to camera not granted. Enable it under "
                "System Settings > Privacy & Security > Camera, then relaunch."
             << endl;
        return;
    }
}


void ias::startVideo(int index) {

    videoDisplayWidth  = 1920;
    videoDisplayHeight = 1080;

    saveVideoIndex = index;

    if ( index < 0 || index >= availableCameras.size() ) {
        cout << "No camera available at index " << index
             << " (available: " << availableCameras.size() << ")" << endl;
        videoReady = false;
        return;
    }

    QSize resolution(videoDisplayWidth, videoDisplayHeight); //16:9

    camera = new QCamera( availableCameras[index] );
    //camera = new QCamera( QMediaDevices::defaultVideoInput() );

    QCameraDevice cameraDevice = camera->cameraDevice();

    QList imageFormats = cameraDevice.photoResolutions();
    QList videoFormats = cameraDevice.videoFormats();

    int howManyImage = imageFormats.size();

    for (int i=0; i<howManyImage; i++){
        /*
        QSize resolulu = imageFormats[i];
        cout << "IMAGE DEVICE HEIGHT: " << resolulu.height() << endl;
        cout << "IMAGE DEVICE WIDTH : " << resolulu.width() << endl <<  endl;
        */
    }

    int howManyVideo = videoFormats.size();

    int thisOne = 0;

    for (int i=0; i<howManyVideo; i++){
        int videoWidth  = videoFormats[i].resolution().width();
        int videoHeight = videoFormats[i].resolution().height();

        ///* 
        int maxFrame    = videoFormats[i].maxFrameRate();
        int minFrame    = videoFormats[i].minFrameRate();

        cout << "VIDEO DEVICE MAX FRAME: " << maxFrame << endl;
        cout << "VIDEO DEVICE MIN FRAME: " << minFrame << endl;
        cout << "VIDEO DEVICE HEIGHT   : " << videoHeight << endl;
        cout << "VIDEO DEVICE WIDTH    : " << videoWidth << endl <<  endl;
        //*/

        if ( ( videoHeight == videoDisplayHeight ) && ( videoWidth == videoDisplayWidth ) ){
            thisOne = i;
            break;
        }
    }

    /// SETTINGS FÜR IMAGE CAPTURE
    captureSession.setCamera(camera);
    if ( thisOne < videoFormats.size() ) {
        captureSession.camera()->setCameraFormat( videoFormats[thisOne] );
    }
    captureSession.setImageCapture(&imageCapture);

    imageCapture.setResolution(resolution);

    connect(&imageCapture, &QImageCapture::imageCaptured, this, &ias::processImage);
    camera->start();

    //ui->comboResolutionBox->setItemData(0, 0, Qt::UserRole - 1);
    ui->comboResolutionBox->setCurrentIndex(0);
    ui->colorBwBox->setCurrentIndex(0);
    ui->comboCodingBox->setItemData(1, 0, Qt::UserRole - 1);
    ui->comboCodingBox->setCurrentIndex(0);
    ui->comboInterleaverBox->setCurrentIndex(0);

    videoResolution = 'N';
    interleaved = 0;
    BW = 0;
    JPEG = 1;

    //this->createStreamParameter();

    #ifndef BROWSER_VIDEO_TRANSFER

        cout << "SHOW VIDEO WINDOW" << endl;

        this->setWindowFlags( (Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint) & ~Qt::WindowCloseButtonHint );
        this->show();
        this->move(0, 0);
        this->raise();
        this->activateWindow();
    #endif

    videoReady = true;
}

void ias::captureImage() {

    //cout << "CAPTURE SLOT" << endl;

    imageCapture.capture();
}


void ias::processImage(int requestId, const QImage &img) {

/*
    videoImageCounter++;

    /// FOR DSV LECTURE
    //cout << "PROCESS IMAGE: " << videoImageCounter << endl;

    (void)requestId;

/// MEASURE TIME BETWEEN VIDEO CALLBACKS
#ifdef WIN32
    this->gettimeofday(&tVideoCallbackBegin, NULL);
#else
    gettimeofday(&tVideoCallbackBegin, NULL);
#endif

    double zweiteMessung;
    zweiteMessung = (double) ((tVideoCallbackBegin.tv_sec*1000) + (((double) tVideoCallbackBegin.tv_usec) / 1000));
    double ersteMessung;
    ersteMessung = (double) ((tVideoCallbackEnd.tv_sec*1000) + (((double) tVideoCallbackEnd.tv_usec) / 1000));

    double videoInterval = zweiteMessung - ersteMessung;
    (void) videoInterval;

    //cout << "VIDEO INTERVAL: " << videoInterval << endl;

    /// GET TIME FOR CODING LATENCY
    struct timeval tValAfter, tValBefore;

    #ifdef WIN32
        this->gettimeofday(&tValBefore, NULL);
    #else
        gettimeofday(&tValBefore, NULL);
    #endif
    */
}

void ias::getSensorData(){
    cout << "READ SENSOR" << endl;

    if ( sensor == nullptr || sensor->reading() == nullptr ) {
        cout << "No sensor reading available yet" << endl;
        return;
    }

    QAccelerometerReading *reading = sensor->reading();

    cout << "X: " << (int) reading->x() << endl;
    cout << "Y: " << (int) reading->y() << endl;
    cout << "Z: " << (int) reading->z() << endl;

}

