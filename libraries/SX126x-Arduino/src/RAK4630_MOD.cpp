#ifdef NRF52_SERIES
#include <variant.h>
#include <SPI.h>
#ifdef _VARIANT_RAK3400_
#warning USING RAK3400
SPIClass SPI_LORA(NRF_SPIM1, 29, 3, 30);
#elif defined _VARIANT_RAK4630_
#warning USING RAK4630
SPIClass SPI_LORA(NRF_SPIM2, 45, 43, 44);
#endif
#endif