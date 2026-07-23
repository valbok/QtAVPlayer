# Qt AVPlayer

[![example workflow](https://github.com/valbok/QtAVPlayer/actions/workflows/main.yaml/badge.svg)](https://github.com/valbok/QtAVPlayer/actions/workflows/main.yaml/badge.svg)

**A free, open-source media player library for Qt, powered by FFmpeg.**

QtAVPlayer lets you decode, play, filter, and mux video/audio/subtitle streams in your Qt application — with hardware acceleration, accurate seeking, and full control over how frames are rendered.

> Works with Qt 5.6 through Qt 6.x, on **Linux, Windows, macOS, iOS, and Android**.

---

## Table of Contents

- [Why QtAVPlayer?](#why-qtavplayer)
- [Quick Start](#quick-start)
- [Usage Examples](#usage-examples)
  * [Opening a source](#1-opening-a-source)
  * [Getting video, audio & subtitle frames](#2-getting-video-audio--subtitle-frames)
  * [Hardware accelerated decoding](#3-hardware-accelerated-decoding)
  * [Rendering](#4-rendering)
    + [Video](#41-video)
    + [Audio](#42-audio)
    + [Subtitles](#43-subtitles)
  * [FFmpeg filters](#5-ffmpeg-filters)
  * [Multiple streams](#6-multiple-streams)
  * [Muxing streams](#7-muxing-streams)
  * [Accurate seeking](#8-accurate-seeking)
  * [Stepping frame by frame](#9-stepping-frame-by-frame)
  * [Listening to player signals](#10-listening-to-player-signals)
- [Installation & Build Options](#installation--build-options)
  * [Build Flags](#build-flags)
  * [QMake](#qmake)
  * [CMake](#cmake)
  * [Building as a Shared Library](#building-as-a-shared-library-libqtavplayer)
  * [Android Setup](#android-setup)
- [License](#license)
---

## Why QtAVPlayer?

- Demuxes and decodes **video / audio / subtitle** frames.
- Supports [FFmpeg Bitstream Filters](https://ffmpeg.org/ffmpeg-bitstream-filters.html) and [FFmpeg Filters](https://ffmpeg.org/ffmpeg-filters.html), including `filter_complex`. It runs multiple parallel filters on one input — one input frame can produce several output frames.
- Hardware-accelerated decoding or encoding. Also for filters.
- Hardware-accelerated rendering via `QAVWidget_OpenGL`, with support for Windows as well.
- You decide how frames are processed:
  - *Experimental* support for converting frames to `QVideoFrame` for copy-free rendering (note: not all Qt renderers support this, and best performance still depends on both decode and render being hardware-accelerated).
  - Audio playback via `QAVAudioOutput`, a thin wrapper around Qt's `QAudioSink`.
- Muxes and encodes streams from multiple sources into a single output file — or re-muxes the current streams into an output file without re-encoding them.
- Decodes all available streams simultaneously — for example, frames from multiple audio streams at once.
- Accurate seeking — jumps straight to the closest matching frame.
- Designed to be bundled directly into your app (via CMake or QMake), or built as a standalone shared library.
- Great fit for media analytics tools like [qctools](https://github.com/bavc/qctools) or [dvrescue](https://github.com/mipops/dvrescue).

In short, QtAVPlayer replaces a typical `FFmpeg` + `FFplay` pipeline — but integrated natively into Qt (QML or Widgets):

```bash
$ ./examples/qml_video :/valbok "if:you:like[cats];remove[this-sentence]"
```

---

## Quick Start

The simplest possible playback example:

```cpp
QAVPlayer player;
player.setSource(rtsp);

QObject::connect(&player, &QAVPlayer::videoFrame, [&](const QAVVideoFrame &frame) {
    QVideoFrame videoFrame = frame; // compatible with QVideoFrame
    videoSink->setVideoFrame(videoFrame);
}, Qt::DirectConnection);

player.play();
```

Check out the [examples folder](https://github.com/valbok/QtAVPlayer/blob/master/examples) for full QML and Widgets sample apps.

---

## Usage Examples

### 1. Opening a source

QAVPlayer can open a URL, a local file, a `QIODevice`, or a device/camera:

```cpp
player.setSource("~/Videos/Generative-Pre-trained-Transformers-and-WW3.mkv");

// Adaptive streaming (DASH/HLS)
player.setSource("https://bitdash-a.akamaihd.net/content/MI201109210084_1/m3u8s/f08e80da-bf1d-4e3d-8899-f0f6155f6efa.m3u8");

// Playing from a Qt resource (qrc)
QSharedPointer<QIODevice> file(new QFile(":/alarm.wav"));
file->open(QIODevice::ReadOnly);
QSharedPointer<QAVIODevice> dev(new QAVIODevice(file));
player.setSource("alarm", dev);

// Camera input
player.setSource("/dev/video0");                 // Linux
player.setInputFormat("dshow");                  // Windows
player.setSource("video=Integrated Camera");
player.setInputFormat("avfoundation");            // macOS
player.setSource("default");
player.setInputFormat("android_camera");          // Android
player.setSource("0:0");

// These options will go to AVFormatContext
player.setInputOptions({{"user_agent", "QAVPlayer"}});
// Options for AVCodecContext
player.setVideoCodecOptions({{"fflags", "nobuffer"}, {"flags", "low_delay"}});

// Using FFmpeg protocols
player.setSource("subfile,,start,0,end,0,,:/root/Downloads/why-qtmm-must-die.mkv");
```

### 2. Getting video, audio & subtitle frames

```cpp
QObject::connect(&player, &QAVPlayer::videoFrame, [&](const QAVVideoFrame &frame) {
    QVideoFrame videoFrame = frame; // compatible with QVideoFrame

    // Convert to a different pixel format if needed
    auto convertedFrame = frame.convert(AV_PIX_FMT_YUV420P);

    // Map the frame to access raw data (downloads from GPU if needed)
    auto mapped = videoFrame.map();
    qDebug() << mapped.format << mapped.size;

    // Frames may carry OpenGL or Metal textures for copy-free rendering
    qDebug() << frame.handleType() << frame.handle();
}, Qt::DirectConnection);

// Audio
QObject::connect(&player, &QAVPlayer::audioFrame, [&](const QAVAudioFrame &frame) {
    qDebug() << frame.format() << frame.data().size();
}, Qt::DirectConnection);

// Subtitles
QObject::connect(&player, &QAVPlayer::subtitleFrame, &player, [](const QAVSubtitleFrame &frame) {
    for (unsigned i = 0; i < frame.subtitle()->num_rects; ++i) {
        if (frame.subtitle()->rects[i]->type == SUBTITLE_TEXT)
            qDebug() << "text:" << frame.subtitle()->rects[i]->text;
        else
            qDebug() << "ass:" << frame.subtitle()->rects[i]->ass;
    }
}, Qt::DirectConnection);
```

### 3. Hardware accelerated decoding

Hardware decoding is automatically negotiated based on the platform:

| Platform | Backend | Frame type |
|----------|---------|------------|
| Linux | VA-API / VDPAU | OpenGL textures |
| macOS / iOS | Video Toolbox | Metal textures |
| Windows | D3D11 | D3D11Texture2D |
| Android | MediaCodec | OpenGL textures |

Most platforms expose only a single device context, but enabling `CUDA` support adds more options to choose from. In that case, the platform's native device context is prioritized by default. If you want `CUDA` to be used instead, you can force a CUDA-based codec, which selects the CUDA device context:

```cpp
p.setInputVideoCodec("h264_cuvid");
p.setSource(file);
```

Notes:
- Set the `QT_AVPLAYER_NO_HWDEVICE` environment variable to force software decoding.
- You can also call `player.setInputVideoCodec("software")` to force software decoding for a specific player.
- Not every FFmpeg decoder/filter supports hardware acceleration — QtAVPlayer falls back to software decoding automatically when needed.


### 4. Rendering

#### 4.1. Video

- The `QAVWidget_OpenGL` widget renders to OpenGL, also supports `D3D11Texture2D` textures in Windows — see [examples/widget_video_opengl](https://github.com/valbok/QtAVPlayer/blob/master/examples/widget_video_opengl).

```cpp
auto w = new QAVWidget_OpenGL(mainWidget);
QObject::connect(p, &QAVPlayer::videoFrame, w, [w](const QAVVideoFrame &frame) {
    w->setVideoFrame(frame);
}, Qt::DirectConnection);
```

- Since `QAVVideoFrame` is compatible with `QVideoFrame`, `QtMultimedia` can render frames directly to QML or Widgets — see the [examples](https://github.com/valbok/QtAVPlayer/blob/master/examples/qml_video).

```cpp
QObject::connect(p, &QAVPlayer::videoFrame,
                 this, [this](const QAVVideoFrame &frame) {
    if (videoSink) {
        // If the non-copy-free render is requested
        // map the frame before converting to QVideoFrame.
        // This will force rendering mapped data instead of texture handles.
        if (!m_copyFreeRender)
            frame.map();
        QVideoFrame qframe = frame;  // Converts to QVideoFrame
        videoSink->setVideoFrame(qframe);
    }
}, Qt::DirectConnection);
```

#### 4.2. Audio

The audio frames could be played using `QAVAudioOutput`:

```cpp
auto audioOutput = new QAVAudioOutput(&mainWidget);
QObject::connect(p, &QAVPlayer::audioFrame, audioOutput, [audioOutput](const QAVAudioFrame &frame) {
    audioOutput->play(frame);
}, Qt::DirectConnection);
```

#### 4.3. Subtitles

- Subtitles could be rendered directly to the video frames using `subtitles` filter, but requires to have software decoders:

```cpp
// Render bundled subtitles (requires not using hw_device_ctx — this is a software filter)
player.setFilter("subtitles=file.mkv");

// Render subtitles from an external .srt file
player.setFilter("subtitles=file.srt");
```

- Subtitles could be parsed and extracted from `QAVSubtitleFrame`:

```cpp
QAVSubtitleTextParser subtitleParser;
subtitleParser.load(player.currentSubtitleStreams().first());
QObject::connect(player, &QAVPlayer::subtitleFrame, player, [this](const QAVSubtitleFrame &frame) {
    QString text;
    if (subtitleParser.parseText(frame, text) >= 0)
        emit subtitleTextChanged(text, frame.duration() * 1000);
}
```

- Subtitles could be rendered to `QImage`:

```cpp
QAVASSRenderer subtitleRenderer;
subtitleRenderer.load(player.currentSubtitleStreams().first());
QObject::connect(player, &QAVPlayer::subtitleFrame, player, [this](const QAVSubtitleFrame &frame) {
    auto img = subtitleRenderer.toImage(frame, size.width(), size.height());
    if (!img.isNull())
        emit subtitleImageChanged(img, frame.duration() * 1000);
}
```

See the [qml_player](https://github.com/valbok/QtAVPlayer/blob/master/examples/qml_player).


### 5. FFmpeg filters

```cpp
player1.setFilter("crop=iw/2:ih:0:0,split[left][tmp];[tmp]hflip[right];[left][right] hstack");
player2.setFilter("scale=iw/2:-1");
```

- Apply multiple filters at once, `QAVVideoFrame::filterName()` returns the filter name if any:

```cpp 
player.setFilters({
    "drawtext=text=%{pts\\:hms}:x=(w-text_w)/2:y=(h-text_h)*(4/5):box=1:boxcolor=gray@0.5:fontsize=36[drawtext]",
    "negate[negate]",
    "[0:v]split=3[in1][in2][in3];[in1]boxblur[out1];[in2]negate[out2];[in3]drawtext=text=%{pts\\:hms}:x=(w-text_w)/2:y=(h-text_h)*(4/5):box=1:boxcolor=gray@0.5:fontsize=36[out3]"
});
QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
    qDebug() << frame.pts() << frame.filterName();
});
```

- Some filters could be hardware accelerated, like `scale_cuda` or `scale_vaapi`:

```cpp
// Requires to force cuda based codec
player.setInputVideoCodec("h264_cuvid");
player.setFilter("scale_cuda=1920:1080");
```

### 6. Multiple streams

- `QAVPlayer::availableStreams()` returns all available streams in the source.
- `QAVPlayer::setAudioStream()`, `QAVPlayer::setSubtitleStream()`, `QAVPlayer::setVideoStream()` can change current decoding streams.

```cpp
qDebug() << "Audio streams:" << player.availableAudioStreams().size();
qDebug() << "Current stream:" << player.currentAudioStreams().first().index()
         << player.currentAudioStreams().first().metadata();

player.setAudioStreams(player.availableAudioStreams()); // Decode all available audio streams

// Per-stream progress: pts, fps, frame rate, frame count, etc.
for (const auto &stream : player.availableVideoStreams())
    qDebug() << stream << player.progress(stream);
```

### 7. Muxing streams

- Mux all streams to a file without re-encoding. It will use the same codecs without decoding the frames.

```cpp
player.setOutput("output.mkv");
```

- Combine streams from multiple players into a single file. This will decode frames first and then encode using the same codecs:

```cpp
QAVPlayer p1, p2;
QAVMuxerFrames muxer;

// The players must be loaded before loading the muxer
QTRY_VERIFY(p1.mediaStatus() == QAVPlayer::LoadedMedia);
QTRY_VERIFY(p2.mediaStatus() == QAVPlayer::LoadedMedia);

auto streams = p1.availableStreams() + p2.availableStreams();
muxer.load(streams, "output.mkv");

QObject::connect(&p1, &QAVPlayer::videoFrame, &p1, [&](const QAVVideoFrame &f) { muxer.enqueue(f); }, Qt::DirectConnection);
QObject::connect(&p1, &QAVPlayer::audioFrame, &p1, [&](const QAVAudioFrame &f) { muxer.enqueue(f); }, Qt::DirectConnection);
QObject::connect(&p2, &QAVPlayer::videoFrame, &p2, [&](const QAVVideoFrame &f) { muxer.enqueue(f); }, Qt::DirectConnection);
QObject::connect(&p2, &QAVPlayer::audioFrame, &p2, [&](const QAVAudioFrame &f) { muxer.enqueue(f); }, Qt::DirectConnection);

p1.play();
p2.play();
```

### 8. Accurate seeking

If a frame exists at the requested timestamp, it's returned first.

```cpp
QObject::connect(&player, &QAVPlayer::seeked, &player, [&](qint64 pos) { seekPosition = pos; });
QObject::connect(&player, &QAVPlayer::videoFrame, [&](const QAVVideoFrame &frame) { seekFrame = frame; });

player.seek(5000);
QTRY_COMPARE(seekPosition, 5000);
QTRY_COMPARE(seekFrame.pts(), 5.0);
```

### 9. Stepping frame by frame

Stepping emits exactly one frame.

```cpp
QObject::connect(&player, &QAVPlayer::videoFrame, [&](const QAVVideoFrame &frame) { receivedFrame = frame; });

if (player.state() != QAVPlayer::PausedState) {
    player.pause(); // Pausing always emits exactly one frame
    QTRY_VERIFY(receivedFrame);
}

player.stepForward();  // Advances and emits exactly one frame
player.stepBackward(); // Same, but backward
```

### 10. Listening to player signals

Every action is confirmed with a signal, delivered in the correct order:

```cpp
QObject::connect(p, &QAVPlayer::played, [&](qint64 pos) { qDebug() << "Playing started at" << pos; });
QObject::connect(p, &QAVPlayer::paused, [&](qint64 pos) { qDebug() << "Paused at" << pos; });
QObject::connect(p, &QAVPlayer::stopped, [&](qint64 pos) { qDebug() << "Stopped at" << pos; });
QObject::connect(p, &QAVPlayer::seeked, [&](qint64 pos) { qDebug() << "Seeked to" << pos; });
QObject::connect(p, &QAVPlayer::stepped, [&](qint64 pos) { qDebug() << "Stepped to" << pos; });
QObject::connect(p, &QAVPlayer::mediaStatusChanged, [&](QAVPlayer::MediaStatus status) {
    switch (status) {
    case QAVPlayer::EndOfMedia:
        qDebug() << "Playback finished, no frames left in queue";
        break;
    case QAVPlayer::NoMedia:
        qDebug() << "Demuxer threads finished";
        break;
    default:
        break;
    }
});
```

---

## Installation & Build Options

QtAVPlayer is meant to be bundled directly into your application. A few compile-time defines let you opt into extra features.

### Build Flags

| Define | Enables |
|--------|---------|
| `QT_AVPLAYER_MULTIMEDIA` | `QtMultimedia` support (requires `QtGUI`, `QtQuick`, etc.) |
| `QT_AVPLAYER_CUDA` | CUDA support |
| `QT_AVPLAYER_VA_X11` | `libva-x11` hardware acceleration (Linux only) |
| `QT_AVPLAYER_VA_DRM` | `libva-drm` hardware acceleration (Linux only) |
| `QT_AVPLAYER_VDPAU` | `libvdpau` hardware acceleration (Linux only) |
| `QT_AVPLAYER_WIDGET_OPENGL` | The OpenGL-based widget |
| `QT_AVPLAYER_LIBASS` | `libass` support and `QAVASSRenderer` |

### QMake

Add [QtAVPlayer.pri](https://github.com/valbok/QtAVPlayer/blob/master/src/QtAVPlayer/QtAVPlayer.pri) to your `.pro` file:

```qmake
DEFINES += "QT_AVPLAYER_MULTIMEDIA"
INCLUDEPATH += ../../src/
include(../../src/QtAVPlayer/QtAVPlayer.pri)
```

If FFmpeg is in a custom location:

```bash
qmake INCLUDEPATH+="/usr/local/Cellar/ffmpeg/6.0/include" LIBS="-L/usr/local/Cellar/ffmpeg/6.0/lib"
```

### CMake

Include [QtAVPlayer.cmake](https://github.com/valbok/QtAVPlayer/blob/master/src/QtAVPlayer/QtAVPlayer.cmake) in your project:

```cmake
include_directories(QtAVPlayer/src)
set(QT_AVPLAYER_DIR QtAVPlayer/src/QtAVPlayer)
include(QtAVPlayer/src/QtAVPlayer/QtAVPlayer.cmake)
add_executable(${PROJECT_NAME} ${QtAVPlayer_SOURCES})
target_link_libraries(${PROJECT_NAME} ${QtAVPlayer_LIBS})
```

If FFmpeg is in a custom location:

```bash
cmake ../ -DQT_AVPLAYER_MULTIMEDIA=ON -DCMAKE_PREFIX_PATH=/opt/Qt/6.7.1/macos/lib/cmake -DCMAKE_LIBRARY_PATH=/opt/homebrew/Cellar/ffmpeg/7.0_1/lib
```

### Building as a Shared Library (`libQtAVPlayer`)

If bundling directly isn't convenient, build QtAVPlayer as a standalone shared library instead:

**1. Build and install:**

```bash
cmake ../src/QtAVPlayer \
  -DCMAKE_PREFIX_PATH=/opt/Qt/6.8.2/gcc_64/lib/cmake/Qt6 \
  -DCMAKE_INSTALL_PREFIX=/opt/QtAVPlayer/install \
  -DCMAKE_LIBRARY_PATH="/opt/ffmpeg/install/lib;/opt/Qt/6.8.2/gcc_64/lib" \
  -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=/opt/ffmpeg/install/include \
  -DQT_AVPLAYER_MULTIMEDIA=On \
  -DQT_AVPLAYER_VDPAU=ON
make -j32
make install
```

**2. Link against it in your `CMakeLists.txt`:**

```cmake
find_package(QtAVPlayer REQUIRED)
target_link_libraries(${PROJECT_NAME} QtAVPlayer)
```

**3. Point CMake to the install location if needed:**

```bash
cmake ../ -DCMAKE_PREFIX_PATH="/opt/QtAVPlayer/install/lib/cmake;/opt/Qt/6.8.2/gcc_64/lib/cmake/Qt6" -DCMAKE_LIBRARY_PATH="/opt/Qt/6.8.2/gcc_64/lib"
```

### Android Setup

Point these environment variables to your prebuilt FFmpeg libraries for each target architecture:

```bash
export AVPLAYER_ANDROID_LIB_ARMEABI_V7A=/opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib
export AVPLAYER_ANDROID_LIB_ARMEABI_V8A=/opt/mobile-ffmpeg/prebuilt/android-arm64/ffmpeg/lib
export AVPLAYER_ANDROID_LIB_X86=/opt/mobile-ffmpeg/prebuilt/android-x86/ffmpeg/lib
export AVPLAYER_ANDROID_LIB_X86_64=/opt/mobile-ffmpeg/prebuilt/android-x86_64/ffmpeg/lib
export CPLUS_INCLUDE_PATH=/opt/mobile-ffmpeg/prebuilt/android-arm64/ffmpeg/include:$CPLUS_INCLUDE_PATH

qmake DEFINES+="QT_AVPLAYER_MULTIMEDIA"
```

Then add the extra libraries to your app's `.pro` file:

```qmake
ANDROID_EXTRA_LIBS += \
    /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavdevice.so \
    /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavformat.so \
    /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavutil.so \
    /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavcodec.so \
    /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavfilter.so \
    /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libswscale.so \
    /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libswresample.so
```

---

## License

See the [QtAVPlayer repository](https://github.com/valbok/QtAVPlayer) for license details.
