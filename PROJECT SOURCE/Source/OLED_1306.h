/**************************************************************************************************
 * OLED SSD1306 Display Driver
 * Written by Andrew Lamchenko, November, 2021.
 **************************************************************************************************/

#ifndef OLED_1306_H
#define OLED_1306_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * CONSTANTS
 *********************************************************************/

#define OLED_ADDR                   0x3c

#define OLED_ADDR_READ              (0x3C << 1) | 0x01
#define OLED_ADDR_WRITE             (0x3C << 1) | 0x00

#define SSD1306_HEIGHT              64
#define SSD1306_WIDTH               128
#define SSD1306_BUFFER_SIZE         SSD1306_WIDTH * SSD1306_HEIGHT / 8

/*********************************************************************
 * TYPEDEFS
 *********************************************************************/

typedef enum {
    Black = 0x00,   // Black color, no pixel
    White = 0x01    // Pixel is set. Color depends on OLED
} SSD1306_COLOR;

typedef enum {
    SSD1306_OK = 0x00,
    SSD1306_ERR = 0x01   // Generic error
} SSD1306_Error_t;

// Structure to store display state and cursor position
typedef struct {
    uint16_t CurrentX;      // Current X cursor position
    uint16_t CurrentY;      // Current Y cursor position
    uint8_t Initialized;    // Initialization flag
    uint8_t DisplayOn;      // Display power state
} SSD1306_t;

// Vertex structure for polyline drawing
typedef struct {
    uint8_t x;
    uint8_t y;
} SSD1306_VERTEX;

/*********************************************************************
 * FUNCTION PROTOTYPES
 *********************************************************************/

// Delay functions
void OLED_WaitUs(uint16 microSecs);
void OLED_WaitMs(uint32_t period);

// Initialization and basic control
void ssd1306_Init(void);
void ssd1306_WriteCommand(unsigned char command);
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size);
void ssd1306_SetDisplayOn(const uint8_t on);
uint8_t ssd1306_GetDisplayOn(void);
void ssd1306_SetContrast(const uint8_t value);

// Buffer and screen operations
void ssd1306_Fill(SSD1306_COLOR color);
SSD1306_Error_t ssd1306_FillBuffer(uint8_t* buf, uint32_t len);
void ssd1306_UpdateScreen(void);

// Drawing primitives
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
void ssd1306_DrawBitmap(uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color);
void ssd1306_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color);
void ssd1306_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color);
void ssd1306_DrawCircle(uint8_t par_x, uint8_t par_y, uint8_t par_r, SSD1306_COLOR color);
void ssd1306_DrawArc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, SSD1306_COLOR color);
void ssd1306_Polyline(const SSD1306_VERTEX *par_vertex, uint16_t par_size, SSD1306_COLOR color);

// Text rendering
void ssd1306_SetCursor(uint8_t x, uint8_t y);

#ifdef __cplusplus
}
#endif

#endif /* OLED_1306_H */