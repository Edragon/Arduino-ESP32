#ifndef APDSDEFS
#define APDSDEFS

#ifdef __cplusplus

// I2C address
#define APDS_I2C_ADDR 0x39

// Basic data
#define APDS_PID_1 0xA8
#define APDS_PID_2 0xAB
#define APDS_PWR_UP_ONLY 0x01

// Setup registers
#define APDS_PID_REG 0x92
#define APDS_ENABLE_REG 0x80
#define APDS_IFORCE_REG 0xE4
#define APDS_PICLEAR_REG 0xE5
#define APDS_CICLEAR_REG 0xE6
#define APDS_AICLEAR_REG 0xE7

// Shutter speed
#define APDS_SHUT_LV0 0x00
#define APDS_SHUT_LV1 0x32
#define APDS_SHUT_LV2 0x64
#define APDS_SHUT_LV3 0x96
#define APDS_SHUT_LV4 0xFF

// Mask values
#define APDS_AIL_MASK 0x02
#define APDS_PIL_MASK 0x04
#define APDS_WEN_MASK 0x08
#define APDS_AIL_INT_MASK 0x10
#define APDS_PIL_INT_MASK 0x20
#define APDS_GES_MASK 0x40
#define APDS_CONFIG_2_MASK 0x01
#define APDS_CTRL_LED_CURR_MASK 0xC0
#define APDS_CTRL_PGAIN_MASK 0x0C
#define APDS_CTRL_AGAIN_MASK 0x03
#define APDS_GGAIN_MASK 0x60
#define APDS_GLED_DRIVE_MASK 0x18

// Status registers
#define APDS_DEV_STATUS_REG 0x93
#define APDS_GSTATUS_REG 0xAF

// Timing registers
#define APDS_ATIME_REG 0x81
#define APDS_WTIME_REG 0x83

// Interrupt threshold registers
#define APDS_AILTL_REG 0x84
#define APDS_AILTH_REG 0x85
#define APDS_AIHTL_REG 0x86
#define APDS_AIHTH_REG 0x87
#define APDS_PILT_REG 0x89
#define APDS_PIHT_REG 0x8B
#define APDS_PERS_REG 0x8C

// Configuration registers
#define APDS_CONFIG_REG_1 0x8D
#define APDS_CONFIG_REG_2 0x90
#define APDS_CONFIG_REG_3 0x9F
#define APDS_CTRL_REG 0x8F

// Light Sensor data registers
#define APDS_CDATA_REG_L 0x94
#define APDS_CDATA_REG_H 0x95
#define APDS_RDATA_REG_L 0x96
#define APDS_RDATA_REG_H 0x97
#define APDS_GDATA_REG_L 0x98
#define APDS_GDATA_REG_H 0x99
#define APDS_BDATA_REG_L 0x9A
#define APDS_BDATA_REG_H 0x9B

// Proximity data register
#define APDS_PDATA_REG 0x9C

// Gesture Sensor data registers
#define APDS_GFIFO_REG_UP 0xFC
#define APDS_GFIFO_REG_DOWN 0xFD
#define APDS_GFIFO_REG_LEFT 0xFE
#define APDS_GFIFO_REG_RIGHT 0xFF

// Proximity controls
#define APDS_POFFSET_UR_REG 0x9D
#define APDS_POFFSET_DL_REG 0x9E

// Gesture controls
#define APDS_GPENTH_REG 0xA0
#define APDS_GEXTH_REG 0xA1
#define APDS_GCONFIG_REG_1 0xA2
#define APDS_GCONFIG_REG_2 0xA3
#define APDS_GCONFIG_REG_3 0xAA
#define APDS_GCONFIG_REG_4 0xAB
#define APDS_GOFFSET_UP_REG 0xA4
#define APDS_GOFFSET_DOWN_REG 0xA5
#define APDS_GOFFSET_LEFT_REG 0xA7
#define APDS_GOFFSET_RIGHT_REG 0xA9
#define APDS_GPLNC_REG 0xA6

// Gesture reciever pair combo
#define APDS_GES_ACTIVE_ALL 0x00
#define APDS_GES_ACTIVE_UPDOWN 0x01
#define APDS_GES_ACTIVE_LEFTRIGHT 0x02
#define APDS_GES_INACTIVE 0x03

// ------------ ERRORS -----------------

#define APDS_CHECK_CONN_ERR "APDS9960 is disconnected. Check connections or make sure it is working."
#define APDS_READ_FAILURE "Exception: Failed to read from BME688"
#define APDS_VALUE_INVALID "Invalid value. Use a value within the range."

#endif // __cplusplus
#endif // APDSDEFS