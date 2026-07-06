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
    ui = new Ui::Camera();

    ui->setupUi(this);

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

        paErr = 0;//Pa_StartStream(stream);
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


    /// VIDEO AND SENSOR RELATED
    this->ensureCameraPermission();

    connect(ui->colorBwBox, SIGNAL(currentIndexChanged(int)), this, SLOT(videoBWSlot(int)));
    connect(ui->comboResolutionBox, SIGNAL(currentIndexChanged(int)), this, SLOT(videoResolutionSlot(int)));
    connect(ui->comboCodingBox, SIGNAL(currentIndexChanged(int)), this,SLOT(videoCompressionRatioSlot(int)));

    this->initVideoAndSensor();

    captureTimer = new QTimer(this);
    connect( captureTimer, SIGNAL( timeout() ), this, SLOT( captureImage() ) );
    captureTimer->start(40);
}

void ias::initVideoAndSensor(){

    availableCameras = QMediaDevices::videoInputs();
    cout << "Available cameras: " << availableCameras.size() << endl;

    this->startVideo(0);


    /// SENSOR RELATED
    sensor = new QAccelerometer();
    sensor->setAccelerationMode(QAccelerometer::User);
    sensor->setDataRate(100);
    connect(sensor, &QAccelerometer::readingChanged, this, &ias::getSensorData);
    sensor->start();

    if (!sensor->isActive()) {
        cout << "Accelerometer backend unavailable on this platform; no readings will arrive." << endl;
    }
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

    if (sensor != nullptr){
        sensor->stop();
        delete sensor;
        sensor = nullptr;
    }

    if (camera != nullptr){
        camera->stop();
        captureSession.setCamera(nullptr);
        delete camera;
        camera = nullptr;
    }

    delete ui;
    delete dFC;
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

void ias::videoBWSlot(int index) {
    if (index == 1)
        BW = 1; 
    else 
        BW = 0; 
}

void ias::videoCompressionRatioSlot(int index) {
    if (index == 0)
        JPEG = 1; 
    if (index == 1)
        JPEG = 0; 
}

void ias::videoResolutionSlot(int index) {
     cout << "RATIO SLOT: " <<  index << endl;

    if (index == 0)
        videoResolution = 'N'; 
    if (index == 1)
        videoResolution = 'A'; 
    if (index == 2)
        videoResolution = 'B'; 
    if (index == 3)
        videoResolution = 'C'; 
    if (index == 4)
        videoResolution = 'D'; 
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

    //camera = new QCamera( availableCameras[index] );
    camera = new QCamera( QMediaDevices::defaultVideoInput() );

    QCameraDevice cameraDevice = camera->cameraDevice();

    QList imageFormats = cameraDevice.photoResolutions();
    QList videoFormats = cameraDevice.videoFormats();

    int howManyImage = imageFormats.size();

    for (int i=0; i<howManyImage; i++){
        /*
        QSize resolu = imageFormats[i];
        cout << "IMAGE DEVICE HEIGHT: " << resolu.height() << endl;
        cout << "IMAGE DEVICE WIDTH : " << resolu.width() << endl <<  endl;
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

    /// LIVE PREVIEW: route the camera feed into the viewfinder widget and
    /// bring it in front of the black display label.
    captureSession.setVideoOutput(ui->viewfinder);
    ui->viewfinder->setEnabled(true);
    ui->viewfinder->setGeometry(0, 0, 1280, 672);
    ui->viewfinder->raise();
    ui->viewfinder->setVisible(false);

    imageCapture.setResolution(resolution);

    connect(&imageCapture, &QImageCapture::imageCaptured, this, &ias::processImage);

    camera->start();

    ui->comboResolutionBox->setCurrentIndex(0);
    ui->colorBwBox->setCurrentIndex(0);
    //ui->comboCodingBox->setItemData(1, 0, Qt::UserRole - 1);
    ui->comboCodingBox->setCurrentIndex(0);
    ui->comboInterleaverBox->setVisible(false);

    videoResolution = 'N';
    BW = 0;
    JPEG = 1;

    cout << "SHOW VIDEO WINDOW" << endl;

    this->setWindowFlags( (Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint) & ~Qt::WindowCloseButtonHint );
    this->show();
    this->move(0, 0);
    this->raise();
    this->activateWindow();

    videoReady = true;
}

void ias::captureImage() {
    imageCapture.capture();
}


void ias::processImage(int requestId, const QImage &img) {

    videoImageCounter++;

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

    QSize reso;
        
    //cout << videoResolution << endl;

    switch (videoResolution) {
        case 'D': 
            reso.setWidth(videoDisplayWidth/12);
            reso.setHeight(videoDisplayHeight/12);
            break;
        case 'C': 
            reso.setWidth(videoDisplayWidth/6);
            reso.setHeight(videoDisplayHeight/6);
            break;
        case 'B': 
            reso.setWidth(videoDisplayWidth/3);
            reso.setHeight(videoDisplayHeight/3);
            break;
        case 'A': 
            reso.setWidth(videoDisplayWidth/1.5);
            reso.setHeight(videoDisplayHeight/1.5);
            break;
        case 'N': 
            reso.setWidth(videoDisplayWidth);
            reso.setHeight(videoDisplayHeight);
            break;
    }    

    QImage scaledImage;

    /// SCALE DOWN IMAGE
    scaledImage = img.scaled(reso, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    /// NO ALPHA, SWITCH R&B AND BW
    unsigned int imageSize = scaledImage.sizeInBytes();
    unsigned int newImageSize = (imageSize / 4) * 3; // ALPHA OUT
    unsigned char *imgBuffer = NULL;
    imgBuffer = scaledImage.bits();
    vector<float> pic_vector(newImageSize);
    int counter = 0;

    unsigned char *imgBufferOut = new unsigned char[newImageSize];

    //cout << imageSize << endl;

    if (videoResolution != 'N'){
        for (unsigned int index = 0; index < imageSize; index = index + 4) {
            /// CREATE LOCAL VIDEO BUFFER
            pic_vector[counter] = static_cast<float>(imgBuffer[index + 0]);     //+2
            pic_vector[counter + 1] = static_cast<float>(imgBuffer[index + 1]); //+1
            pic_vector[counter + 2] = static_cast<float>(imgBuffer[index +2]);  //+0

            if (BW == 1) {
                pic_vector[counter] = 0.21 * pic_vector[counter] +
                                      0.72 * pic_vector[counter + 1] +
                                      0.07 * pic_vector[counter + 2];
                pic_vector[counter + 1] = pic_vector[counter];
                pic_vector[counter + 2] = pic_vector[counter];
            }

            imgBufferOut[counter] = static_cast<unsigned char>(pic_vector[counter]);
            imgBufferOut[counter + 1] = static_cast<unsigned char>(pic_vector[counter + 1]);
            imgBufferOut[counter + 2] = static_cast<unsigned char>(pic_vector[counter + 2]);

            counter += 3;
        }
    }

    /// JPEG ARRAY DEKLARIEREN
    QByteArray finalJpg;


    if (JPEG) {
        //cout << videoDisplayHeight << " " << videoDisplayWidth << " " << reso.height() << " " << reso.width() << endl;

        QImage *jpgImg;
        if (videoResolution != 'N')
            jpgImg = new QImage(imgBufferOut, reso.width(), reso.height(), QImage::Format_RGB888);
        else{
            if (BW == 1) jpgImg = new QImage( img.convertToFormat(QImage::Format_Grayscale8) );
            else jpgImg = new QImage(img);
        }

        QByteArray ba;
        QBuffer bufferJpeg(&ba);
        bufferJpeg.open(QIODevice::WriteOnly);

        jpgImg->save(&bufferJpeg, "JPG");
        finalJpg = bufferJpeg.data();

        char *encodedData = finalJpg.data();

        // cout << "JPEG - SIZE: " << finalJpg.size() << endl;

        delete jpgImg;

    } else {
        if (BW == 0){
	    //cout << "COLOR UNCOMPRESSED SIZE: " << newImageSize << endl;
	} else{
	   //cout << "BW UNCOMPRESSED SIZE: " << newImageSize/3 << endl;
	}
    }

    /// CREATE SMALLER IMAGE IN NEW FORMAT
    QImage *displayBufferImg = new QImage(imgBufferOut, reso.width(), reso.height(), QImage::Format_RGB888);

    if (JPEG) {

        //cout << "DECODE JPG" << endl;        

        QPixmap jpgPixmap;
        jpgPixmap.loadFromData(finalJpg, "JPG");

        *displayBufferImg = jpgPixmap.toImage();

        //cout << "JPG-SIZE: " << finalJpg.size() << endl;
    }

    ///SCALE IMAGE UP

    QSize scale_rect( this->ui->centralWidget->width(), this->ui->centralWidget->height() );
    QImage image2 = displayBufferImg->scaled(scale_rect, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    //cout << "DOWNSCALE-SIZE: " << displayBufferImg->sizeInBytes() << endl;

    // SHOW IMAGE
    this->ui->display->setPixmap(QPixmap::fromImage(image2));

    delete[] imgBufferOut;
    delete displayBufferImg;

    #ifdef WIN32
        this->gettimeofday(&tValAfter, NULL);
    #else
        gettimeofday(&tValAfter, NULL);
    #endif

    // cout << "Delay: " << ( tValAfter.tv_usec - tValBefore.tv_usec ) << endl;

    /// GET VIDEOCALLBACK-INSTANT FOR INTERVAL MEASUREMENT
    #ifdef WIN32
        this->gettimeofday(&tVideoCallbackEnd, NULL);
    #else
        gettimeofday(&tVideoCallbackEnd, NULL);
    #endif
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

