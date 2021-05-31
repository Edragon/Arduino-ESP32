/* ------------------------- CUSTOM GPIO PIN MAPPING ------------------------- */


/* -------------------------- Display Config Initialisation -------------------- */

#define MATRIX_WIDTH 64
#define MATRIX_HEIGHT 64

/* -------------------------- Class Initialisation -------------------------- */
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
MatrixPanel_I2S_DMA matrix;

#include <FastLED.h>

#include "Effects.h"
Effects effects;

#include "Drawable.h"
#include "Playlist.h"
//#include "Geometry.h"

#include "Patterns.h"
Patterns patterns;

/* -------------------------- Some variables -------------------------- */
unsigned long fps = 0, fps_timer; // fps (this is NOT a matix refresh rate!)
unsigned int default_fps = 30, pattern_fps = 30;  // default fps limit (this is not a matix refresh conuter!)
unsigned long ms_animation_max_duration = 20000;  // 20 seconds
unsigned long last_frame=0, ms_previous=0;

void setup()
{
  // Setup serial interface
  Serial.begin(115200);
  delay(250);
  matrix.begin();  // setup the LED matrix
  /**
   * this demos runs pretty fine in fast-mode which gives much better fps on large matrixes (>128x64)
   * see comments in the lib header on what does that means
   */
  //dma_display.setFastMode(true);

  // SETS THE BRIGHTNESS HERE. MAX value is MATRIX_WIDTH, 2/3 OR LOWER IDEAL, default is about 50%
  // dma_display.setPanelBrightness(30);
  /* another way to change brightness is to use
   * dma_display.setPanelBrightness8(uint8_t brt);	// were brt is within range 0-255
   * it will recalculate to consider matrix width automatically
   */
  //dma_display.setPanelBrightness8(180);

  Serial.println("**************** Starting Aurora Effects Demo ****************");

   // setup the effects generator
  effects.Setup();

  delay(500);
  Serial.println("Effects being loaded: ");
  listPatterns();


  //patterns.setPattern(0); //   // simple noise
  patterns.moveRandom(1); // start from a random pattern

  Serial.print("Starting with pattern: ");
  Serial.println(patterns.getCurrentPatternName());
  patterns.start();
  ms_previous = millis();
  fps_timer = millis();
}

void loop()
{
    // menu.run(mainMenuItems, mainMenuItemCount);  

  if ( (millis() - ms_previous) > ms_animation_max_duration ) 
  {
       patterns.stop();      
       patterns.moveRandom(1);
       //patterns.move(1);
       patterns.start();  
 
       
       Serial.print("Changing pattern to:  ");
       Serial.println(patterns.getCurrentPatternName());
        
       ms_previous = millis();

       // Select a random palette as well
       //effects.RandomPalette();
    }
 
    if ( 1000 / pattern_fps + last_frame < millis()){
      last_frame = millis();
      pattern_fps = patterns.drawFrame();
      if (!pattern_fps)
        pattern_fps = default_fps;

      ++fps;
    }

    if (fps_timer + 1000 < millis()){
       Serial.printf_P(PSTR("Effect fps: %ld\n"), fps);
       fps_timer = millis();
       fps = 0;
    }
       
}


void listPatterns() {
  patterns.listPatterns();
}
