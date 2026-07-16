# Nixie_Clock_STM32

An STM32-based **Nixie-tube alarm clock** (HH:MM) with audio, SD storage, and a custom enclosure. 
Repository still in developement.

> ⚠️ **High Voltage Warning**
> Nixie tubes require a high-voltage supply (~170VDC for IN-12 used in this project). Basic HV safety knowledge is necessary to avoid injury and or death.

---

## Project Snapshot

**Design tools:** KiCad (PCB), STM32CubeMX / STM32CubeIDE (firmware), SolidWorks (enclosure)

**Repo structure**
- `Current Iteration/` – active hardware + firmware
  - `Current Iteration/Enclosure`
  - `Current Iteration/STM32 Project Files and Code`
  - `Current Iteration/Schematic Layout & PCB`
- `Old_Versions/` – previous spins / archived work

---

## Features

### Hardware / Electronics (status)
**In development**
- low power / battery-power mode
- Oscillator bring-up / stability checks

**Tested (prototype version)**
- Capacitive touch input


**Finalized**
- I2S audio communication
- 5V → 12V boost 
- SPI / I2C communication
- SD Card Interface
- Rotary encoders
- 5V → 3.3V LDO
- Audio output: driving a 4Ω speaker through the MAX98567A (except for bulk cap)
- SD audio file reading

  
---

## Hardware Overview (high level)

- **MCU:** STM32G0B1RETx6
- **UX/UI:** rotary encoder(s) (One encouder includes a push button), capacitive touch
- **Timekeeping:** STM32 RTC + external LSE oscillator
- **Storage:** microSD over SPI (FatFs)
- **Audio:** I2S to class-D amp (MAX98567A) → 4Ω speaker
- **Driving Nixie Tubes** STM MCU drives serial into a 12V level shifter which drives HV5622PG chips. These chips switch the tubes using an external 170V DC source.
- **Capacitive touch senor** Conductive edge banding surrounds large portions of controller. CAP1206 IC interprets changes in banding capacitance and communicates over I2C to MCU.
- **Power:**
  - USB-C 5V input (mechanical + ESD considerations)
  - 5V enters Linear low-dropout regulator -> 3.3V 
  - 3.3V rail for logic
  - HV rail for Nixie tubes (external boost converter)
  - Battery backup is used for logic, oscillator, amplifier, and capacitive touch sensor in the event of 5V loss
  - Battery and 3.3V power is controlled using NID5100, low-forward voltage drop diodes.
### Schematic Preview
![Schematic](https://github.com/filoden/Nixie_Clock_STM32/blob/main/Current%20Iteration/Schematic%20Layout%20%26%20PCB/Schematic_Preview.png)
### Block Diagram
![Block Diagram](https://github.com/filoden/Nixie_Clock_STM32/blob/main/Current%20Iteration/Block_Diagram.png)
#### Notes:
- All Power lines are written in red text
- Arrow direction denotes either flow of power or data (although JTAG should be bidirectional)
- Ground for IN12 Tubes is omitted
---

## Firmware 
### Build / Flash 
1. Open the firmware project in **STM32CubeIDE**.
2. Confirm CubeMX-generated peripherals match the board wiring (SPI for SD, I2S for audio, GPIO/EXTI for inputs). Make sure to include separate driver files for FATFS (Provided by CubeMX). 
4. Build + flash via **ST-LINK**.
5. To avoid frustration, bring-up in this order:
### Firmware Development (Status):
#### Integration:
**1. UI FSM**
- UI is structured as an FSM. Programmatically each input will trigger a user-input function for each respective input (knob A turns CC, knob B turns C, etc.. Each function consists of a long switch case which maps the appropriate response to an input for a given state. Relevant code is in UserInterface source and header files. FSM can be seen below.
<img src="https://github.com/filoden/Nixie_Clock_STM32/blob/main/Current%20Iteration/UI%20FSM.png" width="70%">
<img src="https://github.com/filoden/Nixie_Clock_STM32/blob/main/Current%20Iteration/UI_FSM_Cont.png" width="50%">



**Notes to come: 2. DMA music playing/3. SD card interface/4. Capacitive touch sensor/5. Music FIle Parsing**
#### In development:
**Nixie Driver Logix**
**2. Nixie driver control logic**
- Nixie tubes require 170VDC switching. Thankfully, Many old display tubes and other vintage electronics require high voltage DC. Since there is still demand for this technology an old (relatively speaking) serial to parallel high voltage switching IC known as the HV5622PG is still in production. This $8 chip can handle up to 250VDC and requires a ridiculous 12V logic level. The Nixie driver control logic tells this chip which digits to turn on. Importantly, it must also make sure that no more than one digit is driven at a time, as this can easily destroy the tube. Additionally, it is necesary to include PWM control to allow for dimming, and to be easily accessible by other parts of the code.
- Requirements: 
- - Checks to ensure there is no double digit driving
- - Dimming Control
- - Simple function call
**Low-Power (Blackout) Handling**
- Requirements:
- - Set Pins to LOW (Remove any Pullups as well): Nixie Driving GPIO, JTAG, SD_CARD, CAP1206, knobs B/C
- - Following Interfaces to remain as normal: MAX98357A
- - Sense Power-outage via ST pin on NID5100
- - Be able to play low power alarm song stored solely in on-chip memory
**New external LSE with RTC implementation**

---

## PCB Development
Current version consists of a single main board with three separate smaller boards (separated by user) for the rotary encoder circuitry.
- **V1 PCB:** learning spin (many layout mistakes)
- **V2 PCB:** More fully built out, includes all relevent IO.
- **V3 PCB:** pending completion of wiring/programming development - most up to date version located in current iteration. 
### To-do
- External LSE oscillator testing
- Fix bulk capacitor situation for audio amp
- Add v-groove for rotary encoders
### PCB Preview
**V3 PCB:**
 ![Current Revision](https://github.com/filoden/Nixie_Clock_STM32/blob/main/Current%20Iteration/Schematic%20Layout%20%26%20PCB/PCB_Layout_Preview.png)
---

## Enclosure Development

### Current plan
- Mostly **wood** enclosure in **two glued halves**
- Two halves are **mostly symmetric** and CNC-machinable from a single board  
  - Best results: mill one thick piece and split for mirrored grain (book-matching)
- **Front + back edges:** laser-cut **aluminum edge banding** (strength + aesthetics), glued in place
- **Back plate:** thicker aluminum for rotary encoders + USB-C inlet
- **Front plate:** 1/8" glass (waterjet preferred; hand-fab possible) with a grey tint

### To-do
- Vector files: back plate, glass face, edge banding
- Finalize BOM
- Investigate use of waterjet for glass face
### Enclosure Preview
![Enclosure Front](https://github.com/filoden/Nixie_Clock_STM32/blob/main/Current%20Iteration/Enclosure/Enclosure_Preview_Front.png)
![Enclosure Back](https://github.com/filoden/Nixie_Clock_STM32/blob/main/Current%20Iteration/Enclosure/Enclosure_Preview_Back.png)
---

## BOM
- PCB BOM currently matches latest revision
- Enclosure BOM is out of date

