
// Pete Wentworth cspwcspw@gmail.com.  Oct 2018

// Get my OV7670 camera to deliver frames to my ILI9341 TFT LCD.
// Released under Apache License 2.0

//******** The pins and mapping.  **********
#define TX2   17
#define RX2   16

HardwareSerial Serial2(2);

#define VP    36
#define VN    39

int lcdDataBus[] = {18, 4, 19, 21, 22, 23, 2, 15};  // D7, D6, D5, D4, D3, D2, D1, D0


void busPinMode(int newMode)
{
  for (int i = 0; i < 8; i++) {
    pinMode(lcdDataBus[i], newMode);
  }
  char *modenames[] = {"0", "INPUT", "OUTPUT"};
  //Serial.printf("Bus pins now set to %s\n", modenames[newMode]);
}


#define LCD_WR   25       // When it goes high, the LCD latches bits from the output pin
#define LCD_DC   27       // HIGH means LCD interprets bits as data, LOW means the bits are a command.
#define LCD_RD   12
#define LCD_CS   14

// Uncomment this line if you have a GPIO pin for LCD_RESET
// Consider tying this pin to EN so that the LCD resets when the ESP32 resets.
// #define LCD_RESET   GPIO-pin-needs-to-be-assigned

#define triStatePin 33

#define triggerPin  32

#define SIOD_pin  05
#define SIOC_pin  00
#define VSYNC_pin VN
#define XCLK_pin  26

#include "LCD.h"
LCD *theLCD;

#include "CAM_OV7670.h"
pw_OV7670 *theCam;

#include "BasicTouch.h"

// The control logic is a state machine:
enum State {
  Lost,    // We don't know where we are, and need a VSYNC to synchronize ourselves.
  Priming, // When we hit this state we'll prime the sink: e.g. send a frame header, open a file, set up LCD etc.
  Running, // Queueing blocks as they arrive in the interrupt handlier, and sinking the data in the main loop.
  Wrapup   // We got a VSYNC. We can wrap up the frame / close a file, restart the I2S engine, print stats, etc.
};
char *stateNames[] = {"Lost", "Priming", "Running", "Wrapup"};
State theState;


//******* The camera. SIOD and SIOC pins need pull-ups to Vcc(3.3v) (4.7k ohms seems OK) **********

// "countDown" is our "safe" time when there is no pixel data arriving from the camera.
long countDownEndTime = 0;

// If the camera enables the interrupt, this gets called on every VSYNC.
void IRAM_ATTR handleVSYNC(void* arg)
{
  uint32_t gpio_intr_status_l = 0;
  uint32_t gpio_intr_status_h = 0;

  gpio_intr_status_l = GPIO.status;
  gpio_intr_status_h = GPIO.status1.val;
  GPIO.status_w1tc = gpio_intr_status_l;       //Clear intr for gpio0-gpio31
  GPIO.status1_w1tc.val = gpio_intr_status_h;  //Clear intr for gpio32-39

  switch (theState) {
    case Lost:
      theState = Priming;  // The main loop can start preparing for the next frame.
      break;

    case Priming:
      theState = Lost;
      break;

    case Running:
      theState = Wrapup;  // Tell the main loop we're at the end of a frame.
      // And set a deadline for how long the main loop has before more camera pixels arrive.
      countDownEndTime = micros() + theCam->countDownDuration_micros;
      break;

    case Wrapup:
      theState = Lost;
      break;
  }
}

void reclaim_JTAG_pins()
{ // https://www.esp32.com/viewtopic.php?t=2687
  // At reset, these pins (12,13,14,15) are configured for JTAG function.
  // You need to change function back to GPIO in the IO MUX to make the pins work as GPIOs.
  // If you use GPIO driver (include "driver/gpio.h", not "rom/gpio.h"),
  // it will configure the pin as GPIO for you, once you call gpio_config to configure the pin.
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[12], PIN_FUNC_GPIO);
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[13], PIN_FUNC_GPIO);
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[14], PIN_FUNC_GPIO);
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[15], PIN_FUNC_GPIO);
}

int LCD_x0, LCD_y0;  // Origin of where to setup LCD for data

void tftReleaseBus() {
  digitalWrite(LCD_CS, HIGH);  // Prevent LCD seeing noise on the bus switchover...
  busPinMode(INPUT);   // Should this rather be "high impedance"
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(triStatePin, LOW);  // give bus to camera
  digitalWrite(LCD_CS, LOW);  // Bring LCD back online ...
}

void tftClaimBus() {
  digitalWrite(LCD_CS, HIGH);  // Prevent LCD seeing noise on the bus switchover...
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(triStatePin, HIGH); // give bus to TFT
  busPinMode(OUTPUT);
  digitalWrite(LCD_CS, LOW);  // Bring LCD back online ...
}

int testPattern = 0;  //  0 is normal, 1,2,3 are test patterns

void setup() {

  int serial1BaudRate = 115200 * 1;   // Normal console
  int serial2BaudRate = 115200 * 4;   // Image uploads

  Serial.begin(serial1BaudRate);
  Serial.printf("\nPete's CamToLCD, V2.0 (%s %s)\n%s\n", __DATE__, __TIME__, __FILE__);

  Serial2.begin(serial2BaudRate);  //, SERIAL_8N1, 16, 17);

  reclaim_JTAG_pins();

  pinMode(triStatePin, OUTPUT);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             pinMode(LCD_DC, OUTPUT);
  pinMode(LCD_WR, OUTPUT);
  pinMode(SIOD_pin, OUTPUT);
  pinMode(SIOC_pin, OUTPUT);
  pinMode(LCD_DC, OUTPUT);

  tftReleaseBus();

  theCam = new pw_OV7670(SIOD_pin, SIOC_pin, XCLK_pin, VSYNC_pin, handleVSYNC);
  theCam->start();
  for (int i = 0; i < 1; i++) {
    theCam->TestSuite("Setup");
  }
  theCam->setMode(2, true);
  LCD_x0 = (320 - theCam->xres) / 2;
  LCD_y0 = (240 - theCam->yres) / 2;

  for (int i = 0; i < 0; i++) {
    theCam->TestSuite("Setup");
  }
  theCam->testPattern(testPattern);
  theCam->stop();

  tftClaimBus();
  delay(1);
  theLCD = new LCD();

  theLCD->Setup();
  theLCD->TestSuite();

  theLCD->Address_set(true, LCD_x0, LCD_y0, LCD_x0 + theCam->xres - 1, LCD_y0 + theCam->yres - 1);
  theCam->start();
}


void changeCameraMode()
{
  digitalWrite(triggerPin, HIGH);
  tftClaimBus();
  theCam->softSleep(false);  // wake up camera before changing modes

  int nextMode = (theCam->mode + 1) % 3;
  theCam->setMode(nextMode, true);

  theCam->testPattern(testPattern);
  int colours[] = { YELLOW, MAGENTA, GREEN };
  theLCD->ClearScreen(colours[nextMode]);
  // Tell the LCD where to put the image on the screen.  Center it.
  LCD_x0 = (320 - theCam->xres) / 2; // this will need fixing when I buy a bigger screen.
  LCD_y0 = (240 - theCam->yres) / 2;
  theLCD->Address_set(true, LCD_x0, LCD_y0, LCD_x0 + theCam->xres - 1, LCD_y0 + theCam->yres - 1);
  theState = Lost;

  digitalWrite(triggerPin, LOW);
}

void changeCameraTestPattern()
{
  testPattern = (testPattern + 1) % 4;
  theCam->testPattern(testPattern);
  Serial.printf("Test pattern %d\n", testPattern);
}

int framesGrabbed = 0;
long lastFpsTime = 0;
int fpsReportAfterFrames = 100;


void loop(void)
{
  switch (theState)
  {
    case Lost: break;     // Just be patient and wait to synchronize with the start of the next frame.

    case Priming:        // Set up to sink the next frame.
      theLCD->Address_set(true, LCD_x0, LCD_y0, LCD_x0 + theCam->xres - 1, LCD_y0 + theCam->yres - 1);
      theState = Running;
      tftReleaseBus();
      break;

    case Running:  break;

    case Wrapup:  // Finalize things before the next frame begins.
      // The CountDown time after a VSYNC and before the next scanline
      // is our biggest chunk of idle time,
      // so we do a bit of other housekeeping here too, like polling for user input.
      {
        tftClaimBus();
        if (++framesGrabbed % fpsReportAfterFrames == 0) {
          long timeNow = millis();
          double eSecs = timeNow / 1000.0;
          double fps = (fpsReportAfterFrames * 1000.0) / (timeNow - lastFpsTime);
          Serial.printf("%6.1fs: %dx%d; Last %d frames @ %.1f FPS; Pattern %d;\n",
                        eSecs, theCam->xres, theCam->yres, fpsReportAfterFrames, fps, testPattern);
          lastFpsTime = timeNow;
        }
        
        int touchedPos = getTouchPos();
        putTouchPinsBackLikeTheyShouldBe();
        if (touchedPos >= 0) {  // User interaction
          int touchY = (touchedPos >> 16) & 0xFFFF;
          int touchX = touchedPos & 0xFFFF;

          // Three zones of touch do three different things ...
          if (touchX + touchY < 100) { // Somewhere near the top left corner
            changeCameraMode();
          }
          else if (touchX + touchY > 460) { // Somewhere near the bottom right
            changeCameraTestPattern();
          }
          else {          // interpret it as a request to upload a snapshot...
            byte buf[640];
            int width = theCam->xres;
            int height = theCam->yres;
            int yLimit =  LCD_y0 + height;

            // Send a synchronization header for the receiver, 3 SYN bytes followed by
            // one byte camera mode
            const int SYN = 0x16;
            byte hdr[4] = { SYN, SYN, SYN, (byte) theCam->mode };
            Serial2.write(hdr, 4);

            // Now send each scan line.
            for (int y = 0; y < height; y++)
            {
              theLCD->fetchScanline(buf, LCD_x0, LCD_y0 + y, width);
              int sent = Serial2.write(buf, width * 2);
              theLCD->eraseScanline(LCD_x0, LCD_y0 + y, width);
              delay(4); // Slow down sender to allow slow serial port to catch up...
            }
          }
          theState = Lost;  // Go back for a fresh resync
        }
        else {
          if (micros() < countDownEndTime) {  // Did we beat our deadline?
            theState = Priming;
          }
          else {                          // Oops, we'll have to miss a frame and wait for the next vsync
            theState = Lost;
          }
        }
      }
      break;
  }
}

