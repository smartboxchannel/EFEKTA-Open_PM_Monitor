/**************************************************************************************************
 * OLED SSD1306 Display Driver
 * Written by Andrew Lamchenko, November, 2021.
 **************************************************************************************************/

/*********************************************************************
 * INCLUDES
 *********************************************************************/

#include "OLED_1306.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>     // For memcpy
#include "hal_i2c.h"
#include "imagedata.h"

/*********************************************************************
 * GLOBAL VARIABLES
 *********************************************************************/

// Display framebuffer
static uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE];

// Display state object
static SSD1306_t SSD1306;

/*********************************************************************
 * FUNCTION DEFINITIONS
 *********************************************************************/

/*
 * Fills the screen buffer with values from a given buffer of a fixed length
 */
SSD1306_Error_t ssd1306_FillBuffer(uint8_t* buf, uint32_t len) {
    SSD1306_Error_t ret = SSD1306_ERR;
    if (len <= SSD1306_BUFFER_SIZE) {
        memcpy(SSD1306_Buffer, buf, len);
        ret = SSD1306_OK;
    }
    return ret;
}

/*
 * Write command byte to the display
 */
void ssd1306_WriteCommand(unsigned char command) {
    I2C_WriteMultByte(OLED_ADDR, 0x00, (uint8_t*)&command, 1);
}

/*
 * Write data bytes to the display
 */
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
    I2C_WriteMultByte(OLED_ADDR, 0x40, buffer, buff_size);
}

/*
 * Initialize the SSD1306 display
 */
void ssd1306_Init(void) {
    OLED_WaitMs(100);

    // Display off during initialization
    ssd1306_SetDisplayOn(0);

    // Set Memory Addressing Mode
    ssd1306_WriteCommand(0x20);
    ssd1306_WriteCommand(0x00);     // 00b: Horizontal Addressing Mode
                                    // 01b: Vertical Addressing Mode
                                    // 10b: Page Addressing Mode (RESET)
                                    // 11b: Invalid

    // Set Page Start Address for Page Addressing Mode (0-7)
    ssd1306_WriteCommand(0xB0);
    OLED_WaitMs(100);

    // Set COM Output Scan Direction
    ssd1306_WriteCommand(0xC8);

    // Set low column address
    ssd1306_WriteCommand(0x00);

    // Set high column address
    ssd1306_WriteCommand(0x10);

    // Set start line address
    ssd1306_WriteCommand(0x40);

    // Set contrast
    ssd1306_SetContrast(0xFF);

    // Set segment re-map (0 to 127)
    ssd1306_WriteCommand(0xA1);

    // Set normal color (not inverted)
    ssd1306_WriteCommand(0xA6);

    // Set multiplex ratio
#if (SSD1306_HEIGHT == 128)
    ssd1306_WriteCommand(0xFF);
#else
    ssd1306_WriteCommand(0xA8);     // Set multiplex ratio (1 to 64)
#endif

#if (SSD1306_HEIGHT == 32)
    ssd1306_WriteCommand(0x1F);
#elif (SSD1306_HEIGHT == 64)
    ssd1306_WriteCommand(0x3F);
#elif (SSD1306_HEIGHT == 128)
    ssd1306_WriteCommand(0x3F);     // Works for 128px high displays
#else
#error "Only 32, 64, or 128 lines of height are supported!"
#endif

    // Output follows RAM content (0xA4), ignore RAM content (0xA5)
    ssd1306_WriteCommand(0xA4);

    // Set display offset
    ssd1306_WriteCommand(0xD3);
    ssd1306_WriteCommand(0x00);     // No offset

    // Set display clock divide ratio / oscillator frequency
    ssd1306_WriteCommand(0xD5);
    ssd1306_WriteCommand(0x80);

    // Set pre-charge period
    ssd1306_WriteCommand(0xD9);
    ssd1306_WriteCommand(0x22);

    // Set COM pins hardware configuration
    ssd1306_WriteCommand(0xDA);
#if (SSD1306_HEIGHT == 32)
    ssd1306_WriteCommand(0x02);
#elif (SSD1306_HEIGHT == 64)
    ssd1306_WriteCommand(0x12);
#elif (SSD1306_HEIGHT == 128)
    ssd1306_WriteCommand(0x12);
#else
#error "Only 32, 64, or 128 lines of height are supported!"
#endif

    // Set VCOMH
    ssd1306_WriteCommand(0xDB);
    ssd1306_WriteCommand(0x20);     // 0x20 = 0.77x Vcc

    // Set DC-DC enable
    ssd1306_WriteCommand(0x8D);
    ssd1306_WriteCommand(0x14);

    // Turn on display
    ssd1306_SetDisplayOn(1);

    // Clear screen
    ssd1306_Fill(Black);

    // Flush buffer to screen
    ssd1306_UpdateScreen();

    // Set default values for screen object
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;
    SSD1306.Initialized = 1;
}

/*
 * Draw bitmap image on the display
 */
void ssd1306_DrawBitmap(uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color) {
    int16_t byteWidth = (w + 7) / 8;    // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return;
    }

    for (uint8_t j = 0; j < h; j++, y++) {
        for (uint8_t i = 0; i < w; i++) {
            if (i & 7) {
                byte <<= 1;
            } else {
                byte = (*(const unsigned char *)(&bitmap[j * byteWidth + i / 8]));
            }
            if (byte & 0x80) {
                ssd1306_DrawPixel(x + i, y, color);
            }
        }
    }
    return;
}

/*
 * Turn display on or off
 */
void ssd1306_SetDisplayOn(const uint8_t on) {
    uint8_t value;
    if (on) {
        value = 0xAF;   // Display on
        SSD1306.DisplayOn = 1;
    } else {
        value = 0xAE;   // Display off
        SSD1306.DisplayOn = 0;
    }
    ssd1306_WriteCommand(value);
}

/*
 * Fill entire screen buffer with specified color
 */
void ssd1306_Fill(SSD1306_COLOR color) {
    uint32_t i;

    for (i = 0; i < sizeof(SSD1306_Buffer); i++) {
        SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
    }
}

/*
 * Set display contrast
 */
void ssd1306_SetContrast(const uint8_t value) {
    const uint8_t kSetContrastControlRegister = 0x81;
    ssd1306_WriteCommand(kSetContrastControlRegister);
    ssd1306_WriteCommand(value);
}

/*
 * Update the physical display with framebuffer contents
 */
void ssd1306_UpdateScreen(void) {
    // Write data to each page of RAM.
    // Number of pages depends on screen height:
    //   32px  -> 4 pages
    //   64px  -> 8 pages
    //   128px -> 16 pages
    for (uint8_t i = 0; i < SSD1306_HEIGHT / 8; i++) {
        ssd1306_WriteCommand(0xB0 + i);     // Set current RAM page address
        ssd1306_WriteCommand(0x00);         // Set low column address
        ssd1306_WriteCommand(0x10);         // Set high column address
        ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH * i], SSD1306_WIDTH);
    }
}

/*
 * Draw a single pixel at specified coordinates
 */
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        // Don't write outside the buffer
        return;
    }

    // Draw in the right color
    if (color == White) {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

/*
 * Set cursor position for text rendering
 */
void ssd1306_SetCursor(uint8_t x, uint8_t y) {
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

/*
 * Draw a line using Bresenham's algorithm
 */
void ssd1306_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color) {
    int32_t deltaX = abs(x2 - x1);
    int32_t deltaY = abs(y2 - y1);
    int32_t signX = ((x1 < x2) ? 1 : -1);
    int32_t signY = ((y1 < y2) ? 1 : -1);
    int32_t error = deltaX - deltaY;
    int32_t error2;

    ssd1306_DrawPixel(x2, y2, color);
    while ((x1 != x2) || (y1 != y2)) {
        ssd1306_DrawPixel(x1, y1, color);
        error2 = error * 2;
        if (error2 > -deltaY) {
            error -= deltaY;
            x1 += signX;
        }
        if (error2 < deltaX) {
            error += deltaX;
            y1 += signY;
        }
    }
    return;
}

/*
 * Draw a polyline from a vertex array
 */
void ssd1306_Polyline(const SSD1306_VERTEX *par_vertex, uint16_t par_size, SSD1306_COLOR color) {
    uint16_t i;
    if (par_vertex != 0) {
        for (i = 1; i < par_size; i++) {
            ssd1306_Line(par_vertex[i - 1].x, par_vertex[i - 1].y,
                         par_vertex[i].x, par_vertex[i].y, color);
        }
    }
    return;
}

/*
 * Convert degrees to radians
 */
static float ssd1306_DegToRad(float par_deg) {
    return par_deg * 3.14 / 180.0;
}

/*
 * Normalize angle to [0, 360] range
 */
static uint16_t ssd1306_NormalizeTo0_360(uint16_t par_deg) {
    uint16_t loc_angle;
    if (par_deg <= 360) {
        loc_angle = par_deg;
    } else {
        loc_angle = par_deg % 360;
        loc_angle = ((par_deg != 0) ? par_deg : 360);
    }
    return loc_angle;
}

/*
 * Draw an arc
 * Drawing angle begins from the 4th quadrant (3pi/2)
 * start_angle: starting angle in degrees
 * sweep: sweep angle in degrees
 */
void ssd1306_DrawArc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, SSD1306_COLOR color) {
#define CIRCLE_APPROXIMATION_SEGMENTS 36
    float approx_degree;
    uint32_t approx_segments;
    uint8_t xp1, xp2;
    uint8_t yp1, yp2;
    uint32_t count = 0;
    uint32_t loc_sweep = 0;
    float rad;

    loc_sweep = ssd1306_NormalizeTo0_360(sweep);

    count = (ssd1306_NormalizeTo0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_degree = loc_sweep / (float)approx_segments;

    while (count < approx_segments) {
        rad = ssd1306_DegToRad(count * approx_degree);
        xp1 = x + (int8_t)(sin(rad) * radius);
        yp1 = y + (int8_t)(cos(rad) * radius);
        count++;

        if (count != approx_segments) {
            rad = ssd1306_DegToRad(count * approx_degree);
        } else {
            rad = ssd1306_DegToRad(loc_sweep);
        }
        xp2 = x + (int8_t)(sin(rad) * radius);
        yp2 = y + (int8_t)(cos(rad) * radius);
        ssd1306_Line(xp1, yp1, xp2, yp2, color);
    }
    return;
}

/*
 * Draw a circle using Bresenham's algorithm
 */
void ssd1306_DrawCircle(uint8_t par_x, uint8_t par_y, uint8_t par_r, SSD1306_COLOR par_color) {
    int32_t x = -par_r;
    int32_t y = 0;
    int32_t err = 2 - 2 * par_r;
    int32_t e2;

    if (par_x >= SSD1306_WIDTH || par_y >= SSD1306_HEIGHT) {
        return;
    }

    do {
        ssd1306_DrawPixel(par_x - x, par_y + y, par_color);
        ssd1306_DrawPixel(par_x + x, par_y + y, par_color);
        ssd1306_DrawPixel(par_x + x, par_y - y, par_color);
        ssd1306_DrawPixel(par_x - x, par_y - y, par_color);

        e2 = err;
        if (e2 <= y) {
            y++;
            err = err + (y * 2 + 1);
            if (-x == y && e2 <= x) {
                e2 = 0;
            }
        }
        if (e2 > x) {
            x++;
            err = err + (x * 2 + 1);
        }
    } while (x <= 0);

    return;
}

/*
 * Draw a rectangle outline
 */
void ssd1306_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color) {
    ssd1306_Line(x1, y1, x2, y1, color);
    ssd1306_Line(x2, y1, x2, y2, color);
    ssd1306_Line(x2, y2, x1, y2, color);
    ssd1306_Line(x1, y2, x1, y1, color);
    return;
}

/*
 * Get current display power state
 */
uint8_t ssd1306_GetDisplayOn() {
    return SSD1306.DisplayOn;
}

/*
 * Microsecond delay (approximate)
 */
void OLED_WaitUs(uint16 microSecs) {
    while (microSecs--) {
        // 32 NOPs ˜ 1 µs at typical clock speed
        asm("nop"); asm("nop"); asm("nop"); asm("nop");
        asm("nop"); asm("nop"); asm("nop"); asm("nop");
    }
}

/*
 * Millisecond delay
 */
void OLED_WaitMs(uint32_t period) {
    OLED_WaitUs(period * 1000);
}