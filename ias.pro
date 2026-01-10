TARGET = ias-reference-code

linux{
  CONFIG   += x11
  
  DEFINES  += __LINUX_ALSA__
  DEFINES  += __LITTLE_ENDIAN__

  QMAKE_LIBDIR += ./lib/linux/pa
 
  LIBS     += -lportaudio
  LIBS     += -lpthread
  LIBS     += -lasound
  LIBS     += -lrt
}

macx{
  QMAKE_LIBDIR += ./lib/OSX/celt/
  QMAKE_LIBDIR += ./lib/OSX/pa/  

  LIBS      += -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreServices 
  LIBS      += -lportaudio
  LIBS      += -lpthread

  DEFINES  += __MACOSX_CORE__

  include(./shared.pri)
}

win32{
  CONFIG      += windows
  
  DEFINES     += __LITTLE_ENDIAN__
  DEFINES     += __WINDOWS_ASIO__
  DEFINES     -= UNICODE

  LIBS        += -lole32
  LIBS        += -lwinmm
  LIBS        += -luuid
  LIBS        += -lws2_32
  LIBS        += kernel32.lib
  LIBS        += user32.lib
  LIBS        += winspool.lib
  LIBS        += comdlg32.lib
  LIBS        += advapi32.lib
  LIBS        += shell32.lib
  LIBS        += oleaut32.lib
  LIBS        += odbc32.lib
  LIBS        += odbccp32.lib

  DEFINES     += PA_USE_ASIO=1

  PORTAUDIOPATH= ./include/portaudio

  INCLUDEPATH+= $${PORTAUDIOPATH}/include
  INCLUDEPATH+= $${PORTAUDIOPATH}/src/common
  INCLUDEPATH+= $${PORTAUDIOPATH}/src/os/win
  INCLUDEPATH+= $${PORTAUDIOPATH}/../ASIOSDK/common
  INCLUDEPATH+= $${PORTAUDIOPATH}/../ASIOSDK/host
  INCLUDEPATH+= $${PORTAUDIOPATH}/../ASIOSDK/host/pc

  HEADERS+= $${PORTAUDIOPATH}/include/pa_asio.h

  SOURCES+= $${PORTAUDIOPATH}/../ASIOSDK/common/asio.cpp
  SOURCES+= $${PORTAUDIOPATH}/../ASIOSDK/host/asiodrivers.cpp
  SOURCES+= $${PORTAUDIOPATH}/../ASIOSDK/host/pc/asiolist.cpp

  SOURCES+= $${PORTAUDIOPATH}/src/hostapi/asio/pa_asio.cpp
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_allocation.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_converters.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_cpuload.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_dither.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_front.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_process.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_ringbuffer.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_stream.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_trace.c

  SOURCES+= $${PORTAUDIOPATH}/src/os/win/pa_win_hostapis.c
  SOURCES+= $${PORTAUDIOPATH}/src/os/win/pa_win_util.c
  SOURCES+= $${PORTAUDIOPATH}/src/os/win/pa_win_coinitialize.c
  SOURCES+= $${PORTAUDIOPATH}/src/os/win/pa_win_waveformat.c
  SOURCES+= $${PORTAUDIOPATH}/src/os/win/pa_x86_plain_converters.c
}

INCLUDEPATH += ./include/portaudio

CONFIG += thread qt warn_on exceptions 

CONFIG += console

QT += core network widgets

HEADERS += ias.h \
	   udp.h

SOURCES += main.cpp \
           ias.cpp \
	   udp.cpp



