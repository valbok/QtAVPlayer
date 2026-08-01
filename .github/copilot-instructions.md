# Copilot Instructions for QtAVPlayer

A free, open-source Qt multimedia library powered by FFmpeg. Provides video/audio/subtitle decoding, filtering, encoding, and playback with hardware acceleration support.

## Build & Test

### Prerequisites
- Qt 5.6+ or Qt 6.x
- FFmpeg libraries (libavcodec, libavformat, libavutil, libavfilter, libavdevice, libswscale, libswresample)
- Platform-specific acceleration libraries (optional): libva, libvdpau, libass
- C++ compiler with C++17 support

### Build Commands

**Building the library as shared library (`libQtAVPlayer`):**
```bash
mkdir build && cd build
cmake ../src/QtAVPlayer \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/lib/cmake \
  -DCMAKE_INSTALL_PREFIX=../install \
  -DQT_AVPLAYER_MULTIMEDIA=ON
make -j$(nproc)
make install
```

**Building examples:**
```bash
cd examples/qml_video && qmake && make
cd examples/widget_video_opengl && qmake && make
cd examples/muxer && qmake && make
```

### Running Tests

**Run all integration tests:**
```bash
cd tests/auto/integration/qavdemuxer
qmake DEFINES+="QT_AVPLAYER_LIBASS"
make
./tst_qavdemuxer -platform offscreen

cd ../qavplayer
qmake
make
./tst_qavplayer --platform minimal
```

**Run single test:**
```bash
cd tests/auto/integration/qavplayer
qmake && make && ./tst_qavplayer --platform minimal -maxwarnings 100000
```

### CMake Build Flags

| Flag | Purpose |
|------|---------|
| `QT_AVPLAYER_MULTIMEDIA` | Enable QtMultimedia support |
| `QT_AVPLAYER_CUDA` | Enable CUDA hardware acceleration |
| `QT_AVPLAYER_VA_X11` | Enable VA-API with X11 (Linux) |
| `QT_AVPLAYER_VA_DRM` | Enable VA-API with DRM (Linux) |
| `QT_AVPLAYER_VDPAU` | Enable VDPAU acceleration (Linux) |
| `QT_AVPLAYER_WIDGET_OPENGL` | Enable OpenGL-based widget |
| `QT_AVPLAYER_LIBASS` | Enable libass for subtitle rendering |

## Architecture Overview

### Core Classes

**Main Playback Engine:**
- `QAVPlayer` - Central playback controller. Manages demuxing, decoding, filtering, and frame delivery.
  - Emits `videoFrame`, `audioFrame`, `subtitleFrame` signals
  - State machine: `StoppedState`, `PlayingState`, `PausedState`
  - Media status tracking: `NoMedia`, `LoadedMedia`, `EndOfMedia`, `InvalidMedia`

**Frame Types (all compatible with `QVideoFrame`):**
- `QAVVideoFrame` - Decoded video frames with hardware texture support (OpenGL, Metal, D3D11)
- `QAVAudioFrame` - Decoded audio frames with sample format and channel layout
- `QAVSubtitleFrame` - Decoded subtitles (bitmap or text-based)

**Supporting Components:**
- `QAVDemuxer` - Demuxes streams from various containers
- `QAVCodec` - Wraps FFmpeg codec context and decoding
- `QAVFilter` - Applies FFmpeg filters to frames (scale, crop, negate, etc.)
- `QAVMuxer`/`QAVMuxerFrames`/`QAVMuxerPackets` - Muxes/encodes streams to output
- `QAVStream` - Metadata and properties for video/audio/subtitle streams
- `QAVAudioOutput` - Thin wrapper around `QAudioSink` for audio playback
- `QAVIODevice` - Enables reading from custom `QIODevice` implementations
- `QAVASSRenderer` - Renders ASS/SSA subtitle files to QImage
- `QAVSubtitleTextParser` - Parses text-based subtitles

### Threading Model
- Separate demuxer and decoder threads for efficient multi-stream processing
- Frame signals (`videoFrame`, `audioFrame`, `subtitleFrame`) fire on decoder threads
- Connect with `Qt::DirectConnection` for zero-copy rendering from GPU surfaces
- Use `Qt::QueuedConnection` if thread-safe queuing is needed

### Hardware Acceleration

Platform-specific hardware decoding automatically negotiated:

| Platform | Backend | Frame Type |
|----------|---------|------------|
| Linux | VA-API / VDPAU | OpenGL |
| macOS / iOS | Video Toolbox | Metal |
| Windows | D3D11 | D3D11Texture2D |
| Android | MediaCodec | OpenGL |

Force software decoding:
```cpp
player->setInputVideoCodec("software");
// or environment variable: QT_AVPLAYER_NO_HWDEVICE=1
```

### Stream Handling
- All streams decoded simultaneously (multiple audio tracks, etc.)
- Query available streams: `player->availableVideoStreams()`, `availableAudioStreams()`, `availableSubtitleStreams()`
- Switch active streams on-the-fly: `setVideoStream()`, `setAudioStream()`, `setSubtitleStream()`
- Per-stream progress available via `progress(stream)`

## Key Conventions

### Header Organization
- **Public headers:** `src/QtAVPlayer/qav*.h` (included as `<QtAVPlayer/qavplayer.h>`)
- **Private headers:** `*_p.h` suffix for implementation details (FFmpeg internals, threading)
- **Private-private headers:** `*_p_p.h` for internal-only classes

### Coding Patterns

**Private Implementation Pattern (Pimpl):**
```cpp
class QAVPlayer : public QObject {
    // ...
private:
    std::unique_ptr<QAVPlayerPrivate> d_ptr;
};
```

**Signal Emission (frame delivery):**
```cpp
// Connected with Qt::DirectConnection in decoder threads
Q_EMIT player->videoFrame(decodedFrame);
```

**Platform-Specific Code:**
```cpp
#ifdef Q_OS_LINUX
    // Linux-specific: VA-API, VDPAU
#endif
#ifdef Q_OS_MACOS
    // macOS-specific: Video Toolbox
#endif
#ifdef Q_OS_WIN
    // Windows-specific: D3D11
#endif
#ifdef Q_OS_ANDROID
    // Android-specific: MediaCodec
#endif
```

### FFmpeg Integration
- Direct C API usage (no C++ wrappers)
- Common includes: `<libavformat/avformat.h>`, `<libavcodec/avcodec.h>`, `<libavutil/frame.h>`
- FFmpeg objects managed with RAII (custom deleters in smart pointers)
- Filter graphs: `AVFilterGraph`, `AVFilterContext` for complex multi-input scenarios

### Qt Resource Files
- `qml.qrc` (Qt 5) and `qml_qt6.qrc` (Qt 6) embedded in library
- Examples use both `.qrc` and CMake's `qt_add_resources()`

### Build System Dual Support
Both QMake and CMake build files must be maintained in parallel:
- QMake: `QtAVPlayer.pri`, `*.pro` files
- CMake: `QtAVPlayer.cmake`, `CMakeLists.txt` files
- Both must define same sources and include paths

## Integration Patterns

### Using in Applications

**Link as shared library (recommended):**
```cmake
find_package(QtAVPlayer REQUIRED)
target_link_libraries(my_app QtAVPlayer)
```

**Bundle directly (Qt projects only):**
```cmake
include_directories(QtAVPlayer/src)
set(QT_AVPLAYER_DIR QtAVPlayer/src/QtAVPlayer)
include(QtAVPlayer/src/QtAVPlayer/QtAVPlayer.cmake)
add_executable(my_app ${QtAVPlayer_SOURCES} main.cpp)
target_link_libraries(my_app ${QtAVPlayer_LIBS})
```

### Common Use Cases

**Simple playback:**
```cpp
QAVPlayer player;
player.setSource("file.mkv");
QObject::connect(&player, &QAVPlayer::videoFrame, &player,
    [&](const QAVVideoFrame &frame) {
        videoSink->setVideoFrame(QVideoFrame(frame));
    }, Qt::DirectConnection);
player.play();
```

**Multiple filters:**
```cpp
player->setFilters({
    "scale=1920:1080",
    "[0:v]split=2[a][b];[a]negate[out1];[b]boxblur[out2]"
});
```

**Hardware-accelerated filter:**
```cpp
player->setInputVideoCodec("h264_cuvid");  // Force CUDA
player->setFilter("scale_cuda=1920:1080");
```

## File Structure

```
src/QtAVPlayer/
├── qavplayer.h               # Main API
├── qavstream.h               # Stream metadata
├── qav(video|audio|subtitle)frame.h  # Frame types
├── qav(codec|demuxer|filter|muxer).h # Core internals (mostly private)
├── qavaudiooutput.h          # Audio playback wrapper
├── qaviodevice.h             # Custom I/O support
├── qavassrenderer.h          # Subtitle rendering
├── QtAVPlayer.pri            # QMake configuration
└── QtAVPlayer.cmake          # CMake configuration

examples/
├── qml_video/                # QML-based example (Qt 5/6)
├── qml_player/               # Advanced QML with subtitles (Qt 6 only)
├── widget_video/             # Widgets-based example
├── widget_video_opengl/      # Hardware-accelerated OpenGL rendering
└── muxer/                    # Stream muxing example

tests/auto/integration/
├── qavdemuxer/               # Demuxer/codec tests
└── qavplayer/                # Player state/seeking tests
```

## Debugging

**Enable debug logging:**
```bash
export QT_LOGGING_RULES="qt.QtAVPlayer.debug=true"
./your_app
```

**Force software decoding:**
```bash
export QT_AVPLAYER_NO_HWDEVICE=1
./your_app
```

**Offline rendering (CI/headless):**
```bash
./tst_qavplayer -platform offscreen
./tst_qavplayer -platform minimal  # or minimal (less overhead)
```
