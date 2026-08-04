- https://github.com/valbok/QtAVPlayer/pull/693 - Fixed hardcoded h264_mediacodec for android
- https://github.com/valbok/QtAVPlayer/pull/692 - Added test to scale frames and return original ones
- https://github.com/valbok/QtAVPlayer/pull/691 - Fixed QAVAudioOutput::setBufferSize to allow new size after start of QAudioSink
- https://github.com/valbok/QtAVPlayer/pull/690 - Fixed qml_player to show subtitles in correct size
- https://github.com/valbok/QtAVPlayer/pull/689 - Introduced selection of audio device in QAVAudioOutput
- https://github.com/valbok/QtAVPlayer/pull/687 - Introduced max queued bytes and seconds to limit the demuxed packets
- https://github.com/valbok/QtAVPlayer/pull/686 - Fixed QAVAudioOutputDevice::readData to wait for more bytes if no more bytes available
- https://github.com/valbok/QtAVPlayer/pull/685 - Added subtitleQueue.bytes to total bytes in demuxer thread
- https://github.com/valbok/QtAVPlayer/pull/683 - Fixed tst_QAVPlayer::outputFile
- https://github.com/valbok/QtAVPlayer/pull/680 - Fixed qml_player to how full length streams in qml player
- https://github.com/valbok/QtAVPlayer/pull/679 - Introduced QAVPlayer::MuxerError to QAVPlayer
- https://github.com/valbok/QtAVPlayer/pull/678 - Fixed spurious wakeup in QAVMuxerFramesPrivate::doWork() causing muxerEnqueue failure on macOS
- https://github.com/valbok/QtAVPlayer/pull/677 - Fixed fullscreen player control in qml_player
- https://github.com/valbok/QtAVPlayer/pull/676 - Added android support to CI
- https://github.com/valbok/QtAVPlayer/pull/674 - Fixed position slider jumping on click in qml_player
- https://github.com/valbok/QtAVPlayer/pull/672 - Renamed QAVMuxerSubtitleFrames to QAVSubtitleTextParser

v2026-07-21
-----------

- https://github.com/valbok/QtAVPlayer/pull/670 - Added support for Qt 5.6 - roundedrectangle <129410215+roundedrectangle@users.noreply.github.com>
- https://github.com/valbok/QtAVPlayer/pull/669 - Moved interrupt_callback.callback to QAVFormatContext to prevent the crash
- https://github.com/valbok/QtAVPlayer/pull/667 - Added QGridLayout as example for QAVWidget_OpenGL
- https://github.com/valbok/QtAVPlayer/pull/664 - Fixed QAVWidget_OpenGL::setVideoFrame to delete current frame on gui thread
- https://github.com/valbok/QtAVPlayer/pull/661 - Fixed QAVWidget_OpenGL to call update() on GUI thread
- https://github.com/valbok/QtAVPlayer/pull/659 - Added 6.11.1 to CI

2026-06
-------

- https://github.com/valbok/QtAVPlayer/pull/656 - Introduced QAVMuxerFrames::EncoderStream to set encoding info
- https://github.com/valbok/QtAVPlayer/pull/654 - Moved creating of new stream to separate func in Muxer
- https://github.com/valbok/QtAVPlayer/pull/653 - Added test to mux hw scaled frames
- https://github.com/valbok/QtAVPlayer/pull/650 - Fixed tests to set codec before source
- https://github.com/valbok/QtAVPlayer/pull/649 - Introduced QAVMuxerFrames::outputVideoCodec
- https://github.com/valbok/QtAVPlayer/pull/647 - Added AV_PIX_FMT_D3D12 to isSoftwarePixelFormat
- https://github.com/valbok/QtAVPlayer/pull/641 - Fixed player to apply filters after hw_frames_ctx is negotiated
- https://github.com/valbok/QtAVPlayer/pull/643 - Fixed muxer tests to use software codec
- https://github.com/valbok/QtAVPlayer/pull/638 - Introduced hw devices for filters
- https://github.com/valbok/QtAVPlayer/pull/636 - Negotiated hw device context based on provided codec in demuxer
- https://github.com/valbok/QtAVPlayer/pull/637 - Removed double \n in ffmpeg logs - Fedar <fedorkrivitskiy@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/637 - Introduced QT_AVPLAYER_CUDA define
- https://github.com/valbok/QtAVPlayer/pull/634 - Introduced hwaccel cuda support

2026-05
-------

- https://github.com/valbok/QtAVPlayer/pull/631 - Moved muxers to separate files
- https://github.com/valbok/QtAVPlayer/pull/630 - Moved ass renderer to separate file
- https://github.com/valbok/QtAVPlayer/pull/629 - Fixed test to use filtered frames in Muxer
- https://github.com/valbok/QtAVPlayer/pull/628 - Fixed missed pts after videoFrame.convertTo()
- https://github.com/valbok/QtAVPlayer/pull/627 - Added test to change encoder frame size in muxer
- https://github.com/valbok/QtAVPlayer/pull/625 - Refactored muxer
- https://github.com/valbok/QtAVPlayer/pull/624 - Removed implicit conversion in muxer
- https://github.com/valbok/QtAVPlayer/pull/604 - Added offset functions for change video position - Kioro404 <angelo995.2001@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/621 - Fixed seek slider in QML Player example to avoid double seek
- https://github.com/valbok/QtAVPlayer/pull/620 - Fixed position in QML Player example to avoid jumping
- https://github.com/valbok/QtAVPlayer/pull/619 - Fixed QAVAudioOutput to resume the audio sink before stopping
- https://github.com/valbok/QtAVPlayer/pull/616 - Added helper function to return number of seconds based on audio format and num of bytes
- https://github.com/valbok/QtAVPlayer/pull/613 - Introduced QAVAudioOutput::suspend() and QAVAudioOutput::resume()
- https://github.com/valbok/QtAVPlayer/pull/612 - Fixed QML Player example to destroy the player first
- https://github.com/valbok/QtAVPlayer/pull/611 - Fixed QAVAudioOutput to use preferred format when frame's format is not supported by audio device
- https://github.com/valbok/QtAVPlayer/pull/609 - Added QAVCodec::size(), video dimensions
- https://github.com/valbok/QtAVPlayer/pull/608 - Added QAVASSRenderer to QML Player example
- https://github.com/valbok/QtAVPlayer/pull/607 - Added QAVASSRenderer::flush on seek
- https://github.com/valbok/QtAVPlayer/pull/606 - Introduced QAVASSRenderer with -DQT_AVPLAYER_LIBASS=ON
- https://github.com/valbok/QtAVPlayer/pull/605 - Added fix for compatible with Qt5.13 and below versions Q_DISABLE_COPY_MOVE - 郭汉盛 <guohs01@bwoil.com>
- https://github.com/valbok/QtAVPlayer/pull/602 - Fixed qml player example to handle updated position
- https://github.com/valbok/QtAVPlayer/pull/601 - Introduced shared QAVFormatContext between frames

2026-04
-------

- https://github.com/valbok/QtAVPlayer/pull/600 - Fixed QAVPlayer to send the last frame before EndOfMedia
- https://github.com/valbok/QtAVPlayer/pull/596 - Fixed QAVAudioOutput to flush buffer on EOF
- https://github.com/valbok/QtAVPlayer/pull/594 - Made QAVAudioConverter private
- https://github.com/valbok/QtAVPlayer/pull/593 - Fixed QAVAudioOutput to handle AV_CODEC_ID_PCM_S24BE frames
- https://github.com/valbok/QtAVPlayer/pull/570 - Fixed QAVAudioOutput to restart audio-device on format change
- https://github.com/valbok/QtAVPlayer/pull/591 - Added title to QAVStream
- https://github.com/valbok/QtAVPlayer/pull/590 - Fixed QAVAudioOutput to clear audio queue when changing audio format
- https://github.com/valbok/QtAVPlayer/pull/588 - Fixed vdpau to re-use vdpau mixer on next frame
- https://github.com/valbok/QtAVPlayer/pull/587 - Added copy-free render to qml player example
- https://github.com/valbok/QtAVPlayer/pull/585 - Fixed QAVVideoFrame to share internal video buffer on copy
- https://github.com/valbok/QtAVPlayer/pull/584 - Added isMapped to QAVVideoFrame
- https://github.com/valbok/QtAVPlayer/pull/583 - Added software video codec to qml player example
- https://github.com/valbok/QtAVPlayer/pull/581 - Added audio and video tracks to qml player example
- https://github.com/valbok/QtAVPlayer/pull/578 - Recreated audio output if format has changed in QAVAudioOutput
- https://github.com/valbok/QtAVPlayer/pull/576 - Made QAVAudioOutputDevice private
- https://github.com/valbok/QtAVPlayer/pull/573 - Added hw_frames_ctx to video input filters.
- https://github.com/valbok/QtAVPlayer/pull/571 - Added a fix to avoid using texture handle from QVideoFrame if it is already mapped
- https://github.com/valbok/QtAVPlayer/pull/569 - Fixed memory leak in QAVSubtitleCodec::write
- https://github.com/valbok/QtAVPlayer/pull/567 - Introduced 'software' video codec hardcoded value to avoid hwdevice context
- https://github.com/valbok/QtAVPlayer/pull/566 - Introduced subtitles into qml player example
- https://github.com/valbok/QtAVPlayer/pull/565 - Introduced parsing of text from subtitles by QAVMuxerSubtitleFrames
- https://github.com/valbok/QtAVPlayer/pull/563 - Introduced qml_player example with controls
- https://github.com/valbok/QtAVPlayer/pull/564 - Fixed frame_size was not respected for a non-last frame on muxing

2026-03
-------

- https://github.com/valbok/QtAVPlayer/pull/562 - Moved normalizing of packets pts to demuxer from muxer
- https://github.com/valbok/QtAVPlayer/pull/561 - Fixed to return EINVAL if a packet is not accepted by a muxer
- https://github.com/valbok/QtAVPlayer/pull/560 - Exited the example muxer if EOF
- https://github.com/valbok/QtAVPlayer/pull/559 - Fixed crash during muxing
- https://github.com/valbok/QtAVPlayer/pull/558 - Added widget_video_opengl to CI
- https://github.com/valbok/QtAVPlayer/pull/557 - Normalized pts of packets if these packets are based on start time, f.e. retrieved from video device
- https://github.com/valbok/QtAVPlayer/pull/556 - Introduced support of AV_PIX_FMT_YUYV422 in OpenGL widget
- https://github.com/valbok/QtAVPlayer/pull/555 - Added support for change aspect ratio mode - Kioro404 <angelo61br@gmail.com>

2026-02
-------

- https://github.com/valbok/QtAVPlayer/pull/554 - Added support to build libQtAVPlayer for cases when shared lib is more convenient

2025-11
-------

- https://github.com/valbok/QtAVPlayer/pull/548 - Added rendering cropped frame for MediaCodec to avoid artifacts

2025-10
-------

- https://github.com/valbok/QtAVPlayer/pull/547 - Added a test to mux the streams from different players to one file
- https://github.com/valbok/QtAVPlayer/pull/546 - Added support to mux packets from multiple sources

2025-09
-------

- https://github.com/valbok/QtAVPlayer/pull/543 - Allowed to mux multiple streams to one file from different sources
- https://github.com/valbok/QtAVPlayer/pull/542 - Made QAVPacket public
- https://github.com/valbok/QtAVPlayer/pull/541 - Cleanup
- https://github.com/valbok/QtAVPlayer/pull/540 - Delivered a frame immediately If it has pts in the past.
         This is usefull when frames are synced from different sources.
- https://github.com/valbok/QtAVPlayer/pull/539 - Removed demuxer dep from QAVFilters
- https://github.com/valbok/QtAVPlayer/pull/538 - Removed deprecated `QPacket QDemuxer::read()`
- https://github.com/valbok/QtAVPlayer/pull/537 - Made QDemuxer::decode static and cleaned up
- https://github.com/valbok/QtAVPlayer/pull/535 - Fixed EOF handling in QAVDemuxer::read and its usage in QAVPlayer - tangmingcheng <45536906+tangmingcheng@users.noreply.github.com>

2025-06
-------

- https://github.com/valbok/QtAVPlayer/pull/531 - Fixed the color space for gl widget

2025-04
-------

- https://github.com/valbok/QtAVPlayer/pull/526 - Introduced converting of ID3D11Texture2D to OpenGL textures and use it in `QAVWidget_Opengl`

2025-03
-------

- https://github.com/valbok/QtAVPlayer/pull/524 - Introduced `QAVMuxerPackets` and `QAVMuxerFrames` muxers.
         Fixed `QAVPlayer::setOutput` to use `QAVMuxerPackets`.
         Now possible to save the streams to output files without reencoding.
         But if needed, `QAVMuxerFrames` is used to encode frames retrieved after applied filters.
- https://github.com/valbok/QtAVPlayer/pull/522 - Added missing cmath include
- https://github.com/valbok/QtAVPlayer/pull/523 - Added Qt 6.8.2 to CI
- https://github.com/valbok/QtAVPlayer/pull/522 - Added missing include
- Fixed compile errors for ffmpeg 4
- https://github.com/valbok/QtAVPlayer/pull/502 - Added `QAVWidget_Opengl` to render using OpenGL
- https://github.com/valbok/QtAVPlayer/pull/517 - Added support of muxing of subtitle
- https://github.com/valbok/QtAVPlayer/pull/509 - Added `QAVMuxer` to encode and save to output file

2025-02
-------

- https://github.com/valbok/QtAVPlayer/pull/515 - Fixed hardcoded `h264_mediacodec` for android
- https://github.com/valbok/QtAVPlayer/pull/514 - Fixed VDPAU rendering on 6.8.2.
- https://github.com/valbok/QtAVPlayer/pull/512 - Ported to Qt 6.8.2

2024
----

- https://github.com/valbok/QtAVPlayer/pull/508 - Provided QAVPlayer::avctx API - Maxime Gervais <gervais.maxime@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/506 - Added viewPort to QVideoFrameFormat - Vladyslav Arzhanov <vladarzhanov2001@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/505 - Provided QAVPlayer::setCodecOptions API - Vladyslav Arzhanov <vladarzhanov2001@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/503 - Fixed crash on abort
- https://github.com/valbok/QtAVPlayer/pull/499 - Added ffmpeg debug control - Charlie LEGER <c.leger@borea-dental.com>
- https://github.com/valbok/QtAVPlayer/pull/500 - Aborted before waiting, it allows to terminate not started stream
- https://github.com/valbok/QtAVPlayer/pull/498 - Fixed player hand on terminate - Charlie LEGER <c.leger@borea-dental.com>
- https://github.com/valbok/QtAVPlayer/pull/496 - Added new MediaCodec should create a new AndroidSurfaceTexture - Vladyslav Arzhanov <vladarzhanov2001@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/495 - Fixed memory leak due to not clearing AVDictionary - fibonacci-matrix <176021117+fibonacci-matrix@users.noreply.github.com>
- https://github.com/valbok/QtAVPlayer/pull/493 - Introduced QAVAudioConverter
- https://github.com/valbok/QtAVPlayer/pull/492 - Ported to Qt 6.8.0
- https://github.com/valbok/QtAVPlayer/pull/490 - Ported to Qt 6.7.2 - Transporter <ogre.transporter@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/483 - Fixed a crash on consuming QAVFrame with QGraphicsVideoItem - kimvnhung <kimvnhung@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/480 - Fixed qmake for lib drm
- https://github.com/valbok/QtAVPlayer/pull/475 - Fixed lib drm compile issues - Mauricio Ferrari <m10ferrari1200@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/479 - Fixed memory leak for subtitles - xdcsystems <xdcsystems@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/478 - Added support of QT_AVPLAYER_VA_DRM
- https://github.com/valbok/QtAVPlayer/pull/473 - Introduced cmake support
- https://github.com/valbok/QtAVPlayer/pull/469 - Fixed unprotected eof for demuxer
- https://github.com/valbok/QtAVPlayer/pull/467 - Moved private impl of QAVAudioOutput to audio thread
- https://github.com/valbok/QtAVPlayer/pull/465 - Fixed result of av_read_frame on error
- https://github.com/valbok/QtAVPlayer/pull/464 - Reimplemented QAVAudioOutput to use QThread
- https://github.com/valbok/QtAVPlayer/pull/463 - Ported to libav 59.8 - Transporter <ogre.transporter@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/459 - Adde a check for Qt 6.6.2 for d3d11
- https://github.com/valbok/QtAVPlayer/pull/456 - Ported to Qt 6.6.2 for d3d11
- https://github.com/valbok/QtAVPlayer/pull/453 - Supported only AV_PIX_FMT_D3D11 for d3d11
- https://github.com/valbok/QtAVPlayer/pull/451 - Ported to Qt 6.7
- https://github.com/valbok/QtAVPlayer/pull/449 - Do not use use QT_NO_KEYWORDS to compile - Gilles Caulier <caulier.gilles@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/448 - Used QT_NO_CAST_FROM_ASCII
- https://github.com/valbok/QtAVPlayer/pull/444 - Introduced rotate metadata
- https://github.com/valbok/QtAVPlayer/pull/440 - Reported highest position from all streams
- https://github.com/valbok/QtAVPlayer/pull/438 - Added tests for multi players
- https://github.com/valbok/QtAVPlayer/pull/437 - Fixed deprecated LIBAVUTIL_VERSION_MAJOR
- https://github.com/valbok/QtAVPlayer/pull/435 - Don't create AudioOutput with invalid format
- https://github.com/valbok/QtAVPlayer/pull/431 - Set volume to QAVAudioOutput ASAP
- https://github.com/valbok/QtAVPlayer/pull/432 - Fixed overloaded-virtual warning
- https://github.com/valbok/QtAVPlayer/pull/430 - Waited for more frames in QAVAudioOutput
- https://github.com/valbok/QtAVPlayer/pull/429 - Introduced custom QAVAudioFrame

2023
----

- https://github.com/valbok/QtAVPlayer/pull/422 - Introduced QAVAudioOutput::volume
- https://github.com/valbok/QtAVPlayer/pull/420 - Introduced QAVIODevice::setBufferSize()
- https://github.com/valbok/QtAVPlayer/pull/412 - Fixed deadlock in QAVDemuxer::unload
- https://github.com/valbok/QtAVPlayer/pull/408 - Added c++17 for example
- https://github.com/valbok/QtAVPlayer/pull/402 - Returned only one nameless frame
- https://github.com/valbok/QtAVPlayer/pull/399 - Returned frames if no filters
- https://github.com/valbok/QtAVPlayer/pull/398 - Fixed AVERROR_EOF include
- https://github.com/valbok/QtAVPlayer/pull/396 - Fixed latest ffmpeg on Windows - Dniester Amorim <mrfishjr@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/395 - Mapped AV_PIX_FMT_YUV444P -> VideoFrame::Format_YUV422P
- https://github.com/valbok/QtAVPlayer/pull/391 - Supported 6.5.2 ComPtr
- https://github.com/valbok/QtAVPlayer/pull/390 - Updated qavaudiooutput.cpp - DevelopRepo <philonenko@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/389 - Deprecated QtAVPlayer module
- https://github.com/valbok/QtAVPlayer/pull/387 - Fixed audio filter to use copy of frame
- https://github.com/valbok/QtAVPlayer/pull/386 - Fixed audio frames count after filter
- https://github.com/valbok/QtAVPlayer/pull/385 - Fixed multiple mixed filters and frames count
- https://github.com/valbok/QtAVPlayer/pull/383 - Returned source frames only if filter is set
- https://github.com/valbok/QtAVPlayer/pull/382 - Allowed to have any video or audio input or out filters
- https://github.com/valbok/QtAVPlayer/pull/377 - Removed drawtext from tests since not supported in Win
- https://github.com/valbok/QtAVPlayer/pull/376 - Demuxed packets only when playing
- https://github.com/valbok/QtAVPlayer/pull/373 - Ported to 6.3
- https://github.com/valbok/QtAVPlayer/pull/372 - Fixed android cmake
- https://github.com/valbok/QtAVPlayer/pull/371 - Intorduced D3D11 Textures
- https://github.com/valbok/QtAVPlayer/pull/369 - Updated examples for QFile
- https://github.com/valbok/QtAVPlayer/pull/368 - CI fixes
- https://github.com/valbok/QtAVPlayer/pull/364 - Removed using QObject
- https://github.com/valbok/QtAVPlayer/pull/363 - Introduced AV_PIX_FMT_MEDIACODEC as Format_SamplerExternalOES
- https://github.com/valbok/QtAVPlayer/pull/362 - Ported to Qt 6.5.0 on CI
- https://github.com/valbok/QtAVPlayer/pull/361 - Fixed cmake
- https://github.com/valbok/QtAVPlayer/pull/359 - Added cmake tests
- https://github.com/valbok/QtAVPlayer/pull/356 - Introduced QAVPlayer::supportedVideoCodecs()
- https://github.com/valbok/QtAVPlayer/pull/355 - Added duration to QAVStream::Progress
- https://github.com/valbok/QtAVPlayer/pull/354 - Introduced QAVStream::Progress
- https://github.com/valbok/QtAVPlayer/pull/353 - Fixed QAVStream::framesCount
- https://github.com/valbok/QtAVPlayer/pull/352 - Deprecated channel_layout
- https://github.com/valbok/QtAVPlayer/pull/351 - Removed opengl public features
- https://github.com/valbok/QtAVPlayer/pull/350 - Deprecated pkt_duration
- https://github.com/valbok/QtAVPlayer/pull/349 - Introduced multiple input filters
- https://github.com/valbok/QtAVPlayer/pull/344 - Returned last sent pts of the frame
- https://github.com/valbok/QtAVPlayer/pull/343 - Returned pts from the queue instead of the clock
- https://github.com/valbok/QtAVPlayer/pull/342 - Don't reset codecs on EOF
- https://github.com/valbok/QtAVPlayer/pull/341 - Flushed buffers on seek
- https://github.com/valbok/QtAVPlayer/pull/339 - Skipped not landed decoded frames for prev pkt on seek
- https://github.com/valbok/QtAVPlayer/pull/338 - Reverted flushing events on last frame
- https://github.com/valbok/QtAVPlayer/pull/337 - Removed unneeded filter frames ref
- https://github.com/valbok/QtAVPlayer/pull/336 - Fixed filterNameStep step
- https://github.com/valbok/QtAVPlayer/pull/335 - Fixed bsfInvalid test
- https://github.com/valbok/QtAVPlayer/pull/332 - Added speed for audio frames
- https://github.com/valbok/QtAVPlayer/pull/328 - Returned a frame on each pause
- https://github.com/valbok/QtAVPlayer/pull/329 - Fixed typo in subtitle::pts
- https://github.com/valbok/QtAVPlayer/pull/327 - Cleared frames when filters applied
- https://github.com/valbok/QtAVPlayer/pull/326 - Cleared decoded frames together with packets in queue
- https://github.com/valbok/QtAVPlayer/pull/323 - Flushed codecs on EOF
- https://github.com/valbok/QtAVPlayer/pull/320 - Removed dependency to QtGUI
- https://github.com/valbok/QtAVPlayer/pull/318 - Build without QtMultimedia
- https://github.com/valbok/QtAVPlayer/pull/317 - Resent packet if avcodec_send_packet returned EAGAIN
- https://github.com/valbok/QtAVPlayer/pull/316 - Reported num of frames received on EOF
- https://github.com/valbok/QtAVPlayer/pull/314 - Changed to read all frame after submitting pkt
- https://github.com/valbok/QtAVPlayer/pull/309 - Read all frames after submitting a pkt
- https://github.com/valbok/QtAVPlayer/pull/311 - Fixed subtitlePlayFuture
- https://github.com/valbok/QtAVPlayer/pull/310 - Introduced QAVAudioOutput::setBufferSize
- https://github.com/valbok/QtAVPlayer/pull/302 - Changed return type from int to void for QAVFilter::read
- https://github.com/valbok/QtAVPlayer/pull/301 - Introduced QAVStream::framesCount
- https://github.com/valbok/QtAVPlayer/pull/298 - Introduced decoding multiple streams
- https://github.com/valbok/QtAVPlayer/pull/297 - Introduced QAVPlayer::inputOptions()
- https://github.com/valbok/QtAVPlayer/pull/295 - Introduced flush for filters
- https://github.com/valbok/QtAVPlayer/pull/294 - Added isEmpty() to filters instead of eof()
- https://github.com/valbok/QtAVPlayer/pull/287 - Cleanup
- https://github.com/valbok/QtAVPlayer/pull/287 - Introduced input video codec
- https://github.com/valbok/QtAVPlayer/pull/286 - Fixed codec as ptr
- https://github.com/valbok/QtAVPlayer/pull/284 - Introduced inputFormat
- https://github.com/valbok/QtAVPlayer/pull/283 - Introduced stepping with multiple filters

2022
----

- https://github.com/valbok/QtAVPlayer/pull/281 - Introduceed QAVFrame::filterInOutName
- https://github.com/valbok/QtAVPlayer/pull/278 - Fixed duration tests
- https://github.com/valbok/QtAVPlayer/pull/275 - Fixed DirectAccess map twice test
- https://github.com/valbok/QtAVPlayer/pull/269 - Recreated filter graph when format has changed
- https://github.com/valbok/QtAVPlayer/pull/268 - Removed const from QAVVideoBuffer::map
- https://github.com/valbok/QtAVPlayer/pull/267 - Added test for map twice
- https://github.com/valbok/QtAVPlayer/pull/266 - QAVVideoFrame: Transfered hw data only once
- https://github.com/valbok/QtAVPlayer/pull/263 - Fixed software rendering for Qt6
- https://github.com/valbok/QtAVPlayer/pull/262 - Fixed relock assert
- https://github.com/valbok/QtAVPlayer/pull/259 - Used planes from textures if any
- https://github.com/valbok/QtAVPlayer/pull/256 - Fixed cmake for MacOS
- https://github.com/valbok/QtAVPlayer/pull/253 - Used RhiTextureHandle for metal textures in macOS/iOS
- https://github.com/valbok/QtAVPlayer/pull/251 - Fixed mem leak when deleting QAVInOutFilterPrivate - Novacer <zhangjack27@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/248 - Added fps to example
- https://github.com/valbok/QtAVPlayer/pull/245 - Added cmake on CI
- https://github.com/valbok/QtAVPlayer/pull/244 - Wrapped AV_PIX_FMT_XVMC
- https://github.com/valbok/QtAVPlayer/pull/178 - Added qt6 to ci linux
- https://github.com/valbok/QtAVPlayer/pull/238 - Introduced vdpau
- https://github.com/valbok/QtAVPlayer/pull/239 - Fixed crash when no audio device created
- https://github.com/valbok/QtAVPlayer/pull/237 - Deleted QAudioOutput later
- https://github.com/valbok/QtAVPlayer/pull/234 - Fixed mem leak in filters
- https://github.com/valbok/QtAVPlayer/pull/232 - Renamed applying bsf
- https://github.com/valbok/QtAVPlayer/pull/231 - Added bsf
- https://github.com/valbok/QtAVPlayer/pull/226 - Fixed files with unknown streams
- https://github.com/valbok/QtAVPlayer/pull/224 - Fixed seeking to frames when stream does not have duration
- https://github.com/valbok/QtAVPlayer/pull/222 - Supported any formats that can be returned from the decoder
- https://github.com/valbok/QtAVPlayer/pull/218 - Fixed the version type and build error on MacOS - Longshan Du <dulongshan@baidu.com>
- https://github.com/valbok/QtAVPlayer/pull/213 - Added platform name for debug
- https://github.com/valbok/QtAVPlayer/pull/212 - Introduced isSynced
- https://github.com/valbok/QtAVPlayer/pull/211 - Removed hasVideo/hasAudio
- https://github.com/valbok/QtAVPlayer/pull/206 - Added tests for subtitles
- https://github.com/valbok/QtAVPlayer/pull/205 - Introduced pts/duration to subtitle frames
- https://github.com/valbok/QtAVPlayer/pull/203 - Fixed QAVAudioOutput crash on close
- https://github.com/valbok/QtAVPlayer/pull/202 - Checked format for QAVAudioOutput
- https://github.com/valbok/QtAVPlayer/pull/200 - Used smart ptr for QAVAudioOutput
- https://github.com/valbok/QtAVPlayer/pull/199 - Introduced subtitles
- https://github.com/valbok/QtAVPlayer/pull/196 - Fixed crash of unknown pf
- https://github.com/valbok/QtAVPlayer/pull/194 - Cleanup
- https://github.com/valbok/QtAVPlayer/pull/193 - Added missing header
- https://github.com/valbok/QtAVPlayer/pull/192 - Used const AVCodec
- https://github.com/valbok/QtAVPlayer/pull/190 - Used custom QIODevice for QAVAudioOutput
- https://github.com/valbok/QtAVPlayer/pull/188 - Removed streams from packets
- https://github.com/valbok/QtAVPlayer/pull/187 - Removed streamIndex since not needed
- https://github.com/valbok/QtAVPlayer/pull/183 - Added step dbg
- https://github.com/valbok/QtAVPlayer/pull/179 - Introduced QAVStream
- https://github.com/valbok/QtAVPlayer/pull/177 - Added include to QAudioSink
- https://github.com/valbok/QtAVPlayer/pull/176 - Added cmake for Qt6

2021
----

- https://github.com/valbok/QtAVPlayer/pull/170 - QVideoFrame and qt6 in tests
- https://github.com/valbok/QtAVPlayer/pull/167 - Port to Qt6
- https://github.com/valbok/QtAVPlayer/pull/165 - Fixed redefined warn
- https://github.com/valbok/QtAVPlayer/pull/164 - Fixed parsing params
- https://github.com/valbok/QtAVPlayer/pull/163 - Introduced format in url string
- https://github.com/valbok/QtAVPlayer/pull/162 - Cleanup
- https://github.com/valbok/QtAVPlayer/pull/161 - Added supported formats and protocols
- https://github.com/valbok/QtAVPlayer/pull/160 - Blocked decoder's thread until data is available
- https://github.com/valbok/QtAVPlayer/pull/156 - Changed QUrl -> QString
- https://github.com/valbok/QtAVPlayer/pull/155 - Fixed test for delayed buffering
- https://github.com/valbok/QtAVPlayer/pull/154 - Added delayed test for playing from IO
- https://github.com/valbok/QtAVPlayer/pull/153 - Added` test for playing from buffer
- https://github.com/valbok/QtAVPlayer/pull/152, #151 - Skipped all statuses on EOF
- https://github.com/valbok/QtAVPlayer/pull/149 - Fixed threads for io devices
- https://github.com/valbok/QtAVPlayer/pull/147 - Introduced playing from QIODevice
- https://github.com/valbok/QtAVPlayer/pull/146 - Introduced demuxing from QIODevice
- https://github.com/valbok/QtAVPlayer/pull/145 - Added missed include
- https://github.com/valbok/QtAVPlayer/pull/144 - Fixed example to restart surface if size has changed
- https://github.com/valbok/QtAVPlayer/pull/141 - Fixed PRIx64 ci
- https://github.com/valbok/QtAVPlayer/pull/140 - Changed name if new input
- https://github.com/valbok/QtAVPlayer/pull/139 - Removed space PRIx64
- https://github.com/valbok/QtAVPlayer/pull/138 - Fixed matching constructor for initialization of 'struct InOutDeleter'
- https://github.com/valbok/QtAVPlayer/pull/136 - Introduced video widget example
- https://github.com/valbok/QtAVPlayer/pull/134 - Removed unused members in filters
- https://github.com/valbok/QtAVPlayer/pull/132 - Added test to change source with filter
- https://github.com/valbok/QtAVPlayer/pull/130 - Introduced setFilter
- https://github.com/valbok/QtAVPlayer/pull/128 - Introduced setVideoFilter
- https://github.com/valbok/QtAVPlayer/pull/127 - Removed private headers from Qt
- https://github.com/valbok/QtAVPlayer/pull/125 - Added missing include
- https://github.com/valbok/QtAVPlayer/pull/124 - Introduced stepBackward
- https://github.com/valbok/QtAVPlayer/pull/122 - Introduced accurate seek
- https://github.com/valbok/QtAVPlayer/pull/121 - Fixed queue timers after clear
- https://github.com/valbok/QtAVPlayer/pull/120 - Introduced negative seek
- https://github.com/valbok/QtAVPlayer/pull/118 - Removed cond demuxer
- https://github.com/valbok/QtAVPlayer/pull/117 - Added mutex to demuxer
- https://github.com/valbok/QtAVPlayer/pull/116 - Added sleep demuxer when is not needed
- https://github.com/valbok/QtAVPlayer/pull/114 - Added NoMedia after frames
- https://github.com/valbok/QtAVPlayer/pull/109 - Sent NoMedia after all frames
- https://github.com/valbok/QtAVPlayer/pull/108 - Sent EndOfMedia before stop
- https://github.com/valbok/QtAVPlayer/pull/107 - Removed empty frame on end
- https://github.com/valbok/QtAVPlayer/pull/106 - Removed empty frame on stop
- https://github.com/valbok/QtAVPlayer/pull/103 - Added include shared pointer
- https://github.com/valbok/QtAVPlayer/pull/101 - Introduced queue list for audio output
- https://github.com/valbok/QtAVPlayer/pull/99 - Fixed crash with audio output
- https://github.com/valbok/QtAVPlayer/pull/91 - Removed explicit software decoding in tests
- https://github.com/valbok/QtAVPlayer/pull/95 - Respected choices from ffmpeg
- https://github.com/valbok/QtAVPlayer/pull/94 - Introduced audio/video streams
- https://github.com/valbok/QtAVPlayer/pull/89 - Fixed tests
- https://github.com/valbok/QtAVPlayer/pull/88 - Used hw device only if there is hw context available
- https://github.com/valbok/QtAVPlayer/pull/87 - Fixed  macos seeked test
- https://github.com/valbok/QtAVPlayer/pull/85 - Cleanup
- https://github.com/valbok/QtAVPlayer/pull/86 - Temporarily fixed test for macOS and seek
- https://github.com/valbok/QtAVPlayer/pull/84 - Added frames count
- https://github.com/valbok/QtAVPlayer/pull/83 - Increased max logs
- https://github.com/valbok/QtAVPlayer/pull/82 - Introduced stepForward
- https://github.com/valbok/QtAVPlayer/pull/81 - Introduced casting to QVideoFrame
- https://github.com/valbok/QtAVPlayer/pull/80 - Added check for sws ctx
- https://github.com/valbok/QtAVPlayer/pull/79 - Added test for map data
- https://github.com/valbok/QtAVPlayer/pull/78 - Fixed missing EOF when waiting for more packets
- https://github.com/valbok/QtAVPlayer/pull/77 - Introduced frame converting
- https://github.com/valbok/QtAVPlayer/pull/76 - Fixed macos tests
- https://github.com/valbok/QtAVPlayer/pull/75 - Fixed duration test
- https://github.com/valbok/QtAVPlayer/pull/74 - Introduced pending statuses
- https://github.com/valbok/QtAVPlayer/pull/72 - Fixed events handling
- https://github.com/valbok/QtAVPlayer/pull/70 - Added dv files
- https://github.com/valbok/QtAVPlayer/pull/71 - Removed position member
- https://github.com/valbok/QtAVPlayer/pull/68 - Replaced media statuses by signals
- https://github.com/valbok/QtAVPlayer/pull/67 - Fixed sending EOF
- https://github.com/valbok/QtAVPlayer/pull/66 - Fixed pause/seek on eof
- https://github.com/valbok/QtAVPlayer/pull/65 - Introduced stopping media
- https://github.com/valbok/QtAVPlayer/pull/63 - Introduced empty frame on stop
- https://github.com/valbok/QtAVPlayer/pull/58 - Cleared frame rate on terminate
- https://github.com/valbok/QtAVPlayer/pull/57 - Introduced video frame rate
- https://github.com/valbok/QtAVPlayer/pull/56 - Added check if pending pos has changed
- https://github.com/valbok/QtAVPlayer/pull/55 - Added include logging category
- https://github.com/valbok/QtAVPlayer/pull/54 - Introduced logging category
- https://github.com/valbok/QtAVPlayer/pull/53 - Introduced paused signal
- https://github.com/valbok/QtAVPlayer/pull/51 - Fixed seek
- https://github.com/valbok/QtAVPlayer/pull/50 - Fixed tests with seek frame
- https://github.com/valbok/QtAVPlayer/pull/49 - Introduced seeked signal
- https://github.com/valbok/QtAVPlayer/pull/48 - Used signals when sending frames
- https://github.com/valbok/QtAVPlayer/pull/46 - Updated media status on first frame after seeking
- https://github.com/valbok/QtAVPlayer/pull/45 - Added thread pool
- https://github.com/valbok/QtAVPlayer/pull/44 - Used one mutex for queues
- https://github.com/valbok/QtAVPlayer/pull/38 - Introduced SeekingMedia media status
- https://github.com/valbok/QtAVPlayer/pull/43 - Unlocked the mutex while waiting sync
- https://github.com/valbok/QtAVPlayer/pull/42 - Introduced wait for finished for queues
- https://github.com/valbok/QtAVPlayer/pull/40 - Fixed pending position mutex
- https://github.com/valbok/QtAVPlayer/pull/37 - Removed dispatching frames on main thread
- https://github.com/valbok/QtAVPlayer/pull/36 - Removed mute property
- https://github.com/valbok/QtAVPlayer/pull/35 - Added cmake for Qt5.15
- https://github.com/valbok/QtAVPlayer/pull/34 - Introduced QAVPlayer::videoFrame/audioFrame signals
- https://github.com/valbok/QtAVPlayer/pull/33 - Added logs for Windows
- https://github.com/valbok/QtAVPlayer/pull/32 - Fixed ts issues
- https://github.com/valbok/QtAVPlayer/pull/31 - Added windows CI
- https://github.com/valbok/QtAVPlayer/pull/30 - Fixed QList on macOS
- https://github.com/valbok/QtAVPlayer/pull/29 - Added macOS CI
- https://github.com/valbok/QtAVPlayer/pull/28 - Added env var for make
- https://github.com/valbok/QtAVPlayer/pull/27 - Added override
- https://github.com/valbok/QtAVPlayer/pull/25 - Added warning if not enough threads
- https://github.com/valbok/QtAVPlayer/pull/26 - Added waitCond for wait
- https://github.com/valbok/QtAVPlayer/pull/22 - Fixed CI
- https://github.com/valbok/QtAVPlayer/pull/21 - Fixed windows
- https://github.com/valbok/QtAVPlayer/pull/19 - Fixed build issue with the latest ffmpeg
- https://github.com/valbok/QtAVPlayer/pull/16 - Fixed white window on windows - Alexander Ivash <elderorb@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/12 - Added seek to the first video frame - Alexander Ivash <elderorb@gmail.com>
- https://github.com/valbok/QtAVPlayer/pull/10 - Fixed cmake
- https://github.com/valbok/QtAVPlayer/pull/9 - Fixed video buffer
- https://github.com/valbok/QtAVPlayer/pull/7 - Added wo surface

2020-09-07
----------

- Initial commit
