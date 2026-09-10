# shell-preview

The Basecamp UI shell, running against a fixture, with **no Logos code in the
process**.

It loads the real, shipped `main_ui.so` — the same artifact the real host loads,
unmodified — and drives it through `IShellHost`. It links Qt and
`app/interfaces/` and nothing else: not liblogos, not logos-protocol, not
logos-qt-host, not logos-view-module-runtime.

That is the whole point. `main_ui.so` exports **zero** Logos symbols (check with
`nm -DC`), so the UI can be built and iterated on while the core is being
rewritten underneath, and a core change cannot reach this binary.

## Why this exists (and why it is the mobile starting point)

`.#app-mock` replaces the module *runtime* with a fixture, but keeps the entire
protocol/SDK layer: it ships `liblogos_protocol` and `liblogos_qt_host`, spawns
`ui-host`, `dlopen`s plugins, and runs the SDK code generator at build time. It
is the right tool for developing Basecamp on desktop without a working core.

`shell-preview` cuts lower. Nothing from the core is compiled, linked or shipped.
For a first mobile bring-up that matters, because it reduces "port Basecamp" to
"cross-compile Qt, the QML modules, and one small host you own".

| | `.#app-mock` | `shell-preview` |
|---|---|---|
| Module runtime | fixture | absent |
| logos-protocol / qt-host | **shipped** | absent |
| `ui-host` subprocess | **spawned** | absent |
| UI plugins (e.g. PMUI) | load and run | **do not load** |
| SDK code generation at build | **yes** | no |

## What renders

Everything in `main_ui`: the sidebar, App Manager, and Settings (Dashboard, Apps
Inspector, Module Inspector, Repositories, Plugin Interface) — 33 QML files.

What does **not** render: any UI plugin. Package Manager is
`plugins/package_manager_ui/`, loaded through `PluginLoader`, which is host-side
and pulls in `LogosAPI` and `ui-host`. Have the fixture report no installed UI
plugins and the sidebar simply shows no app icons.

## Build and run

```bash
nix run .#shell-preview
# or: nix build '.#shell-preview' && ./result/bin/basecamp-shell-preview
```

The wrapper defaults `--shell` to the `main_ui` built alongside it, so it works
with no arguments. Both defaults can be overridden:

```
-s, --shell    <path>   the main_ui plugin to load
-f, --fixture  <path>   fixture JSON (otherwise the compiled-in copy)
```

`fixtures/shell-fixture.json` supplies the backend data; its keys map to
`FixtureBackend`'s QML-bound members. Editing it and passing `--fixture` avoids
a rebuild.

Verified of the built output: **zero** logos-protocol / liblogos / logos-qt-host
/ logos-view-module-runtime paths in its nix closure, and no Logos symbols in the
binary.

## Scope of the fixture

`FixtureBackend` implements the surface the shell's QML binds to — 22
`Q_PROPERTY`, 32 `Q_INVOKABLE` and 40 signals on `MainUIBackend`, of which the
QML actually touches ~130 members, concentrated in `Shell/ContentViews.qml` and
`Sidebar/SidebarPanel.qml`. Adding a view usually means adding a property here.

## iOS simulator

Static Qt cannot dlopen the shell, so on iOS `main_ui` (built with `MAIN_UI_STATIC`)
is linked into the host (`SHELL_PREVIEW_STATIC_SHELL`, usable on any platform)
and reached through `QPluginLoader::staticInstances()`; the
`IShellView` contract and the ABI check are the same. The pure half builds the
archives on Xcode's clang in nix, the impure half links and launches:

```bash
nix build .#packages.aarch64-ios-simulator.shell-preview-ios   # static archives only
nix run .#run-ios-sim                                          # xcodebuild + simctl, unsigned
nix run .#run-ios-sim -- --fixture /abs/path.json              # arguments reach the app
```

Sources: `platform/ios/stage` (Ninja, the fixture host archive) and
`platform/ios/app` (Xcode generator, `main.cpp` and the bundle). A fixture may
set `currentActiveSectionIndex` to open on App Manager (1) or Settings (3);
the desktop layout's 800px minimum clips on a phone, expected for now. Logs:
`xcrun simctl spawn booted log stream --level info --predicate 'process == "BasecampShellPreview"'`.

Gotcha: Qt registers default plugins only from its own prefix, so the static
qsvg plugin from the qtsvg prefix is linked by hand in `platform/ios/app`.

## For mobile

The host is a desktop one — `QApplication`, `QMainWindow`, `QQuickWidget`. A
mobile variant is a second `IShellHost` implementation over `QGuiApplication` +
`QQuickWindow`, reusing this fixture backend unchanged. That file is the port;
the UI beneath it is not.

See `../MOBILE-HANDOFF.md`.
