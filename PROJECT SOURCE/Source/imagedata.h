/*********************************************************************
 * IMAGEDATA.H - OLED Display Image Data
 *********************************************************************/

#ifndef IMAGEDATA_H
#define IMAGEDATA_H

/*********************************************************************
 * INCLUDES
 *********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * IMAGE DATA DECLARATIONS
 *********************************************************************/

// Logo bitmap (128x64)
extern const unsigned char logo[];

// Zigbee status icons (10x10)
extern const unsigned char zbblank[];   // Blank icon
extern const unsigned char zboff[];     // Zigbee off state
extern const unsigned char zbon[];      // Zigbee on state

// Large digits (0-9) for numeric display (23x23)
extern const unsigned char zeroL[];
extern const unsigned char oneL[];
extern const unsigned char twoL[];
extern const unsigned char threeL[];
extern const unsigned char fourL[];
extern const unsigned char fiveL[];
extern const unsigned char sixL[];
extern const unsigned char sevenL[];
extern const unsigned char eightL[];
extern const unsigned char nineL[];

// Sensor labels
extern const unsigned char pm25[];      // "PM2.5" label
extern const unsigned char mg3[];       // "mg/m³" unit
extern const unsigned char tvoc[];      // "TVOC" label
extern const unsigned char eco2[];      // "eCO2" label
extern const unsigned char ppb[];       // "ppb" unit
extern const unsigned char ppm[];       // "ppm" unit

// Background masks
extern const unsigned char b1[];        // Top background mask
extern const unsigned char b2[];        // Bottom background mask
extern const unsigned char backL[];     // Full background fill (23x23)

#ifdef __cplusplus
}
#endif

#endif /* IMAGEDATA_H */