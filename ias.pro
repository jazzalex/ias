TARGET = ias-reference-code

PORTAUDIOPATH= ./include/portaudio

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
  LIBS      += -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreServices 
  LIBS      += -lpthread

  DEFINES  += __MACOSX_CORE__

  include(./shared.pri)

  INCLUDEPATH+= $${PORTAUDIOPATH}/include
  INCLUDEPATH+= $${PORTAUDIOPATH}/src/common
  INCLUDEPATH+= $${PORTAUDIOPATH}/src/os/unix

  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_debugprint.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_ringbuffer.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_front.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_process.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_allocation.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_dither.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_cpuload.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_stream.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_trace.c
  SOURCES+= $${PORTAUDIOPATH}/src/common/pa_converters.c
  SOURCES+= $${PORTAUDIOPATH}/src/hostapi/skeleton/pa_hostapi_skeleton.c

  SOURCES+= $${PORTAUDIOPATH}/src/hostapi/coreaudio/pa_mac_core_utilities.c
  SOURCES+= $${PORTAUDIOPATH}/src/hostapi/coreaudio/pa_mac_core_blocking.c
  SOURCES+= $${PORTAUDIOPATH}/src/hostapi/coreaudio/pa_mac_core.c
  DEFINES+= PA_USE_COREAUDIO=1

  SOURCES+= $${PORTAUDIOPATH}/src/os/unix/pa_unix_hostapis.c
  SOURCES+= $${PORTAUDIOPATH}/src/os/unix/pa_unix_util.c
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

CONFIG += thread qt warn_on exceptions 

CONFIG += console

QT += core network widgets multimedia multimediawidgets sensors

# Qt 6.10.3 qyieldcpu.h uses the ARM __yield() intrinsic without including
# <arm_acle.h>, which is fatal under -Werror on Apple Silicon clang.
macos:QMAKE_CXXFLAGS += -include arm_acle.h

HEADERS += ias.h \
	   udp.h

SOURCES += main.cpp \
           ias.cpp \
	   udp.cpp

FORMS += camera.ui
