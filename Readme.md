Raspberry Pi 4 Custom UEFI Firmware 
===================================
___
<img  src="edk2-tools\ressources\EFIPIbooting.jpg" class="transparent"> 
###### This is a port of 64-bit Tiano Core UEFI firmware for the Raspberry Pi 4B platform.

  <img src="https://avatars2.githubusercontent.com/u/352162?s=200&amp;v=4" class="transparent" height="42"> [TianoCore UEFI Tiano Core 64-bit firmware](https://github.com/tianocore) <img src="https://avatars2.githubusercontent.com/u/352162?s=200&amp;v=4" class="transparent" height="42">

###### 64-bit TF-A + UEFI Raspberry Pi 4 based on the BCM2711 SoC

* Do __NOT__ expect this firmware to be fully functional when it comes to supporting your ARM64 OS of choice 

* you are using an __EXPERIMENTAL__ firmware, it is expected to be working and that may have to wait for a more stable release

## Installation

* Download the latest archive from the [Releases](https://github.com/pftf/RPi4/releases) repository.

* Create an SD card in `MBR` mode with a single partition of type `0x0c` (`FAT32 LBA`)
  or `0x0e` (`FAT16 LBA`). Then format this partition to `FAT`.

  __Note:__ Do not try to use `GPT` for the partition scheme or `0xef` (`EFI System
  Partition`)  for the type, as these are unsupported by the Raspberry Pi bootloader(s).

* Extract all the files from the archive onto the partition you created above.  
  Note that outside of this `Readme.md`, which you can safely remove, you should not
  change the name of the extracted files and directories.

## Usage

Insert the SD card/plug the USB drive and power up your Raspberry Pi. You should see a multicoloured screen (which indicates that the CPU-embedded bootloader is reading the data from the SD/USB partition) and then the Raspberry Pi black and white logo once the UEFI firmware is ready.

### efi/boot/bootaa64.efi  UEFI BOOTLOADER
At this stage, you can press <kbd>Esc</kbd> to enter the firmware setup, <kbd>F1</kbd> to launch the UEFI Shell, or, provided you also have an UEFI bootloader on the SD card or on a USB drive in `efi/boot/bootaa64.efi`, you can let the UEFI system run that (which will be the default if no action is taken).

## Building

Build instructions from the top level edk2-platforms Readme.md apply.
Booting the firmware

Format a uSD card as FAT16 or FAT32
Copy the generated RPI_EFI.fd firmware onto the partition Download and copy the following files from https://github.com/raspberrypi/firmware/tree/master/boot

#####UART ON (or..)
```c
    bcm2711-rpi-4-b.dtb
    fixup4.dat
    start4.elf
    overlays/miniuart-bt.dbto
```
#####(Optional) Bluetooth OFF
```c
    bcm2711-rpi-4-b.dtb
    fixup4.dat
    start4.elf
    overlays/disable-bt.dtbo 
```
  Create a config.txt with the following content:
```c
arm_64bit=1
enable_uart=1
enable_gic=1
armstub=RPI_EFI.fd
disable_commandline_tags=2
device_tree_address=0x1f0000
device_tree_end=0x200000
```

PL011 instead of the miniUART, you can add the lines:
```c
dtoverlay=miniuart-bt
```

__Note:__ doing so requires miniuart-bt.dbto to have been copied into an overlays/ directory on the SD card. Alternatively, you may use disable-bt instead of miniuart-bt if you don't require Bluetooth.

####Insert the SD card and power up the Pi

___

#####ARM Trusted Firmware (TF-A)
The TF-A binary was compiled from the latest TF-A release.
For more details on the TF-A compilation, see the relevant Readme in the TrustedFirmware/ directory from edk2-non-osi.

#####Device Tree

By default, UEFI will use the device tree loaded by the VideoCore firmware. This depends on the model/variant, and relies on the presence on specific files on your boot media. E.g.:

    bcm2711-rpi-4-b.dtb (for Pi 4B)

You can override the DTB and provide a custom one. Copy the relevant .dtb into the root of the SD or USB, and then edit your config.txt so that it looks like:

```c
disable_commandline_tags=2
device_tree_address=0x1f0000
device_tree_end=0x200000
device_tree=your_fdt_file.dtb
```
__Note:__ the address range must be```[0x1f0000:0x200000]``` dtoverlay and dtparam parameters are also supported.

####Custom bootargs

This firmware will honor the command line passed by the GPU via cmdline.txt.
Note, that the ultimate contents of /chosen/bootargs are a combination of several pieces:
GPU-passed hardware configuration.
Additional boot options passed via cmdline.txt.

---

####NVRAM

#####The Raspberry Pi has no NVRAM.

NVRAM is emulated, with the non-volatile store backed by the UEFI image itself. This means that any changes made in UEFI proper are persisted, but changes made from within an Operating System aren't.

####RTC

#####The Rasberry Pi has no RTC.

An RtcEpochSeconds NVRAM variable is used to store the boot time. This should allow you to set whatever date/time you want using the Shell date and time commands. While in UEFI or HLOS, the time will tick forward. RtcEpochSeconds is not updated on reboots.

#####USB
This UEFI supports both the USB3 xHCI ports (front ports), and the Pi 3-style DesignWare USB2 controller via the Type-C port (host only).

#####Type-C port:

SB1 BBB mass storage devices untested (USB2 and USB3 devices are fine).
Some USB1 CBI (e.g. UFI floppy) mass storage devices may not work.

#####ACPI
OS support for ACPI description of Pi-specific devices is still in development. Not all functionality may be available.

___

#####Missing Functionality
 Network booting via onboard NIC.
 SPCR hardcodes type to PL011, and thus will not expose correct (miniUART) UART if DT overlays to switch UART are used on Pi 4B.

---

## Additional Notes

The firmware provided in the zip archive is the `RELEASE` version but you can also find a `DEBUG` build of the firmware in the [AppVeyor artifacts](https://ci.appveyor.com/project/pbatard/RPi4/build/artifacts).

The provided firmwares should be able to auto-detect the UART being used (PL011 or mini UART) according to whether `config.txt` contains the relevant overlay or not. The default baudrate for serial I/O is `115200` and the console device to use under Linux is either `/dev/ttyAMA0` when using PL011 or `/dev/ttyS0` when using miniUART.

At the moment, the published firmwares default to enforcing ACPI as well as a 3 GB RAM limit, which is done to ensure Linux boot. These settings can be changed by going to `Device Manager` &rarr; `Raspberry Pi Configuration` &rarr; `Advanced Configuration`.

![](https://raspiproject.altervista.org/wp-content/uploads/2018/05/Windows10-Raspberry-Pi.jpg)

#### License
The firmware (`RPI_EFI.fd`) is licensed under the current EDK2 license, which is [BSD-3-Clause](https://github.com/ARM-software/arm-trusted-firmware/blob/master/license.rst).

---