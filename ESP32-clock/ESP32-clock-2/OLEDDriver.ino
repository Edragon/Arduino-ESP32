// LED Matrix OLED Driver Functions
// Handles display output and color management

//MatrixPanel_I2S_DMA dma_display;

// Define color constants using proper color565 conversion
// Note: These will be initialized in initOLED() after dma_display is created
uint16_t myBLACK;
uint16_t myWHITE;
uint16_t myRED;
uint16_t myGREEN;
uint16_t myBLUE;

// Initialize color constants (called after dma_display creation)
void initColors() {
  myBLACK = dma_display->color565(0, 0, 0);
  myWHITE = dma_display->color565(255, 255, 255);
  myRED = dma_display->color565(255, 0, 0);
  myGREEN = dma_display->color565(0, 255, 0);
  myBLUE = dma_display->color565(0, 0, 255);
}
void cleanTab() {
  for (int i = 0; i < 64; i++) {      // Changed from 128 to 64
    for (int j = 0; j < 64; j++) {
      ledtab[i][j] = 0x0000;
    }
  }
}
void fillTab(int x, int y, uint16_t color) {
  // Bounds protection for single 64x64 panel
  if (x < 0 || x >= 64 || y < 0 || y >= 64) return;
  if(!isnight){
      ledtab[x][y] = color;
  }else{
      ledtab[x][y] = 0x8800;
  }
}
void fillScreenTab() {
//  for (int i = 0; i < 64; i++) {
//    ledtab[64][0 + i] = TFT_LIGHTGREY; //分隔线
//  }
  screen_num = 0;  // Always 0 for single panel
  
  for (int i = 0; i < 64; i++) {      // Changed from 128 to 64
    for (int j = 0; j < 64; j++) {
      dma_display->drawPixel(i, j, ledtab[i][j]);
    }
  }
}
void fillCircle(int x, int y, int r, int color) {
  for (int i = x - r; i < x + r; i++) {
    for (int j = y - r; j < y + r; j++) {
      if (((i - x) * (i - x) + (j - y) * (j - y)) <= (r * r)) {
        fillTab(i, j, color);
      }
    }
  }
}
void drawBit(int x, int y, const uint8_t *bitmap , int width, int height, uint16_t color)
{

  int32_t i, j, byteWidth = (width + 7) / 8;

  for (j = 0; j < height; j++) {
    for (i = 0; i < width; i++ ) {
      if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
        //dma_display->drawPixel(x + i, y + j, color);
        fillTab(x + i, y + j, color);
      }
    }
  }
}
void drawLine(int x0, int y0, int x1, int sec) {
  for (int i = x0; i < x1; i++) { //横向
    colorl = rand() % 0xffff;
    if (!isnight) {
      //dma_display->drawPixel(x0 + i, y0, colorl);
      fillTab(x0 + i, y0, colorl);
    }
  }
}
void drawHLine(int x0, int y0, int y1, int sec) {
  for (int i = 0; i < y1; i++) { //横向
    colorl = rand() % 0xffff;
    if (!isnight) {
      //dma_display->drawPixel(x0, y0+i, colorl);
      fillTab(x0, y0 + i, colorl);
    }
  }
}
void drawBit2(int x, int y, const uint8_t *bitmap , int width, int height, uint16_t color)
{

  int32_t i, j, byteWidth = (width + 7) / 8;
  int first = 0;
  for (j = 0; j < height; j++) {
    for (i = 0; i < width; i++ ) {
      if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
        if (first == 0 & !isnight) {
          //dma_display->drawPixel(x + i, y + j, color+TFT_SILVER);
          fillTab(x + i, y + j, color + TFT_SILVER);
        } else {
          //dma_display->drawPixel(x + i, y + j, color);
          fillTab(x + i, y + j, color);
        }
        first++;
      }
    }
    first = 0;
  }
}
void drawSmBit(int x, int y, const uint8_t *bitmap , int width, int height, uint16_t color) {
  int32_t i, j;
  unsigned char a[3];
  unsigned char temp;
  for (j = 0; j < height; j++) {
    temp = bitmap[j];
    for (i = width; i > 0; i--) {
      a[i - 1] = temp & 0x01;
      temp >>= 1;
      if (a[i - 1] == 1) {
        // dma_display->drawPixel(x + i, y + j, color);
        fillTab(x + i, y + j, color);
      }
    }
  }
}
void display30Number(int c, int x, int y, uint16_t color)
{
  int hz_width;
  for (int k = 0; k < 10; k++) {
    if (shuzi30[k].Index == c)   {
      drawBit(x, y, shuzi30[k].sz30_Id, 15 , 30, color);
    }
  }

}
void display30Numbers(int numbers, int x, int y, uint16_t color)
{
  int count = 0;
  int temp = 0;
  char buffer[32];
  if (numbers == 0) {
    display30Number(numbers, x, y, color);
  }
  while (numbers)
  {
    temp = numbers % 10;
    if (numbers < 10)
    {
      temp = numbers;
    }

    display30Number(temp, x - (count * 16), y, color);
    numbers = numbers / 10;
    count++;  //count表示val是一个几位数
  }

}
void drawColorBit3(int x, int y, const uint16_t *bitmap , int width, int height)
{

  int32_t i, j, byteWidth = (width + 7) / 8;

  for (j = 0; j < height; j++) {
    for (i = 0; i < width; i++ ) {
      if (bitmap[i + j * width] != 0) {
        dma_display->drawPixel(x + i, y + j, bitmap[i + j * width]);
      }
    }
  }
}
void drawColorBit(int x, int y, const uint16_t *bitmap , int width, int height)
{

  int32_t i, j, byteWidth = (width + 7) / 8;

  for (j = 0; j < height; j++) {
    for (i = 0; i < width; i++ ) {
      if (bitmap[i + j * width] != 0) {
          fillTab(x + i, y + j, bitmap[i + j * width]);
      }
    }
  }
}
void drawColorBit2(int x, int y, const uint16_t *bitmap , int width, int height)
{

  int32_t i, j, byteWidth = (width + 7) / 8;

  for (j = 0; j < height; j++) {
    for (i = 0; i < width; i++ ) {
      if (bitmap[i + j * width] != 0) {
          fillTab(x + i, y + j, bitmap[i + j * width]);
      }
    }
  }
}
void showTQ(int c, int x, int y)
{
  int hz_width;
  for (int k = 0; k < 61; k++) {
    if (tq20[k].Index == c)   {
      drawColorBit2(x, y, tq20[k].tq20_Id, 20 , 20);
    }
  }

}
void drawText(String words, int x, int y)
{
  dma_display->setCursor(x, y);
  dma_display->setTextColor(dma_display->color565(255, 255, 0));
  dma_display->print(words);
}

void drawText(const char* text, int x, int y)
{
  dma_display->setCursor(x, y);
  dma_display->setTextColor(dma_display->color565(255, 255, 0));
  dma_display->print(text);
}
void clearOLED() {
  dma_display->clearScreen();
}

void initOLED() {

  // Module configuration for 64x64 single panel - CONSERVATIVE SETTINGS
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,   // module width (64)
    PANEL_RES_Y,   // module height (64)
    PANEL_CHAIN    // Chain length (1)
  );

  // CONSERVATIVE GPIO settings to prevent memory corruption
  mxconfig.gpio.e = 32;           // E pin for 64x64 panels
  
  // Use SHIFTREG (most compatible) instead of specific drivers
  mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;  // Most compatible generic driver
  
  // VERY CONSERVATIVE timing to prevent memory issues
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_8M;   // Lower speed for stability
  mxconfig.clkphase = false;                   // Standard clock phase
  
  // Display Setup with comprehensive error checking
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  
  if (!dma_display) {
    Serial.println("ERROR: Failed to create display object!");
    while(1) { delay(1000); } // Stop execution if display fails
  }
  
  bool initResult = dma_display->begin();
  if (!initResult) {
    Serial.println("ERROR: Display initialization failed!");
    while(1) { delay(1000); } // Stop execution if display fails
  }
  
  // Wait for display to stabilize
  delay(100);
  
  // Initialize color constants after display creation
  initColors();
  
  // Very low brightness to reduce power consumption and interference
  dma_display->setBrightness8(20); // Very low brightness
  dma_display->clearScreen();
  dma_display->fillScreen(myBLACK);
  
  Serial.println("Display initialized - SHIFTREG driver, conservative settings");
}
void setBrightness(int dianya) {
  dma_display->setBrightness8(light*dianya); //0-255
}

void setBrightness(float voltage) {
  dma_display->setBrightness8(light*voltage); //0-255
}
void displayNumber(int c, int x, int y, uint16_t color)
{
  int hz_width;
  for (int k = 0; k < 10; k++) {
    if (shuzi24[k].Index == c)   {
      drawBit2(x, y, shuzi24[k].hz24_Id, 12 , 24, color);
    }
  }

}
void disSmallNumber(int c, int x, int y, uint16_t color)
{
  int hz_width;
  for (int k = 0; k < 10; k++) {
    if (smsz[k].Index == c)   {
      //  Serial.println("run here");
      drawSmBit(x, y, smsz[k].smsz_Id, 3 , 5, color);
    }
  }
}
void disSmallNumbers(int numbers, int x, int y, uint16_t color)
{
  int count = 0;
  int temp = 0;
  char buffer[32];
  if (numbers == 0) {
    disSmallNumber(numbers, x, y, color);
  }
  while (numbers)
  {
    temp = numbers % 10;
    if (numbers < 10)
    {
      temp = numbers;
    }

    disSmallNumber(temp, x - (count * 4), y, color);
    numbers = numbers / 10;
    count++;  //count表示val是一个几位数
  }

}
void dis30Number(int c, int x, int y, uint16_t color)
{
  int hz_width;
  for (int k = 0; k < 10; k++) {
    if (shuzi30[k].Index == c)   {
      //  Serial.println("run here");
      drawBit(x, y, shuzi30[k].sz30_Id, 30 , 60, color);
    }
  }
}
void dis30Numbers(int numbers, int x, int y, uint16_t color)
{
  int count = 0;
  int temp = 0;
  char buffer[32];
  if (numbers == 0) {
    dis30Number(numbers, x, y, color);
  }
  while (numbers)
  {
    temp = numbers % 10;
    if (numbers < 10)
    {
      temp = numbers;
    }

    dis30Number(temp, x - (count * 30), y, color);
    numbers = numbers / 10;
    count++;  //count表示val是一个几位数
  }

}
void displayNumbers(int numbers, int x, int y, uint16_t color)
{
  int count = 0;
  int temp = 0;
  char buffer[32];
  if (numbers == 0) {
    displayNumber(numbers, x, y, color);
  }
  while (numbers)
  {
    temp = numbers % 10;
    if (numbers < 10)
    {
      temp = numbers;
    }

    displayNumber(temp, x - (count * 12), y, color);
    numbers = numbers / 10;
    count++;  //count表示val是一个几位数
  }

}
void disSmallChar(char c, int x, int y, uint16_t color)
{
  int hz_width;
  for (int k = 0; k < 11; k++) {
    if (smchar[k].Index == c)   {
      //Serial.println("run here!DrawChar");
      drawSmBit(x, y, smchar[k].smchar_Id, 3 , 5, color);
    }
  }
}
void drawChars(int32_t x, int32_t y, const char str[], uint32_t color) {
  int x0 = x;
  for (int i = 0; i < strlen(str); i++) {
    // Serial.print("当前char:");
    // Serial.println(str[i]);
    disSmallChar(str[i], x0, y, color);
    x0 += 4;
  }
}
void displayNumber2(int c, int x, int y, uint16_t color)
{
  int hz_width;
  for (int k = 0; k < 10; k++) {
    if (shuzi14[k].Index == c)   {
      drawBit(x, y, shuzi14[k].hz14_Id, 8 , 14, color);
    }
  }

}
void displayNumbers2(int numbers, int x, int y, uint16_t color)
{
  int count = 0;
  int temp = 0;
  char buffer[32];
  if (numbers == 0) {
    displayNumber2(numbers, x, y, color);
  }
  while (numbers)
  {
    temp = numbers % 10;
    if (numbers < 10)
    {
      temp = numbers;
    }

    displayNumber2(temp, x - (count * 7), y, color);
    numbers = numbers / 10;
    count++;  //count表示val是一个几位数
  }

}
void drawHz (int xx,int yy,unsigned char*names,uint32_t color){
  unsigned char buffs[24];
  //建立缓存空间
  getfont(names, sizeof(names), buffs);
  int kj = 0;
  int x=0;
  int y=0;
  for (int i = 0; i < 24; i++) {
    for (int s = 7 ; s >= 0 ; s--)
    {
      if (buffs[i] & (0x01 << s))
      {  
         if(7-s+xx>=0){
         if(i%2==0){
         fillTab(7-s+xx,y+yy,color);
         }else{
         fillTab(15-s+xx,y+yy,color);
         }
         }
      }
    }
    if ((i + 1) % 2 == 0)
    {
      y+=1;
    }
  }
  }
void drawHanziS(int32_t x, int32_t y, const char str[], uint32_t color) {
  int x0 = x;
  unsigned char b[3];
  for (int i = 0; i < strlen(str); i = i + 3) {
    b[0] = str[i];
    b[1] = str[i + 1];
    b[2] = str[i + 2];
    drawHz(x0, y, b, color);
    x0 += 11;
  }
}
// Simple color test function - less memory intensive
void testColorsSimple() {
  Serial.println("Testing basic colors...");
  
  // Clear display
  dma_display->clearScreen();
  delay(1000);
  
  // Test one color at a time to avoid memory stress
  Serial.println("Red test...");
  dma_display->fillScreen(myRED);
  delay(2000);
  
  Serial.println("Green test...");
  dma_display->fillScreen(myGREEN);
  delay(2000);
  
  Serial.println("Blue test...");
  dma_display->fillScreen(myBLUE);
  delay(2000);
  
  Serial.println("White test...");
  dma_display->fillScreen(myWHITE);
  delay(2000);
  
  // Clear to black
  dma_display->fillScreen(myBLACK);
  
  Serial.println("Simple color test completed!");
}

// Test function to verify color output - call this in setup() for debugging
void testColors() {
  Serial.println("Testing display colors...");
  
  // Clear display
  dma_display->clearScreen();
  delay(500);
  
  // Test basic colors
  dma_display->fillRect(0, 0, 16, 16, myRED);     // Top-left: Red
  dma_display->fillRect(16, 0, 16, 16, myGREEN);  // Top-middle: Green  
  dma_display->fillRect(32, 0, 16, 16, myBLUE);   // Top-right: Blue
  dma_display->fillRect(48, 0, 16, 16, myWHITE);  // Top-far right: White
  
  // Test RGB565 color values
  dma_display->fillRect(0, 16, 16, 16, 0xF800);   // Pure Red (RGB565)
  dma_display->fillRect(16, 16, 16, 16, 0x07E0);  // Pure Green (RGB565)
  dma_display->fillRect(32, 16, 16, 16, 0x001F);  // Pure Blue (RGB565)
  dma_display->fillRect(48, 16, 16, 16, 0xFFE0);  // Yellow (RGB565)
  
  // Test mixed colors
  dma_display->fillRect(0, 32, 16, 16, 0xF81F);   // Magenta
  dma_display->fillRect(16, 32, 16, 16, 0x07FF);  // Cyan
  dma_display->fillRect(32, 32, 16, 16, 0xFDA0);  // Orange
  dma_display->fillRect(48, 32, 16, 16, 0x8410);  // Gray
  
  Serial.println("Color test pattern displayed. Check your panel!");
}
