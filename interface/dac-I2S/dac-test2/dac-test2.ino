#include "SoundData.h";
#include "XT_DAC_Audio.h"

XT_Wav_Class ForceWithYou(Force);     // create an object of type XT_Wav_Class that is used by 
                                      // the dac audio class (below), passing wav data as parameter.
                                      
XT_DAC_Audio_Class DacAudio(25,0);    // Create the main player class object. 
                                      // Use GPIO 25, one of the 2 DAC pins and timer 0

void setup() {
  Serial.begin(115200); 
}

void loop() {
  static uint32_t i=0;                // simple counter to output
  if(ForceWithYou.Completed)          // if completed playing, play again
    DacAudio.PlayWav(&ForceWithYou);  // play the wav (pass the wav class object created at top of code
  Serial.println(i);                  // print out the value of i
  i++;                                // increment the value of i
}
