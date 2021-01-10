TEMPLATE = subdirs

include($$OUT_PWD/avplayer/qtavplayer-config.pri)
QT_FOR_CONFIG += aplayer-private

sub_src.file = avplayer/avplayerlib.pro
SUBDIRS += sub_src
