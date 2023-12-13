#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"

#include "cJSON.h"
#include "trace.h"
#include "ezlopi_timer.h"
#include "items.h"
#include "math.h"

/*Ezlop Libs*/
#include "ezlopi_valueformatter.h"

/*Custom led Libs*/
#include "led.h"
#include "config.h"
#include "set_cjson_value.h"

static struct LED_CONFIG *led_config;

char* LEDModesStrings[] = {
    #define FX_MODE(x, y) y,
    #include "effect_list.h"
};

static void processLedSegments(void *arg)
{
    printf("GET: Processing LED Segments\n");
    char *formatted_key = NULL;
    cJSON *cjson_properties = (cJSON *)arg;
    
    cJSON_AddStringToObject(cjson_properties, "elementType", "led_segment");
    cJSON_AddNumberToObject(cjson_properties, "elementsMaxNumber", 10);
    cJSON *segment_name_json = cJSON_AddObjectToObject(cjson_properties, "value");
    if (segment_name_json)
    {

        for(int i = 0; i < config_get_channels(); i++)
        {
            formatted_key = ezlopi_valueformatter_int(led_config->channel[i]->segment_id);
            cJSON *segment_key_json = cJSON_AddObjectToObject(segment_name_json, formatted_key);
            cJSON *segment_key_value_json = cJSON_AddStringToObject(segment_key_json, "name", led_config->channel[i]->segment_name);
            segment_key_value_json = cJSON_AddNumberToObject(segment_key_json, "start", led_config->channel[i]->start);
            segment_key_value_json = cJSON_AddNumberToObject(segment_key_json, "end", led_config->channel[i]->stop);
            free(formatted_key);
        }
        cJSON_AddStringToObject(cjson_properties, "valueFormatted", "");
    }
}

static void processLedEffects(void *arg)
{
    printf("GET: Processing LED Effects\n");

    char *formatted_value = NULL;
    cJSON* cjson_properties = (cJSON*)arg;
    cJSON *segment_name_json = cJSON_AddObjectToObject(cjson_properties, "value");
    if (segment_name_json)
    {
        for(int i = 0; i < 1; i++)
        {
            formatted_value = ezlopi_valueformatter_int(i);
            cJSON *segment_key_json = cJSON_AddObjectToObject(segment_name_json, formatted_value);
            cJSON *segment_key_value_json = cJSON_AddStringToObject(segment_key_json, "name", LEDModesStrings[i]);
            segment_key_value_json = cJSON_AddStringToObject(segment_key_json, "description", "Some description");
            segment_key_value_json = cJSON_AddStringToObject(segment_key_json, "headpieceUrl", "https://some.url");
            free(formatted_value);
        }
        cJSON_AddStringToObject(cjson_properties, "valueFormatted", "");
    }

    ESP_LOGE("", "Heap Memory %d", esp_get_minimum_free_heap_size());

}

static void processLedPalettes(void *arg) 
{
    printf("GET: Processing LED Palettes\n");
}

static void processSwitch(void *arg) 
{
    printf("GET: Processing Switch\n");
}

static void processDimmer(void *arg) 
{
    printf("GET: Processing Dimmer\n");
}

static void processColorEffectSelect(void *arg) 
{
    printf("GET: Processing Color Effect Select\n");
}

static void processColorPaletteSelect(void *arg) 
{
    printf("GET: Processing Color Palette Select\n");
}

static void processColorSegmentGrouping(void *arg) 
{
    printf("GET: Processing Color Segment Grouping\n");
}

static void processColorSegmentSpacing(void *arg) 
{
    printf("GET: Processing Color Segment Spacing\n");
}

static void processColorSegmentOffset(void *arg) 
{
    printf("GET: Processing Color Segment Offset\n");
}

static void processColorSegmentReverse(void *arg) 
{
    printf("GET: Processing Color Segment Reverse\n");
}

static void processColorSegmentMirror(void *arg) 
{
    printf("GET: Processing Color Segment Mirror\n");
}

static void processColorEffectSpeed(void *arg) 
{
    printf("GET: Processing Color Effect Speed\n");
}

static void processColorEffectIntensity(void *arg) 
{
    printf("GET: Processing Color Effect Intensity\n");
}

static void processColorEffectCustomSlider1(void *arg) 
{
    printf("GET: Processing Color Effect Custom Slider 1\n");
}

static void processColorEffectCustomSlider2(void *arg) 
{
    printf("GET: Processing Color Effect Custom Slider 2\n");
}

static void processColorEffectCustomSlider3(void *arg) 
{
    printf("GET: Processing Color Effect Custom Slider 3\n");
}

static void processColorEffectOption1(void *arg) 
{
    printf("GET: Processing Color Effect Option 1\n");
}

static void processColorEffectOption2(void *arg) 
{
    printf("GET: Processing Color Effect Option 2\n");
}

static void processColorEffectOption3(void *arg) 
{
    printf("GET: Processing Color Effect Option 3\n");
}

static void processColorSegmentPrimaryColor(void) 
{
    printf("GET: Processing Color Segment Primary Color\n");
}

static void processColorSegmentSecondaryColor(void) 
{
    printf("GET: Processing Color Segment Secondary Color\n");
}

static void processColorSegmentTertiaryColor(void) 
{
    printf("GET: Processing Color Segment Tertiary Color\n");
}

static void processUnknown(void) 
{
    printf("Unknown item name\n");
}

void process_item_get(const char* itemName, void *arg) 
{
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
    itemHandlers[i].itemHandler(arg);
}