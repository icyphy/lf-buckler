# Lingua Franca, nRF52, and Buckler

This repo provides resources for using Lingua Franca to program an nRF52x Thread/BLE embedded board from Nordic Semiconductor with and without a Berkeley Buckler daughter card. The first part of this README assumes you have already set up your machine and have connected the board to a USB port.  The second part explains how to do the setup.  The code has been tested on macOS and Ubuntu, with a pre-configured Ubuntu virtual machine available for download from [this Google Drive](https://drive.google.com/drive/folders/1jN6DjV-S9AAZPqaJNieDIQ9FXOE0bkLv).

# Using Lingua Franca with the nRF52+Buckler

This section assumes you are either using a preconfigured virtual machine or have followed the set up instructions below.

## Compile a Lingua Franca File

Plug the nRF52 board into the USB port of your machine.
In the directory where cloned the `lf-buckler` repository (e.g. `~/lf-bucker`):
```
lfc src/BucklerLED.lf 
```
If your nRF52 does not have the Buckler board mounted on it, then:
```
lfc src/BuiltInLED.lf
```
These two programs flash three LEDs on the Buckler board and the nRF52, respectively.
The second program also toggles a fourth LED when you push Button 1 on the board.

You can equivalently compile these programs from within Visual Studio Code or the Epoch IDE using their own built-in mechanisms.

You should see something like this. Code will be auto flashed onto the board if is connected.

```
Generating code for: file:/Users/eal/git/lf-buckler/src/BucklerLED.lf
******** mode: STANDALONE
******** generated sources: /Users/eal/git/lf-buckler/src-gen/BucklerLED
******** Using 1 threads to compile the program.
--- Current working directory: /Users/eal/git/lf-buckler/src
--- Executing command: ../scripts/build_nrf.sh BucklerLED.lf
starting NRF generation script into /Users/eal/git/lf-buckler/src-gen/BucklerLED
pwd is /Users/eal/git/lf-buckler/src
Created /Users/eal/git/lf-buckler/src-gen/BucklerLED/Makefile
BUILD OPTIONS:
  Version     c869
  Chip        nrf52832
  RAM         64 kB
  FLASH       512 kB
  SDK         15
  SoftDevice  s132 6.1.1
  Board       Buckler_revC
 
  CC        BucklerLED.c

... possibly a few warnings ...

  LD        _build/BucklerLED_sdk15_s132.elf
 HEX        _build/BucklerLED_sdk15_s132.hex
 BIN        _build/BucklerLED_sdk15_s132.hex
 SIZE       _build/BucklerLED_sdk15_s132.elf
   text	   data	    bss	    dec	    hex	filename
  78760	   2680	   4612	  86052	  15024	_build/BucklerLED_sdk15_s132.elf
**** To flash the code onto the device: cd /Users/eal/git/lf-buckler/src-gen/BucklerLED; make flash
Compiled binary is in /Users/eal/git/lf-buckler/bin
Code generation finished.
```

## Understanding the Lingua Franca Code

In the `lf-buckler/src` directory are a number of Lingua Franca files (with `.lf` extensions).
These are best viewed within VS Code or Epoch, but any text editor will do.
VS Code and Epoch both provide syntax highlighting, which makes the code easier to read,
and, most importantly, an automatically generated diagram that shows you the structure of the code.

The following example programs will help you understand how to write programs for these boards:

* `BucklerLED.lf`: A reactor the blinks LEDs on the Buckler board. Import this reactor into other programs to have a distinctive flashing pattern that tells you that your program is alive.
* `BuiltInLED.lf`: Similar to `BucklerLED.lf`, but using only the nRF52 board, without the Buckler daughter card. Also, this program shows you how to react to button pushes on the board.

# Setting Up Your Machine

The following instructions will guide you to set up your macOS or Ubuntu machine to use Lingua Franca to program the nRF52 board with or without the Berkeley Buckler daughter card. The installation requires sudo permissions on the machines. These instructions can be used to create or update a virtual machine image.

### Java Installation
Java 17 or above is required for Lingua Franca code generation. On Ubuntu:

```
sudo apt update && sudo apt upgrade -y
sudo apt-get install openjdk-17-jre
sudo apt-get install openjdk-17-jdk
```

On macOS, you can [download from Oracle](https://www.oracle.com/java/technologies/downloads/#jdk17-mac) or [OpenJDK](https://openjdk.org).

### Lingua Franca Compiler

[Download the Lingua Franca compiler, VS Code extension, or Epoch IDE](https://www.lf-lang.org/download) (Integrated Development Environment).
For convenient access to the command-line compiler, add it to PATH. There are many ways to do this. The instructions below use `~/.bashrc`.
Note that macOS uses `~/.bash_profile` instead of `~/.bashrc`.
```
vim ~/.bashrc
```
Add the following to the botton of the file. The run the source command to load into current shell instance.
```
export PATH="$HOME/lfc_0.3.0/bin:$PATH"
source ~/.bashrc
```
Replace the above `$HOME/lfc_0.3.0` wiht the actual path to the version you downloaded.

## Clone this Repository

```
$ git clone https://github.com/icyphy/lf-buckler.git
$ git submodule update --init --recursive
```

## Install Cross-Compilation and Loading Tools

In order to get code compiling and loading over JTAG, you'll need at least two tools. Not required if using Lab VM.

* [JLinkExe](https://www.segger.com/downloads/jlink). You want to the "J-Link Software and Documentation Pack". There are various packages available depending on operating system.

  Ubuntu (64-bit):
  ```
  $ apt-get install wget gdebi
  $ wget https://www.segger.com/downloads/jlink/JLink_Linux_x86_64.deb 
  $ sudo dpkg -i JLink_Linux_x86_64.deb
  ```

* **arm-none-eabi-gcc** is the cross-compiler version of GCC for building embedded ARM code.

  MacOS:
  ```
  $ brew tap ARMmbed/homebrew-formulae && brew update && brew install arm-none-eabi-gcc
  ```

  Ubuntu:
  ```
  $ sudo apt-get update -y && sudo apt-get install -y gcc-arm-none-eabi
  ```

## Details

* The [Berkeley Buckler board git repository](https://github.com/lab11/buckler) is included as a submodule. That repo includes a (rather large) nrf52x-base submodule and has all the hardware and software designs for the board. 

* [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain). The "AArch32 bare-metal target (arm-none-eabi)" is the relevant toolchain for this board. You probably don't need this.

