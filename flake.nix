{
  description = "Logos App POC - Qt application with UI plugins";

  inputs = {
    # Follow the same nixpkgs as logos-liblogos to ensure compatibility
    nixpkgs.follows = "logos-liblogos/nixpkgs";
    logos-cpp-sdk.url = "github:logos-co/logos-cpp-sdk";
    logos-liblogos.url = "github:logos-co/logos-liblogos";
    logos-package-manager.url = "github:logos-co/logos-package-manager";
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
      });
    in
    {
      packages = forAllSystems ({ pkgs, logosSdk, logosLiblogos, logosPackageManager, logosCapabilityModule }: 
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
          
          mainUIPlugin = import ./nix/main-ui.nix { 
            inherit pkgs common src logosSdk logosPackageManager; 
          };
          
          # App package
          app = import ./nix/app.nix { 
            inherit pkgs common src logosLiblogos logosSdk logosPackageManager logosCapabilityModule;
            counterPlugin = counterPlugin;
            mainUIPlugin = mainUIPlugin;
          };
        in
        {
          # Individual outputs
          counter-plugin = counterPlugin;
          main-ui-plugin = mainUIPlugin;
          app = app;
          
          # Default package
          default = app;
        }
      );

      devShells = forAllSystems ({ pkgs, logosSdk, logosLiblogos, logosPackageManager, logosCapabilityModule }: {
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
            export LOGOS_CPP_SDK_ROOT="${logosSdk}"
            export LOGOS_LIBLOGOS_ROOT="${logosLiblogos}"
            echo "Logos App POC development environment"
            echo "LOGOS_CPP_SDK_ROOT: $LOGOS_CPP_SDK_ROOT"
            echo "LOGOS_LIBLOGOS_ROOT: $LOGOS_LIBLOGOS_ROOT"
          '';
        };
      });
    };
}
