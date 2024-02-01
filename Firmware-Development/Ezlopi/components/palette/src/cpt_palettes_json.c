#include "cpt_palettes_json.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static struct paletteContainer{
    const uint8_t* palette;
    uint8_t size;
};

char pre_allocated_buffer_3k[3000];

const struct paletteContainer gGradientPalettes[];

// Function to convert a palette to cJSON format
cJSON *paletteToJSON(const uint8_t *palette, uint8_t size) {
    if (palette == NULL) {
        fprintf(stderr, "Error: Invalid palette or size\n");
        return NULL;
    }
    if(size == 0)
    {
        fprintf(stderr, "Error: Size %d\n", size);
        return NULL;
    }

    cJSON *json_palette = cJSON_CreateArray();
    for (size_t i = 0; i < size * 4; i+=4) {
        // Create a cJSON array for each color
        cJSON *color = cJSON_CreateArray();
        // Add each component of the color to the cJSON array
        cJSON_AddItemToArray(color, cJSON_CreateNumber(*(palette + ((i * 4) + 0))));  // R
        cJSON_AddItemToArray(color, cJSON_CreateNumber(*(palette + ((i * 4) + 1))));  // G
        cJSON_AddItemToArray(color, cJSON_CreateNumber(*(palette + ((i * 4) + 2))));  // B
        cJSON_AddItemToArray(color, cJSON_CreateNumber(*(palette + ((i * 4) + 3))));  // A

        // Print the color components
        // printf("Color %zu: R=%u, G=%u, B=%u, A=%u\n", i / 4, *(palette + ((i * 4) + 0)), *(palette + ((i * 4) + 1)), *(palette + ((i * 4) + 2)), *(palette + ((i * 4) + 3)));

        // Add the cJSON color array to the palette array
        cJSON_AddItemToArray(json_palette, color);
    }
    return json_palette;
}

// Facade function to convert all palettes to cJSON format
cJSON *allPalettesToJSON(const struct paletteContainer *palettes, size_t numPalettes) {
    cJSON *root = cJSON_CreateObject();
    cJSON *pRoot = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "p", pRoot);
    for (size_t i = 0; i < numPalettes; ++i) {
        vTaskDelay(10/portTICK_PERIOD_MS);
        cJSON *json_palette = paletteToJSON(palettes[i].palette, palettes[i].size);
        if (json_palette != NULL) {
            printf("Converting Palette %d to json\n", i);
            char palette_name[5]; // Adjust the size as needed
            snprintf(palette_name, sizeof(palette_name), "%d", (uint8_t)i);
            cJSON_AddItemToObject(pRoot, palette_name, json_palette);
        }
        else {
            printf(stderr,"Unable to create json for palette %d\n", i);
        }
    }
    return root;
}

// Function to get all palettes cJSON as a string
char *getAllPalettesJSONString(const struct paletteContainer *palettes, size_t numPalettes) {
    cJSON *root = allPalettesToJSON(palettes, numPalettes);
    char *json_string = NULL;
    if(root == NULL)
    {
        printf(stderr,"unable to get json from allPalettesToJSON\n");
    }
    else{
        json_string = cJSON_PrintUnformatted(root);
        if(json_string == NULL)
        {
            printf(stderr,"unable to format!\n");
        }
        else
        {
            printf(stderr,"printunformatted successfull!\n");
        }
    }
    cJSON_Delete(root);
    return json_string;
}

char* palettesToJsonString()
{
    // Assuming gGradientPalettes is defined and contains the palettes
    char *json_string = getAllPalettesJSONString(gGradientPalettes, 30); //TODO: Size 
    if (json_string) {
        printf("Palette preview JSON String is ready"); // Print the JSON string for debugging
    } else {
        fprintf(stderr, "Error: Failed to create JSON string.\n");
    }
    return json_string; // Return the JSON string (might be NULL if an error occurred)
}