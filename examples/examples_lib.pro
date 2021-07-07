QTAVPLAYER = $$absolute_path($$_PRO_FILE_PWD_/..)
message('QTAVPLAYER: ' $$QTAVPLAYER)

include(../QtAVPlayerLib.pri)

TEMPLATE = subdirs

SUBDIRS += \
    qml_video \
    qtavplayerlib

qml_video.file = qml_video/qml_video_lib.pro
qtavplayerlib.file = $$QTAVPLAYER/QtAVPlayerLib.pro

qml_video.depends = qtavplayerlib
