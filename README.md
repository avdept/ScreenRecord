# ScreenRecord

Screen recording and editing application built with Qt6 and FFmpeg.

## Development

### Linux

#### Dependencies

Package names shown for Arch Linux — adjust for your distro.

```bash
sudo pacman -S qt6-base qt6-declarative qt6-multimedia qt6-wayland ffmpeg pipewire libpipewire gpu-screen-recorder cmake pkg-config gcc
```

#### Commands

| Command | Description |
| --- | --- |
| `make build` | Configure (first time) and compile a debug build into `build/default/` |
| `make run` | Build then launch the app |
| `make release` | Configure and compile a release build into `build/release/` |
| `make test` | Run the test suite via `ctest` |
| `make clean` | Remove the `build/` directory |

The first `make build` runs `cmake --preset default` automatically; subsequent builds skip reconfiguration.
