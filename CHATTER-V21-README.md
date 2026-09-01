# Meshtastic for CircuitMess Chatter V2.1

Experimental Meshtastic firmware port for the **CircuitMess Chatter green PCB V2.1**, based on **Meshtastic firmware 2.7.26**.

This project adds working support for the Chatter V2.1 hardware, including its ST7735S display, LLCC68 LoRa radio, physical 12-key keyboard, battery monitoring, and Bluetooth connectivity.

## Status

Tested on two physical CircuitMess Chatter green PCB V2.1 devices.

### Working

- Meshtastic boots and operates normally
- LLCC68 LoRa radio
- Bluetooth LE pairing and communication
- BLE reconnection after power cycle
- ST7735S 128x160 color display
- Physical 12-key keyboard
- Multi-tap text entry
- Multi-tap visual feedback
- On-screen text entry
- Battery voltage and percentage
- Display brightness control
- Screen color selection
- Persistent brightness and screen color settings
- Normal Meshtastic messaging

## Hardware

### Display

| Function | GPIO |
| --- | ---: |
| TFT CS | 15 |
| TFT DC | 33 |
| TFT Reset | 13 |
| TFT SCLK | 27 |
| TFT MOSI | 26 |
| Backlight | 32 |

Display: ST7735S, 128x160

The display uses VSPI and the backlight on GPIO 32 is active-low.

### LoRa

| Function | GPIO |
| --- | ---: |
| SCK | 16 |
| MISO | 17 |
| MOSI | 5 |
| CS | 14 |
| DIO1 | 18 |
| BUSY | 4 |

Radio: LLCC68

The LoRa radio uses a dedicated HSPI bus.

### Keyboard

| Function | GPIO |
| --- | ---: |
| Load | 21 |
| Clock | 22 |
| Data | 23 |

The physical 12-key keypad supports multi-tap text entry with a 1-second selection timeout.

### Battery

Battery ADC: GPIO 36

The Chatter V2.1 uses three AAA batteries.

### Buzzer

Buzzer: GPIO 19

Buzzer support is currently limited because of an LEDC resource conflict.

## Building

The custom PlatformIO environment is:

`chatter2-v21`

Build with:

    pio run -e chatter2-v21

The application binary is generated at:

    .pio/build/chatter2-v21/firmware-chatter2-v21-2.7.26.54e0d8d.bin

## Flashing

The application can be flashed with esptool.

Example:

    python -m esptool --chip esp32 --port COM12 --baud 460800 write_flash 0x10000 .pio/build/chatter2-v21/firmware-chatter2-v21-2.7.26.54e0d8d.bin

Replace `COM12` with the appropriate serial port.

## Bluetooth / Meshtastic 2.8 Investigation

Initial Chatter V2.1 development was performed against Meshtastic 2.8.

Both physical test devices experienced repeatable ESP32 Bluetooth controller crashes under 2.8, including:

    ASSERT_ERR(0), in ld_acl.c at line 7098

The crashes persisted after testing smaller BLE MTUs and disabling the external-notification buzzer.

Testing was therefore moved to Meshtastic 2.7.26.

During 2.7 testing, an additional issue was identified involving incompatible NimBLE bond data remaining in NVS from firmware using a newer NimBLE implementation.

The Chatter V2.1 variant includes a targeted patch that validates stored NimBLE bond-data sizes and performs a one-time cleanup of the `nimble_bond` NVS namespace when necessary.

After this change, BLE pairing, traffic, power cycling, and reconnection have been stable during testing.

## Chatter-Specific Changes

The port currently includes:

- New `variants/esp32/chatter2-v21` hardware variant
- ST7735S display configuration
- Dedicated HSPI implementation for the LLCC68 radio
- Chatter V2.1 battery ADC configuration
- Physical keyboard support and Chatter key mapping
- Multi-tap keyboard visual feedback
- Physical keyboard integration with Meshtastic text-entry UI
- Text-entry timeout/cancel ownership fix
- Chatter support for native Meshtastic brightness controls
- Chatter support for native Meshtastic Screen Color controls
- NimBLE NVS bond-data compatibility handling

## Known Issues / Work in Progress

- Buzzer support needs additional work because GPIO 19 can encounter LEDC resource conflicts.
- GPS is disabled. GPS probing caused memory/heap problems during testing.
- Meshtastic 2.8 is not currently used because of the repeatable BLE controller crash described above.
- Additional long-range LoRa testing is still planned.
- This port has currently been tested specifically on the **CircuitMess Chatter green PCB V2.1**.

## Upstream

This project is based on the Meshtastic firmware project.

The goal of this repository is to develop and test CircuitMess Chatter V2.1 support and, where appropriate, work with the Meshtastic project to determine how the hardware support could be upstreamed.

## Disclaimer

This is experimental community work and is not an official CircuitMess or Meshtastic firmware release.
