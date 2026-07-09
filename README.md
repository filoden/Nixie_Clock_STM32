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
- `Old_Versions/` – previous spins / archived work

---

## Features

### Firmware / Electronics (status)
**In development**
- low power / battery-power mode
- RTC timer setup
- Oscillator bring-up / stability checks

**Tested (prototype version)**
- I2S audio communication
- Capacitive touch input
- Audio output: driving a 4Ω speaker through the MAX98567A
- SD audio file reading

**Finalized (wiring/programming)**
- 5V → 12V boost (logic-side power rail)
- SPI / I2C communication
- Rotary encoders
- 5V → 3.3V LDO

---

## Hardware Overview (high level)

- **MCU:** STM32G0B1RETx6
- **UX/UI:** rotary encoder(s) (One encouder includes a push button), capacitive touch
- **Timekeeping:** STM32 RTC + external LSE crystal (or dedicated RTC module, depending on revision)
- **Storage:** microSD over SPI (FatFs)
- **Audio:** I2S to class-D amp (MAX98567A) → 4Ω speaker
- **Driving Nixie Tubes** STM MCU drives serial into a 12V level shifter which drives HV5622PG chips. These chips switch the tubes using an external 170V DC source.
- **Capacitive touch senor** Conductive edge banding surrounds large portions of controller. CAP1206 IC interprets changes in banding capacitance and communicates over I2C to MCU.
- **Power:**
  - USB-C 5V input (mechanical + ESD considerations)
  - 3.3V rail for logic
  - HV rail for Nixie tubes (external boost converter)
### Schematic Preview
![Schematic](https://github.com/filoden/Nixie_Clock_STM32/blob/main/Current%20Iteration/Schematic%20Layout%20%26%20PCB/Schematic_Layout_Preview.png)
### Block Diagram
![Block Diagram](https://github.com/filoden/Nixie_Clock_STM32/blob/main/Current%20Iteration/Block_Diagram.png)
#### Notes:
- All Power lines are written in red text
- Arrow direction denotes either flow of power or data (although JTAG should be bidirectional)
- Ground for IN12 Tubes is omitted
---

## Firmware Build / Flash 

1. Open the firmware project in **STM32CubeIDE** (or your VSCode + CMake workflow if you use one).
2. Confirm CubeMX-generated peripherals match the board wiring (SPI for SD, I2S for audio, GPIO/EXTI for inputs).
3. Ensure MIDWARE in additon to Middlwares are included in compilation
4. Build + flash via **ST-LINK**.
5. To avoid frustration, bring-up in this order:
   1) power rails → 2) clocks/RTC → 3) SD/FatFs → 4) display/HV → 5) audio → 6) UI

---

## PCB Development

- **V1 PCB:** learning spin (many layout mistakes)
- **V2 PCB:** pending completion of wiring/programming development - most up to date version located in current iteration. 
### To-do
- External oscillator addition and testing
### PCB Preview
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
- Investigate use of waterjet for custom glass face
### Enclosure Preview
![Enclosure Front](https://github.com/filoden/Nixie_Clock_STM32/blob/main/Current%20Iteration/Enclosure/Enclosure_Preview_Front.png)
![Enclosure Back](https://github.com/filoden/Nixie_Clock_STM32/blob/main/Current%20Iteration/Enclosure/Enclosure_Preview_Back.png)
---

## BOM
- PCB BOM currently matches latest revision
- Enclosure BOM is out of date

