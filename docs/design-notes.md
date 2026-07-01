# Design Notes — ATtiny85 USB HID

## Overview
Minimal USB HID device based on ATtiny85 using V-USB software USB library.
Designed as a bare-metal C project, no Arduino or HAL abstractions.

## IC Selection — ATtiny85
- 8-pin SOIC package, 8KB flash, 512B SRAM
- No hardware USB peripheral — USB handled via V-USB in firmware
- Internal 16.5MHz PLL oscillator used — no external crystal needed
- Frees PB3/PB4 from XTAL duty for USB D+/D- lines

## Fuse Bits
- lfuse: 0xE1 — internal PLL clock 16.5MHz
- hfuse: 0xDD — RESET enabled, SPI programming on
- Program with: `avrdude -c usbasp -p attiny85 -U lfuse:w:0xE1:m -U hfuse:w:0xDD:m`

## USB Signal Path
- D- (USB pad 2) → R1 68Ω → PB3 (pin 2)
- D+ (USB pad 3) → R2 68Ω → PB4 (pin 3)
- Zener diodes D1/D2 3.6V clamp 5V GPIO swing to USB-safe 3.3V max
- R4 1.5kΩ pull-up on D+ for host device detection

## V-USB Configuration
In usbconfig.h set:
- USB_CFG_DPLUS_BIT 3 (PB3)
- USB_CFG_DMINUS_BIT 4 (PB4)

## PCB Design Decisions
- Board dimensions: 31.31×11.8mm — fits inside USB-A female socket
- PCB thickness: 0.8mm — required for USB-A insertion (standard is 1.6mm)
- All SMD components 0402 — fits tight boarLd dimensions
- Zeners in SOD-123 package
- Ground pour on F.Cu — improves signal integrity and simplifies routing
- Custom USB-FX footprint — PCB edge pads act as USB-A plug contact

## Bill of Materials
| Reference | Value | Package |
|-----------|-------|---------|
| U1 | ATtiny85-20SU | SOIC-8 |
| R1, R2 | 68Ω | 1206 |
| R3 | 1.5kΩ | 1206 |
| R4 | 1.5kΩ | 1206 |
| C1 | 100nF | 0805 |
| D1, D2 | 3.6V Zener | SOD-123 |
| D3 | LED | 0402 |
| U2 | USB-FX | Custom |

## Programming
Flash chip before soldering using USBasp programmer on breadboard.
No bootloader needed — pure bare metal.