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
| `make run-linux` | Build then launch the app (Linux) |
| `make release` | Configure and compile a release build into `build/release/` |
| `make test` | Run the test suite via `ctest` |
| `make clean` | Remove the `build/` directory |

### macOS

#### Dependencies

```bash
brew install qt@6 ffmpeg cmake pkg-config
```

#### One-time: create a self-signed code-signing identity

macOS TCC (Screen Recording, Microphone, etc.) keys permissions by code signature. Without a stable signing identity, every rebuild produces a new signature and the OS re-prompts for permissions on every launch. Run once:

```bash
make setup-signing
```

This creates a `ScreenCopyDev` self-signed certificate, imports it into your login keychain, and trusts it for code signing. CMake auto-detects it and signs every build with it, so TCC grants persist across rebuilds. Safe to re-run — it exits early if the identity already exists.

> If you'd rather use a paid Apple Developer identity, pass it when configuring:
> `cmake --preset default -DCODESIGN_IDENTITY="Apple Development: Your Name (TEAMID)"`

#### Commands

| Command | Description |
| --- | --- |
| `make build` | Configure (first time) and compile a debug build |
| `make run-mac` | Build then launch the `.app` bundle |
| `make reset-tcc` | Wipe granted macOS permissions (Screen Recording / Microphone / Camera) for a fresh prompt |

The first `make build` runs `cmake --preset default` automatically; subsequent builds skip reconfiguration.
