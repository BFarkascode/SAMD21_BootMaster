# SAMD21_BootMaster
Arduino-based master device code to process a hex file on an SDcard and publish it to UART. Code written for Adafruit Adalogger (M0 Feather).

## General description
This is the master controller for the bootloader I wrote for the STM32_L0 device.

## Previous relevant projects
The following projects should be checked:
-	STM32_Bootloader

## To read
I recommend getting familiar with the Arduino "SD" library.

## Particularities
Here I want to touch upon the modifications that I had to implement on the projects I mentioned above to make them work together.

### Arduino moves in misterious ways...
Arduino is great for what it is, but it is not a professional environment. It allows basic functions very well, but the moment someone takes a step off the beaten path, things turn very messy very fast. A little bit like HAL, but much-much worse.
This demands some "creative" solutions to coding where lines and elements seemingly unrelated to each other must be kept in the code just to not break it. I very much dislike this approach of "black magic" and will attempt to port the master to a bare metal environment once time allows it. 

### ASCI/char based coding philosophy
The Arduino environment is very heavily built around the idea of user friendliness. This means that the entire philosophy of the libraries is built on the "char" type, a type that is just a uint8_t formulated into readable letters using ASCII. We publish chars, we read chars. Chars are usually letetrs and numbers, but can also be "unseen" elements which are rather frustrating to deal with (for example, every line in the hex file ends with a linebreak character that are not seen in editors, but will totally be there whenyou read in the line using the Arduino library).
All in all, purely binary numbers can not be extracted from the Arduino libraries. They must be converted manually from ASCII and - often times - converted back to ASCII when published.

## User guide
Let’s look at the code specifically written for this project!
