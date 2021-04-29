requires(qtHaveModule(multimedia))

load(qt_configure)

load(qt_build_config)

TEMPLATE = subdirs

exists($$_PRO_FILE_PWD_/src/srclib.pro) {
    sub_src.file = $$_PRO_FILE_PWD_/src/srclib.pro
    SUBDIRS += sub_src
}
