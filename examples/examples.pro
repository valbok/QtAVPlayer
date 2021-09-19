TEMPLATE = subdirs
SUBDIRS += qml_video extract_frames

lessThan(QT_MAJOR_VERSION, 6): SUBDIRS += widget_video
