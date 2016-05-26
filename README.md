Arduino KeyBus
==============

This repository contains two different sketches able to read the DSC KeyBus wire protocol.

Messages are printed out to Serial1 (the sketches were developed on an Atmega32U4).  Reading messages sent by the 'panel' (DSC parlance, refers to the alarm mainboard) and by the 'slave' devices (keypads, zone expanders(?)) is supported.

Write support is possible and may be added later. The Atmel AVR C implementation contains write support.

KeyBus-IRQ
----------
IRQ based implementation. IRQ is fired for each clock cycle and bits are captured in the ISR. Timers are used to dtermine end of message.

KeyBus-Loop
-------------------
This uses a tight main-loop to try to minimise bit errors arising from IRQs firing during processing. This implementation performed slightly better on an 8MHz Atmega32U4.


----------