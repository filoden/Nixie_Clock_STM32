# Nixie_Clock_STM32

An STM32-based **Nixie-tube alarm clock** (HH:MM) with audio, SD storage, and a custom enclosure. 
Repository still in developement.

> ⚠️ **High Voltage Warning**
> Nixie tubes require a high-voltage supply (~170V for IN-12 used in this project). Treat the HV section as hazardous:

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
- Rotary encoders + push switches
- 5V → 3.3V LDO

---

## Hardware Overview (high level)

- **MCU:** STM32 (exact part depends on PCB revision)
- **Inputs/UI:** rotary encoder(s), push buttons, capacitive touch
- **Timekeeping:** STM32 RTC + external LSE crystal (or dedicated RTC module, depending on revision)
- **Storage:** microSD over SPI (FatFs)
- **Audio:** I2S to class-D amp (MAX98567A) → 4Ω speaker
- **Power:**
  - USB-C 5V input (mechanical + ESD considerations)
  - 3.3V rail for logic
  - HV rail for Nixie tubes (separate boost converter)

---

## Firmware Build / Flash (typical STM32 flow)

1. Open the firmware project in **STM32CubeIDE** (or your VSCode + CMake workflow if you use one).
2. Confirm CubeMX-generated peripherals match the board wiring (SPI for SD, I2S for audio, GPIO/EXTI for inputs).
3. Build + flash via **ST-LINK**.
4. Bring-up in this order:
   1) power rails → 2) clocks/RTC → 3) SD/FatFs → 4) display/HV → 5) audio → 6) UI

- PLEASE NOTE: for spi throughput to be able to supply i2s driver, a custom SD/SPI driver is used which is not yet included in the build. In the current configuration, SPI will not run. 
---

## PCB Development

- **V1 PCB:** learning spin (many layout mistakes)
- **V2 PCB:** pending completion of wiring/programming development - most up to date version located in current iteration.

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
- Mechanical mounting: rotary encoders, USB-C input, PCB
- Vector files: back plate, glass face, edge banding
- Enclosure feet
- Generate + maintain BOM

---

## BOM

A starter BOM is provided in `docs/BOM_starter.md` (or `bom.csv`) and is currently unpopulated.
