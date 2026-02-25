2026-02
-------

- #554 - Added support to build libQtAVPlayer for cases when shared lib is more convenient

2025-10
-------

- #548 - Added rendering cropped frame for MediaCodec to avoid artifacts

2025-10
-------

- #547 - Added a test to mux the streams from different players to one file
- #546 - Added support to mux packets from multiple sources

2025-09
-------

- #543 - Allowed to mux multiple streams to one file from different sources
- #542 - Made QAVPacket public
- #541 - Cleanup
- #540 - Delivered a frame immediately If it has pts in the past.
         This is usefull when frames are synced from different sources.
- #539 - Removed demuxer dep from QAVFilters
- #538 - Removed deprecated `QPacket QDemuxer::read()`
- #537 - Made QDemuxer::decode static and cleaned up
- #535 - Fixed EOF handling in QAVDemuxer::read and its usage in QAVPlayer - tangmingcheng <45536906+tangmingcheng@users.noreply.github.com>

2025-06
-------

- #531 - Fixed the color space for gl widget

2025-04
-------

- #526 - Introduced converting of ID3D11Texture2D to OpenGL textures and use it in `QAVWidget_Opengl`

2025-03
-------

- #524 - Intorduced `QAVMuxerPackets` and `QAVMuxerFrames` muxers.
         Fixed `QAVPlayer::setOutput` to use `QAVMuxerPackets`.
         Now possible to save the streams to output files without reencoding.
         But if needed, `QAVMuxerFrames` is used to encode frames retrieved after applied filters.
- #522 - Added missing cmath include
- #523 - Added Qt 6.8.2 to CI
- #522 - Added missing include
- Fixed compile errors for ffmpeg 4
- #502 - Added `QAVWidget_Opengl` to render using OpenGL
- #517 - Added support of muxing of subtitle
- #509 - Added `QAVMuxer` to encode and save to output file

2025-02
-------

- #515 - Fixed hardcoded `h264_mediacodec` for android
- #514 - Fixed VDPAU rendering on 6.8.2.
- #512 - Ported to Qt 6.8.2

2024
----

- #508 - Provided QAVPlayer::avctx API - Maxime Gervais <gervais.maxime@gmail.com>
- #506 - Added viewPort to QVideoFrameFormat - Vladyslav Arzhanov <vladarzhanov2001@gmail.com>
- #505 - Provided QAVPlayer::setCodecOptions API - Vladyslav Arzhanov <vladarzhanov2001@gmail.com>
- #503 - Fixed crash on abort
- #499 - Added ffmpeg debug control - Charlie LEGER <c.leger@borea-dental.com>
- #500 - Aborted before waiting, it allows to terminate not started stream
- #498 - Fixed player hand on terminate - Charlie LEGER <c.leger@borea-dental.com>
- #496 - Added new MediaCodec should create a new AndroidSurfaceTexture - Vladyslav Arzhanov <vladarzhanov2001@gmail.com>
- #495 - Fixed memory leak due to not clearing AVDictionary - fibonacci-matrix <176021117+fibonacci-matrix@users.noreply.github.com>
- #493 - Introduced QAVAudioConverter
- #492 - Ported to Qt 6.8.0
- #490 - Ported to Qt 6.7.2 - Transporter <ogre.transporter@gmail.com>
- #483 - Fixed a crash on consuming QAVFrame with QGraphicsVideoItem - kimvnhung <kimvnhung@gmail.com>
- #480 - Fixed qmake for lib drm
- #475 - Fixed lib drm compile issues - Mauricio Ferrari <m10ferrari1200@gmail.com>
- #479 - Fixed memory leak for subtitles - xdcsystems <xdcsystems@gmail.com>
- #478 - Added support of QT_AVPLAYER_VA_DRM
- #473 - Introduced cmake support
- #469 - Fixed unprotected eof for demuxer
- #467 - Moved private impl of QAVAudioOutput to audio thread
- #465 - Fixed result of av_read_frame on error
- #464 - Reimplemented QAVAudioOutput to use QThread
- #463 - Ported to libav 59.8 - Transporter <ogre.transporter@gmail.com>
- #459 - Adde a check for Qt 6.6.2 for d3d11
- #456 - Ported to Qt 6.6.2 for d3d11
- #453 - Supported only AV_PIX_FMT_D3D11 for d3d11
- #451 - Ported to Qt 6.7
- #449 - Do not use use QT_NO_KEYWORDS to compile - Gilles Caulier <caulier.gilles@gmail.com>
- #448 - Used QT_NO_CAST_FROM_ASCII
- #444 - Introduced rotate metadata
- #440 - Reported highest position from all streams
- #438 - Added tests for multi players
- #437 - Fixed deprecated LIBAVUTIL_VERSION_MAJOR
- #435 - Don't create AudioOutput with invalid format
- #431 - Set volume to QAVAudioOutput ASAP
- #432 - Fixed overloaded-virtual warning
- #430 - Waited for more frames in QAVAudioOutput
- #429 - Introduced custom QAVAudioFrame

2023
----

- #422 - Introduced QAVAudioOutput::volume
- #420 - Introduced QAVIODevice::setBufferSize()
- #412 - Fixed deadlock in QAVDemuxer::unload
- #408 - Added c++17 for example
- #402 - Returned only one nameless frame
- #399 - Returned frames if no filters
- #398 - Fixed AVERROR_EOF include
- #396 - Fixed latest ffmpeg on Windows - Dniester Amorim <mrfishjr@gmail.com>
- #395 - Mapped AV_PIX_FMT_YUV444P -> VideoFrame::Format_YUV422P
- #391 - Supported 6.5.2 ComPtr
- #390 - Updated qavaudiooutput.cpp - DevelopRepo <philonenko@gmail.com>
- #389 - Deprecated QtAVPlayer module
- #387 - Fixed audio filter to use copy of frame
- #386 - Fixed audio frames count after filter
- #385 - Fixed multiple mixed filters and frames count
- #383 - Returned source frames only if filter is set
- #382 - Allowed to have any video or audio input or out filters
- #377 - Removed drawtext from tests since not supported in Win
- #376 - Demuxed packets only when playing
- #373 - Ported to 6.3
- #372 - Fixed android cmake
- #371 - Intorduced D3D11 Textures
- #369 - Updated examples for QFile
- #368 - CI fixes
- #364 - Removed using QObject
- #363 - Introduced AV_PIX_FMT_MEDIACODEC as Format_SamplerExternalOES
- #362 - Ported to Qt 6.5.0 on CI
- #361 - Fixed cmake
- #359 - Added cmake tests
- #356 - Introduced QAVPlayer::supportedVideoCodecs()
- #355 - Added duration to QAVStream::Progress
- #354 - Introduced QAVStream::Progress
- #353 - Fixed QAVStream::framesCount
- #352 - Deprecated channel_layout
- #351 - Removed opengl public features
- #350 - Deprecated pkt_duration
- #349 - Introduced multiple input filters
- #344 - Returned last sent pts of the frame
- #343 - Returned pts from the queue instead of the clock
- #342 - Don't reset codecs on EOF
- #341 - Flushed buffers on seek
- #339 - Skipped not landed decoded frames for prev pkt on seek
- #338 - Reverted flushing events on last frame
- #337 - Removed unneeded filter frames ref
- #336 - Fixed filterNameStep step
- #335 - Fixed bsfInvalid test
- #332 - Added speed for audio frames
- #328 - Returned a frame on each pause
- #329 - Fixed typo in subtitle::pts
- #327 - Cleared frames when filters applied
- #326 - Cleared decoded frames together with packets in queue
- #323 - Flushed codecs on EOF
- #320 - Removed dependency to QtGUI
- #318 - Build without QtMultimedia
- #317 - Resent packet if avcodec_send_packet returned EAGAIN
- #316 - Reported num of frames received on EOF
- #314 - Changed to read all frame after submitting pkt
- #309 - Read all frames after submitting a pkt
- #311 - Fixed subtitlePlayFuture
- #310 - Introduced QAVAudioOutput::setBufferSize
- #302 - Changed return type from int to void for QAVFilter::read
- #301 - Introduced QAVStream::framesCount
- #298 - Introduced decoding multiple streams
- #297 - Introduced QAVPlayer::inputOptions()
- #295 - Introduced flush for filters
- #294 - Added isEmpty() to filters instead of eof()
- #287 - Cleanup
- #287 - Introduced input video codec
- #286 - Fixed codec as ptr
- #284 - Introduced inputFormat
- #283 - Introduced stepping with multiple filters

2022
----

- #281 - Introduceed QAVFrame::filterInOutName
- #278 - Fixed duration tests
- #275 - Fixed DirectAccess map twice test
- #269 - Recreated filter graph when format has changed
- #268 - Removed const from QAVVideoBuffer::map
- #267 - Added test for map twice
- #266 - QAVVideoFrame: Transfered hw data only once
- #263 - Fixed software rendering for Qt6
- #262 - Fixed relock assert
- #259 - Used planes from textures if any
- #256 - Fixed cmake for MacOS
- #253 - Used RhiTextureHandle for metal textures in macOS/iOS
- #251 - Fixed mem leak when deleting QAVInOutFilterPrivate - Novacer <zhangjack27@gmail.com>
- #248 - Added fps to example
- #245 - Added cmake on CI
- #244 - Wrapped AV_PIX_FMT_XVMC
- #178 - Added qt6 to ci linux
- #238 - Introduced vdpau
- #239 - Fixed crash when no audio device created
- #237 - Deleted QAudioOutput later
- #234 - Fixed mem leak in filters
- #232 - Renamed applying bsf
- #231 - Added bsf
- #226 - Fixed files with unknown streams
- #224 - Fixed seeking to frames when stream does not have duration
- #222 - Supported any formats that can be returned from the decoder
- #218 - Fixed the version type and build error on MacOS - Longshan Du <dulongshan@baidu.com>
- #213 - Added platform name for debug
- #212 - Introduced isSynced
- #211 - Removed hasVideo/hasAudio
- #206 - Added tests for subtitles
- #205 - Introduced pts/duration to subtitle frames
- #203 - Fixed QAVAudioOutput crash on close
- #202 - Checked format for QAVAudioOutput
- #200 - Used smart ptr for QAVAudioOutput
- #199 - Introduced subtitles
- #196 - Fixed crash of unknown pf
- #194 - Cleanup
- #193 - Added missing header
- #192 - Used const AVCodec
- #190 - Used custom QIODevice for QAVAudioOutput
- #188 - Removed streams from packets
- #187 - Removed streamIndex since not needed
- #183 - Added step dbg
- #179 - Introduced QAVStream
- #177 - Added include to QAudioSink
- #176 - Added cmake for Qt6

2021
----

- #170 - QVideoFrame and qt6 in tests
- #167 - Port to Qt6
- #165 - Fixed redefined warn
- #164 - Fixed parsing params
- #163 - Introduced format in url string
- #162 - Cleanup
- #161 - Added supported formats and protocols
- #160 - Blocked decoder's thread until data is available
- #156 - Changed QUrl -> QString
- #155 - Fixed test for delayed buffering
- #154 - Added delayed test for playing from IO
- #153 - Added` test for playing from buffer
- #152, #151 - Skipped all statuses on EOF
- #149 - Fixed threads for io devices
- #147 - Introduced playing from QIODevice
- #146 - Introduced demuxing from QIODevice
- #145 - Added missed include
- #144 - Fixed example to restart surface if size has changed
- #141 - Fixed PRIx64 ci
- #140 - Changed name if new input
- #139 - Removed space PRIx64
- #138 - Fixed matching constructor for initialization of 'struct InOutDeleter'
- #136 - Introduced video widget example
- #134 - Removed unused members in filters
- #132 - Added test to change source with filter
- #130 - Introduced setFilter
- #128 - Introduced setVideoFilter
- #127 - Removed private headers from Qt
- #125 - Added missing include
- #124 - Introduced stepBackward
- #122 - Introduced accurate seek
- #121 - Fixed queue timers after clear
- #120 - Introduced negative seek
- #118 - Removed cond demuxer
- #117 - Added mutex to demuxer
- #116 - Added sleep demuxer when is not needed
- #114 - Added NoMedia after frames
- #109 - Sent NoMedia after all frames
- #108 - Sent EndOfMedia before stop
- #107 - Removed empty frame on end
- #106 - Removed empty frame on stop
- #103 - Added include shared pointer
- #101 - Introduced queue list for audio output
- #99 - Fixed crash with audio output
- #91 - Removed explicit software decoding in tests
- #95 - Respected choices from ffmpeg
- #94 - Introduced audio/video streams
- #89 - Fixed tests
- #88 - Used hw device only if there is hw context available
- #87 - Fixed  macos seeked test
- #85 - Cleanup
- #86 - Temporarily fixed test for macOS and seek
- #84 - Added frames count
- #83 - Increased max logs
- #82 - Introduced stepForward
- #81 - Introduced casting to QVideoFrame
- #80 - Added check for sws ctx
- #79 - Added test for map data
- #78 - Fixed missing EOF when waiting for more packets
- #77 - Introduced frame converting
- #76 - Fixed macos tests
- #75 - Fixed duration test
- #74 - Introduced pending statuses
- #72 - Fixed events handling
- #70 - Added dv files
- #71 - Removed position member
- #68 - Replaced media statuses by signals
- #67 - Fixed sending EOF
- #66 - Fixed pause/seek on eof
- #65 - Introduced stopping media
- #63 - Introduced empty frame on stop
- #58 - Cleared frame rate on terminate
- #57 - Introduced video frame rate
- #56 - Added check if pending pos has changed
- #55 - Added include logging category
- #54 - Introduced logging category
- #53 - Introduced paused signal
- #51 - Fixed seek
- #50 - Fixed tests with seek frame
- #49 - Introduced seeked signal
- #48 - Used signals when sending frames
- #46 - Updated media status on first frame after seeking
- #45 - Added thread pool
- #44 - Used one mutex for queues
- #38 - Introduced SeekingMedia media status
- #43 - Unlocked the mutex while waiting sync
- #42 - Introduced wait for finished for queues
- #40 - Fixed pending position mutex
- #37 - Removed dispatching frames on main thread
- #36 - Removed mute property
- #35 - Added cmake for Qt5.15
- #34 - Introduced QAVPlayer::videoFrame/audioFrame signals
- #33 - Added logs for Windows
- #32 - Fixed ts issues
- #31 - Added windows CI
- #30 - Fixed QList on macOS
- #29 - Added macOS CI
- #28 - Added env var for make
- #27 - Added override
- #25 - Added warning if not enough threads
- #26 - Added waitCond for wait
- #22 - Fixed CI
- #21 - Fixed windows
- #19 - Fixed build issue with the latest ffmpeg
- #16 - Fixed white window on windows - Alexander Ivash <elderorb@gmail.com>
- #12 - Added seek to the first video frame - Alexander Ivash <elderorb@gmail.com>
- #10 - Fixed cmake
- #9 - Fixed video buffer
- #7 - Added wo surface

2020-09-07
----------

- Initial commit
