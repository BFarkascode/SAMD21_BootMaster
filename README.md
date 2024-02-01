# SAMD21_BootMaster
Arduino-based master device code to process a hex file on an SDcard and publish it to UART. Code written for Adafruit Adalogger (M0 Feather).

## General description
This is the master controller for the bootloader I wrote for the STM32_L0 device. It is using the Arduino "SD" library extensively since I wanted to avoid writing a designated driver for SD card and related file management due to time restraints (also I deemed to project to be a one-time use solution that won't be updated or modified much in the future).

The code takes in a hex file - name of the file is hard wired in the code - and then, after removing the offset, the checksum and the additional characters (line breaks, double dots), it stores the data section of the hex file locally in RAM.

## Previous relevant projects
The following projects should be checked:
-	STM32_Bootloader

## To read
I recommend getting familiar with the Arduino "SD" library.

## Particularities
Here I want to touch upon the modifications that I had to implement on the projects I mentioned above to make them work together.

### Arduino moves in mysterious ways...
Arduino is great for what it is, but it is not a professional environment. It allows basic functions very well, but the moment someone takes a step off the beaten path, things turn very messy very fast. A little bit like HAL, but much-much worse.
This demands some "creative" solutions to coding where lines and elements seemingly unrelated to each other must be kept in the code just to not break it. Similarly, a rather annoying naming convention that comes form the SD library: one MUST have a file name that is less than 8 characters, otherwise the library will not be able to open the file.

I very much dislike this approach of "unknown voodoo black magic" to make things work and will attempt to port the master to a bare metal environment once time allows it.

### ASCI/char based coding philosophy
The Arduino environment is very heavily built around the idea of user friendliness. This means that the entire philosophy of the libraries is built on the "char" type, a type that is just a uint8_t formulated into readable letters using ASCII. We publish chars, we read chars. Chars are usually letters and numbers, but can also be "unseen" elements which are rather frustrating to deal with (for example, every line in the hex file ends with a line break character that are not seen in editors, but will totally be there when you read in the line using the Arduino library).

All in all, purely binary numbers can not be extracted from the Arduino libraries. They must be converted manually from ASCII and - often times - converted back to ASCII when published.

### SAMD21G18
I am using an Adalogger for the project which runs a SAMD21G micro. This micro has 32 kB of RAM which puts the upper theoretical limit of the app code to 32 kB. In reality, that is not feasible since the code needs some RAM to run its calculations. Thus, the code is hard wired to manage , at maximum, 24 kB of data. This "code size" must be adjusted to the microcontroller that is being used.

The other particularity is that I am using Serial1 as the "output" of the master. This is the pre-set additional USART serial on the Adalogger and it does not exist out of the box, on, say, an Arduino UNO.

## User guide
The bootmaster is rather simple regarding use. One needs to provide it a hex file with a specific name - "blinky.hex" in the shared version of the code, though this can be changed - and then follow the instructions published on the master's serial port. The Arduino IDE is perfectly adequate for this.

One sends commands to the master by writing command numbers to the serial port and then sending them to the master. The commands themselves are self-explanatory and match the "expected" commands that were defined within the STM32_L0 bootloader.

The commands are:
0 - turn on external control (necessary to "hijack" the attention of the bootloader we wish to communicate with, the bootloader on the STM32_L0 will ignore everything unless we start with this command!)

1 - tell the bootloader to jump to the app

2 - tell the bootloader to engage programming mode (turns off red LED on the master)

3 - tell the bootloader to reboot the device (after which one needs to again send "0" to take control of the device)

4 - publish the app code to the device (will only do anything IF we are in programming mode first)

5 - wipe the app code from device (will only do anything IF we are in programming mode first)

Of note, I also made use of the green and red LEDs on the Adalogger to provide additional visual feedback over what is the master doing. If both red and green are HIGH after initialization, the master is ready. If red is LOW, we whether had an error during initialization, or we have the master in programming mode. The green LED goes LOW if we had a problem with the hex file or the master has published the app code and must be reset. All in all, the combination of the two LEDs gives us 4 different scenarios which cover the 4 different states the master might be during use.
