{ pkgs ? import <nixpkgs> {} }:

let
  llvm = pkgs.llvmPackages_22;
in
pkgs.mkShell {
  buildInputs = [
    llvm.clang-unwrapped    # Der pure, nackte Clang ohne Nix-Einflüsse
    llvm.bintools           # LLVM standard tools
    llvm.lld                # Der native LLVM Linker
    pkgs.python3
    pkgs.gnumake
  ];

  hardeningDisable = [ "all" ];

  shellHook = ''
    # Verzeichnisstruktur für das Makefile simulieren
    mkdir -p .dummy_home/.rustup/toolchains/nix/bin
    ln -sf ${llvm.lld}/bin/lld .dummy_home/.rustup/toolchains/nix/bin/rust-lld
    export HOME=$PWD/.dummy_home

    export CC="${llvm.clang-unwrapped}/bin/clang"
    export CXX="${llvm.clang-unwrapped}/bin/clang++"

    echo "Openscope DevShell ready !"
  '';
}
