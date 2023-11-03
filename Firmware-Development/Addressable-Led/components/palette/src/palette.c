#include <stdint.h>
#include <palette.h>
#include "esp_log.h"

static const char* TAG = "uart_select_example";


// Function to spread the color palette evenly across LEDs and return one uint32_t color
uint32_t colorFromPalette(const byte* palette, int paletteSize, int numLeds, int ledIndex) {


    int baseIndex = (ledIndex * paletteSize) / numLeds;
    int nextIndex = ((baseIndex + 1) % paletteSize);
    int offset = ((255 * ledIndex) / numLeds);

    //TODO: Palette Mirror Option
    // Calculate the base and next indices based on whether to reverse the palette or not
    // if (1) {
    //     baseIndex = paletteSize - 1 - (ledIndex * paletteSize) / numLeds;
    //     nextIndex = paletteSize - 1 - (((ledIndex + 1) * paletteSize) / numLeds);
    // } else {
    //     baseIndex = (ledIndex * paletteSize) / numLeds;
    //     nextIndex = ((baseIndex + 1) % paletteSize);
    // }
    // offset = ((255 * ledIndex) / numLeds);

    //print size
    ESP_LOGI(TAG, "Palette Size : %d", paletteSize);

    // Calculate the blended color
    uint8_t r = palette[baseIndex * 4 + 1] + ((palette[nextIndex * 4 + 1] - palette[baseIndex * 4 + 1]) * offset) / 255;
    uint8_t g = palette[baseIndex * 4 + 2] + ((palette[nextIndex * 4 + 2] - palette[baseIndex * 4 + 2]) * offset) / 255;
    uint8_t b = palette[baseIndex * 4 + 3] + ((palette[nextIndex * 4 + 3] - palette[baseIndex * 4 + 3]) * offset) / 255;
    

    // Pack the RGB values into a uint32_t
    uint32_t color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;

    return color;
}

// Function to set all elements in a byte array with a uint32_t color
void createDefaultPalette(uint32_t color, byte* palette) {

    int arraySize = 14*4;    

    // Extract the R, G, and B components from the uint32_t color
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    // Fill the entire byte array with the color components
    for (int i = 0; i < arraySize; i += 4) {
        palette[i+1] = r;     // R component
        palette[i + 2] = g; // G component
        palette[i + 3] = b; // B component
    }
}
