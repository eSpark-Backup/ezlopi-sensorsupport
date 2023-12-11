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

void set_segment_callback(cJSON* cjson_params, struct LED_CONFIG* led_config, void *arg)
{
    cJSON* element = cJSON_GetObjectItem(cjson_params, "element");
    cJSON* value = cJSON_GetObjectItem(element, "value");
    int start = 0, end = 0; 
    char *key = NULL, *name = NULL;
    CJSON_GET_VALUE_INT(value, "start", start);
    CJSON_GET_VALUE_INT(value, "end", end);
    CJSON_GET_VALUE_STRING(cjson_params, "key", key);
    // addressable_led_strip->length = end - start;
    // led_strip_flush(addressable_led_strip);
    // ezlopi_device_value_updated_from_device_v3(segment_item);
    // config_set_segment_start_stop(ctoi(key) , start, end);
    if(atoi(key) < config_get_channels())
    {
        config_set_segment_start_stop(atoi(key) , start, end);
        CJSON_GET_VALUE_STRING(value, "name", name);
        led_config->channel[atoi(key)]->segment_name = strdup(name);
    }
}

void processLedSegments(void* arg) {
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

void processLedEffects(void* arg) {
    //There will be no implementation for this as this is just for getting palette names setting api is listed below
    printf("Processing LED Effects\n");
}

void processLedPalettes(void* arg) {
    //There will be no implementation for this as this is just for getting palette names setting api is listed below
    printf("Processing LED Palettes\n");
}

void processSwitch(void* arg) {
    printf("Processing Switch\n");
}

void processDimmer(void* arg) {
    printf("Processing Dimmer\n");
}

void processColorEffectSelect(void* arg) {
    printf("Processing Color Effect Select\n");

    char *key = NULL, *name = NULL;
    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    // cJSON* element = cJSON_GetObjectItem(cjson_params, "element");
    // cJSON* value = cJSON_GetObjectItem(cjson_params, "value");

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    config_setMode(value_recieved);
}

void processColorPaletteSelect(void* arg) {
    printf("Processing Color Palette Select\n");
}

void processColorSegmentGrouping(void* arg) {
    printf("Processing Color Segment Grouping\n");
}

void processColorSegmentSpacing(void* arg) {
    printf("Processing Color Segment Spacing\n");
}

void processColorSegmentOffset(void* arg) {
    printf("Processing Color Segment Offset\n");
}

void processColorSegmentReverse(void* arg) {
    printf("Processing Color Segment Reverse\n");
}

void processColorSegmentMirror(void* arg) {
    printf("Processing Color Segment Mirror\n");
}

void processColorEffectSpeed(void* arg) {
    printf("Processing Color Effect Speed\n");
}

void processColorEffectIntensity(void* arg) {
    printf("Processing Color Effect Intensity\n");
}

void processColorEffectCustomSlider1(void* arg) {
    printf("Processing Color Effect Custom Slider 1\n");
}

void processColorEffectCustomSlider2(void* arg) {
    printf("Processing Color Effect Custom Slider 2\n");
}

void processColorEffectCustomSlider3(void* arg) {
    printf("Processing Color Effect Custom Slider 3\n");
}

void processColorEffectOption1(void* arg) {
    printf("Processing Color Effect Option 1\n");
}

void processColorEffectOption2(void* arg) {
    printf("Processing Color Effect Option 2\n");
}

void processColorEffectOption3(void* arg) {
    printf("Processing Color Effect Option 3\n");
}

void processColorSegmentPrimaryColor(void* arg) {
    printf("Processing Color Segment Primary Color\n");
}

void processColorSegmentSecondaryColor(void* arg) {
    printf("Processing Color Segment Secondary Color\n");
}

void processColorSegmentTertiaryColor(void* arg) {
    printf("Processing Color Segment Tertiary Color\n");
}

void processUnknown(void* arg) {
    printf("Unknown item name\n");
}


void processItem(const char* itemName, void *arg) {
    led_config = config_return_config_pointer();
 static const ItemHandler itemHandlers[] = {
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
        {"color_segment_tertiary_color", processColorSegmentTertiaryColor},
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
    itemHandlers[i].itemHandler(arg);
}