QT       += core gui network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
CONFIG -= console

TARGET = Qt_ASR_Demo
TEMPLATE = app

SOURCES += \
    src/audiocollector.cpp \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    src/audiocollector.h \
    src/mainwindow.h

FORMS += \
    src/mainwindow.ui

# Sherpa-ONNX 32 位预编译（可选）：解压到 third_party/sherpa-onnx 或设置环境变量 SHERPA_ONNX_ROOT
isEmpty(SHERPA_ONNX_ROOT): SHERPA_ONNX_ROOT = $$PWD/third_party/sherpa-onnx

win32 {
    QMAKE_CXXFLAGS += /utf-8

    CONFIG(debug, debug|release) {
        DESTDIR = $$PWD/build_debug/
    } else {
        DESTDIR = $$PWD/build_release/
    }

    message(DESTDIR = $$DESTDIR)

    dstDir = $$DESTDIR
    dstDir = $$replace(dstDir, /, \\)

    exists($$SHERPA_ONNX_ROOT/include) {
        INCLUDEPATH += $$SHERPA_ONNX_ROOT/include
        LIBS += $$SHERPA_ONNX_ROOT/lib/sherpa-onnx-c-api.lib
        DEFINES += HAVE_SHERPA_ONNX
        message(Using Sherpa-ONNX from $$SHERPA_ONNX_ROOT)

        sherpaBinDir = $$SHERPA_ONNX_ROOT/bin/*.dll
        sherpaBinDir = $$replace(sherpaBinDir, /, \\)
        sherpaLibDll = $$SHERPA_ONNX_ROOT/lib/sherpa-onnx-c-api.dll
        sherpaLibDll = $$replace(sherpaLibDll, /, \\)

        QMAKE_POST_LINK += xcopy $$sherpaBinDir $$dstDir /e /r /q /y
        message(COPY_FILE $$sherpaBinDir->$$dstDir)
        QMAKE_POST_LINK  +=  &&  xcopy $$sherpaLibDll $$dstDir /e /r /q /y
        message(COPY_FILE $$sherpaLibDll->$$dstDir)
    }
}
