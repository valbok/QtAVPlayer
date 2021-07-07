QTAVPLAYER = $$absolute_path($$_PRO_FILE_PWD_/../../..)
message('QTAVPLAYER: ' $$QTAVPLAYER)

include(../../../QtAVPlayerLib.pri)

TEMPLATE = subdirs

SUBDIRS += qavdemuxer qavplayer qtavplayerlib

qavdemuxer.file = qavdemuxer/qavdemuxer_lib.pro
qavplayer.file = qavplayer/qavplayer_lib.pro
qtavplayerlib.file = $$QTAVPLAYER/QtAVPlayerLib.pro

qavdemuxer.depends = qtavplayerlib
qavplayer.depends = qtavplayerlib
