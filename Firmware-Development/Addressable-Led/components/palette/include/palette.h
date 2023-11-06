#ifndef PALETTE_H_
#define PALETTE_H_

typedef uint8_t byte;

// Define an enumeration for the palette names in all uppercase
enum PALETTE_NAME {
    DEFAULT_PALETTE,
    SUNSET_REAL,
    RIVENDELL,
    BREEZE,
    RED_BLUE,
    YELLOWOUT,
    ANALOGOUS,
    SPLASH,
    PASTEL,
    SUNSET2,
    BEECH,
    VINTAGE,
    DEPARTURE,
    LANDSCAPE,
    BEACH,
    SHERBET,
    HULT,
    HULT64,
    DRYWET,
    JUL,
    GRINTAGE,
    REWHI,
    TERTIARY,
    FIRE,
    ICEFIRE,
    CYANE,
    LIGHT_PINK,
    AUTUMN,
    MAGENTA,
    MAGRED,
    YELMAG,
    YELBLU,
    ORANGE_TEAL,
    TIAMAT,
    APRIL_NIGHT,
    ORANGERY,
    C9,
    SAKURA,
    AURORA,
    ATLANTICA,
    C9_2,
    C9_NEW,
    TEMPERATURE,
    AURORA2,
    RETRO_CLOWN,
    CANDY,
    TOXY_REAF,
    FAIRY_REAF,
    SEMI_BLUE,
    PINK_CANDY,
    RED_REAF,
    AQUA_FLASH,
    YELBLU_HOT,
    LITE_LIGHT,
    RED_FLASH,
    BLINK_RED,
    RED_SHIFT,
    RED_TIDE,
    CANDY2,
    ZAMBZI
};

// Function to spread the color palette evenly across LEDs
// void spreadPalette(Color* palette, int paletteSize, Color* leds, int numLeds);

uint32_t colorFromPalette(const byte* palette, int paletteSize, int numLeds, int ledIndex, uint8_t brightness);
void createDefaultPalette(uint32_t color, byte* palette);

#endif /* PALETTE_H_ */