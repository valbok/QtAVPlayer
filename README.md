# Qt AVPlayer 
![example workflow](https://github.com/valbok/QtAVPlayer/actions/workflows/main.yaml/badge.svg)

Free and open-source Qt Media Player library based on FFmpeg.
- Demuxes and decodes _video_/_audio_/_subtitle_ frames.
- Muxes, encodes and saves the streams from different sources to one output file.
- [FFmpeg Bitstream Filters](https://ffmpeg.org/ffmpeg-bitstream-filters.html) and [FFmpeg Filters](https://ffmpeg.org/ffmpeg-filters.html) including `filter_complex`.
- Multiple parallel filters for one input (one input frame produces multiple outputs).
- Decoding of all available streams at the same time.
- Hardware acceleration.
- It is up to an application to decide how to process the frames.
  * But there is _experimental_ support of converting the video frames to QtMultimedia's [QVideoFrame](https://doc.qt.io/qt-5/qvideoframe.html) for copy-free rendering if possible.
  Note: Not all Qt's renders support copy-free rendering. Also QtMultimedia does not always provide public API to render the video frames. And, of course, for best performance both decoding and rendering should be accelerated.
  * Audio frames could be played by `QAVAudioOutput` which is a wrapper of QtMultimedia's [QAudioSink](https://doc-snapshots.qt.io/qt6-dev/qaudiosink.html)
- Accurate seek, it starts playing the closest frame.
- Might be used for media analytics software like [qctools](https://github.com/bavc/qctools) or [dvrescue](https://github.com/mipops/dvrescue).
- Implements and replaces a combination of FFmpeg and FFplay:

      ffmpeg -i we-miss-gst-pipeline-in-qt6mm.mkv -filter_complex "qt,nev:er,wanted;[ffmpeg];what:happened" - | ffplay -

  but using QML or Qt Widgets:

      ./qml_video :/valbok "if:you:like[cats];remove[this-sentence]"

# Features

1. QAVPlayer supports playing from an url or QIODevice or from avdevice:

       player.setSource("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4");
       player.setSource("/home/lana/The Matrix Resurrections.mov");
       // Dash player
       player.setSource("https://bitdash-a.akamaihd.net/content/MI201109210084_1/m3u8s/f08e80da-bf1d-4e3d-8899-f0f6155f6efa.m3u8")

       // Playing from qrc
       QSharedPointer<QIODevice> file(new QFile(":/alarm.wav"));
       file->open(QIODevice::ReadOnly);
       QSharedPointer<QAVIODevice> dev(new QAVIODevice(file));
       player.setSource("alarm", dev);

       // Getting frames from the camera in Linux
       player.setSource("/dev/video0");
       // Or Windows
       player.setInputFormat("dshow");
       player.setSource("video=Integrated Camera");
       // Or MacOS
       player.setInputFormat("avfoundation");
       player.setSource("default");
       // Or Android
       player.setInputFormat("android_camera");
       player.setSource("0:0");
       
       player.setInputOptions({{"user_agent", "QAVPlayer"}});
       // Save to file
       player.setOutput("output.mkv");
       // Using various protocols
       player.setSource("subfile,,start,0,end,0,,:/root/Downloads/why-qtmm-must-die.mkv");

3. Easy getting video and audio frames:

       QObject::connect(&player, &QAVPlayer::videoFrame, [&](const QAVVideoFrame &frame) {
           // QAVVideoFrame is comppatible with QVideoFrame
           QVideoFrame videoFrame = frame;
           
           // QAVVideoFrame can be converted to various pixel formats
           auto convertedFrame = frame.convert(AV_PIX_FMT_YUV420P);
           
           // Easy getting data from video frame
           auto mapped = videoFrame.map(); // downloads data if it is in GPU
           qDebug() << mapped.format << mapped.size;
           
           // The frame might contain OpenGL or MTL textures, for copy-free rendering
           qDebug() << frame.handleType() << frame.handle();
       }, Qt::DirectConnection);

       // Audio frames could be played using QAVAudioOutput
       QAVAudioOutput audioOutput;
       QObject::connect(&player, &QAVPlayer::audioFrame, [&](const QAVAudioFrame &frame) { 
            // Access to the data
            qDebug() << autioFrame.format() << autioFrame.data().size();
            audioOutput.play(frame);
       }, Qt::DirectConnection);
       
       QObject::connect(&p, &QAVPlayer::subtitleFrame, &p, [](const QAVSubtitleFrame &frame) {
            for (unsigned i = 0; i < frame.subtitle()->num_rects; ++i) {
                if (frame.subtitle()->rects[i]->type == SUBTITLE_TEXT)
                    qDebug() << "text:" << frame.subtitle()->rects[i]->text;
                else
                    qDebug() << "ass:" << frame.subtitle()->rects[i]->ass;
           }
       }, Qt::DirectConnection);
       

4. Each action is confirmed by a signal:

       // All signals are added to a queue and guaranteed to be emitted in proper order.
       QObject::connect(&player, &QAVPlayer::played, [&](qint64 pos) { qDebug() << "Playing started from pos" << pos;  });
       QObject::connect(&player, &QAVPlayer::paused, [&](qint64 pos) { qDebug() << "Paused at pos" << pos; });
       QObject::connect(&player, &QAVPlayer::stopped, [&](qint64 pos) { qDebug() << "Stopped at pos" << pos; });
       QObject::connect(&player, &QAVPlayer::seeked, [&](qint64 pos) { qDebug() << "Seeked to pos" << pos; });
       QObject::connect(&player, &QAVPlayer::stepped, [&](qint64 pos) { qDebug() << "Made a step to pos" << pos; });
       QObject::connect(&player, &QAVPlayer::mediaStatusChanged, [&](QAVPlayer::MediaStatus status) { 
           switch (status) {
               case QAVplayer::EndOfMedia:
                   qDebug() << "Finished to play, no frames in queue"; 
                   break;
               case QAVplayer::NoMedia:
                   qDebug() << "Demuxer threads are finished";
                   break;
               default:
                   break;
              }
        });
    
5. Accurate seek:

       QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });
       QObject::connect(&player, &QAVPlayer::videoFrame, [&](const QAVVideoFrame &frame) { seekFrame = frame; });
       player.seek(5000)
       QTRY_COMPARE(seekPosition, 5000);
       QTRY_COMPARE(seekFrame.pts(), 5.0);
       
   If there is a frame with needed pts, it will be returned as first frame.

6. FFmpeg filters:

       player.setFilter("crop=iw/2:ih:0:0,split[left][tmp];[tmp]hflip[right];[left][right] hstack");
       // Render bundled subtitles
       player.setFilter("subtitles=file.mkv");
       // Render subtitles from srt file
       player.setFilter("subtitles=file.srt");
       // Multiple filters
       player.setFilters({
            "drawtext=text=%{pts\\\\:hms}:x=(w-text_w)/2:y=(h-text_h)*(4/5):box=1:boxcolor=gray@0.5:fontsize=36[drawtext]",
            "negate[negate]",
            "[0:v]split=3[in1][in2][in3];[in1]boxblur[out1];[in2]negate[out2];[in3]drawtext=text=%{pts\\\\:hms}:x=(w-text_w)/2:y=(h-text_h)*(4/5):box=1:boxcolor=gray@0.5:fontsize=36[out3]"
       }); // Return frames from 3 filters with 5 outputs

7. Step by step:

       // Pausing will always emit one frame
       QObject::connect(&player, &QAVPlayer::videoFrame, [&](const QAVVideoFrame &frame) { receivedFrame = frame; });
       if (player.state() != QAVPlayer::PausedState) { // No frames if it is already paused
           player.pause();
           QTRY_VERIFY(receivedFrame);
       }

       // Always makes a step forward and emits only one frame
       player.stepForward();
       // the same here but backward
       player.stepBackward();

8. Multiple streams:

       qDebug() << "Audio streams" << player.availableAudioStreams().size();
       qDebug() << "Current audio stream" << player.currentAudioStreams().first().index() << player.currentAudioStreams().first().metadata();
       player.setAudioStreams(player.availableAudioStreams()); // Return all frames for all available audio streams
       // Reports progress of playing per stream, like current pts, fps, frame rate, num of frames etc
       for (const auto &s : p.availableVideoStreams())
           qDebug() << s << p.progress(s);

9. Muxing the streams:

        // Muxes all streams to the file without reencoding the packets.
        // `QAVMuxerPackets` is used internally.
        player.setOutput("output.mkv");

        // Multiple players could be used to mux to one files
        QAVPlayer p1;
        QAVPlayer p2;
        QAVMuxerFrames m;

        // Wait until QAVPlayer::LoadedMedia
        QTRY_VERIFY(p1.mediaStatus() == QAVPlayer::LoadedMedia);
        QTRY_VERIFY(p2.mediaStatus() == QAVPlayer::LoadedMedia);
        // Use all available streams from both players
        auto streams = p1.availableStreams() + p2.availableStreams();
        // Mux the streams to one file
        m.load(streams, "output.mkv");

        QObject::connect(&p1, &QAVPlayer::videoFrame, &p1, [&](const QAVVideoFrame &f) { m.enqueue(f); }, Qt::DirectConnection);
        QObject::connect(&p1, &QAVPlayer::audioFrame, &p1, [&](const QAVAudioFrame &f) { m.enqueue(f); }, Qt::DirectConnection);
        QObject::connect(&p2, &QAVPlayer::videoFrame, &p2, [&](const QAVVideoFrame &f) { m.enqueue(f); }, Qt::DirectConnection);
        QObject::connect(&p2, &QAVPlayer::audioFrame, &p2, [&](const QAVAudioFrame &f) { m.enqueue(f); }, Qt::DirectConnection);

        p1.play();
        p2.play();

10. HW accelerations:

   QT_AVPLAYER_NO_HWDEVICE can be used to force using software decoding. The video codec is negotiated automatically.
   
  * `VA-API` and `VDPAU` for **Linux**: the frames are returned with _OpenGL_ textures.
  * `Video Toolbox` for **macOS** and **iOS**: the frames are returned with _Metal_ Textures.
  * `D3D11` for **Windows**: the frames are returned with _D3D11Texture2D_ textures. 
  * `MediaCodec` for **Android**: the frames are returned with _OpenGL_ textures.

> [!NOTE]
> Not all ffmpeg decoders or filters support HW acceleration. In this case software decoders are used.

11. QtMultimedia could be used to render video frames to QML or Widgets. See [examples](examples)
12. Widget `QAVWidget_OpenGL` could be used to render to OpenGL. See [examples/widget_video_opengl](examples/widget_video_opengl)
13. Qt **5.12 - 6.x** is supported

# How to build
## CMake usage

_QtAvPlayer_ can be used as an embedded library in a subdirectory of your project (like a git submodule for example):
1. In the **root** CMakeLists, add instructions:
```cmake
add_subdirectory(qtavplayer) # Or if library is put in a folder "dependencies": add_subdirectory(dependencies/qtavplayer)
```

2. In the **application/library** CMakeLists, add instructions:
```cmake
target_link_libraries(${PROJECT_NAME} PRIVATE qtavplayer)
```

## CMake options

This library provide some **CMake** build options:
- Features:
  - `QT_AVPLAYER_MULTIMEDIA` (default: `OFF`): enables support of `QtMultimedia` which will used Qt packages `Multimedia`.
  - `QTAVPLAYER_WIDGET_OPENGL` (default: `OFF`): builds the widget based on OpenGL which will used Qt packages `OpenGLWidgets` on Qt6 and `QWidgets` on Qt5.
- Hardware acceleration:
  - `QTAVPLAYER_HW_SUPPORT_WINDOWS` (default: `ON`): enable HW acceleration for **Windows** platforms.
  - `QTAVPLAYER_HW_SUPPORT_MACOS` (default: `ON`): enable HW acceleration for **MacOS** platforms.
  - `QTAVPLAYER_HW_SUPPORT_LINUX_VA_DRM` (default: `OFF`): enable HW acceleration for **Linux** platforms using `libva-drm`. Under Linux Ubuntu, needed packages are `libva-dev`, `libegl1-mesa-dev` and `libgles2-mesa-dev`.
  - `QTAVPLAYER_HW_SUPPORT_LINUX_VA_X11` (default: `OFF`): enable HW acceleration for **Linux** platforms using `libva-x11`. Under Linux Ubuntu, needed packages are `libva-dev`, `libva-x11-2` and `libx11-dev`.
  - `QTAVPLAYER_HW_SUPPORT_LINUX_VDPAU` (default: `OFF`): enable HW acceleration for **Linux** platforms using `libvdpau`. Under Linux Ubuntu, needed packages are `libvdpau-dev`
  - `QTAVPLAYER_HW_SUPPORT_ANDROID` (default: `ON`): enable HW acceleration for **Android** platforms.
  - `QTAVPLAYER_HW_SUPPORT_IOS` (default: `ON`): enable HW acceleration for **iOS** platforms.

## Android:

Some exports could be also used: vars that point to libraries in armeabi-v7a, arm64-v8a, x86 and x86_64 target archs.

    $ export AVPLAYER_ANDROID_LIB_ARMEABI_V7A=/opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib
    $ export AVPLAYER_ANDROID_LIB_ARMEABI_V8A=/opt/mobile-ffmpeg/prebuilt/android-arm64/ffmpeg/lib
    $ export AVPLAYER_ANDROID_LIB_X86=/opt/mobile-ffmpeg/prebuilt/android-x86/ffmpeg/lib
    $ export AVPLAYER_ANDROID_LIB_X86_64=/opt/mobile-ffmpeg/prebuilt/android-x86_64/ffmpeg/lib
    $ export CPLUS_INCLUDE_PATH=/opt/mobile-ffmpeg/prebuilt/android-arm64/ffmpeg/include:$CPLUS_INCLUDE_PATH
    $ qmake DEFINES+="QT_AVPLAYER_MULTIMEDIA"

Don't forget to set extra libs in _pro_ file for your app:

    ANDROID_EXTRA_LIBS += /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavdevice.so /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavformat.so /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavutil.so /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavcodec.so /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavfilter.so /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libswscale.so /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libswresample.so


