# Screencopy

Screen recording and editing application built with Qt6 and FFmpeg.

## Development

### Linux

#### Dependencies

Package names shown for Arch Linux — adjust for your distro.

```bash
sudo pacman -S qt6-base qt6-declarative qt6-multimedia qt6-wayland ffmpeg pipewire libpipewire gpu-screen-recorder cmake pkg-config gcc
```

#### Build

```bash
cd qt
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

#### Run

```bash
./build/screencopy
```
