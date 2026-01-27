#pragma once
#include "main.h"

// NSEL (CS) -> PA11
#define RFM22_NSEL_GPIO_Port   GPIOA
#define RFM22_NSEL_Pin         GPIO_PIN_11

// nIRQ -> PA12
#define RFM22_nIRQ_GPIO_Port   GPIOA
#define RFM22_nIRQ_Pin         GPIO_PIN_12

// SDN je natvrdo na GND
#define RFM22_HAS_SDN          0
// default frekvencia (zmeň na 915000 ak máte 915 MHz verziu modulov)
#define RFM22_DEFAULT_FREQ_KHZ 433920u
