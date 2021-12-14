# Qt AVPlayer

Free and open-source Qt Media Player library based on FFmpeg with a predictable behavior and control.

# Features

1. QAVPlayer supports playing from an url or QIODevice or from avdevice:

       player.setSource("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4");
       player.setSource("/home/lana/The Matrix Resurrections.mov");

       // Playing from qrc
       QFile file(QLatin1String(":/alarm.wav"));
       file.open(QIODevice::ReadOnly));
       player.setSource("alarm", &file);

       // Getting frames from the camera in Linux
       player.setSource("-f v4l2 -i /dev/video0");
       // Or Windows
       player.setSource("-f dshow -i video=Integrated Camera");
       // Or MacOS
       player.setSource("-f avfoundation -i default");
       // Or Android
       player.setSource("-f android_camera -i 0:0");
    
       // Using various protocols
       player.setSource("subfile,,start,0,end,0,,:/root/Downloads/qtmm-must-die.mkv");

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

6. FFmpeg filters:

       player.setFilter("crop=iw/2:ih:0:0,split[left][tmp];[tmp]hflip[right];[left][right] hstack");

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

       qDebug() << "Audio streams" << player.audioStreamsCount();
       qDebug() << "Current audio stream" << player.audioStream();
       player.setAudioStream(player.audioStreamsCount() - 1);

9. HW accelerations:

   * VA-API decoding for Linux: DRM with EGL or X11 with GLX.
   * Video Toolbox for macOS and iOS.
   * D3D11 for Windows. 
   * MediaCodec for Android. 

   QT_AVPLAYER_NO_HWDEVICE can be used to force using software decoding. The video codec is negotiated automatically.

10. QtMultimedia5 could be used to render video frames to QML or Widgets. See [examples](examples).

At least Qt 5.12 is supported.

# Build


Linux:

cmake:

    cd build; /opt/cmake-3.19.2/bin/cmake .. -DCMAKE_PREFIX_PATH=/opt/dev/qtbase/lib/cmake/Qt5 -DCMAKE_INSTALL_PREFIX=/opt/QtAVPlayer/install -DCMAKE_LIBRARY_PATH="/opt/dev/qtbase/lib;/opt/ffmpeg/install/lib" -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=/opt/ffmpeg/install/include

qmake:

Install ffmpeg visible with pkg-config.

    $ cd QtAVPlayer && qmake && make -j8
    Info: creating stash file QtAVPlayer/.qmake.stash
    Info: creating cache file QtAVPlayer/.qmake.cache

    Running configuration tests...
    Checking for FFmpeg... yes
    Checking for va_drm... yes
    Checking for va_x11... yes
    Done running configuration tests.

    Configure summary:

    Qt AVPlayer:
    FFmpeg ................................. yes
    va_x11 ................................. yes
    va_drm ................................. yes

    Qt is now configured for building. Just run 'make'.
    Once everything is built, Qt is installed.
    You should NOT run 'make install'.
    Note that this build cannot be deployed to other machines or devices.

    Prior to reconfiguration, make sure you remove any leftovers from
    the previous build.

macOS and iOS:

    $ export FFMPEG_ROOT=/usr/local/Cellar/ffmpeg/4.3_1
    $ export LIBRARY_PATH=$FFMPEG_ROOT/lib:$LIBRARY_PATH
    $ export CPLUS_INCLUDE_PATH=$FFMPEG_ROOT/include:$CPLUS_INCLUDE_PATH
    $ cd QtAVPlayer && qmake && make -j8    

Android:

Set vars that point to libraries in armeabi-v7a, arm64-v8a, x86 and x86_64 target archs.

    $ export AVPLAYER_ANDROID_LIB_ARMEABI_V7A=/opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib
    $ export AVPLAYER_ANDROID_LIB_ARMEABI_V8A=/opt/mobile-ffmpeg/prebuilt/android-arm64/ffmpeg/lib
    $ export AVPLAYER_ANDROID_LIB_X86=/opt/mobile-ffmpeg/prebuilt/android-x86/ffmpeg/lib
    $ export AVPLAYER_ANDROID_LIB_X86_64=/opt/mobile-ffmpeg/prebuilt/android-x86_64/ffmpeg/lib
    $ export CPLUS_INCLUDE_PATH=/opt/mobile-ffmpeg/prebuilt/android-arm64/ffmpeg/include:$CPLUS_INCLUDE_PATH
    $ cd QtAVPlayer && qmake && make -j8

Windows and MSVC:

cmake:

    cd build
    cmake .. -DCMAKE_PREFIX_PATH=c:\dev\qtbase\lib\cmake\Qt5 -DCMAKE_LIBRARY_PATH="c:\dev\qtbase\lib;c:\ffmpeg\lib" -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=c:\ffmpeg\include -DCMAKE_INSTALL_PREFIX=c:\QtAVPlayer\install
    msbuild QtAVPlayer.sln
    cmake --build . --target install


qmake:

    SET FFMPEG=C:\ffmpeg
    SET PATH=%FFMPEG%\lib;%PATH%
    SET INCLUDE=%FFMPEG%\include;%INCLUDE%
    SET LIB=%FFMPEG%\lib;%LIB%
    cd QtAVPlayer && qmake && nmake


