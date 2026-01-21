{
  description = "Logos App POC - Qt application with UI plugins";

  inputs = {
    # Follow the same nixpkgs as logos-liblogos to ensure compatibility
    nixpkgs.follows = "logos-liblogos/nixpkgs";
    logos-cpp-sdk.url = "github:logos-co/logos-cpp-sdk";
    #logos-liblogos.url = "github:logos-co/logos-liblogos";
    logos-liblogos.url = "path:/Users/iurimatias/Projects/Logos/LogosCore/logos-liblogos/";
    logos-package-manager.url = "github:logos-co/logos-package-manager";
    logos-capability-module.url = "github:logos-co/logos-capability-module";
    logos-package.url = "github:logos-co/logos-package";
    #logos-package-manager-ui.url = "github:logos-co/logos-package-manager-ui";
    logos-package-manager-ui.url = "path:/Users/iurimatias/Projects/Logos/logos-package-manager-ui/";
    #logos-webview-app.url = "github:logos-co/logos-webview-app";
    logos-webview-app.url = "path:/Users/iurimatias/Projects/Logos/logos-webview-app/";
  };

  outputs = { self, nixpkgs, logos-cpp-sdk, logos-liblogos, logos-package-manager, logos-capability-module, logos-package, logos-package-manager-ui, logos-webview-app }:
    let
      systems = [ "aarch64-darwin" "x86_64-darwin" "aarch64-linux" "x86_64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f {
        pkgs = import nixpkgs { inherit system; };
        logosSdk = logos-cpp-sdk.packages.${system}.default;
        logosLiblogos = logos-liblogos.packages.${system}.default;
        logosPackageManager = logos-package-manager.packages.${system}.default;
        logosCapabilityModule = logos-capability-module.packages.${system}.default;
        logosPackageLib = logos-package.packages.${system}.lib;
        logosPackageManagerUI = logos-package-manager-ui.packages.${system}.default;
        logosWebviewApp = logos-webview-app.packages.${system}.default;
        logosCppSdkSrc = logos-cpp-sdk.outPath;
        logosLiblogosSrc = logos-liblogos.outPath;
        logosPackageManagerSrc = logos-package-manager.outPath;
        logosCapabilityModuleSrc = logos-capability-module.outPath;
      });
    in
    {
      packages = forAllSystems ({ pkgs, logosSdk, logosLiblogos, logosPackageManager, logosCapabilityModule, logosPackageLib, logosPackageManagerUI, logosWebviewApp, ... }: 
        let
          # Common configuration
          common = import ./nix/default.nix { 
            inherit pkgs logosSdk logosLiblogos; 
          };
          src = ./.;
          
          # Plugin packages (development builds)
          counterPlugin = import ./nix/counter.nix { 
            inherit pkgs common src; 
          };

          counterQmlPlugin = import ./nix/counter-qml.nix {
            inherit pkgs common src;
          };
          
          mainUIPlugin = import ./nix/main-ui.nix { 
            inherit pkgs common src logosSdk logosPackageManager logosLiblogos logosPackageLib; 
          };
          
          # Use external package-manager-ui package
          packageManagerUIPlugin = logosPackageManagerUI;
          
          # Use external logos-webview-app package
          webviewAppPlugin = logosWebviewApp;
          
          # Plugin packages (distributed builds for DMG/AppImage)
          mainUIPluginDistributed = import ./nix/main-ui.nix { 
            inherit pkgs common src logosSdk logosPackageManager logosLiblogos logosPackageLib;
            distributed = true;
          };
          
          # Use external package-manager-ui package for distributed builds too
          packageManagerUIPluginDistributed = logosPackageManagerUI;
          
          # App package (development build)
          app = import ./nix/app.nix { 
            inherit pkgs common src logosLiblogos logosSdk logosPackageManager logosCapabilityModule;
            counterPlugin = counterPlugin;
            counterQmlPlugin = counterQmlPlugin;
            mainUIPlugin = mainUIPlugin;
            packageManagerUIPlugin = packageManagerUIPlugin;
            webviewAppPlugin = webviewAppPlugin;
          };
          
          # App package (distributed build for DMG/AppImage)
          appDistributed = import ./nix/app.nix { 
            inherit pkgs common src logosLiblogos logosSdk logosPackageManager logosCapabilityModule;
            counterPlugin = counterPlugin;
            counterQmlPlugin = counterQmlPlugin;
            mainUIPlugin = mainUIPluginDistributed;
            packageManagerUIPlugin = packageManagerUIPluginDistributed;
            webviewAppPlugin = webviewAppPlugin;
          };
          
          # macOS distribution packages (only for Darwin)
          appBundle = if pkgs.stdenv.isDarwin then
            import ./nix/macos-bundle.nix {
              inherit pkgs src;
              app = appDistributed;
            }
          else null;
          
          dmg = if pkgs.stdenv.isDarwin then
            import ./nix/macos-dmg.nix {
              inherit pkgs;
              appBundle = appBundle;
            }
          else null;

          # Linux AppImage (only for Linux)
          appImage = if pkgs.stdenv.isLinux then
            import ./nix/appimage.nix {
              inherit pkgs src;
              app = appDistributed;
              version = common.version;
            }
          else null;
        in
        {
          # Individual outputs
          counter-plugin = counterPlugin;
          counter-qml-plugin = counterQmlPlugin;
          main-ui-plugin = mainUIPlugin;
          package-manager-ui-plugin = packageManagerUIPlugin;
          webview-app-plugin = webviewAppPlugin;
          app = app;
          
          # Default package
          default = app;
        } // (if pkgs.stdenv.isDarwin then {
          # macOS distribution outputs
          app-bundle = appBundle;
          inherit dmg;
        } else {}) // (if pkgs.stdenv.isLinux then {
          # Linux distribution output
          appimage = appImage;
        } else {})
      );

      devShells = forAllSystems ({ pkgs, logosSdk, logosLiblogos, logosPackageManager, logosCapabilityModule, logosPackageLib, logosCppSdkSrc, logosLiblogosSrc, logosPackageManagerSrc, logosCapabilityModuleSrc }: {
        default = pkgs.mkShell {
          nativeBuildInputs = [
            pkgs.cmake
            pkgs.ninja
            pkgs.pkg-config
          ];
          buildInputs = [
            pkgs.qt6.qtbase
            pkgs.qt6.qtremoteobjects
            pkgs.zstd
            pkgs.krb5
            pkgs.abseil-cpp
          ];
          
          shellHook = ''
            # Nix package paths (pre-built for host system)
            export LOGOS_CPP_SDK_ROOT="${logosSdk}"
            export LOGOS_LIBLOGOS_ROOT="${logosLiblogos}"
            export LOGOS_PACKAGE_MANAGER_ROOT="${logosPackageManager}"
            export LOGOS_CAPABILITY_MODULE_ROOT="${logosCapabilityModule}"
            export LGX_ROOT="${logosPackageLib}"
            
            # Source paths for iOS builds (from flake inputs)
            export LOGOS_CPP_SDK_SRC="${logosCppSdkSrc}"
            export LOGOS_LIBLOGOS_SRC="${logosLiblogosSrc}"
            export LOGOS_PACKAGE_MANAGER_SRC="${logosPackageManagerSrc}"
            export LOGOS_CAPABILITY_MODULE_SRC="${logosCapabilityModuleSrc}"
            
            echo "Logos App POC development environment"
            echo ""
            echo "Nix packages (host builds):"
            echo "  LOGOS_CPP_SDK_ROOT: $LOGOS_CPP_SDK_ROOT"
            echo "  LOGOS_LIBLOGOS_ROOT: $LOGOS_LIBLOGOS_ROOT"
            echo "  LOGOS_PACKAGE_MANAGER_ROOT: $LOGOS_PACKAGE_MANAGER_ROOT"
            echo "  LOGOS_CAPABILITY_MODULE_ROOT: $LOGOS_CAPABILITY_MODULE_ROOT"
            echo "  LGX_ROOT: $LGX_ROOT"
            echo ""
            echo "Source paths (for iOS builds):"
            echo "  LOGOS_CPP_SDK_SRC: $LOGOS_CPP_SDK_SRC"
            echo "  LOGOS_LIBLOGOS_SRC: $LOGOS_LIBLOGOS_SRC"
            echo "  LOGOS_PACKAGE_MANAGER_SRC: $LOGOS_PACKAGE_MANAGER_SRC"
            echo "  LOGOS_CAPABILITY_MODULE_SRC: $LOGOS_CAPABILITY_MODULE_SRC"
          '';
        };
      });
    };
}
