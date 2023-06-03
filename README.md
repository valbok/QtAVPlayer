# Qt AVPlayer 
![example workflow](https://github.com/valbok/QtAVPlayer/actions/workflows/main.yaml/badge.svg)

Free and open-source Qt Media Player library based on FFmpeg.
- Designed to decode _video_/_audio_/_subtitle_ frames.
- Supports a synchronization of frames like in FFplay and a decoding like in FFmpeg.
- Supports [FFmpeg Bitstream Filters](https://ffmpeg.org/ffmpeg-bitstream-filters.html) and [FFmpeg Filters](https://ffmpeg.org/ffmpeg-filters.html) including _simplified_ `filter_complex`.
- Supports multiple parallel filters for one input (one input frame and multiple output ones).
- Based on Qt platform the video frames are sent using specific hardware context:
  * `VA-API` for Linux: DRM with EGL or X11 with GLX.
  * `VDPAU` for Linux.
  * `Video Toolbox` for macOS and iOS.
  * `D3D11` for Windows. 
  * `MediaCodec` for Android.
  
  Note: Not all ffmpeg decoders or filters support HW acceleration. In this case software decoders are used.
- It is up to an application to decide how to process the frames.
  * But there is _experimental_ support of converting the video frames to QtMultimedia's [QVideoFrame](https://doc.qt.io/qt-5/qvideoframe.html) for copy-free rendering if possible.
  Note: Not all Qt's renders support copy-free rendering. Also QtMultimedia does not always provide public API to render the video frames. And, of course, for best performance both decoding and rendering should be accelerated.
  * Audio frames could be played by `QAVAudioOutput` which is a wrapper of QtMultimedia's [QAudioSink](https://doc-snapshots.qt.io/qt6-dev/qaudiosink.html)
- Supports accurate seek, it starts playing the closest frame. No weird jumps on pts anymore.
- Designed to be as simple and understandable as possible, to share knowledge about creating efficient FFmpeg applications.
- Might be used for media analytics software like [qctools](https://github.com/bavc/qctools) or [dvrescue](https://github.com/mipops/dvrescue).
- Implemented as a Qt module using QMake and also supports CMake.
- Strange to say this in 21st century, but each feature is covered by integration tests.
- Implements and replaces a combination of FFmpeg and FFplay:

      ffmpeg -i we-miss-gst-pipeline-in-qt6mm.mkv -filter_complex "qt,nev:er,wanted;[ffmpeg];what:happened" - | ffplay -

  but using QML or Qt Widgets:

      ./qml_video :/valbok "if:you:like[cats];remove[this]"

# Features

1. QAVPlayer supports playing from an url or QIODevice or from avdevice:

       player.setSource("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4");
       player.setSource("/home/lana/The Matrix Resurrections.mov");

       // Playing from qrc
       QFile file(QLatin1String(":/alarm.wav"));
       file.open(QIODevice::ReadOnly));
       player.setSource("alarm", &file);

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
    
       // Using various protocols
       player.setSource("subfile,,start,0,end,0,,:/root/Downloads/why-qtmm-must-die.mkv");

2. Easy getting video and audio frames:

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
       });

       // Audio frames could be played using QAVAudioOutput
       QAVAudioOutput audioOutput;
       QObject::connect(&player, &QAVPlayer::audioFrame, [&](const QAVAudioFrame &frame) { 
            // Access to the data
            qDebug() << autioFrame.format() << autioFrame.data().size();
            audioOutput.play(frame);
       });
       
       QObject::connect(&p, &QAVPlayer::subtitleFrame, &p, [](const QAVSubtitleFrame &frame) {
            for (unsigned i = 0; i < frame.subtitle()->num_rects; ++i) {
                if (frame.subtitle()->rects[i]->type == SUBTITLE_TEXT)
                    qDebug() << "text:" << frame.subtitle()->rects[i]->text;
                else
                    qDebug() << "ass:" << frame.subtitle()->rects[i]->ass;
           }
       });
       

3. Each action is confirmed by a signal:

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

9. HW accelerations:

   QT_AVPLAYER_NO_HWDEVICE can be used to force using software decoding. The video codec is negotiated automatically.
   
  * `VA-API` and `VDPAU` for Linux: the frames are returned with OpenGL textures.
  * `Video Toolbox` for macOS and iOS: the frames are returned with Metal Textures.
  * `D3D11` for Windows: the frames should be mapped to CPU memory. D3D11 Textures are not yet supported. 
  * `MediaCodec` for Android: the frames are returned with OpenGL textures.

10. QtMultimedia could be used to render video frames to QML or Widgets. See [examples](examples)
11. Qt 5.12 - **6**.x is supported

# How to build

**qmake is deprecated since Qt 6.5**

## Linux:

- cmake:

      $ cd build; /opt/cmake-3.19.2/bin/cmake .. -DCMAKE_PREFIX_PATH=/opt/dev/qtbase/lib/cmake/Qt5 -DCMAKE_INSTALL_PREFIX=/opt/QtAVPlayer/install -DCMAKE_LIBRARY_PATH="/opt/dev/qtbase/lib;/opt/ffmpeg/install/lib" -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=/opt/ffmpeg/install/include

- qmake:

    Install ffmpeg visible with pkg-config.

      $ cd QtAVPlayer && qmake && make -j8

## macOS and iOS:

    $ export FFMPEG_ROOT=/usr/local/Cellar/ffmpeg/4.3_1
    $ export LIBRARY_PATH=$FFMPEG_ROOT/lib:$LIBRARY_PATH
    $ export CPLUS_INCLUDE_PATH=$FFMPEG_ROOT/include:$CPLUS_INCLUDE_PATH
    $ cd QtAVPlayer && qmake && make -j8    

## Android:

Set vars that point to libraries in armeabi-v7a, arm64-v8a, x86 and x86_64 target archs.

    $ export AVPLAYER_ANDROID_LIB_ARMEABI_V7A=/opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib
    $ export AVPLAYER_ANDROID_LIB_ARMEABI_V8A=/opt/mobile-ffmpeg/prebuilt/android-arm64/ffmpeg/lib
    $ export AVPLAYER_ANDROID_LIB_X86=/opt/mobile-ffmpeg/prebuilt/android-x86/ffmpeg/lib
    $ export AVPLAYER_ANDROID_LIB_X86_64=/opt/mobile-ffmpeg/prebuilt/android-x86_64/ffmpeg/lib
    $ export CPLUS_INCLUDE_PATH=/opt/mobile-ffmpeg/prebuilt/android-arm64/ffmpeg/include:$CPLUS_INCLUDE_PATH
    $ cd QtAVPlayer && qmake && make -j8

Don't forget to set extra libs in pro file for your app:

    ANDROID_EXTRA_LIBS += /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavdevice.so /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavformat.so /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavutil.so /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavcodec.so /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavfilter.so /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libswscale.so /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libswresample.so

- cmake

      cmake .. -DCMAKE_PREFIX_PATH=/opt/Qt/6.4.3/android_armv7/lib/cmake/Qt6 -DCMAKE_INSTALL_PREFIX=/opt/QtAVPlayer/install -DCMAKE_LIBRARY_PATH="/opt/6.4.3/android_armv7/lib;/opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/" -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=/opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/include -DBUILD_EXAMPLES=ON -DQT_QMAKE_TARGET_MKSPEC=android-clang -DANDROID_SDK_ROOT=/home/val/Android/Sdk -DCMAKE_TOOLCHAIN_FILE=/opt/android-ndk-r25c/build/cmake/android.toolchain.cmake -DANDROID_ABI=armeabi-v7a -DQt6_DIR=/opt/Qt/6.4.3/android_armv7/lib/cmake/Qt6 -DQT_DEBUG_FIND_PACKAGE=ON -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ON -DCMAKE_FIND_DEBUG_MODE=OFF -DANDROID_STL="c++_shared"


## Windows and MSVC:

- cmake:

      cd build
      cmake .. -DCMAKE_PREFIX_PATH=c:\dev\qtbase\lib\cmake\Qt5 -DCMAKE_LIBRARY_PATH="c:\dev\qtbase\lib;c:\ffmpeg\lib" -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=c:\ffmpeg\include -DCMAKE_INSTALL_PREFIX=c:\QtAVPlayer\install
      msbuild QtAVPlayer.sln
      cmake --build . --target install


- qmake:

      SET FFMPEG=C:\ffmpeg
      SET PATH=%FFMPEG%\lib;%PATH%
      SET INCLUDE=%FFMPEG%\include;%INCLUDE%
      SET LIB=%FFMPEG%\lib;%LIB%
      cd QtAVPlayer && qmake && nmake


