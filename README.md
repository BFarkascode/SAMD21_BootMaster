# SAMD21_BootMaster
Arduino core-based master device to process a hex file on an SDcard and publish it to UART.

## General description
This is the master controller for the bootloader I wrote for the STM32_L0 device.

I wrote this code in the Arduino environment since I decided to rely on the Arduino "SD" library extensively. The reason for that is that I wanted to avoid writing a designated (and verified) driver for the SD card plus the related file management due to time restraints. Also, I deemed to project to be a one-time use solution that won't be updated or modified much in the future. These days, I mostly am working on STM32 after all...

Regarding the code, it takes in a hex file - mind, the name of the file is hard wired in the code, see line "char file_name[] = "app.hex";" - and then, after removing the start line/byte count/offset/record type, the checksum and any additional characters (e.g., line breaks), it stores the raw data section of the hex file locally in RAM.

The code is originally written for Adafruit Adalogger (M0 Feather) though any similar board running a SAMD21 (like Arduino Zero) should work. (Of note, not all SAMD21s have the same memory allocation...)

Of note, I have provided both the ".ino" as well as the ".cpp" vesion of the code to this repository.

## Previous relevant projects
The following projects should be checked:
-	STM32_Bootloader

## To read
I recommend getting familiar with the Arduino "SD" library and the various commands it uses. For details, cheack here: https://www.arduino.cc/reference/en/libraries/sd/

## Particularities
Here I want to touch upon the modifications that I had to implement on the projects I mentioned above to make them work together.

### Arduino moves in mysterious ways...
Arduino is absolutely great for what it is, but it is simply not a professional environment. It allows basic functions very well (and multiple advanced ones too), but the moment someone takes a step off the beaten path, things turn very messy very fast. A little bit like HAL in STM32-s, but much-much worse.

This often means that one has to rely on some "creative" solutions to coding where lines and elements seemingly unrelated to each other must be kept in the code just to not break it.

Similarly annoying is how the libraries are written to cater to user-friendliness, not utility. A rather annoying naming convention that comes form the SD library for instance is that one MUST have a file name that is less than 8 characters, otherwise the library will not be able to open the file. Changing this proved to be a can of worms I chose not to open...

At any rate, I very much dislike this approach of "unknown voodoo black magic" to make things work and will attempt to port the master - and a proper SD library - to bare metal STM32 or SAMD21 once time allows it.

### ASCI/char based coding philosophy
As mentioned above, the Arduino environment is very heavily built around the idea of user-friendliness. This means that the entire philosophy of the libraries is built on the "char" type, a type that is just a uint8_t formulated into readable letters using ASCII. We publish chars, we read chars. Mind, char "0" - a one byte information - will be stored as "0x30", i.e. the ASCII coded version of "0" - which will be two bytes of information. Chars can also be "unseen" elements which are rather frustrating to deal with (for example, every line in the hex file ends with a line break character that are not seen in editors, but will totally be there when you read in the line using the Arduino library).

All in all, purely binary numbers can not be directly extracted from the Arduino libraries. They must be converted manually from ASCII and - often times - converted back to ASCII when published. This must be kept in mind if we intend to work with purely binary values (as I personally like to).

### SAMD21G18
I am using an Adafruit Adalogger for the project which runs a SAMD21G18 micro. This micro has 32 kB of RAM which puts the upper theoretical limit of the app code to 32 kB. In reality, that is not feasible since the code needs some RAM to run its calculations. Thus, the code is hard wired to manage, at maximum, 24 kB of data/"app size". This maximum "app size" must be adjusted to the microcontroller that is being used.

The other particularity is that I am using Serial1 as the "output" of the master. This is the pre-set additional USART serial on the Adalogger and it may not exist out of the box on simialr boards.

## User guide
The bootmaster is rather simple regarding use. One needs to provide it a hex file with a specific name - "app.hex" in the shared version of the code - and then follow the instructions published on the master's serial port. The Arduino IDE is perfectly adequate for this.

One sends commands to the master by writing command numbers to the serial port and then publishing them to the master. The commands themselves are self-explanatory and match the "expected" commands that were defined within the STM32_L0 bootloader.

The commands are:

0 - Turn on external control (necessary to "hijack" the attention of the bootloader we wish to communicate with, the bootloader on the STM32_L0 will ignore everything unless we start with this command!)

1 - Tell the bootloader to jump to the app

2 - Tell the bootloader to engage programming mode (turns off red LED on the master)

3 - Tell the bootloader to reboot the device (after which one needs to again send "0" to take control of the device)

4 - Publish the app code to the device (will only do anything IF we are in programming mode first)

5 - Wipe the app code from device (will only do anything IF we are in programming mode first)

Of note, I also made use of the green and red LEDs on the Adalogger to provide additional visual feedback over what is the master doing. If both red and green are HIGH after initialization, the master is ready. If red is LOW, we whether had an error during initialization, or we have the master in programming mode. The green LED goes LOW if we had a problem with the hex file or the master has published the app code and must be reset. All in all, the combination of the two LEDs gives us 4 different scenarios which cover the 4 different states the master might be during use.
