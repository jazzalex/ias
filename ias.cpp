#include <ias.h>
#include <QCoreApplication>
#include <udp.h>

using namespace std::chrono;

auto start = high_resolution_clock::now();
auto over  = high_resolution_clock::now();

static int portAudioCallback( const void *inputBuffer, void *outputBuffer,
                              unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void *data ){
   
    (void) framesPerBuffer;
    (void) timeInfo;
    (void) statusFlags;
 
    ias::callbackdata *my;
    my = (ias::callbackdata *) data;
 
    /// END TIME MEASUREMENT
    timeval result;

    #ifdef WIN32
        over = high_resolution_clock::now();

        duration_cast<microseconds>(over - start).count();

        cout << "CALLBACK-INTERVAL: " << duration_cast<microseconds>(over - start).count() << " us" << endl;
    #else
        gettimeofday(&result, NULL);

	my->after = (double) ((result.tv_sec*1000) + (((double) result.tv_usec) / 1000));
        my->callbackInterval = my->after - my->before;
  
        //cout << "SAMPLES: " << framesPerBuffer << " EVEN: " << my->even <<  " CALLBACK-INTERVAL: " << my->callbackInterval << endl;
    #endif

    /// DEFINE INPUT AND OUTPUT BUFFER
    char *input  = (char*) inputBuffer;
    char *output = (char*) outputBuffer;
 
    /// WRITE INPUTBUFFER TO OUTPUTBUFFER
    for (unsigned short i=0; i<framesPerBuffer*2;i++) {
        //output[i] = input[i];
    }

    /// FILL INPUT BUFFER
    for (unsigned short i=0; i<framesPerBuffer*2;i++) {
        my->myIAS->client.inputPacket[i] = input[i];
    }

    /// SEND AUDIO PACKET
    my->myIAS->client.sendInput();

    unsigned int threshold = 2;

    /// READ FROM FIFO IF THRESHOLD IS REACHED
    if (my->myIAS->client.fifo.size() > (threshold * framesPerBuffer * 2) ){
        for (unsigned int i=0; i < framesPerBuffer*2;i++) output[i] = my->myIAS->client.fifo.dequeue();
    }

    /// RECTANGLE GENERATOR
    for (int i=0; i<(int)framesPerBuffer*2;i++){
        //if (my->even == true) output[i] = 244; else output[i] = 0;
    }
 
    /// EVEN / ODD VERÄNDERN
    if (my->even == true) my->even = false; else my->even = true;

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

    this->ensureAVPermissions();

    /// SPEICHER FÜR CALLBACK-DATA STRUCT RESERVIEREN
    dFC = new callbackdata();
    dFC->even = false;

    dFC->myIAS = this;

    /// PORTAUDIO INITIALISIEREN
    paErr = Pa_Initialize();

    if (paErr == paNoError){
        amountOfAudioDevices = -1;
        amountOfAudioDevices = Pa_GetDeviceCount();

        if( amountOfAudioDevices < 0 ){
            cout << "ERROR: CountDevices returned:" << amountOfAudioDevices << endl;
        }
    }
 
    /// INFO ZU VORHANDENEN AUDIODEVICES AUSGEBEN
    for (int i=0;i<=amountOfAudioDevices-1;i++) {
        info = Pa_GetDeviceInfo(i);

        cout << "device = " << i << endl;
        cout << "name                    = " << info->name << endl;
        cout << "maximum output channels = " << info->maxOutputChannels << endl;
        cout << "maximum input  channels = " << info->maxInputChannels << endl;
        cout << "default sample rate     = " << info->defaultSampleRate << endl;
        cout << endl;
    }

    /// PARAMETER FESTLEGEN 
    audiofault = false;
    soundIsRunning = false;
    frameSize  = 512;//128;
    sampleRate = 48000;
    audioChannels = 1;

    /// AUDIODEVICE KONFIGURIEREN
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

    paErr = Pa_OpenStream( &stream,
                          &inputParameters,
                          &outputParameters,
                          sampleRate,
                          frameSize,
                          paClipOff,
                          portAudioCallback,
                          dFC);

    if (paErr==paNoError) audiofault = false; else audiofault = true;


    /// AUDIODEVICE STARTEN
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
}


ias::~ias(){
    cout << "Program is destructed now ..." << endl;

    if (soundIsRunning){
        paErr = Pa_CloseStream( stream );
        if (paErr == 0){ 
            soundIsRunning = false;
        } 
    } 
}

void ias::ensureAVPermissions(){

    QMicrophonePermission microphonePermission;
       
    switch (qApp->checkPermission(microphonePermission)) {

    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(microphonePermission, this, &ias::ensureAVPermissions);
        return;

    case Qt::PermissionStatus::Denied:
        qWarning("Microphone permission is not granted!");
        return;

    case Qt::PermissionStatus::Granted:
	cout << "Mic permission is granted !" << endl;
        break;
    }   
}

