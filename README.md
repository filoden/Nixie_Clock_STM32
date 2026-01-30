# Nixie_Clock_STM32
## Summary:
An STM32 MCU based Nixie-tube alarm clock 

Entirety of clock is custom designed and in development.
KiCAD, STM32cube, and Solidworks are the design software used in this project.


## Wiring/Programming Development:
  ### Functionality in development:
  * I2S audio communication
  * Driving 4 $\Omega$ speaker through the MAX98567A
  * SD audio file reading
  * 3V Battery Backup
  * Battery-power mode
  * RTC timer setup
  * Oscillator
  
  
  ### Functionality tested in trial version:
  * Capacitive touch sensor
  
  ### Wiring/Programming Finalized:
  * 5V-12V boost converter
  * SPI/I2C Communication
  * Rotary encoders and push switches
  * 5V-3.3V LDO

## PCB Development:
* V1 PCB completed mostly to become more familiar with the PCB layout and design (many errors made haha)
* V2 PCB to be finalized pending completion of wiring/programming development

## Enclosure Development:
### Summary of current state:
* enclosure consist primarily of wood
* Enclosure is to consist of two pieces glued together
* Pieces will be mostly symmetrical and capable of being machine from a single piece of wood on a CNC
  * (For best results mill one thick piece of lumber along the grain for two mirrored grains (see: book-matching wood)
* Both the front and back edges will use an alumninum edge-banding (laser-cut) for strength and aesthetics, which is glued in place
* Back plate will user a thicker aluminum to hold rotary encoders, and USB-C power input
* Front plate is a piece of 1/8inch glass (waterjet cut, although could be made by hand) with a grey tint applied
### Items to be completed:
* Mounting for rotary encoders, USB-C input, PCB
  *   Vector files for backplate
* Enclosure feet
* Vector files for glass face and edge banding
* BOM
