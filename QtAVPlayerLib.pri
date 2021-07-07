isEmpty(QTAVPLAYER) {
    QTAVPLAYER = $$(QTAVPLAYER)
}
message('QTAVPLAYER: ' $$QTAVPLAYER)

FFMPEG = $$(FFMPEG)
message('FFMPEG: ' $$FFMPEG)
!isEmpty(FFMPEG) {

    oldConf = $$cat($$QTAVPLAYER/.qmake.conf.backup, lines)
    isEmpty(oldConf) {
        oldConf = $$cat($$QTAVPLAYER/.qmake.conf, lines)
        message('writting backup of original .qmake.conf')
        write_file($$QTAVPLAYER/.qmake.conf.backup, oldConf)
    } else {
        message('reading backup of original .qmake.conf.backup')
    }

    message('oldConf: ' $$oldConf)
    write_file($$QTAVPLAYER/.qmake.conf, oldConf)

    exists($$FFMPEG/include) {
        FFMPEG_INCLUDES=$$absolute_path($$FFMPEG/include)
    } else {
        FFMPEG_INCLUDES=$$FFMPEG
    }

    exists($$absolute_path($$FFMPEG/lib)) {
        FFMPEG_LIBS += -L$$absolute_path($$FFMPEG/lib) \
                    -lavdevice \
                    -lavcodec \
                    -lavfilter \
                    -lavformat \
                    -lpostproc \
                    -lswresample \
                    -lswscale \
                    -lavutil

        FFMPEG_EXTRA_LIBS += $$absolute_path($$FFMPEG/lib)
    } else {
        FFMPEG_AVDEVICE=$$absolute_path($$FFMPEG/libavdevice)
        FFMPEG_AVCODEC=$$absolute_path($$FFMPEG/libavcodec)
        FFMPEG_AVFILTER=$$absolute_path($$FFMPEG/libavfilter)
        FFMPEG_AVFORMAT=$$absolute_path($$FFMPEG/libavformat)
        FFMPEG_POSTPROC=$$absolute_path($$FFMPEG/libpostproc)
        FFMPEG_SWRESAMPLE=$$absolute_path($$FFMPEG/libswresample)
        FFMPEG_SWSCALE=$$absolute_path($$FFMPEG/libswscale)
        FFMPEG_AVUTIL=$$absolute_path($$FFMPEG/libavutil)

        FFMPEG_EXTRA_LIBS += \
                        $$FFMPEG_AVDEVICE \
                        $$FFMPEG_AVCODEC \
                        $$FFMPEG_AVFILTER \
                        $$FFMPEG_AVFORMAT \
                        $$FFMPEG_POSTPROC \
                        $$FFMPEG_SWRESAMPLE \
                        $$FFMPEG_SWSCALE \
                        $$FFMPEG_AVUTIL

        FFMPEG_LIBS += \
                     -L$$FFMPEG_AVDEVICE -lavdevice \
                     -L$$FFMPEG_AVCODEC -lavcodec \
                     -L$$FFMPEG_AVFILTER -lavfilter \
                     -L$$FFMPEG_AVFORMAT -lavformat \
                     -L$$FFMPEG_POSTPROC -lpostproc \
                     -L$$FFMPEG_SWRESAMPLE -lswresample \
                     -L$$FFMPEG_SWSCALE -lswscale \
                     -L$$FFMPEG_AVUTIL -lavutil
    }

    ffmpegIncludes = "INCLUDEPATH+=$$FFMPEG_INCLUDES"
    ffmpegLibs = "LIBS+=$$FFMPEG_LIBS"
    ffmpegExtraIncludes = "EXTRA_INCLUDEPATH+=$$FFMPEG_INCLUDES"
    ffmpegExtraLibs = "EXTRA_LIBDIR+=$$FFMPEG_EXTRA_LIBS"

    write_file($$QTAVPLAYER/.qmake.conf, ffmpegIncludes, append)
    write_file($$QTAVPLAYER/.qmake.conf, ffmpegLibs, append)

    write_file($$QTAVPLAYER/.qmake.conf, ffmpegExtraIncludes, append)
    write_file($$QTAVPLAYER/.qmake.conf, ffmpegExtraLibs, append)
}
