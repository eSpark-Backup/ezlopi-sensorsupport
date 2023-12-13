#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"

#include "cJSON.h"
#include "trace.h"
#include "ezlopi_timer.h"
#include "items.h"
#include "math.h"

#include "led.h"
#include "config.h"
#include "set_cjson_value.h"

static struct LED_CONFIG *led_config;

static void processLedSegments(void* arg) {
    printf("Processing LED Segments\n");
    
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    cJSON* element = cJSON_GetObjectItem(cjson_params, "element");
    cJSON* value = cJSON_GetObjectItem(element, "value");
    int start = 0, end = 0; 
    char *key = NULL, *name = NULL;

    CJSON_GET_VALUE_INT(value, "start", start);
    CJSON_GET_VALUE_INT(value, "end", end);
    CJSON_GET_VALUE_STRING(cjson_params, "key", key);

    if(atoi(key) < config_get_channels())
    {
        config_set_segment_start_stop(atoi(key) , start, end);
        CJSON_GET_VALUE_STRING(value, "name", name);
        led_config->channel[atoi(key)]->segment_name = strdup(name);
    }
}

static void processLedEffects(void* arg) {
    //There will be no implementation for this as this is just for getting palette names setting api is listed below
    printf("Processing LED Effects\n");
}

static void processLedPalettes(void* arg) {
    //There will be no implementation for this as this is just for getting palette names setting api is listed below
    printf("Processing LED Palettes\n");
}

static void processSwitch(void* arg) {
    printf("Processing Switch\n");
}

static void processDimmer(void* arg) {
    printf("Processing Dimmer\n");
}

static void processColorEffectSelect(void* arg) {
    printf("Processing Color Effect Select\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    config_setMode(value_recieved);
}

static void processColorPaletteSelect(void* arg) {
    printf("Processing Color Palette Select\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    config_apply_palette(value_recieved);
}

static void processColorSegmentGrouping(void* arg) {
    printf("Processing Color Segment Grouping\n");
}

static void processColorSegmentSpacing(void* arg) {
    printf("Processing Color Segment Spacing\n");
}

static void processColorSegmentOffset(void* arg) {
    printf("Processing Color Segment Offset\n");
}

static void processColorSegmentReverse(void* arg) {
    printf("Processing Color Segment Reverse\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    config_set_reverse_option(0, value_recieved);

}

static void processColorSegmentMirror(void* arg) {
    printf("Processing Color Segment Mirror\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    //TODO: Mirror funciton
}

static void processColorEffectSpeed(void* arg) {
    printf("Processing Color Effect Speed\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    config_set_effect_speed(value_recieved);
}

static void processColorEffectIntensity(void* arg) {
    printf("Processing Color Effect Intensity\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    config_set_effect_intensity(value_recieved);
}

static void processColorEffectCustomSlider1(void* arg) {
    printf("Processing Color Effect Custom Slider 1\n");
}

static void processColorEffectCustomSlider2(void* arg) {
    printf("Processing Color Effect Custom Slider 2\n");
}

static void processColorEffectCustomSlider3(void* arg) {
    printf("Processing Color Effect Custom Slider 3\n");
}

static void processColorEffectOption1(void* arg) {
    printf("Processing Color Effect Option 1\n");
}

static void processColorEffectOption2(void* arg) {
    printf("Processing Color Effect Option 2\n");
}

static void processColorEffectOption3(void* arg) {
    printf("Processing Color Effect Option 3\n");
}



uint32_t extract_rgb_value_from_json(void *arg)
{
    static uint32_t extracted_rgb;
    cJSON* cjson_params = (cJSON*)arg;
    cJSON* value_object = cJSON_GetObjectItem(cjson_params, "value");

    uint8_t red = cJSON_GetObjectItem(value_object, "red")->valueint;
    uint8_t green = cJSON_GetObjectItem(value_object, "green")->valueint;
    uint8_t blue = cJSON_GetObjectItem(value_object, "blue")->valueint;
    extracted_rgb = ((uint32_t)red << 16) | ((uint32_t)green << 8) | ((uint32_t)blue << 0);

    return extracted_rgb;
}

uint32_t rgbcolor[3] = {0};
static void processColorSegmentPrimaryColor(void* arg) {
    printf("Processing Color Segment Primary Color\n");
    rgbcolor[0] = extract_rgb_value_from_json(arg);
    config_setColor(rgbcolor);
}

static void processColorSegmentSecondaryColor(void* arg) {
    printf("Processing Color Segment Secondary Color\n");
    rgbcolor[1] = extract_rgb_value_from_json(arg);
    config_setColor(rgbcolor);
}

static void processColorSegmentTertiaryColor(void* arg) {
    printf("Processing Color Segment Tertiary Color\n");
    rgbcolor[2] = extract_rgb_value_from_json(arg);
    config_setColor(rgbcolor);
}

static void processUnknown(void* arg) {
    // printf("Unknown item name\n");
    ESP_LOGE("set_cjson_value", "Unknown Item name!");
}


void processItem(const char* itemName, void *arg) {
    led_config = config_return_config_pointer();
 
  const ItemHandler itemHandlers[] = {
        {"led_segments", processLedSegments},
        {"led_effects", processLedEffects},
        {"led_palettes", processLedPalettes},
        {"switch", processSwitch},
        {"dimmer", processDimmer},
        {"color_effect_select", processColorEffectSelect},
        {"color_palette_select", processColorPaletteSelect},
        {"color_segment_grouping", processColorSegmentGrouping},
        {"color_segment_spacing", processColorSegmentSpacing},
        {"color_segment_offset", processColorSegmentOffset},
        {"color_segment_reverse", processColorSegmentReverse},
        {"color_segment_mirror", processColorSegmentMirror},
        {"color_effect_speed", processColorEffectSpeed},
        {"color_effect_intensity", processColorEffectIntensity},
        {"color_effect_custom_slider_1", processColorEffectCustomSlider1},
        {"color_effect_custom_slider_2", processColorEffectCustomSlider2},
        {"color_effect_custom_slider_3", processColorEffectCustomSlider3},
        {"color_effect_option_1", processColorEffectOption1},
        {"color_effect_option_2", processColorEffectOption2},
        {"color_effect_option_3", processColorEffectOption3},
        {"color_segment_primary_color", processColorSegmentPrimaryColor},
        {"color_segment_secondary_color", processColorSegmentSecondaryColor},
        {"color_segment_ternary_color", processColorSegmentTertiaryColor},
        {"unknown", processUnknown}  // Default case
    };

    size_t i;
    for (i = 0; i < sizeof(itemHandlers) / sizeof(itemHandlers[0]); ++i) {
        if (strcmp(itemName, itemHandlers[i].itemName) == 0) {
            itemHandlers[i].itemHandler(arg);
            return;
        }
    }

    // If itemName is not found, execute default handler
    itemHandlers[i-1].itemHandler(arg);
}


