#ifndef SET_CJSON_VALUE_FILE_H
#define SET_CJSON_VALUE_FILE_H

enum ItemNames {
    LED_SEGMENTS =  0,
    LED_EFFECTS =   1,
    LED_PALETTES =  2,
    SWITCH =        3,
    DIMMER =        4,
    COLOR_EFFECT_SELECT =   5,
    COLOR_PALETTE_SELECT =  6,
    COLOR_SEGMENT_GROUPING =    7,
    COLOR_SEGMENT_SPACING = 8,
    COLOR_SEGMENT_OFFSET =  9,
    COLOR_SEGMENT_REVERSE = 10,
    COLOR_SEGMENT_MIRROR =  11,
    COLOR_EFFECT_SPEED =    12,
    COLOR_EFFECT_INTENSITY =    13,
    COLOR_EFFECT_CUSTOM_SLIDER_1 =  14,
    COLOR_EFFECT_CUSTOM_SLIDER_2 =  15,
    COLOR_EFFECT_CUSTOM_SLIDER_3 =  16,
    COLOR_EFFECT_OPTION_1 = 17,
    COLOR_EFFECT_OPTION_2 = 18,
    COLOR_EFFECT_OPTION_3 = 19,
    COLOR_SEGMENT_PRIMARY_COLOR =   20,
    COLOR_SEGMENT_SECONDARY_COLOR = 21,
    COLOR_SEGMENT_TERNARY_COLOR = 22,
    ITEM_UNKNOWN_MAX = 23
};



typedef struct {
    const char* itemName;
    void (*itemHandler)(void* arg);
} ItemHandler;

void processItem(const char* itemName, void *arg);
void process_item_get(const char* itemName, void *arg);


#endif // SET_CJSON_VALUE_FILE_H
