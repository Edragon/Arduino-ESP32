#include "APDS9960.h"

APDS9960::APDS9960() : sensorHub(APDS_I2C_ADDR) {}

bool APDS9960::begin()
{
    if (isConnected())
        sensorHub.i2c_execute(APDS_ENABLE_REG, setupByte);
    else
        printLog(APDS_CHECK_CONN_ERR);
    return isConnected();
}

bool APDS9960::beginAll()
{
    if (isConnected())
    {
        enableAllSensors(true);
        enableAllInterrupts(true);
        sensorHub.i2c_execute(APDS_ENABLE_REG, setupByte);
    }
    else
        printLog(APDS_CHECK_CONN_ERR);
    return isConnected();
}

uint8_t APDS9960::getPID()
{
    uint8_t pid = 0;
    sensorHub.i2c_readByte(APDS_PID_REG, &pid, 1);
    return pid;
}

void APDS9960::enableAllSensors(bool state)
{
    enableLightSensing(state);
    enableProximitySensing(state);
    enableGestureSensing(state);
}

void APDS9960::enableAllInterrupts(bool state)
{
    enableLightInterrupt(state);
    enableProximityInterrupt(state);
}

void APDS9960::enableLightSensing(bool state)
{
    setupByte = (setupByte & ~APDS_AIL_MASK) | (-state & APDS_AIL_MASK);
    sensorHub.i2c_execute(APDS_ENABLE_REG, setupByte);
}

void APDS9960::enableProximitySensing(bool state)
{
    setupByte = (setupByte & ~APDS_PIL_MASK) | (-state & APDS_PIL_MASK);
    sensorHub.i2c_execute(APDS_ENABLE_REG, setupByte);
}

void APDS9960::enableGestureSensing(bool state)
{
    setupByte = (setupByte & ~APDS_GES_MASK) | (-state & APDS_GES_MASK);
    sensorHub.i2c_execute(APDS_ENABLE_REG, setupByte);
    enableProximitySensing(state);
}

void APDS9960::enableLightInterrupt(bool state)
{
    setupByte = (setupByte & ~APDS_AIL_INT_MASK) | (-state & APDS_AIL_INT_MASK);
    sensorHub.i2c_execute(APDS_ENABLE_REG, setupByte);
}

void APDS9960::enableProximityInterrupt(bool state)
{
    setupByte = (setupByte & ~APDS_PIL_INT_MASK) | (-state & APDS_PIL_INT_MASK);
    sensorHub.i2c_execute(APDS_ENABLE_REG, setupByte);
}

void APDS9960::enableWaitTimer(bool state, uint8_t waitTime, bool WLONG)
{
    setupByte = (setupByte & ~APDS_WEN_MASK) | (-state & APDS_WEN_MASK);
    sensorHub.i2c_execute(APDS_ENABLE_REG, setupByte);
    sensorHub.i2c_execute(APDS_WTIME_REG, waitTime);
    sensorHub.i2c_execute(APDS_CONFIG_REG_1, WLONG ? 0x62 : 0x60);
}

bool APDS9960::isConnected()
{
    uint8_t pid = 0x00;
    if (sensorHub.is_sensor_connected() && sensorHub.i2c_readByte(APDS_PID_REG, &pid, 1))
        return pid == APDS_PID_1 || pid == APDS_PID_2;
    return false;
}

void APDS9960::setLightSensitivity(uint8_t shutterSpeed)
{
    sensorHub.i2c_execute(APDS_ATIME_REG, 255 - shutterSpeed);
}

uint8_t APDS9960::readProximity()
{
    uint8_t pData = 0;
    if (!sensorHub.i2c_readByte(APDS_PDATA_REG, &pData, 1))
        printLog(APDS_READ_FAILURE);
    return pData;
}

void APDS9960::correctProximity(int8_t upRight, int8_t downLeft)
{
    sensorHub.i2c_execute(APDS_POFFSET_UR_REG, upRight);
    sensorHub.i2c_execute(APDS_POFFSET_DL_REG, downLeft);
}

void APDS9960::setLightSensingInterruptThreshold(uint16_t low, uint16_t high)
{
    sensorHub.i2c_execute(APDS_AILTL_REG, 0xFF & low);
    sensorHub.i2c_execute(APDS_AILTH_REG, low >> 8);
    sensorHub.i2c_execute(APDS_AIHTL_REG, 0xFF & high);
    sensorHub.i2c_execute(APDS_AIHTH_REG, high >> 8);
}

void APDS9960::setProximitySensingInterruptThreshold(uint8_t low, uint8_t high)
{
    sensorHub.i2c_execute(APDS_PILT_REG, low);
    sensorHub.i2c_execute(APDS_PIHT_REG, high);
}

void APDS9960::setPersistence(uint8_t light, uint8_t proximity)
{
    if (light > 15 || proximity > 15)
    {
        printLog(APDS_VALUE_INVALID);
        return;
    }
    sensorHub.i2c_execute(APDS_PERS_REG, light << 4 | proximity);
}

void APDS9960::setProximitySensitivity(uint8_t sensitivity)
{
    if (sensitivity > 3)
    {
        printLog(APDS_VALUE_INVALID);
        return;
    }
    sensorHub.i2c_execute(APDS_CONFIG_REG_2, APDS_CONFIG_2_MASK | sensitivity << 4);
    ctrl = (ctrl & ~APDS_CTRL_PGAIN_MASK) | sensitivity << 2;
    sensorHub.i2c_execute(APDS_CTRL_REG, ctrl);
}

void APDS9960::setProximitySensorRange(uint8_t level)
{
    if (level > 3)
    {
        printLog(APDS_VALUE_INVALID);
        return;
    }
    ctrl = (ctrl & ~APDS_CTRL_LED_CURR_MASK) | level << 6;
    sensorHub.i2c_execute(APDS_CTRL_REG, ctrl);
}

void APDS9960::setLightGain(uint8_t gainFactor)
{
    if (gainFactor > 3)
    {
        printLog(APDS_VALUE_INVALID);
        return;
    }
    ctrl = (ctrl & ~APDS_CTRL_AGAIN_MASK) | gainFactor;
    sensorHub.i2c_execute(APDS_CTRL_REG, ctrl);
}

Color APDS9960::readColorData()
{
    Color color;
    if (
        !(sensorHub.i2c_read_Xbit_LE(APDS_CDATA_REG_L, &(color.clear), 16) &&
          sensorHub.i2c_read_Xbit_LE(APDS_RDATA_REG_L, &(color.red), 16) &&
          sensorHub.i2c_read_Xbit_LE(APDS_GDATA_REG_L, &(color.green), 16) &&
          sensorHub.i2c_read_Xbit_LE(APDS_BDATA_REG_L, &(color.blue), 16)))
        printLog(APDS_READ_FAILURE);
    return color;
}

void APDS9960::setGestureGain(uint8_t gainFactor)
{
    if (gainFactor > 3)
    {
        printLog(APDS_VALUE_INVALID);
        return;
    }
    gesCtrl = (gesCtrl & ~APDS_GGAIN_MASK) | gainFactor << 5;
    gainFactor = 3 - gainFactor;
    gesCtrl = (gesCtrl & ~APDS_GLED_DRIVE_MASK) | gainFactor << 3;
    sensorHub.i2c_execute(APDS_GCONFIG_REG_2, gesCtrl);
}

void APDS9960::setGestureSensitivity(uint8_t pulseLength, uint8_t pulseCount)
{
    if (pulseLength > 3 || pulseCount > 63)
    {
        printLog(APDS_VALUE_INVALID);
        return;
    }
    sensorHub.i2c_execute(APDS_GPLNC_REG, pulseLength << 6 | pulseCount);
}

void APDS9960::setGestureDetectorMode(uint8_t mode)
{
    if (mode > 3)
    {
        printLog(APDS_VALUE_INVALID);
        return;
    }
    sensorHub.i2c_execute(APDS_GCONFIG_REG_3, mode);
}

void APDS9960::enableGestureInterrupt(bool state)
{
    sensorHub.i2c_execute(APDS_GCONFIG_REG_4, state ? 0x02 : 0x00);
}

Gesture APDS9960::readGesture()
{
    Gesture gesture;
    if (
        !(sensorHub.i2c_readByte(APDS_GFIFO_REG_UP, &(gesture.up), 1) &&
          sensorHub.i2c_readByte(APDS_GFIFO_REG_DOWN, &(gesture.down), 1) &&
          sensorHub.i2c_readByte(APDS_GFIFO_REG_LEFT, &(gesture.left), 1) &&
          sensorHub.i2c_readByte(APDS_GFIFO_REG_RIGHT, &(gesture.right), 1)))
        printLog(APDS_READ_FAILURE);
    return gesture;
}

String APDS9960::resolveGesture(Gesture gesture, uint8_t threshold)
{
    struct Pair
    {
        String name;
        int value;
    };

    Pair directions[] = {
        {"Up", gesture.up},
        {"Down", gesture.down},
        {"Left", gesture.left},
        {"Right", gesture.right}};

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3 - i; j++)
        {
            if (directions[j].value < directions[j + 1].value)
            {
                Pair temp = directions[j];
                directions[j] = directions[j + 1];
                directions[j + 1] = temp;
            }
        }
    }

    String primary = directions[0].name;

    String secondary = "";
    if (directions[1].value >= (directions[0].value * (threshold / 100)))
    {
        secondary = directions[1].name;
    }

    return secondary == "" ? primary : primary + " + " + secondary;
}

void APDS9960::printLog(String log)
{
    if (printLogs)
        Serial.println(log);
}

void APDS9960::clearInterrupts()
{
    sensorHub.i2c_execute(APDS_AICLEAR_REG, 0);
}