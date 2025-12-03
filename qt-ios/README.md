# Logos iOS App

## Dependencies

- Nix with flakes enabled
- Qt 6.8.2 for iOS and macOS installed at `~/Qt6/6.8.2/`
- Xcode with iOS SDK

## Build

```bash
# Enter nix development shell
nix --extra-experimental-features "nix-command flakes" develop

# Build for iOS Simulator
./build.sh --target sim

# Build for iOS Device (note: untested)
./build.sh --target device
```

### Options

- `--clean` - Clean build directories before building
- `--skip-deps` - Skip rebuilding dependencies (liblogos, modules)

## Run on Simulator

exit nix develop and run:

```bash
./run.sh
```
