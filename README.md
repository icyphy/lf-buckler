# Lingua Franca, nRF, and Buckler

This repo provides resources for using Lingua Franca to program an nRF52x Thread/BLE embedded board from Nordic Semiconductor with a Berkeley Buckler daughter card.

## Clone this Repository

```
git clone https://github.com/icyphy/lf-buckler.git
git submodule update --init --recursive
```

## Install Cross-Compilation and Loading Tools

In order to get code compiling and loading over JTAG, you'll need at least two tools.

* [JLinkExe](https://www.segger.com/downloads/jlink). You want to the "J-Link Software and Documentation Pack". There are various packages available depending on operating system.

* **arm-none-eabi-gcc** is the cross-compiler version of GCC for building embedded ARM code.

  MacOS:
  ```
  $ brew tap ARMmbed/homebrew-formulae && brew update && brew install arm-none-eabi-gcc
  ```

  Ubuntu:
  ```
  $ sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa && sudo apt update && sudo apt install gcc-arm-embedded
  ```

## Use lfc to compile the Lingua Franca file

```
cd src
lfc BucklerLED.lf 
```

You should see something like this:

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

## Details

* The [Berkeley Buckler board git repository](https://github.com/lab11/buckler) is included as a submodule. That repo includes a (rather large) nrf52x-base submodule and has all the hardware and software designs for the board. 

* [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain). The "AArch32 bare-metal target (arm-none-eabi)" is the relevant toolchain for this board. You probably don't need this.

