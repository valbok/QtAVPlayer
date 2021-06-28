# Qt AVPlayer

Free and open-source Qt Media Player based on FFmpeg.

Currently implemented software decoding everywhere and following hardware accelerations:
* VA-API decoding for Linux: DRM with EGL or X11 with GLX.
* Video Toolbox for macOS and iOS.
* D3D11 for Windows. 
* MediaCodec for Android. 

QT_AVPLAYER_NO_HWDEVICE can be used to force using software decoding.
The video codec is negotiated automatically.

QtAVPlayer decodes the audio and video streams and returns in `QAVPlayer::ao()` and `QAVPlayer::vo()`.
Also the video frames can contain OpenGL, Direct3d, Vulkan or Metal textrures.

At least Qt 5.12 is supported.

# Example:

    QAVPlayer p;
    p.ao([&](const QAVAudioFrame &frame) { qDebug() << "audioFrame" << frame; });
    p.vo([&](const QAVVideoFrame &frame) { qDebug() << "videoFrame" << frame; });
    p.setSource(QUrl("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4"));
    p.setSpeed(1);
    p.play();

To play audio `QAVAudioOutput` can be used:

    QAVAudioOutput audioOutput;
    audioOutput.play(audioFrame);

To render video QAbstractVideoSurface can be used:

    class PlanarVideoBuffer : public QAbstractPlanarVideoBuffer
    {
    public:
        PlanarVideoBuffer(const QAVVideoFrame &frame, HandleType type = NoHandle)
            : QAbstractPlanarVideoBuffer(type), m_frame(frame)
        {
        }

        MapMode mapMode() const override { return m_mode; }
        int map(MapMode mode, int *numBytes, int bytesPerLine[4], uchar *data[4]) override
        {        
            if (m_mode != NotMapped || mode == NotMapped)
                return 0;

            auto mapData = m_frame.map();
            m_mode = mode;
            if (numBytes)
                *numBytes = mapData.size;

            int i = 0;
            for (; i < 4; ++i) {
                bytesPerLine[i] = mapData.bytesPerLine[i];
                data[i] = mapData.data[i];
            }

            return i;        
        }
        void unmap() override { m_mode = NotMapped; }

    private:
        QAVVideoFrame m_frame;
        MapMode m_mode = NotMapped;
    };

    p.vo([&videoSurface](const QAVVideoFrame &frame) {
        QVideoFrame::PixelFormat pf = QVideoFrame::Format_Invalid;
        switch (frame.frame()->format)
        {
        case AV_PIX_FMT_YUV420P:
            pf = QVideoFrame::Format_YUV420P;
            break;
        case AV_PIX_FMT_NV12:
            pf = QVideoFrame::Format_NV12;
            break;
        default:
            break;
        }

        QVideoFrame videoFrame(new PlanarVideoBuffer(frame), frame.size(), pf);
        if (!videoSurface->isActive())
            videoSurface->start({videoFrame.size(), videoFrame.pixelFormat(), videoFrame.handleType()});
        if (videoSurface->isActive())
            videoSurface->present(videoFrame);
    });

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


