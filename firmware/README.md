# AGPKeebo Firmware

The firmware of AGPKeebo is written with QMK and VIA. 

## Setup with VIA

In this folder you find the `via.json` which is necessary for VIA to detect AGPKeebo. In VIA (v3), switch to the design tab and load the JSON as the draft definition file. V2 should be disabled. Now the keyboard definition `AGP Keebo` should appear under `Shown Keyboard Definition` and you can use VIA to configure the keymaps.

## Initial Firmware Flash

If there is no QMK firmware on the board already, you must use STM32 flashing techniques. You can use the USB bootloader option by manually switch `STM32 boot` on the board (far right corner on the upper side of the board) and press `STM32 NRST` to restart. Now you can use STM32CubeIDE to flash firmware. Alternatively, use STM32CubeProgrammer with an ST-LINK debugger and connect it to the JTAG or USART connector on the board.

## Update Firmware

AGPKeeb is configured with magicboot option enabled. Hence, you only need to keep pressing `ESC` and plugin the keyboard. This starts the bootloader. From here on, you can use [QMK Toolbox](https://github.com/qmk/qmk_toolbox).