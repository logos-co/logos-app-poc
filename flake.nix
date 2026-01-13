{
  description = "Logos App POC - Qt application with UI plugins";

  inputs = {
    # Follow the same nixpkgs as logos-liblogos to ensure compatibility
    nixpkgs.follows = "logos-liblogos/nixpkgs";
    logos-cpp-sdk.url = "github:logos-co/logos-cpp-sdk";
    logos-liblogos.url = "github:logos-co/logos-liblogos";
    logos-package-manager.url = "path:/Users/iurimatias/Projects/Logos/LogosCore/logos-package-manager";
    logos-capability-module.url = "github:logos-co/logos-capability-module";
  };

  outputs = { self, nixpkgs, logos-cpp-sdk, logos-liblogos, logos-package-manager, logos-capability-module }:
    let
      systems = [ "aarch64-darwin" "x86_64-darwin" "aarch64-linux" "x86_64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f {
        pkgs = import nixpkgs { inherit system; };
        logosSdk = logos-cpp-sdk.packages.${system}.default;
        logosLiblogos = logos-liblogos.packages.${system}.default;
        logosPackageManager = logos-package-manager.packages.${system}.default;
        logosCapabilityModule = logos-capability-module.packages.${system}.default;
        logosCppSdkSrc = logos-cpp-sdk.outPath;
        logosLiblogosSrc = logos-liblogos.outPath;
        logosPackageManagerSrc = logos-package-manager.outPath;
        logosCapabilityModuleSrc = logos-capability-module.outPath;
      });
    in
    {
      packages = forAllSystems ({ pkgs, logosSdk, logosLiblogos, logosPackageManager, logosCapabilityModule, ... }: 
        let
          # Common configuration
          common = import ./nix/default.nix { 
            inherit pkgs logosSdk logosLiblogos; 
          };
          src = ./.;
          
          # Plugin packages
          counterPlugin = import ./nix/counter.nix { 
            inherit pkgs common src; 
          };

          counterQmlPlugin = import ./nix/counter-qml.nix {
            inherit pkgs common src;
          };
          
          mainUIPlugin = import ./nix/main-ui.nix { 
            inherit pkgs common src logosSdk logosPackageManager logosLiblogos; 
          };
          
          packageManagerUIPlugin = import ./nix/package-manager-ui.nix { 
            inherit pkgs common src logosSdk logosPackageManager logosLiblogos; 
          };
          
          webviewAppPlugin = import ./nix/webview-app.nix { 
            inherit pkgs common src; 
          };
          
          # App package
          app = import ./nix/app.nix { 
            inherit pkgs common src logosLiblogos logosSdk logosPackageManager logosCapabilityModule;
            counterPlugin = counterPlugin;
            counterQmlPlugin = counterQmlPlugin;
            mainUIPlugin = mainUIPlugin;
            packageManagerUIPlugin = packageManagerUIPlugin;
            webviewAppPlugin = webviewAppPlugin;
          };
          
          # macOS distribution packages (only for Darwin)
          appBundle = if pkgs.stdenv.isDarwin then
            import ./nix/macos-bundle.nix {
              inherit pkgs app src;
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
              inherit pkgs app src;
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

      devShells = forAllSystems ({ pkgs, logosSdk, logosLiblogos, logosPackageManager, logosCapabilityModule, logosCppSdkSrc, logosLiblogosSrc, logosPackageManagerSrc, logosCapabilityModuleSrc }: {
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
