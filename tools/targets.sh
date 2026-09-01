# Target definitions for ./gg. Sourced by bash 3.2 (macOS default), so no arrays/maps.
# This file is the single source of truth for board options; docs refer to it.

STM32_CORE_ID="STMicroelectronics:stm32"
STM32_CORE_VERSION="2.9.0"
STM32_INDEX_URL="https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json"

ESP32_CORE_ID="esp32:esp32"
ESP32_CORE_VERSION="2.0.17"
ESP32_INDEX_URL="https://espressif.github.io/arduino-esp32/package_esp32_index.json"

ALL_TARGETS="controller screen"

# Extra property the Arduino IDE always passed.
COMMON_BUILD_PROPS="build.warn_data_percentage=75"

# Sets T_* variables for the given target name. Returns 1 for an unknown target.
target_load() {
  case "$1" in
    controller)
      T_NAME=controller
      T_DESC="STM32F411CEU6 Black Pill: sensors, pump, boiler"
      T_SKETCH=gaggiano-controller-v1
      T_CORE="$STM32_CORE_ID@$STM32_CORE_VERSION"
      T_FQBN="STMicroelectronics:stm32:GenF4:pnum=GENERIC_F411CEUX,xserial=generic,usb=CDCgen,xusb=FS,opt=osstd,dbg=none,rtlib=nanofp,upload_method=dfuMethod"
      T_BAUD=9600
      T_FLASH=dfu
      # USB identities (vid:pid, lower-case hex). run = our firmware's CDC port,
      # dfu = the ROM bootloader (no serial port), stlink = probe if ever attached.
      T_USB="run=0483:5740 dfu=0483:df11 stlink=0483:3748 stlink=0483:374b"
      ;;
    screen)
      T_NAME=screen
      T_DESC="ESP32-S3 Sunton 8048S043 4.3in touch display"
      T_SKETCH=gaggiano-v2
      T_CORE="$ESP32_CORE_ID@$ESP32_CORE_VERSION"
      T_FQBN="esp32:esp32:esp32s3:UploadSpeed=460800,USBMode=hwcdc,CDCOnBoot=default,MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=default,DebugLevel=none,PSRAM=opi,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default"
      T_BAUD=115200
      T_FLASH=esptool
      # run = the board's CH340 UART bridge. native = the S3's own USB, unused by
      # this build (CDCOnBoot off) but listed so `detect` can name it.
      T_USB="run=1a86:7523 native=303a:1001"
      ;;
    *)
      return 1
      ;;
  esac
}
