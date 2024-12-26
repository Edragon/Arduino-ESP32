//Author: Ackerman
//Direct DAC Sound (Not PWM, PDM, PPM or PAM)
//Play WAV in format RAW
//8, 16, 32 Khz Mono and Stereo
//GPIO 25 Right channel
//GPIO 26 Left channel
//TTGO VGA32 v1.x (GPIO 26 - PS/2 mouse socket(Pin 5 CLK))
//Compatible ESP32 board (WEMOS D1 R32, Dev Kit V1, etc...)
//Not PSRAM
//Arduino IDE 1.8.11 Espressif System 1.0.6
//ArduinoDroid (6.3.1)
//Visual Studio 1.66.1 PLATFORMIO 2.4.3 Espressif32 v3.5.0 (python 3.6)
//Works with Visual Studio 1.66.1 PLATFORMIO 2.4.3 Espressif32 v3.5.0 (python 3.6) but project structure not included
//Test 16000 Hz Mono Sub Urban Cradles NCS Release
//44 khz has not been tested
//stereo has not been tested

#include "gbConfig.h"
#include <Arduino.h>
#include <driver/dac.h>
//#include "customDAC.h" //En futuro sera necesario para custom sin dac.h
//#include "soc/timer_group_struct.h" //No se necesita
#include "wavraw.h"

//No necesita rtc_io.reg ni soc.h, compila PLATFORMIO,Arduino IDE y ArduinoDROID
//DAC 1 GPIO25
#define DR_REG_RTCIO_BASE 0x3ff48400
#define RTC_IO_PAD_DAC1_REG (DR_REG_RTCIO_BASE + 0x84)
#define RTC_IO_PDAC1_DAC 0x000000FF
#define RTC_IO_PDAC1_DAC_S 19

//DAC 2 GPIO26
#define RTC_IO_PAD_DAC2_REG (DR_REG_RTCIO_BASE + 0x88)
#define RTC_IO_PDAC2_DAC 0x000000FF
#define RTC_IO_PDAC2_DAC_S 19


#define SENS_SAR_DAC_CTRL2_REG (DR_REG_SENS_BASE + 0x009c)
#define SENS_DAC_CW_EN1_M  (BIT(24))
#define SENS_DAC_CW_EN2_M  (BIT(25))


#ifdef use_lib_music_sample_44KHZ
#define max_dac_buffer 32768
#else
#ifdef use_lib_music_sample_32KHZ
//#define max_dac_buffer 2048 //(256*2)
#define max_dac_buffer 32768
#else
#ifdef use_lib_music_sample_16KHZ
#define max_dac_buffer 32768 //(256*2)
#else
//#define max_dac_buffer 2048
//#define max_dac_buffer 256
#define max_dac_buffer 32768
#endif
#endif
#endif

#ifdef use_lib_stereo
static DRAM_ATTR unsigned char gb_dac_buf_r[max_dac_buffer];
static DRAM_ATTR unsigned char gb_dac_buf_l[max_dac_buffer];
unsigned int *gb_dac_buf32_r = (unsigned int *)gb_dac_buf_r;
unsigned int *gb_dac_buf32_l = (unsigned int *)gb_dac_buf_l;
unsigned int *gb_wav_raw32_r = (unsigned int *)gb_wav_raw_r;
unsigned int *gb_wav_raw32_l = (unsigned int *)gb_wav_raw_l;
#else
static DRAM_ATTR unsigned char gb_dac_buf_r[max_dac_buffer];
unsigned int *gb_dac_buf32_r = (unsigned int *)gb_dac_buf_r;
unsigned int *gb_wav_raw32_r = (unsigned int *)gb_wav_raw_r;
#endif


volatile unsigned int gb_dac_read = 0;
volatile unsigned int gb_dac_write = max_dac_buffer >> 1; //max_dac_buffer/2;
volatile unsigned int gb_dac_write32 = max_dac_buffer >> 3; //(max_dac_buffer/2)/4;


//volatile unsigned int gb_time_snd_cur=0;
//volatile unsigned int gb_time_snd_min=9999999;
//volatile unsigned int gb_time_snd_max=0;


volatile unsigned char gb_spk_data_r = 0; //canal derecho
volatile unsigned char gb_spk_data_r_before = 0;
volatile unsigned char gb_spk_data_l = 0; //canal izquierdo
volatile unsigned char gb_spk_data_l_before = 0;
volatile unsigned int gb_contador_muestra = 0;
volatile unsigned int gb_contador_muestra32 = 0;

hw_timer_t *gb_timerSound = NULL;
hw_timer_t *gb_timerRellenoSonido = NULL;




void IRAM_ATTR onTimerSoundDAC(void);
void IRAM_ATTR GeneraRAW(void);




//*********
//* SETUP *
//*********
void setup()
{
  //Serial.begin(9600);
  Serial.begin(115200);
  //pinMode(25, OUTPUT);
#ifdef use_lib_stereo
  memset(gb_dac_buf_r, 0x80, sizeof(gb_dac_buf_r));
  memset(gb_dac_buf_l, 0x80, sizeof(gb_dac_buf_l));
  dac_output_enable(DAC_CHANNEL_1);
  //dac_output_voltage(DAC_CHANNEL_1, 0x7f); //Evita llamar CLEAR_PERI_REG_MASK
  CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
  SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, 0x7f, RTC_IO_PDAC1_DAC_S);
  dac_output_enable(DAC_CHANNEL_2);
  //dac_output_voltage(DAC_CHANNEL_2, 0x7f); //Evita llamar CLEAR_PERI_REG_MASK
  CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
  SET_PERI_REG_BITS(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, 0x7f, RTC_IO_PDAC2_DAC_S);
#else
  memset(gb_dac_buf_r, 0x80, sizeof(gb_dac_buf_r));
  dac_output_enable(DAC_CHANNEL_1);
  //dac_output_voltage(DAC_CHANNEL_1, 0x7f); //Evita llamar CLEAR_PERI_REG_MASK
  CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
  SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, 0x7f, RTC_IO_PDAC1_DAC_S);
#endif
  gb_timerSound = timerBegin(0, 80, true);
  timerAttachInterrupt(gb_timerSound, &onTimerSoundDAC, true);
  gb_timerRellenoSonido = timerBegin(1, 80, true);
  timerAttachInterrupt(gb_timerRellenoSonido, &GeneraRAW, true);
#ifdef use_lib_music_sample_8KHZ
  timerAlarmWrite(gb_timerSound, 125, true); //1000000 1 segundo  125 es 8000 hz
  timerAlarmWrite(gb_timerRellenoSonido, 8000, true); //1000000 1 segundo  8 ms
#else
#ifdef use_lib_music_sample_16KHZ
  timerAlarmWrite(gb_timerSound, 62, true); //1000000 1 segundo  62 es 16000 Hz
  timerAlarmWrite(gb_timerRellenoSonido, 8000, true); //1000000 1 segundo  8 ms
#else
#ifdef use_lib_music_sample_32KHZ
  timerAlarmWrite(gb_timerSound, 31, true); //1000000 1 segundo  31 es 32000 hz
  timerAlarmWrite(gb_timerRellenoSonido, 8000, true); //1000000 1 segundo  8 ms
#else
#ifdef use_lib_music_sample_44KHZ
  timerAlarmWrite(gb_timerSound, 22, true); //1000000 1 segundo  22 es 44100 hz
  timerAlarmWrite(gb_timerRellenoSonido, 8000, true); //1000000 1 segundo  8 ms
#endif
#endif
#endif
#endif



  Serial.printf("Wait\r\n");
  GeneraRAW(); //Relleno 8 primeros ms
  timerAlarmEnable(gb_timerRellenoSonido); //Lo arranco despues de rellenar buffer
  delay(8);
  Serial.printf("Init Play WAV\r\n");
  timerAlarmEnable(gb_timerSound); //Lo arranco despues de rellenar buffer
}




void IRAM_ATTR onTimerSoundDAC()
{
  //Para DAC
  if (gb_spk_data_r != gb_spk_data_r_before)
  {
    //    dac_output_voltage(DAC_CHANNEL_1, gb_spk_data_r);

    //CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
    SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, gb_spk_data_r, RTC_IO_PDAC1_DAC_S);   //dac_output

    //    //En ESP32S2 es base + 0x0120 y en esp32 base+0x009C sens_reg.h y soc.h
    //    #define SENS_SAR_DAC_CTRL2_REG          (DR_REG_SENS_BASE + 0x0120)
    //    #define SENS_DAC_CW_EN1_M  (BIT(24))
    //    //CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
    //    SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, gb_spk_data, RTC_IO_PDAC1_DAC_S);   //dac_output

    gb_spk_data_r_before = gb_spk_data_r;
  }

#ifdef use_lib_stereo
  if (gb_spk_data_l != gb_spk_data_l_before)
  {
    //     dac_output_voltage(DAC_CHANNEL_2, gb_spk_data_l);
    SET_PERI_REG_BITS(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, gb_spk_data_l, RTC_IO_PDAC2_DAC_S);   //dac_output
    gb_spk_data_l_before = gb_spk_data_l;
  }
  gb_spk_data_l = gb_dac_buf_l[gb_dac_read];
#endif

  gb_spk_data_r = gb_dac_buf_r[gb_dac_read++];
#ifdef use_lib_music_sample_44KHZ
  //if(gb_dac_read>=max_dac_buffer){ gb_dac_read=0; }
  //gb_dac_read= gb_dac_read & 0x07FF;
  gb_dac_read = gb_dac_read & 0x7FFF; //32768
#else
#ifdef use_lib_music_sample_32KHZ
  //gb_dac_read= gb_dac_read & 0x07FF;
  //gb_dac_read= gb_dac_read & 0x3FFF; //16384
  gb_dac_read = gb_dac_read & 0x7FFF; //32768
#else
#ifdef use_lib_music_sample_16KHZ
  //gb_dac_read= gb_dac_read & 0x07FF;
  gb_dac_read = gb_dac_read & 0x7FFF; //32768
#else
  //gb_dac_read= gb_dac_read & 0xFF;
  gb_dac_read = gb_dac_read & 0x7FFF; //32768
#endif
#endif
#endif
  //gb_dac_read= gb_dac_read & 0x7FF;

  //gb_dac_read++;
  //if(gb_dac_read>=max_dac_buffer)
  //{
  // gb_dac_read=0;
  //}
  //   return;


}



//Reproducir WAVE RAW
void IRAM_ATTR GeneraRAW()
{

  // unsigned int timeIni= micros();

  //Entre 23 y 28 microsegundos copia 32 bits
#ifdef use_lib_music_sample_8KHZ
  for (unsigned char i = 0; i < 16; i++)
#else
#ifdef use_lib_music_sample_16KHZ
  for (unsigned char i = 0; i < 32; i++)
#else
#ifdef use_lib_music_sample_32KHZ
  for (unsigned char i = 0; i < 64; i++)
#else
#ifdef use_lib_music_sample_44KHZ
  for (unsigned char i = 0; i < 96; i++)
#endif
#endif
#endif
#endif
  {
    //aux32= gb_wav_raw32[gb_contador_muestra32++];
#ifdef use_lib_stereo
    gb_dac_buf32_l[gb_dac_write32] = gb_wav_raw32_l[gb_contador_muestra32];
#endif
    gb_dac_buf32_r[gb_dac_write32++] = gb_wav_raw32_r[gb_contador_muestra32++];

    if (gb_contador_muestra32 >= gb_wav_tope32)
    {
      gb_contador_muestra32 = 0;
    }

    gb_dac_write32 = gb_dac_write32 & 0x1FFF; //Es 0x7FFF DIV 4

  }


  // unsigned int timeFin= micros();
  // gb_time_snd_cur= timeFin-timeIni;
  // if (gb_time_snd_cur> gb_time_snd_max){ gb_time_snd_max= gb_time_snd_cur; }
  // if (gb_time_snd_cur< gb_time_snd_min){ gb_time_snd_min= gb_time_snd_cur; }
}


//********
//* LOOP *
//********
void loop()
{
  //unsigned int tiempo_ini= millis();
  //unsigned int tiempo_cur= tiempo_ini;

  delay(4000);
}
