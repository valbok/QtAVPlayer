# Qt AVPlayer

Free Qt Media Player based on FFmpeg.
Requires QtMultimedia 6.0 with Qt Rendering Hardware Interface support.

Currently implemented software decoding everywhere and following hardware accelerations:
* VA-API decoding for Linux: DRM with EGL or X11 with GLX.
* Video Toolbox for macOS and iOS.
* D3D11 for Windows. 
* MediaCodec for Android. 

QT_AVPLAYER_NO_HWDEVICE can be used to force using software decoding.
The video codec is negotiated automatically.

QtAVPlayer uses QAbstractVideoSurface interface to push QVideoFrame objects to it.
The QVideoFrame can be used to extract the video frame data or/and to render it.
Also such video frames can contain OpenGL, Direct3d, Vulkan or Metal textrures,
which helps to render the video and avoid downloading data from GPU to CPU memory and uploading it back.

QAudioOutput is used to play audio.

# Example:

The video surface must provide a list of supported pixel formats from QAbstractVideoSurface::supportedPixelFormats().
One of them will be used when QVideoFrame is sent to QAbstractVideoSurface::present().
QVideoWidget::videoSurface(), QGraphicsVideoItem::videoSurface() and QML VideoOutput::videoSurface() provide the video surfaces.

    struct Surface : QAbstractVideoSurface {
        QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType) const override {
            return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_YUV420P;
        }

        bool present(const QVideoFrame &f) override {
            // Handle frame here...
            return true;
        }
    } s;
    
    QAVPlayer p;
    p.setVideoSurface(&s);
    p.setSource(QUrl("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4"));
    p.setVolume(50);
    p.setSpeed(1);
    p.play();

# Build

Linux:

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

    SET FFMPEG=C:\ffmpeg
    SET PATH=%FFMPEG%\lib;%PATH%
    SET INCLUDE=%FFMPEG%\include;%INCLUDE%
    SET LIB=%FFMPEG%\lib;%LIB%
    cd QtAVPlayer && qmake && nmake

# btw:

Since QtAVPlayer uses QRHI to render the video frames (also means available only in QML), there are some limitations.

Windows:

Even if D3D11 is used, array texture ID3D11Texture2D in NV12 format is not yet implemented in QRHI, thus decided to download memory from GPU to CPU.

Android:

MediaCodec provides possibility to use android.view.Surface which currently is not implemented, thus copying from GPU to CPU is performed.


