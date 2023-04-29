TEMPLATE = subdirs
qtConfig(multimedia): {
	SUBDIRS += qml_video
	lessThan(QT_MAJOR_VERSION, 6): SUBDIRS += widget_video
}
