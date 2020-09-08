# Qt AVPlayer

Free Qt Media Player based on FFmpeg.
Requires QtMultimedia 6.0 with Qt Rendering Hardware Interface support.

Currently implemented:
* Software decoding everywhere.
* VAAPI decoding for Linux:
  DRM with EGL or X11 with GLX.
* Video Toolbox for macOS and iOS.
* D3D11 for Windows. 

QT_AVPLAYER_NO_HWDEVICE can be used to force using software decoding.
The video codec is negotiated automatically.

QtAVPlayer uses QAbstractVideoSurface interface to push QVideoFrame objects to it.
The QVideoFrame can be used to extract the video frame data or/and to render it.
Also such video frames can contain OpenGL, Direct3d, Vulkan or Metal textrures,
which helps to render the video and avoid downloading data from GPU to CPU memory and uploading it back.

QAudioOutput is used to play audio.

# Example:
    QAVPlayer p;
    p.setVideoSurface(VideoOutput->videoSurface());
    p.setSource(QUrl("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4"));
    p.play();

