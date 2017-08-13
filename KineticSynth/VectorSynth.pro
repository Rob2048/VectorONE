#-------------------------------------------------
#
# Project created by QtCreator 2017-04-27T21:39:11
#
#-------------------------------------------------

QT       += core gui \
			network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VectorSynth
TEMPLATE = vcapp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp \
        mainwindow.cpp \
        cameraView.cpp \
        sceneView.cpp \
        decoder.cpp \
        tracker.cpp \
        take.cpp \
        timelineWidget.cpp \
        objLoader.cpp \
        liveTracker.cpp \
        serverThreadWorker.cpp

HEADERS  += mainwindow.h \
			cameraView.h \
			sceneView.h \
			decoder.h \
			tracker.h \
			take.h \
			timelineWidget.h \
			objLoader.h \
			liveTracker.h \
        	serverThreadWorker.h

FORMS    += mainwindow.ui


CONFIG(debug, debug|release) {
	LIBS += -L$(OPENCV_DIR)/x64/vc14/lib \	
			-LE:/ffmpeg/lib \
			-lopencv_world320d \
			-lavcodec \
			-lavdevice \
			-lavfilter \
			-lavformat \
			-lavutil \
			-lpostproc \
			-lswresample \
			-lswscale
}
else {
	LIBS += -L$(OPENCV_DIR)/x64/vc14/lib \
			-LE:/ffmpeg/lib \
			-lopencv_world320 \
			-lavcodec \
			-lavdevice \
			-lavfilter \
			-lavformat \
			-lavutil \
			-lpostproc \
			-lswresample \
			-lswscale
}

INCLUDEPATH += "$(OPENCV_DIR)/include" \
			"E:/ffmpeg/include"