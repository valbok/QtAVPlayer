# Qt AVPlayer

Free Qt Media Player based on FFmpeg.
Requires QtMultimedia 6.0 with Qt Rendering Hardware Interface support.

Currently implemented:
* Software decoding everywhere.
* VAAPI decoding for Linux: DRM with EGL or X11 with GLX.
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

