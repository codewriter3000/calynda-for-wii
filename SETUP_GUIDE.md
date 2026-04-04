# Calynda for Wii Setup Guide

This guide walks you through setting up the Calynda for Wii toolchain and running a basic "Hello World" program on the Dolphin emulator.

## Prerequisites

### 1. Install devkitPPC

devkitPPC is the PowerPC cross-compiler toolchain required to build Wii executables.

**Linux (Debian/Ubuntu):**

```sh
wget https://apt.devkitpro.org/install-devkitpro-pacman.sh
chmod +x install-devkitpro-pacman.sh
sudo ./install-devkitpro-pacman.sh
sudo dkp-pacman -S wii-dev
```

**macOS (via Homebrew):**

```sh
brew install --cask devkitpro-pacman
sudo dkp-pacman -S wii-dev
```

After installation, add devkitPPC to your shell profile:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=$DEVKITPRO/devkitPPC
export PATH=$DEVKITPPC/bin:$PATH
```

Verify the toolchain is available:

```sh
powerpc-eabi-gcc --version
```

### 2. Install the Dolphin Emulator

Dolphin is a Wii and GameCube emulator used for testing DOL executables on your PC.

**Linux (Debian/Ubuntu):**

```sh
sudo apt-add-repository ppa:dolphin-emu/ppa
sudo apt update
sudo apt install dolphin-emu
```

**Linux (Flatpak):**

```sh
flatpak install flathub org.DolphinEmu.dolphin-emu
```

**macOS:**

```sh
brew install --cask dolphin
```

Or download directly from [https://dolphin-emu.org/download/](https://dolphin-emu.org/download/).

Verify Dolphin is available:

```sh
dolphin-emu --version
```

### 3. Install Build Tools

You also need `make` and standard Unix tools:

```sh
# Debian/Ubuntu
sudo apt install build-essential

# macOS (Xcode command-line tools)
xcode-select --install
```

## Installing Calynda for Wii

Clone the repository and install:

```sh
git clone https://github.com/codewriter3000/calynda-for-wii.git
cd calynda-for-wii
sh ./compiler/install.sh
```

Or build from source without installing:

```sh
make -C compiler calynda
```

The compiler binary will be at `compiler/build/calynda`.

## Hello World

### Step 1: Write the Program

Create a file named `hello.cal`:

```cal
import io.stdlib;

start(string[] args) -> {
    stdlib.print("Hello, Wii!");
    return 0;
};
```

### Step 2: Compile to a DOL Executable

```sh
calynda build hello.cal
```

This produces a DOL executable at `./build/hello.dol`.

### Step 3: Run in the Dolphin Emulator

Launch the DOL directly in Dolphin:

```sh
dolphin-emu -b -e ./build/hello.dol
```

Flags:

- `-b` — start in batch (no-GUI) mode
- `-e` — execute the specified DOL file

To launch with the graphical interface instead:

```sh
dolphin-emu -e ./build/hello.dol
```

### One-Step Build and Run

You can compile and launch in Dolphin in a single command:

```sh
calynda run hello.cal
```

This builds the DOL and opens it in Dolphin automatically.

## Running on Real Wii Hardware

To run Calynda programs on a real Nintendo Wii:

1. **Install the Homebrew Channel** on your Wii. Follow the guide at [https://wii.hacks.guide/](https://wii.hacks.guide/).

2. **Copy the DOL** to your SD card:

   ```
   SD:/apps/hello/boot.dol
   ```

   Create a `meta.xml` alongside it:

   ```xml
   <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
   <app version="1">
     <name>Hello Calynda</name>
     <coder>Your Name</coder>
     <short_description>Hello World in Calynda for Wii</short_description>
     <long_description>A basic Calynda for Wii program.</long_description>
   </app>
   ```

3. **Insert the SD card** into your Wii and launch from the Homebrew Channel.

## Troubleshooting

### `powerpc-eabi-gcc: command not found`

Make sure devkitPPC is installed and on your `PATH`:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=$DEVKITPRO/devkitPPC
export PATH=$DEVKITPPC/bin:$PATH
```

### Dolphin shows a black screen

- Make sure you compiled with `calynda build`, not `calynda asm` or `calynda bytecode`.
- Verify the output is a valid DOL file: `file ./build/hello.dol`.
- Try updating Dolphin to the latest development build.

### `calynda: command not found`

Either install via `./compiler/install.sh` and ensure the install bin directory is on `PATH`, or use the local build directly:

```sh
./compiler/build/calynda build hello.cal
```

## Next Steps

- See [README.md](README.md) for the full CLI reference and language overview.
- See [CONTRIBUTING.md](CONTRIBUTING.md) to get involved with development.
- See [version_wishlist.md](version_wishlist.md) for the V2 language specification and roadmap.
