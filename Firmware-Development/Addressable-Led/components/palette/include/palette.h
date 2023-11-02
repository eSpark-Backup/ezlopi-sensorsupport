#ifndef PALETTE_H_
#define PALETTE_H_

typedef uint8_t byte;


// Function to spread the color palette evenly across LEDs
// void spreadPalette(Color* palette, int paletteSize, Color* leds, int numLeds);
uint32_t colorFromPalette(const byte* palette, int paletteSize, int numLeds, int ledIndex);


#endif /* PALETTE_H_ */