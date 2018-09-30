
// Pete Wentworth cspwcspw@gmail.com.  October 2018
// Released under Apache License 2.0

// I don't suggest re-using this code - it was a quick and dirty "spike"
// to improve my understanding.  It is not encapsulated into a class, nor generalized, etc.

// The ideas are better polished in the MCUFriend_kbv library which is where I started from.

// Return values: -1 means no touch.  Otherwise high two bytes give Y, low two bytes give X.
int getTouchPos()
{
  int rpts = 3;  // how many readings to be averaged 

                           // On my touch shield, the four XP, XM, YP, YM touch pins
  int YP = LCD_CS;         // are shared with existing LCD_pins.  Here is the wiring.
  int YM = lcdDataBus[6];  // LCD_D1
  int XP = lcdDataBus[7];  // LCD_D0
  int XM = LCD_DC;

  pinMode(YP, INPUT);
  pinMode(YM, INPUT);
  digitalWrite(YP, LOW);  // Pull down the top/bottom edges of the one axis
  digitalWrite(YM, LOW);

  pinMode(XP, OUTPUT);    // Get current to flow in the other direction.
  pinMode(XM, OUTPUT);    
  digitalWrite(XP, HIGH);
  digitalWrite(XM, LOW);  // Then measure and interpret voltage.
  
  int rawY = 0;
  int yLB = 550;          // Smallest reading I saw.  
  int yUB = 3133;         // Biggest reading I saw.
  for (int i = 0; i < rpts; i++) {
    int u = analogRead(YP);   
    if (u < yLB) return -1;
    rawY += u;
    //delayMicroseconds(10);
  }
  rawY = rawY / rpts;

  // Scale, remap, clamp the value, etc.
  float fy = (rawY - yLB) * 240.0 / (yUB - yLB);
  int remappedY = (int) fy;
  if (remappedY > 239) remappedY = 239;
  remappedY = 239 - remappedY; // Flip Y coordinate system


  // Now do it all again for the other axis ...
  pinMode(XP, INPUT);
  pinMode(XM, INPUT);
  digitalWrite(XP, LOW);
  digitalWrite(XM, LOW);

  pinMode(YP, OUTPUT);
  pinMode(YM, OUTPUT);
  digitalWrite(YP, HIGH);
  digitalWrite(YM, LOW);

  int xLB = 460;        // Observed LB, UB values...
  int xUB = 3800;
  int rawX = 0;
  for (int i = 0; i < rpts; i++) {
    int u = analogRead(XP);
    if (u < xLB) return -1;
    rawX += u;
   // delayMicroseconds(10);
  }

  rawX = rawX / rpts;
  float fx = (rawX - xLB) * 320.0 / (xUB - xLB);
  int remappedX = (int) fx;
  if (remappedX > 319) remappedX = 319;
  return (remappedY << 16) | remappedX;
}


void putTouchPinsBackLikeTheyShouldBe()
{
  pinMode(lcdDataBus[6], OUTPUT);
  pinMode(lcdDataBus[7], OUTPUT);
  pinMode(LCD_DC, OUTPUT);
  digitalWrite(LCD_DC, HIGH);
  pinMode(LCD_CS, OUTPUT);
  digitalWrite(LCD_CS, LOW);
}
