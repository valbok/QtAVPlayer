QTAVPLAYER = $$absolute_path(../../..)
INCLUDEPATH += $$QTAVPLAYER/src $$QTAVPLAYER/src/QtAVPlayer
DEFINES += QTAVPLAYERLIB

message('QTAVPLAYER: ' $$QTAVPLAYER)

win32: {
    LIBS += -L$$QTAVPLAYER/src/QtAVPlayer/debug -lQtAVPlayer
}
!win32: {
    LIBS += -L$$QTAVPLAYER/src/QtAVPlayer -lQtAVPlayer
}
