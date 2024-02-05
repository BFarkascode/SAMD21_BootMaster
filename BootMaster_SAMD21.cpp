/*
BootMaster for SAMD21 and an attached SDcard. Written for an Adafruit Adalogger.
Code reads in a hex code from an SDcard, then publishes it to Serial1. Hex data package is limited to 24 kB
Code is directly compatible with the "STM32_Bootloader".

*/
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <stdlib.h>

#define LED_GREEN (8u)              //Green LED is on pin 8

//------------For hex readout-----------//
const int SD_CS = 4;                //SD card CS is on D4 on an Adalogger
const uint16_t code_size = 24576;   //size of the code buffer and the app. Theoretical limit is 32kB, which is the SRAM size of the micro on the Adalogger. Actual limit is less than that...
uint8_t code_byte_buf[code_size];   //binary data converted from char
uint8_t char_line_buf[36];          //we read the file in as ASCII char: 32 data char plus 2 chesksum char plus a linebreak
uint8_t ofs_buf[8];
uint8_t *code_byte_buf_ptr;         //data buffer pointer
uint16_t code_byte_cnt;
uint16_t full_page_padding_cnt;     //we work in pages with the "STM32_Bootloader", as such we must add padding to our code to reach a full page in case it does not
uint16_t erase_byte_no;
uint16_t code_tx_delay;             //the amount of time we need to wait for the serial communication to conclude
                                    //Note: Serial1 is not blocking!
File hex_file;
char file_name[] = "app.hex";     //<<<<<<<<<<<<<<<<<<< set the hex file name here <<<<<<<<<<<<<<<
                                    //Note: file name must be less than 8 bytes otherwise the SD open can not deal with it
bool red_LED_on;            //we use the red LED on the Adalogger as feedback
bool green_LED_on;          //we use the green LED on the Adalogger as feedback

//------------For hex readout-----------//

//------------For reply reception (if active)-----------//

int page_counter;                   //this is currently not in use

//------------For reply reception (if active)-----------//

//-------------------Function prototypes----------------//

void openFileOnSD(void);
void ReadFromFile(void);

//-------------------Function prototypes----------------//

//LED feedback
//If both RED and GREEN are HIGH after initialization, the master is ready.
//RED is LOW if:
  //1) There was something wrong with reading in the hex file. This happens during initialization.
  //2) The master is in programming mode.

//GREEN is LOW if:
  //1) There was something wrong with reading in the hex file. This happens during initialization.
  //2) The master has published the app code and must be reset.

void setup() {

  //variables
  full_page_padding_cnt = 0;
  red_LED_on = true;
  green_LED_on = false;
  delay(1);

  // start serial port at 9600 bps:
  Serial.begin(9600);

  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB
  }

  Serial.println("Starting");
  Serial.println("SD card hex file processing...");
  Serial.print("Created: ");
  Serial.print(__TIME__);
  Serial.print(", ");
  Serial.println(__DATE__);

  //turn off L13    -   Red LED by the USB connector
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  //turn off green LED
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, LOW);


  for (int i = 0; i < code_size; i++) code_byte_buf[i] = (uint8_t)0x0;          //we wipe the code buffer

  openFileOnSD();                                                               //we load all the hex code data into the code buffer

}

void loop() {


  //--------Provide error feedback on hex file availability  using LEDs-----//
  //Note: lack of SDcard or hex file does not prevent external control

  if (red_LED_on == true) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }


  if (green_LED_on == true) {
    digitalWrite(LED_GREEN, HIGH);
  } else {
    digitalWrite(LED_GREEN, LOW);
  }


  //--------Provide error feedback on hex file availability using LEDs-----//

  //--------Send commands-------//

  Serial1.begin(57600);                                                           //Note: baud rate and parity control must match between the master and the bootloader!
  Serial.print("Decimal code for the command byte: ");                            //we write the command byte into the serial monitor

  while (Serial.available() == 0) {
  }

  uint8_t Command_Byte_Dec = Serial.parseInt();

  Serial.println(Command_Byte_Dec);

  switch (Command_Byte_Dec) {
    case 0:  //Turn on external control mode in the DTRTA
      Serial.println(" ");
      Serial.println("Activating device external control... ");
      Serial.println(" ");
      Serial1.write(0xF0);
      Serial1.write(0xF0);
      Serial1.write(0xC3); 
      delay(6);                                                                   //delay for 3 commands plus idle time
      break;

    case 1:  //Jump to app in DTRTA
      Serial.println(" ");
      Serial.println("Activating APP... ");
      Serial.println(" ");
      Serial1.write(0xF0);
      Serial1.write(0xF0);
      Serial1.write(0xAA);
      delay(6);
      break;

    case 2:  //Engage programmer mode on the DTRTA
      Serial.println(" ");
      Serial.println("Switching device to programmer mode... ");
      Serial1.write(0xF0);
      Serial1.write(0xF0);
      Serial1.write(0xBB);
      delay(6);
      red_LED_on = false;
      Serial.println(" ");
      break;

    case 3:  //Reboot DTRTA
      Serial.println(" ");
      Serial.println("Rebooting connected device... ");
      Serial.println(" ");
      Serial1.write(0xF0);
      Serial1.write(0xF0);
      Serial1.write(0xCC);  //00001111
      delay(6);
      break;

    case 4:  //send the processed code over the UART, byte by byte

      if (green_LED_on == true) {
        //------------Publish code------------//
        code_byte_buf_ptr = &code_byte_buf[0];
        for (int i = 0; i < (code_byte_cnt); i++) {
          Serial1.write(*code_byte_buf_ptr);
          code_byte_buf_ptr++;
        }
        //------------Publish code------------//

        //------------Code padding------------//
        //we add padding to reach full page with our code. This is necessarry since we are working only (!) with full pages of data burst on the device!!!
        full_page_padding_cnt = ((code_byte_cnt / 256) + 1) * 256;
        for (int i = code_byte_cnt; i < full_page_padding_cnt; i++) Serial1.write((uint8_t)0x0);
        //------------Code padding------------//

        //------------Delay calculations------------//
        //Note: Serial1 is non-blocking, so we need to wait until is has finished
        code_tx_delay = ((full_page_padding_cnt) / 256) * 136;
        delay(code_tx_delay);                                                      //delay value for 9600 baud rate - 4 pages (8x32 bytes a page), 1 page 136 ms, n bytes will be (n / 256) * 136
        Serial.println(" ");
        Serial.print("The delay will be: ");
        Serial.println(code_tx_delay);
        //------------Delay calculations------------//

        //------------Confirmation from device------------//

        page_counter = Serial1.read();                                            //this line is for the possible device feedback. Currently not in use
                                                                                  //Note: due to some unknown black magic in the compiler, removing or commenting out the line above blocks the code from processing the hex file...
        Serial.print("The app has been replaced... ");

        Serial.println(" ");
        //------------Confirmation from device------------//
      } else {
        Serial.println("App update failed...");
        Serial.println("Please reset the master device...");
      }

      green_LED_on = false;
      delay(1);

      break;

    case 5:  //wipe the app memory section

      //------------Wipe app memory------------//
      erase_byte_no = code_size;                                                  //we wipe only the size of app memory that correlates to the pre-defined code buffer

      for (int i = 0; i < erase_byte_no; i++) {                                   //we wipe the entire app memory section
        Serial1.write((uint8_t)0x0);
      }
      //------------Wipe app memory------------//

      //------------Delay calculations------------//
      code_tx_delay = (erase_byte_no / 256) * 136;
      delay(code_tx_delay);                                                       //delay value for 9600 baud rate - 4 pages (8x32 bytes a page), 1 page 136 ms, n bytes will be (n / 256) * 136

      Serial.print("The app has been erased... ");
      Serial.println(" ");
      //------------Delay calculations------------//

      break;

    default:
      Serial.println("Error! Command not found");
      break;
  }

  Serial1.end();

  //--------Send commands-------//
}


//-----------------------Test SD card and file availability-------------------------------//
void openFileOnSD(void) {
  Serial.println("Initializing SD card... ");
  // see if the card is present and can be initialized:
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR: card failed, or not present...");
    Serial.println(" ");
  }
  Serial.println("Card initialized.");
  hex_file = SD.open(file_name, FILE_READ);
  if (hex_file) {
    Serial.println("HEX file found...");
    delay(500);
    Serial.println("Processing hex file... ");
    ReadFromFile();
  } else {
    Serial.println("ERROR: failed opening file...");
    red_LED_on = false;
    Serial.println(" ");
  }
  hex_file.close();
}

//-----------------------Test SD card and file availability-------------------------------//


//-----------------------Read file-------------------------------//
void ReadFromFile(void) {
  File myFile = SD.open(file_name);
  uint16_t char_cnt = 0;                                                           //how many characters we have read in
  uint8_t offset_cnt = 0;                                                          //where are we in the offset area
  uint8_t char_line_cnt = 0;                                                       //where are we in the line of characters
  uint8_t *char_line_buf_ptr;                                                      //pointer for the line of character buffer
  uint8_t *ofs_buf_ptr;                                                            //offset buffer pointer
  uint8_t file_start_line_cnt = 0;                                                 //counter to skip first lines of code
  bool data_package_start = false;                                                 //we define the start of the data package when we have found the second ":" character
  code_byte_buf_ptr = &code_byte_buf[0];                                           //we place the pointer to the start of the binary data buffer
  char_line_buf_ptr = &char_line_buf[0];                                           //we place the pointer to the start of the character buffer

  while (!((ofs_buf[6] == 48) & (ofs_buf[7] == 53))) {                             //while we are NOT at the end of the file!!!!!

    if (myFile.available() > 0) {
      uint8_t byte_read = myFile.read();
      //Note: we read in every elements of the hex code as a byte, although it technically is only half a byte of data (example: one byte of 0xD2 will be read in as character "D" and "2")
      //Note: we convert each line of read-in char as 16 bytes of binary data below
      if (offset_cnt == 0) {

        if (byte_read == 58) {                                                     //if we see the ":" char and it is the first element in the line
                                                                                   //Note: the readout value will be decimal ASCII of the character
          ofs_buf_ptr = &ofs_buf[0];                                               //we reset the offset buffer pointer
          char_line_buf_ptr = &char_line_buf[0];                                   //we reset the char pointer

          //----------Skip start of the file--------------//
          if (file_start_line_cnt == 0) {
            offset_cnt = 14;                                                       //we skip the first line of the hex file completely
            file_start_line_cnt++;
            char_line_cnt = 0;                                                     //we reset the char counter
          } else if (file_start_line_cnt == 1) {
            offset_cnt = 8;                                                        //we skip the offset section of the second line
            data_package_start = true;                                             //we define the data package start from the xeond ":" that we found
                                                                                   //Note: linebreak characters ARE NOT SEEN in editing but will be read in as characters by the microcontroller. We need to skip them
            file_start_line_cnt++;
            char_line_cnt = 0;                                                     //we reset the char counter
                                                                                   //Note: on the second line, we don't have the checksum yet to remove from the char counter!
          //----------Skip start of the file--------------//

          //----------What shall we do at the beginning of every line?--------------//
          } else {
            offset_cnt = 8;                                                        //we should skip the offset section by setting the offset counter
            char_cnt -= 4;                                                         //we should remove the checksum from our char count

            //----------Convert the line of 32 ASCII char into 16 bytes of binary data--------------//
            for (int i = 0; i < (char_line_cnt - 4); i += 2) {
              uint8_t upper_half_byte, lower_half_byte;

              //below we convert an ASCII character in the buffer into an actual integer
              //Note: data is actually captured by the file.read command as ASCII characters...
              //one byte of data is stored as two ASCII characters, so 4 bytes instead of two

              if (char_line_buf[i] == 48) upper_half_byte = 0x0;
              else if (char_line_buf[i] == 49) upper_half_byte = 0x1;
              else if (char_line_buf[i] == 50) upper_half_byte = 0x2;
              else if (char_line_buf[i] == 51) upper_half_byte = 0x3;
              else if (char_line_buf[i] == 52) upper_half_byte = 0x4;
              else if (char_line_buf[i] == 53) upper_half_byte = 0x5;
              else if (char_line_buf[i] == 54) upper_half_byte = 0x6;
              else if (char_line_buf[i] == 55) upper_half_byte = 0x7;
              else if (char_line_buf[i] == 56) upper_half_byte = 0x8;
              else if (char_line_buf[i] == 57) upper_half_byte = 0x9;
              else if (char_line_buf[i] == 65) upper_half_byte = 0xA;
              else if (char_line_buf[i] == 66) upper_half_byte = 0xB;
              else if (char_line_buf[i] == 67) upper_half_byte = 0xC;
              else if (char_line_buf[i] == 68) upper_half_byte = 0xD;
              else if (char_line_buf[i] == 69) upper_half_byte = 0xE;
              else if (char_line_buf[i] == 70) upper_half_byte = 0xF;
              else upper_half_byte = 0x0;

              if (char_line_buf[i + 1] == 48) lower_half_byte = 0x0;
              else if (char_line_buf[i + 1] == 49) lower_half_byte = 0x1;
              else if (char_line_buf[i + 1] == 50) lower_half_byte = 0x2;
              else if (char_line_buf[i + 1] == 51) lower_half_byte = 0x3;
              else if (char_line_buf[i + 1] == 52) lower_half_byte = 0x4;
              else if (char_line_buf[i + 1] == 53) lower_half_byte = 0x5;
              else if (char_line_buf[i + 1] == 54) lower_half_byte = 0x6;
              else if (char_line_buf[i + 1] == 55) lower_half_byte = 0x7;
              else if (char_line_buf[i + 1] == 56) lower_half_byte = 0x8;
              else if (char_line_buf[i + 1] == 57) lower_half_byte = 0x9;
              else if (char_line_buf[i + 1] == 65) lower_half_byte = 0xA;
              else if (char_line_buf[i + 1] == 66) lower_half_byte = 0xB;
              else if (char_line_buf[i + 1] == 67) lower_half_byte = 0xC;
              else if (char_line_buf[i + 1] == 68) lower_half_byte = 0xD;
              else if (char_line_buf[i + 1] == 69) lower_half_byte = 0xE;
              else if (char_line_buf[i + 1] == 70) lower_half_byte = 0xF;
              else lower_half_byte = 0x0;

              *code_byte_buf_ptr = (upper_half_byte * 16) + lower_half_byte;  //we load the binary byte to our code buffer
              code_byte_buf_ptr++;
            }
            char_line_cnt = 0;  //we reset the char counter
            //----------Convert the line of 32 ASCII char into 16 bytes of binary data--------------//

          }

          //----------What shall we do at the beginning of every line?--------------//

        //----------What shall we do when we are IN a line?--------------//
        } else {  //if not
          //----------Read-in data--------------//
          //Note: data will be stored in the line char buffer, which will hold 32 char of data, 2 char of checksum and 2 char of line break (all in ASCII!!!!!)
          if (data_package_start == true) {
            *char_line_buf_ptr = byte_read;                                           //we publish through the ptr the read value to the char buf
            char_line_cnt++;
            char_cnt++;                                                               //each char is half byte of code
            char_line_buf_ptr++;                                                      //and step the char pointer
          } else {
            //do nothing
          }
          //----------Read-in data--------------//
        }
        //----------What shall we do when we are IN a line?--------------//

      //----------What shall we do in the offset area?--------------//
      } else {
        //----------Read-in offset--------------//
        *ofs_buf_ptr = byte_read;  //we read in the offset separately
        ofs_buf_ptr++;
        offset_cnt--;
        //----------Read-in offset--------------//
      }
      //----------What shall we do in the offset area?--------------//

    //----------What shall we do once the file has no more char?--------------//
    } else {
      //do nothing
    }
    //----------What shall we do once the file has no more char?--------------//

  }

  myFile.close();

  //----------What shall we do once the file has been read in?--------------//
  code_byte_cnt = char_cnt / 2;                                                         //we have half as many data bytes as char
  ofs_buf[7] = 48;  //we remove the block
  Serial.print("The hex file has been read in as ");
  Serial.print(code_byte_cnt);
  Serial.println(" bytes of data...");
  Serial.println(" ");
  green_LED_on = true;
  //----------What shall we do once the file has been read in?--------------//
}
#ifdef debug_functions
void print_data_buf() {                                                                 //print out the data buffer

  for (int i = 0; i < code_byte_cnt; i++) Serial.print(code_byte_buf[i], HEX);          //Note: we print in HEX, so "0x00" will be only be just one "0"
  Serial.println(" ");
  Serial.println(" ");
}

void print_ofs_buf() {                                                                  //print out the offset buffer

  for (int i = 0; i < 8; i++) Serial.write(ofs_buf[i]);
  Serial.println(" ");
}
#endif

//-----------------------Read file-------------------------------//
