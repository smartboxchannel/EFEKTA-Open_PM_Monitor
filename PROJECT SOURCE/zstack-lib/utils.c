/*********************************************************************
 * INCLUDES
 *********************************************************************/

#include "utils.h"
#include "hal_adc.h"

/*********************************************************************
 * MACROS
 *********************************************************************/

// Macros for range limiting (commented out as they are defined in utils.h)
// #define MAX(x, y) (((x) > (y)) ? (x) : (y))
// #define MIN(x, y) (((x) < (y)) ? (x) : (y))

/*********************************************************************
 * @fn      mapRange
 * @brief   Maps a value from one range to another and clamps the result
 * @param   a1 - start of source range
 * @param   a2 - end of source range
 * @param   b1 - start of target range
 * @param   b2 - end of target range
 * @param   s  - input value to map
 * @return  mapped value clamped to target range
 *********************************************************************/

double mapRange(double a1, double a2, double b1, double b2, double s)
{
    double result = b1 + (s - a1) * (b2 - b1) / (a2 - a1);
    return MIN(b2, MAX(result, b1));
}

/*********************************************************************
 * @fn      adcReadSampled
 * @brief   Reads ADC multiple times and returns average value
 * @param   channel      - ADC channel to read
 * @param   resolution   - ADC resolution (8, 10, 12, or 14 bits)
 * @param   reference    - Voltage reference source
 * @param   samplesCount - Number of samples to average
 * @return  Averaged ADC reading
 *********************************************************************/

uint16 adcReadSampled(uint8 channel, uint8 resolution, uint8 reference, uint8 samplesCount)
{
    HalAdcSetReference(reference);
    uint32 samplesSum = 0;
    
    for (uint8 i = 0; i < samplesCount; i++) {
        samplesSum += HalAdcRead(channel, resolution);
    }
    
    return samplesSum / samplesCount;
}