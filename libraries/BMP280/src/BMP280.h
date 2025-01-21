/*!
 * @file BMP280.h
 * @brief Provides an Arduino library for reading and interpreting Bosch BMP280 data over I2C. 
 * @n Used to read current temperature, air pressure and calculate altitude.
 *
 * @copyright   Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author [Frank](jiehan.guo@dfrobot.com)
 * @version  V1.0
 * @date  2022-11-01
 * @url https://github.com/dvarrel/BMP280.git
 */

#ifndef BMP280_H
#define BMP280_H

#include "Arduino.h"
#include "Wire.h"

#define BMP280_DEFAULT_ADDRESS 0x77

class BMP280
{
public:
  typedef enum {
    eStatusOK,
    eStatusErr,
    eStatusErrDeviceNotDetected,
    eStatusErrParameter
  } eStatus_t;

  typedef struct {
    uint16_t    t1;
    int16_t     t2, t3;
    uint16_t    p1;
    int16_t     p2, p3, p4, p5, p6, p7, p8, p9;
    uint16_t    reserved0;
  } sCalibrateDig_t;

  typedef struct {
    uint8_t   im_update: 1;
    uint8_t   reserved: 2;
    uint8_t   measuring: 1;
  } sRegStatus_t;

  /**
   * @enum eCtrlMeasMode_t
   * @brief Enum control measurement mode (power)
   */
  typedef enum {
    eCtrlMeasModeSleep,
    eCtrlMeasModeForced,
    eCtrlMeasModeNormal = 0x03
  } eCtrlMeasMode_t;

  /**
   * @enum  eSampling_t
   * @brief Enum sampling
   */
  typedef enum {
    eSampling_no,
    eSampling_X1,
    eSampling_X2,
    eSampling_X4,
    eSampling_X8,
    eSampling_X16
  } eSampling_t;

  typedef struct {
    uint8_t   mode: 2;
    uint8_t   osrs_p: 3;
    uint8_t   osrs_t: 3;
  } sRegCtrlMeas_t;

  typedef enum {
    eConfigSpi3w_en_disable,
    eConfigSpi3w_en_enable
  } eConfigSpi3w_en_t;

  /**
   * @enum eConfigFilter_t
   * @brief Enum config filter
   */
  typedef enum {
    eConfigFilter_off,
    eConfigFilter_X2,
    eConfigFilter_X4,
    eConfigFilter_X8,
    eConfigFilter_X16
  } eConfigFilter_t;

  /**
   * @enum eConfigTStandby_t
   * @brief Enum config standby time, unit ms
   */
  typedef enum {
    eeStatus_tConfigTStandby_0_5,    /**< 0.5 ms */
    eConfigTStandby_62_5,
    eConfigTStandby_125,
    eConfigTStandby_250,
    eConfigTStandby_500,
    eConfigTStandby_1000,
    eConfigTStandby_2000,
    eConfigTStandby_4000
  } eConfigTStandby_t;

  typedef struct {
    uint8_t   spi3w_en: 1;
    uint8_t   reserved1: 1;
    uint8_t   filter: 3;
    uint8_t   t_sb: 3;
  } sRegConfig_t;

  typedef struct {
    uint8_t   msb, lsb;
    uint8_t   reserved: 4;
    uint8_t   xlsb: 4;
  } sRegPress_t;

  typedef struct {
    uint8_t   msb, lsb;
    uint8_t   reserved: 4;
    uint8_t   xlsb: 4;
  } sRegTemp_t;

  #define BMP280_REG_START    0x88
  typedef struct {
    sCalibrateDig_t   calib;
    uint8_t   reserved0[(0xd0 - 0xa1 - 1)];
    uint8_t   chip_id;
    #define BMP280_REG_CHIP_ID_DEFAULT    0x58
    uint8_t   reserved1[(0xe0 - 0xd0 - 1)];
    uint8_t   reset;
    uint8_t   reserved2[(0xf3 - 0xe0 - 1)];
    sRegStatus_t    status;
    sRegCtrlMeas_t    ctrlMeas;
    sRegConfig_t      config;
    uint8_t   reserved3;
    sRegPress_t   press;
    sRegTemp_t    temp;
  } sRegs_t;

public:
  explicit BMP280(const uint8_t deviceAddress = BMP280_DEFAULT_ADDRESS);

  /**
   * @fn begin
   * @brief begin Sensor begin
   * @return Enum of eStatus_t
   */
  uint8_t begin();

  /**
   * @fn getTemperature
   * @brief getTemperature Get temperature
   * @return Temprature in Celsius
   */
  float  getTemperature();

  /**
   * @fn getPressure
   * @brief getPressure Get pressure
   * @return Pressure in pa
   */
  uint32_t getPressure();

  /**
   * @fn calAltitude
   * @brief calAltitude Calculate altitude
   * @param seaLevelPressure Sea level pressure
   * @param pressure Pressure in pa
   * @return Altitude in meter
   */
  int16_t calAltitude(uint32_t pressure, float seaLevelPressure = 1013.0);

  /**
   * @fn reset
   * @brief reset Reset sensor
   */
  void reset();

  /**
   * @fn setCtrlMeasMode
   * @brief setCtrlMeasMode Set control measure mode
   * @param eMode One enum of eCtrlMeasMode_t
   */
  void setCtrlMeasMode(eCtrlMeasMode_t eMode);

  /**
   * @fn setCtrlMeasSamplingTemp
   * @brief setCtrlMeasSamplingTemp Set control measure temperature oversampling
   * @param eSampling One enum of eSampling_t
   */
  void setCtrlMeasSamplingTemp(eSampling_t eSampling);

  /**
   * @fn setCtrlMeasSamplingPress
   * @brief setCtrlMeasSamplingPress Set control measure pressure oversampling
   * @param eSampling One enum of eSampling_t
   */
  void setCtrlMeasSamplingPress(eSampling_t eSampling);

  /**
   * @fn setConfigFilter
   * @brief setConfigFilter Set config filter
   * @param eFilter One enum of eConfigFilter_t
   */
  void setConfigFilter(eConfigFilter_t eFilter);

  /**
   * @fn setConfigTStandby
   * @brief setConfigTStandby Set config standby time
   * @param eT One enum of eConfigTStandby_t
   */
  void setConfigTStandby(eConfigTStandby_t eT);

protected:
  void    getCalibrate();

  int32_t   getTemperatureRaw();
  int32_t   getPressureRaw();

  uint8_t   getReg(uint8_t reg);
  void      writeRegBits(uint8_t reg, uint8_t field, uint8_t val);

  virtual void    writeReg(uint8_t reg, uint8_t *pBuf, uint8_t len);
  virtual void    readReg(uint8_t reg, uint8_t *pBuf, uint8_t len);


public:
  eStatus_t   lastOperateStatus; /**< lastOperateStatus Last operate status*/

protected:
  int32_t   _t_fine;
  uint8_t   _deviceAddress;
  sCalibrateDig_t   _sCalib;
};

#endif
