# Qt AVPlayer

FFmpeg based media player library.
Requires QtMultimedia 6.0 with Qt Rendering Hardware Interface support.

Currently implemented:
* Software decoding everywhere.
* VAAPI hardware decoding for Linux:
  DRM with EGL or X11 with GLX.
* Video Toolbox for macOS and iOS.
* D3D11 for Windows. 

# Example:
    QAVPlayer p;
    p.setVideoSurface(VideoOutput->videoSurface());
    p.setSource(QUrl("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4"));
    p.play();

