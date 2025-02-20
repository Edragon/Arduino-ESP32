# Yet Another Arduino BMP180 Digital Pressure Sensor Library

home: https://bitbucket.org/christandlg/bmp180mi

sensor: https://www.bosch-sensortec.com/bst/products/all_products/bmp180 

## Features:
- Supports I2C via the Wire library
- Supports other I2C libraries via inheritance
- Never blocks or delays (except for convenience functions)

## Changelog:
- 1.0.1
	- fixed an include guard error. thanks to https://github.com/wollewald for reporting this error. 
- 1.0.0
	- added new class BMP180TwoWire for TwoWire interfaces
	- moved BMP180I2C class into its own source files, further separating data processing from communications
	- when updating from an earlier version and using the BMP180I2C class, change ```#include <BMP180MI.h>``` to ```#include <BMP180I2C.h>```

- 0.1.0
	- initial release
