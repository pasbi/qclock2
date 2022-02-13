# qclock2
Code, Schematics, Photos of the EortUhr Hack


The WORTUHR is a clock that displays the current time in form of glowing words.
This is about the German variant.

Note: The instance I got is presumably a cheap copy of a more expensive design https://qlocktwo.com/.

// image of the package

## Original Design

There is a metal cover of size 20cm x 20cm.
25 German words are blanked out.
It has a DC 5V 1A power connector and two buttons (`+`) and (`-`).
The plastic housing is 4 cm thick and encloses the metal sheet completey.
Inside there is a PCB whith 25 LED-groups, one group per word, with varying number of LEDs per group.
There's further two groups with each five LED drivers (transitor and multiple resistors), one IC, a quartz and few other passive components.
After powering the the PCB, the clock displays 12:00 ("ES IST ZWÖLF UHR").
The time can be adjusted using the two buttons.

## Problem

The problem with the original product is that it does reset every time power is disconnected.
That makes it inconvenient to use occasionally.

## Idea

Implement some circuit that synchronizes the time via NTP.

