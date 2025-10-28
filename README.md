# logos-app-poc

## How to Build

### Using Nix (Recommended)

#### Build Complete Application

```bash
# Build everything (default)
nix build

# Or explicitly
nix build '.#default'
```

The result will include:
- `/bin/logos-app-poc` - The main Logos application executable
- All required modules and dependencies

#### Build Individual Components

```bash
# Build only the main application
nix build '.#app'
```

#### Development Shell

```bash
# Enter development shell with all dependencies
nix develop
```

**Note:** In zsh, you need to quote the target (e.g., `'.#default'`) to prevent glob expansion.

If you don't have flakes enabled globally, add experimental flags:

```bash
nix build --extra-experimental-features 'nix-command flakes'
```

The compiled artifacts can be found at `result/`

#### Running the Application

After building with `nix build`, you can run the application:

```bash
# Run the main Logos application
./result/bin/logos-app-poc
```

The application will automatically load all required modules and dependencies. All components are bundled in the Nix store layout.

#### Nix Organization

The nix build system is organized into modular files in the `/nix` directory:
- `nix/default.nix` - Common configuration and main application build
- `nix/app.nix` - Application-specific compilation settings
- `nix/main-ui.nix` - UI components compilation
- `nix/counter.nix` - Counter module compilation

## Requirements

### Build Tools
- CMake (3.16 or later)
- Ninja build system
- pkg-config

### Dependencies
- Qt6 (qtbase)
- Qt6 Widgets (included in qtbase)
- Qt6 Remote Objects (qtremoteobjects)
- logos-liblogos
- logos-cpp-sdk (for header generation)
- logos-capability-module
- logos-package-manager
- zstd
- krb5
- abseil-cpp
