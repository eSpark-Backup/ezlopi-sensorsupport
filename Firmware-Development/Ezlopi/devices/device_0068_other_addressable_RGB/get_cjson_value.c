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
#include "ezlopi_item_name_str.h"

/*Custom led Libs*/
#include "led.h"
#include "config.h"
#include "palette.h"
#include "set_cjson_value.h"

static struct LED_CONFIG *led_config;

char key_buffer[4];

/*Defines*/
#define RED     0xFF0000
#define GREEN   0x00FF00
#define BLUE    0x0000FF

#define REVERSE (uint8_t)0x80

/*Prototypes*/
static char* intToString(int num);
uint8_t get_segment_number(void *arg);


char* LEDModesStrings[] = {
    #define FX_MODE(x, y) y,
    #include "effect_list.h"
};

char* PaletteNames[] = {
    #define PALETTE(x, y) y,
    #include "palette_list.h"
};

static void processLedSegments(void *arg)
{
    printf("GET: Processing LED Segments\n");
    char *formatted_key = NULL;
    cJSON *cjson_properties = (cJSON *)arg;
    
    cJSON_AddStringToObject(cjson_properties, "elementType", "led_segment");
    cJSON_AddNumberToObject(cjson_properties, "elementsMaxNumber", 10);
    cJSON *segment_name_json = cJSON_AddObjectToObject(cjson_properties, "value");
    
    cJSON *segment_key_json = NULL;
    cJSON *segment_key_value_json = NULL;

    if (segment_name_json)
    {

        for(int i = 0; i < config_get_channels(); i++)
        {
            formatted_key = ezlopi_valueformatter_int(led_config->channel[i]->segment_id);
            segment_key_json = cJSON_AddObjectToObject(segment_name_json, formatted_key);
            segment_key_value_json = cJSON_AddStringToObject(segment_key_json, "name", led_config->channel[i]->segment_name);
            segment_key_value_json = cJSON_AddNumberToObject(segment_key_json, "start", led_config->channel[i]->start);
            segment_key_value_json = cJSON_AddNumberToObject(segment_key_json, "end", led_config->channel[i]->stop);
            free(formatted_key);
        }
        cJSON_AddStringToObject(cjson_properties, "valueFormatted", "");
    }
}

const char* key_str_list[54] = 
{
"0",
"1",
"2",
"3",
"4",
"5",
"6",
"7",
"8",
"9",
"10",
"11",
"12",
"13",
"14",
"15",
"16",
"17",
"18",
"19",
"20",
"21",
"22",
"23",
"24",
"25",
"26",
"27",
"28",
"29",
"30",
"31",
"32",
"33",
"34",
"35",
"36",
"37",
"38",
"39",
"40",
"41",
"42",
"43",
"44",
"45",
"46",
"47",
"48",
"49",
"50",
"51",
"52",
"53",
"54"
};

static void processLedEffects(void *arg)
{
    printf("GET: Processing LED Effects\n");

    char *formatted_value = NULL;
    cJSON* cjson_properties = (cJSON*)arg;
    cJSON *segment_name_json = cJSON_AddObjectToObject(cjson_properties, "value");

    cJSON *segment_key_json = NULL;
    cJSON *segment_key_value_json = NULL;

    if (segment_name_json)
    {
        for(int i = 0; i < FX_MODE_MAX; i++)
        {
            // formatted_value = ezlopi_valueformatter_int(i);
            // sprintf(key_buffer, "%d", (uint8_t)i);
            // itoa(i, key_buffer, 10);
            segment_key_json = cJSON_AddObjectToObject(segment_name_json,  key_str_list[i]);
            segment_key_value_json = cJSON_AddStringToObject(segment_key_json, "name", LEDModesStrings[i]);
            // segment_key_value_json = cJSON_AddStringToObject(segment_key_json, "description", "Some description");
            // segment_key_value_json = cJSON_AddStringToObject(segment_key_json, "headpieceUrl", "https://some.url");
            // free(formatted_value);
            // cJSON_Delete(segment_key_json);
            // cJSON_Delete(segment_key_value_json);
        }
        cJSON_AddStringToObject(cjson_properties, "valueFormatted", "");
    }

    ESP_LOGE("", "Heap Memory %d", esp_get_minimum_free_heap_size());

}

static void processLedPalettes(void *arg) 
{
    printf("GET: Processing LED Palettes\n");

    char *formatted_value = NULL;
    cJSON* cjson_properties = (cJSON*)arg;
    cJSON *segment_name_json = cJSON_AddObjectToObject(cjson_properties, "value");

    
    cJSON *segment_key_json = NULL;
    cJSON *segment_key_value_json = NULL;
    
    if (segment_name_json)
    {
        for(int i = 0; i < PALETTE_MAX; i++)
        {
            // formatted_value = ezlopi_valueformatter_int(i);
            // sprintf(key_buffer, "%d", (uint8_t)i);
            itoa(i, key_buffer, 10);
            segment_key_json = cJSON_AddObjectToObject(segment_name_json, key_buffer);
            segment_key_value_json = cJSON_AddStringToObject(segment_key_json, "name", PaletteNames[i]);
            // segment_key_value_json = cJSON_AddStringToObject(segment_key_json, "description", "Some description");
            // segment_key_value_json = cJSON_AddStringToObject(segment_key_json, "headpieceUrl", "https://some.url");
            // free(formatted_value);
            // cJSON_Delete(segment_key_json);
            // cJSON_Delete(segment_key_value_json);
        }
        cJSON_AddStringToObject(cjson_properties, "valueFormatted", "");
    }

    ESP_LOGE("", "Heap Memory %d", esp_get_minimum_free_heap_size());
}


static void processSwitch(void *arg) 
{
    printf("GET: Processing Switch\n");
    bool switch_state = (led_config->channel[segment_index]->brightness) ? true : false;

    cJSON *param = (cJSON *)arg;

    cJSON_AddBoolToObject(param, "value", switch_state);
    char *formatted_val = ezlopi_valueformatter_bool(switch_state);
    cJSON_AddStringToObject(param, "valueFormatted", formatted_val);
}

static void processDimmer(void *arg) 
{
    printf("GET: Processing Dimmer\n");

    cJSON *param = (cJSON *)arg;

    cJSON_AddNumberToObject(param, "value", (int)(led_config->channel[segment_index]->brightness * 100));
    char *formatted_val = ezlopi_valueformatter_int((int)(led_config->channel[segment_index]->brightness * 100));
    cJSON_AddStringToObject(param, "valueFormatted", formatted_val);
    free(formatted_val);
}

static void processColorEffectSelect(void *arg) 
{
    printf("GET: Processing Color Effect Select\n");

    cJSON *param = (cJSON *)arg;

    cJSON_AddNumberToObject(param, "value", (int)(led_config->channel[segment_index]->mode));
    char *formatted_val = ezlopi_valueformatter_int((int)(led_config->channel[segment_index]->mode));
    cJSON_AddStringToObject(param, "valueFormatted", formatted_val);
    free(formatted_val);
}

static void processColorPaletteSelect(void *arg) 
{
    printf("GET: Processing Color Palette Select\n");

    cJSON *param = (cJSON *)arg;

    cJSON_AddNumberToObject(param, "value", (int)(led_config->channel[segment_index]->palette));
    char *formatted_val = ezlopi_valueformatter_int((int)(led_config->channel[segment_index]->palette));
    cJSON_AddStringToObject(param, "valueFormatted", formatted_val);
    free(formatted_val);
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

    bool reverse_state = (led_config->channel[segment_index]->options & REVERSE) == REVERSE;

    cJSON *param = (cJSON *)arg;

    cJSON_AddBoolToObject(param, "value", reverse_state);
    char *formatted_val = ezlopi_valueformatter_bool(reverse_state);
    cJSON_AddStringToObject(param, "valueFormatted", formatted_val);
}

static void processColorSegmentMirror(void *arg) 
{
    printf("GET: Processing Color Segment Mirror\n");
}

static void processColorEffectSpeed(void *arg) 
{
    printf("GET: Processing Color Effect Speed\n");

    int effect_speed_int = ((float)(led_config->channel[segment_index]->speed)/ 2000.f)*255.f;

    cJSON *param = (cJSON *)arg;

    cJSON_AddNumberToObject(param, "value", effect_speed_int);
    char *formatted_val = ezlopi_valueformatter_int(effect_speed_int);
    cJSON_AddStringToObject(param, "valueFormatted", formatted_val);
    free(formatted_val);
}

static void processColorEffectIntensity(void *arg) 
{
    printf("GET: Processing Color Effect Intensity\n");

    cJSON *param = (cJSON *)arg;

    cJSON_AddNumberToObject(param, "value", (int)(led_config->channel[segment_index]->intensity));
    char *formatted_val = ezlopi_valueformatter_int((int)(led_config->channel[segment_index]->intensity));
    cJSON_AddStringToObject(param, "valueFormatted", formatted_val);
    free(formatted_val);
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

static void processColorSegmentPrimaryColor(void *arg) 
{
    printf("GET: Processing Color Segment Primary Color\n");

    cJSON *param = (cJSON *)arg;
    cJSON *value_obj = cJSON_AddObjectToObject(param, "value");
    cJSON_AddNumberToObject(value_obj, "wwhite", 0);
    cJSON_AddNumberToObject(value_obj, "red",   (led_config->channel[segment_index]->color[0] & RED)   >> 16   );
    cJSON_AddNumberToObject(value_obj, "green", (led_config->channel[segment_index]->color[0] & GREEN) >> 8    );
    cJSON_AddNumberToObject(value_obj, "blue",  (led_config->channel[segment_index]->color[0] & BLUE)          );
}

static void processColorSegmentSecondaryColor(void *arg) 
{
    printf("GET: Processing Color Segment Secondary Color\n");
    
    cJSON *param = (cJSON *)arg;
    cJSON *value_obj = cJSON_AddObjectToObject(param, "value");
    cJSON_AddNumberToObject(value_obj, "wwhite", 0);
    cJSON_AddNumberToObject(value_obj, "red",   (led_config->channel[segment_index]->color[1] & RED)   >> 16   );
    cJSON_AddNumberToObject(value_obj, "green", (led_config->channel[segment_index]->color[1] & GREEN) >> 8    );
    cJSON_AddNumberToObject(value_obj, "blue",  (led_config->channel[segment_index]->color[1] & BLUE)          );
}

static void processColorSegmentTertiaryColor(void *arg) 
{
    printf("GET: Processing Color Segment Tertiary Color\n");

    cJSON *param = (cJSON *)arg;
    cJSON *value_obj = cJSON_AddObjectToObject(param, "value");
    cJSON_AddNumberToObject(value_obj, "wwhite", 0);
    cJSON_AddNumberToObject(value_obj, "red",   (led_config->channel[segment_index]->color[2] & RED)   >> 16   );
    cJSON_AddNumberToObject(value_obj, "green", (led_config->channel[segment_index]->color[2] & GREEN) >> 8    );
    cJSON_AddNumberToObject(value_obj, "blue",  (led_config->channel[segment_index]->color[2] & BLUE)          );
}

static void processUnknown(void *arg) 
{
    printf("Unknown item name\n");
}

void process_item_get(const char* itemName, void *arg) 
{
    led_config = config_return_config_pointer();
 
  const ItemHandler itemHandlers[] = {
        {ezlopi_item_name_led_segments, processLedSegments},
        {ezlopi_item_name_switch, processSwitch},
        {ezlopi_item_name_dimmer, processDimmer},
        {ezlopi_item_name_led_effects, processLedEffects},
        {ezlopi_item_name_led_palettes, processLedPalettes},
        {ezlopi_item_name_color_effect_select, processColorEffectSelect},
        {ezlopi_item_name_color_palette_select, processColorPaletteSelect},
        {ezlopi_item_name_color_segment_grouping, processColorSegmentGrouping},
        {ezlopi_item_name_color_segment_spacing, processColorSegmentSpacing},
        {ezlopi_item_name_color_segment_offset, processColorSegmentOffset},
        {ezlopi_item_name_color_segment_revers, processColorSegmentReverse},
        {ezlopi_item_name_color_segment_mirror, processColorSegmentMirror},
        {ezlopi_item_name_color_effect_speed, processColorEffectSpeed},
        {ezlopi_item_name_color_effect_intensity, processColorEffectIntensity},
        {ezlopi_item_name_color_effect_custom_slider_1, processColorEffectCustomSlider1},
        {ezlopi_item_name_color_effect_custom_slider_2, processColorEffectCustomSlider2},
        {ezlopi_item_name_color_effect_custom_slider_3, processColorEffectCustomSlider3},
        {ezlopi_item_name_color_effect_option_1, processColorEffectOption1},
        {ezlopi_item_name_color_effect_option_2, processColorEffectOption2},
        {ezlopi_item_name_color_effect_option_3, processColorEffectOption3},
        {ezlopi_item_name_color_segment_primary_color, processColorSegmentPrimaryColor},
        {ezlopi_item_name_color_segment_secondary_color, processColorSegmentSecondaryColor},
        {ezlopi_item_name_color_segment_ternary_color, processColorSegmentTertiaryColor},
        {"unknown", processUnknown}  // Default case
    };

    size_t i;
    for (i = 0; i < sizeof(itemHandlers) / sizeof(itemHandlers[0]); ++i) {
        if (strcmp(itemName, itemHandlers[i].itemName) == 0) {
            get_segment_number(arg);

            vTaskDelay(10/portTICK_PERIOD_MS);

            itemHandlers[i].itemHandler(arg);
            return;
        }
    }

    // If itemName is not found, execute default handler
    itemHandlers[i-1].itemHandler(arg);
}