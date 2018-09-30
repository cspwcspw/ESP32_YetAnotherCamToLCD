

#pragma once

// Pete Wentworth cspwcspw@gmail.com.  Oct 2018
// Released under Apache License 2.0

// Parts of this code (the LCD setup, drawing rectangles, clearing the screen) started from code at
// https://www.banggood.com/2_4-Inch-TFT-LCD-Shield-240320-Touch-Board-Display-Module-With-Touch-Pen-For-Arduino-UNO-p-1171082.html
// LCD_Touch\2.4inch_Arduino_HX8347G_V1.0\Arduino Demo_ArduinoUNO&Mega2560\Example01-Simple test\Simple test for UNO\_HX8347_uno
// His "Simple test" for the UNO has no dependecies on any libraries (i.e. doesn't use AdaFruit, etc.)
// It just clears the screen with some colours, and draws some rectangles.

// My contribution was to clean it up, port it to the ESP32, wrap it into a class, 
// find some optimizations, and figure out how to read data back from the framebuffer memory.
// Pete Wentworth, September 2018.

// This LCD is not a serial SPI LCD - it is a parallel, 8-bit-at-a-time model.


#define LCD_WR_SETOUTPUT() pinMode(LCD_WR, OUTPUT)
#define LCD_WR_HIGH()      GPIO.out_w1ts=(1 << LCD_WR)
#define LCD_WR_LOW()       GPIO.out_w1tc=(1 << LCD_WR)

#define LCD_DC_SETOUTPUT() pinMode(LCD_DC, OUTPUT)
#define LCD_DC_HIGH()      GPIO.out_w1ts=(1 << LCD_DC)
#define LCD_DC_LOW()       GPIO.out_w1tc=(1 << LCD_DC)

#ifdef LCD_RD
#define LCD_RD_SETOUTPUT()    pinMode(LCD_RD, OUTPUT)
#define LCD_RD_HIGH()         GPIO.out_w1ts=(1 << LCD_RD)
#define LCD_RD_LOW()          GPIO.out_w1tc=(1 << LCD_RD)
#else
#define LCD_RD_SETOUTPUT()
#define LCD_RD_HIGH()
#define LCD_RD_LOW()
#endif


#ifdef LCD_CS
#define LCD_CS_SETOUTPUT()    pinMode(LCD_CS, OUTPUT)
#define LCD_CS_HIGH()         GPIO.out_w1ts=(1 << LCD_CS)
#define LCD_CS_LOW()          GPIO.out_w1tc=(1 << LCD_CS)
#else
#define LCD_CS_SETOUTPUT()
#define LCD_CS_HIGH()
#define LCD_CS_LOW()
#endif

#ifdef LCD_RESET
#define LCD_RESET_SETOUTPUT()    pinMode(LCD_RESET, OUTPUT)
#define LCD_RESET_HIGH(dly)      { GPIO.out_w1ts=(1 << LCD_RESET); delay(dly); }
#define LCD_RESET_LOW(dly)       { GPIO.out_w1tc=(1 << LCD_RESET); delay(dly); }
#else
#define LCD_RESET_SETOUTPUT()
#define LCD_RESET_HIGH(dly)
#define LCD_RESET_LOW(dly)
#endif



const int RED  = 0xf800;
const int GREEN = 0x07E0;
const int BLUE = 0x001F;
const int YELLOW = 0xFFE0;
const int MAGENTA = 0xF81F;
const int CYAN = 0x07FF;
const int BLACK = 0x0000;
const int WHITE = 0xFFFF;

int backGroundColours[] = {RED, GREEN, BLUE};
int mixedColours[] = {YELLOW, MAGENTA, CYAN};


class LCD
{
  public:

    unsigned int outputMaskMap[256];  // Each 8-bit byte (used as an index) has a 32-bit mask to set the appropriate GPIO lines
    unsigned int busPinsLowMask;      // This has clear bits for all the lcdDataBus pins and also for the LCD_WR pin,
    //                                   which we set low at the same time.

    // Key optimization idea: Ahead of time, prepare output masks for all possible byte values. Store them in outputMaskMap.
    //  To write a byte to the LCD, set all databus GPIOs low (also include LCD_WR) by writing a fixed mask to w1tc.
    //  Then use the byte to be written as an index into outputMaskMap, and write that mask to w1ts to set the GPIOs.
    //  Then latch that data onto the LCD device by making LCD_WR go high.
    // #define Write_Byte(d) { GPIO.out_w1tc=busPinsLowMask; NOP(); GPIO.out_w1ts=outputMaskMap[d]; NOP(); LCD_WR_HIGH(); NOP(); }
      #define Write_Byte(d) { GPIO.out_w1tc=busPinsLowMask; GPIO.out_w1ts=outputMaskMap[d]; LCD_WR_HIGH(); }
//#define Write_Byte(d) { GPIO.out_w1tc=busPinsLowMask; GPIO.out_w1ts=outputMaskMap[d]; LCD_WR_HIGH(); LCD_WR_LOW();}

    void Write_Command(unsigned char d)
    {
      LCD_DC_LOW();    // Here comes a command
      Write_Byte(d);
    }

    void Write_Data(unsigned char d)
    {
      LCD_DC_HIGH();    // Here comes data
      Write_Byte(d);
    }


    // Where on the LCD do you want the data to go / come from?
    void Address_set(bool writeMode, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
    {
      Write_Command(0x2a);
      Write_Data(x1 >> 8);
      Write_Data(x1);
      Write_Data(x2 >> 8);
      Write_Data(x2);
      Write_Command(0x2b);
      Write_Data(y1 >> 8);
      Write_Data(y1);
      Write_Data(y2 >> 8);
      Write_Data(y2);
      if (writeMode) {
        Write_Command(0x2c); // Memory Write
      }
      else {
        Write_Command(0x2e); // Memory Read
      }
      LCD_WR_LOW();
      LCD_DC_HIGH();
    }

    void Init(void)
    {
      LCD_RESET_HIGH(5);
      LCD_RESET_LOW(15);
      LCD_RESET_HIGH(15);

      LCD_CS_LOW();

      Write_Command(0xCB);
      Write_Data(0x39);
      Write_Data(0x2C);
      Write_Data(0x00);
      Write_Data(0x34);
      Write_Data(0x02);

      Write_Command(0xCF);
      Write_Data(0x00);
      Write_Data(0XC1);
      Write_Data(0X30);

      Write_Command(0xE8);
      Write_Data(0x85);
      Write_Data(0x00);
      Write_Data(0x78);

      Write_Command(0xEA);
      Write_Data(0x00);
      Write_Data(0x00);

      Write_Command(0xED);
      Write_Data(0x64);
      Write_Data(0x03);
      Write_Data(0X12);
      Write_Data(0X81);

      Write_Command(0xF7);
      Write_Data(0x20);

      Write_Command(0xC0);    //Power control
      Write_Data(0x23);       //VRH[5:0]

      Write_Command(0xC1);    //Power control
      Write_Data(0x10);       //SAP[2:0];BT[3:0]

      Write_Command(0xC5);    //VCM control
      Write_Data(0x3e);       //Contrast
      Write_Data(0x28);

      Write_Command(0xC7);    //VCM control2
      Write_Data(0x86);       //--

      Write_Command(0x36);    // Memory Access Control
      // I like the settings for a forward-facing camera, so text appears normally on the LCD.
      // Write_Data(0x28);    // Exchange rows and cols for landscape
      Write_Data(0xE8);       // Exchange rows and cols for landscape, and flip X (which is now Y) and flip Y


      Write_Command(0x3A);
      Write_Data(0x55);

      Write_Command(0xB1);
      Write_Data(0x00);
      Write_Data(0x18);

      Write_Command(0xB6);    // Display Function Control
      Write_Data(0x08);

      Write_Data(0x82);
      Write_Data(0x27);

      Write_Command(0x11);    //Exit Sleep
      delay(120);

      Write_Command(0x29);    //Display on
      Write_Command(0x2c);
    }


    void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c)
    {
      unsigned int i, j;
      Write_Command(0x02c); //write_memory_start

      LCD_CS_LOW();
      l = l + x;
      Address_set(true, x, y, l, y);
      j = l * 2;
      for (i = 1; i <= j; i++)
      {
        Write_Data(c);
      }
      LCD_CS_HIGH();
    }

    void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c)
    {
      unsigned int i, j;
      Write_Command(0x02c); //write_memory_start

      LCD_CS_LOW();
      l = l + y;
      Address_set(true, x, y, x, l);
      j = l * 2;
      for (i = 1; i <= j; i++)
      {
        Write_Data(c);
      }
      LCD_CS_HIGH();
    }

    void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c)
    {
      H_line(x  , y  , w, c);
      H_line(x  , y + h, w, c);
      V_line(x  , y  , h, c);
      V_line(x + w, y  , h, c);
    }

    void ClearScreen(unsigned int backColour)
    {
      // digitalWrite(triggerPin, HIGH);
      LCD_CS_LOW();
      // Assume we're in landscape
      Address_set(true, 0, 0, 320, 240);

      unsigned char u = backColour >> 8;
      unsigned char v = backColour & 0xFF;
      LCD_DC_HIGH();
      for (int i = 0; i < 240 * 320; i++)
      {
        Write_Byte(u);
        Write_Byte(v);
      }
      LCD_CS_HIGH();
    }

    void createOutputMasks()
    {
      busPinsLowMask = (1 << LCD_WR);
      for (int i = 0; i < 256; i++) { // possible byte values
        int v = i;
        for (int k = 7; k >= 0; k--) { // look at each bit in turn, starting at least significant
          int pinNum = lcdDataBus[k];
          unsigned int pinBit = (0x0001 << pinNum);
          if (v & 0x01) {
            outputMaskMap[i] |=  pinBit;
          }
          else {
            if (i == 0) {
              busPinsLowMask |= pinBit;
              //   Serial.printf("Setting pinbit %08x so busPinsLowMask is %08x\n", pinBit, busPinsLowMask);
            }
          }
          v >>= 1;
        }
      }
      // Let's print the masks, in hex.
      /*
        Serial.println("Table of pins to make high for each byte value (in Hex):");
        for (int row = 0; row < 8; row++)
        {
          char sep = ' ';
          for (int col = 0; col < 8; col++) {
            int indx = row * 8 + col;
            Serial.printf("%c %08x", sep, outputMaskMap[indx]);
            sep = ',';
          }
          Serial.println();
        }
        Serial.println("-----------------");
      */
    }

    unsigned char read_Byte()   // From the next position in the framebuffer
    {
      unsigned char b = 0;
      LCD_WR_HIGH();
      LCD_RD_LOW();
      LCD_RD_HIGH();
      //LCD_WR_LOW();

      for (int i = 0; i < 8; i++) {
        b = (b << 1) | digitalRead(lcdDataBus[i]);
      }

      return b;
    }

    int repackToRGB565(int r, int g, int b)
    {
      // Pack RGB888 into RGB565
      int rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
      return rgb565;

    }

    int getPixel(int x, int y)   // return a 16-bit RGB565 encoding of pixel at x,y
    {
      LCD_CS_LOW();
      Address_set(false, x, y, x, y);
      LCD_DC_HIGH();
      busPinMode(INPUT);
      byte dummy = read_Byte();
      byte r = read_Byte();
      byte g = read_Byte();
      byte b = read_Byte();
      busPinMode(OUTPUT);
      LCD_CS_HIGH();
      return repackToRGB565(r, g, b);
    }

    void fetchScanline(byte *buf, int x, int y, int width)
    {
      LCD_CS_LOW();

      Address_set(false, x, y, x + width-1, y);
      LCD_DC_HIGH();
      busPinMode(INPUT);

      byte dummy = read_Byte();

      for (int i = 0; i < width * 2; i += 2)
      {
        byte r = read_Byte();
        byte g = read_Byte();
        byte b = read_Byte();
        int v = repackToRGB565(r, g, b);
        buf[i] = (v >> 8) & 0xFF;
        buf[i + 1] = v & 0xFF;
      }
      busPinMode(OUTPUT);
      LCD_CS_HIGH();
    }

    void writeScanline(byte *buf, int x, int y, int width)
    {
      LCD_CS_LOW();
      Address_set(true, x, y, x + width - 1, y);
      LCD_DC_HIGH();
      for (int i = 0; i < width * 2; i++)
      {
        Write_Byte(buf[i]);
      }
      LCD_CS_HIGH();
    }

    void eraseScanline(int x, int y, int width)
    {
      LCD_CS_LOW();
      Address_set(true, x, y, x + width - 1, y);

      unsigned char u = CYAN >> 8;
      unsigned char v = CYAN & 0xFF;
      LCD_DC_HIGH();
      for (int i = 0; i < width; i++)
      {
        Write_Byte(u);
        Write_Byte(v);
      }
      LCD_CS_HIGH();
    }


    void snapShotTest(int x0, int y0, int width, int height) {

      if (width > 320) width = 320;
      byte buf[640];
      int yLimit = y0 + height;
      for (int y = 0; y < height; y++)
      {
        fetchScanline(buf, x0, y0 + y, width);
        writeScanline(buf, x0, y0 + ((y + height / 2) % height), width);
        eraseScanline(x0, y0 + y, width);
        delay(20);
      }
    }


    void Setup()
    {
      Serial.println("Setting up LCD data and control pins");
      for (int p = 0; p < 8; p++)
      {
        pinMode(lcdDataBus[p], OUTPUT);
      }

      createOutputMasks();

      LCD_RD_SETOUTPUT();
      LCD_RD_HIGH();

      LCD_WR_SETOUTPUT();
      LCD_WR_HIGH();

      LCD_DC_SETOUTPUT();
      LCD_DC_HIGH();

      LCD_CS_SETOUTPUT();
      LCD_CS_HIGH();

      LCD_RESET_SETOUTPUT();
      LCD_RESET_HIGH(15);

      Init();
    }

    void fillRegion(int x, int y, int w, int h, unsigned int colour)
    {
      LCD_CS_LOW();
      // Assume we're in landscape
      Address_set(true, x, y, x + w - 1, y + h - 1);

      unsigned char u = colour >> 8;
      unsigned char v = colour & 0xFF;
      LCD_DC_HIGH();
      for (int i = 0; i < w * h; i++)
      {
        Write_Byte(u);
        Write_Byte(v);
      }
      LCD_CS_HIGH();
    }

    void TestPattern()
    {
      fillRegion(0, 0, 320, 48, RED);
      fillRegion(0, 48, 320, 48, GREEN);
      fillRegion(0, 96, 320, 48, BLUE);
      fillRegion(0, 144, 320, 48, BLACK);
      fillRegion(0, 196, 320, 48, WHITE);

    }

    void TestSuite()
    { // It's not really a test suite, it just does some colourful things.
      int dly = 512;
      long t0, t1;
      for (int i = 0; i < 8; i++)
      {
        if (dly > 0 && i > 0) {
          delay(dly);
          dly >>= 1;
        }
        t0 = micros();
        int backColour = backGroundColours[i % 3];
        ClearScreen(backColour);
        t1 = micros();
        if (i >= 3)
        {
          // Now draw some rectangles on the background.
          for (int i = 0; i < 100; i++)
          {
            Rect(random(300), random(300), random(300), random(300), random(65535));
            // rectangle at x, y, width, height, color
          }
        }
      }
      Serial.printf("%d microsecs to set all screen pixels\n", t1 - t0);
      float fps = 1000000.0 / (t1 - t0);
      float dataRate = ((fps / (1024.0 * 1024.0)) * 240 * 320 * 2);
      Serial.printf("Theoretical max LCD FPS=%.2f.\nClearScreen() data rate=%.2f MB per sec.\n", fps, dataRate);
    }
};

