#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

MatrixPanel_I2S_DMA *display = nullptr;

uint16_t myDARK = display->color565(64, 64, 64);
uint16_t myWHITE = display->color565(192, 192, 192);
uint16_t myRED = display->color565(255, 0, 0);
uint16_t myGREEN = display->color565(0, 255, 0);
uint16_t myBLUE = display->color565(0, 0, 255);

uint16_t colours[5] = { myDARK, myWHITE, myRED, myGREEN, myBLUE };

struct Square
{
  float xpos, ypos;
  float velocityx;
  float velocityy;
  boolean xdir, ydir;
  uint16_t square_size;
  uint16_t colour;
};

const int numSquares = 25;
Square Squares[numSquares];

void setup()
{
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(115200);
  delay(200);

  Serial.println("...Starting Display");
  HUB75_I2S_CFG mxconfig;
  //mxconfig.double_buff = true; // Turn of double buffer
  mxconfig.clkphase = false;

  // OK, now we can create our matrix object
  display = new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();  // setup display with pins as pre-defined in the library

  // Create some Squares
  for (int i = 0; i < numSquares; i++)
  {
    Squares[i].square_size = random(2,10);
    Squares[i].xpos = random(0, display->width() - Squares[i].square_size);
    Squares[i].ypos = random(0, display->height() - Squares[i].square_size);
    Squares[i].velocityx = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    Squares[i].velocityy = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    //Squares[i].xdir = (random(2) == 1) ? true:false;
    //Squares[i].ydir = (random(2) == 1) ? true:false;

    int random_num = random(6);
    Squares[i].colour = colours[random_num];
  }
}

void loop()
{
  display->flipDMABuffer(); // not used if double buffering isn't enabled
  delay(25);
  display->clearScreen();

  for (int i = 0; i < numSquares; i++)
  {
    // Draw rect and then calculate
    display->fillRect(Squares[i].xpos, Squares[i].ypos, Squares[i].square_size, Squares[i].square_size, Squares[i].colour);

    if (Squares[i].square_size + Squares[i].xpos >= display->width()) {
      Squares[i].velocityx *= -1;
    } else if (Squares[i].xpos <= 0) {
      Squares[i].velocityx = abs (Squares[i].velocityx);
    }

    if (Squares[i].square_size + Squares[i].ypos >= display->height()) {
      Squares[i].velocityy *= -1;
    } else if (Squares[i].ypos <= 0) {
      Squares[i].velocityy = abs (Squares[i].velocityy);
    }

    Squares[i].xpos += Squares[i].velocityx;
    Squares[i].ypos += Squares[i].velocityy;
  }
}
