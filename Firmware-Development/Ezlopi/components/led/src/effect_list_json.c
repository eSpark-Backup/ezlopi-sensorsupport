#include "effect_list_json.h"

#include "cJSON.h"
#include <stdio.h>
#define MAX_EFFECTS__ (sizeof(effect_name_list)/sizeof(effect_name_list[0]) )
#define MAX_PALETTES__ (sizeof(palette_name_list)/sizeof(palette_name_list[0]) )

#define FX_MODE(x , y, z) y,
char* effect_name_list[] = {
    #include "effect_list.h"
};
#undef FX_MODE(x , y, z)

#define FX_MODE(x , y, z) z,
char* effect_configuration_list[] = {
    #include "effect_list.h"
};
#undef FX_MODE(x , y, z)

#define PALETTE(x, y) y,
char* palette_name_list[] = {
    #include "palette_list.h"
};
#undef PALETTE(x, y)

extern const char _data_FX_MODE_BLINK[];

void create_list_json_string(char** json_string, enum PAL_EFF pal_eff)
{
    // cJSON *json_object = cJSON_CreateObject();
    cJSON *json_array = cJSON_CreateArray();
    size_t total_items = (pal_eff == EFFECT_JSON)? MAX_EFFECTS__ : MAX_PALETTES__;
    char** list = (pal_eff == EFFECT_JSON)? effect_name_list : palette_name_list;
    for(size_t i = 0; i < total_items-1; i++)
    {
        cJSON_AddItemToArray(json_array, cJSON_CreateString(list[i]));
    }
    // cJSON_AddItemToObject(json_object, "palettes", json_array);
    *json_string = cJSON_Print(json_array);
    // printf("%s\n",json_string);
    // cJSON_Minify(json_string);
    cJSON_Delete(json_array);
}

void create_fxdata_json_string(char** json_string)
{
    cJSON *json_array = cJSON_CreateArray();
    char** list = effect_configuration_list;
    for(size_t i = 0; i < MAX_EFFECTS__-1; i++)
    {
        cJSON_AddItemToArray(json_array, cJSON_CreateString(list[i]));
    }
    // cJSON_AddItemToArray(json_array, cJSON_CreateString(""));
    // cJSON_AddItemToArray(json_array, cJSON_CreateString(_data_FX_MODE_BLINK));
    // cJSON_AddItemToArray(json_array, cJSON_CreateString("!,Duty cycle;!,!;!;01"));
    *json_string = cJSON_Print(json_array);
    cJSON_Delete(json_array);

}