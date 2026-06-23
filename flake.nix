{
  description = "Simulateur d'Émergence Déterministe (SED) - Rust Edition";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, utils }:
    utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        # Macroquad dependencies
        x11Deps = with pkgs; [
          xorg.libX11
          xorg.libXcursor
          xorg.libXi
          xorg.libXrandr
          xorg.libXinerama
          libGL
          libxkbcommon
          alsa-lib
        ];
      in
      {
        # Development Shell (nix develop)
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            cargo
            rustc
            pkg-config
          ] ++ x11Deps;

          shellHook = ''
            export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${pkgs.libGL}/lib:${pkgs.libxkbcommon}/lib
          '';
        };

        # Rust package (nix build)
        packages.default = pkgs.rustPlatform.buildRustPackage {
          pname = "rust_sed";
          version = "9.0.0";
          src = ./.;

          cargoLock = {
            lockFile = ./Cargo.lock;
          };

          nativeBuildInputs = [ pkgs.pkg-config ];
          buildInputs = x11Deps;

          doCheck = false; # GUI tests might fail in headless sandbox
        };
      }
    );
}
