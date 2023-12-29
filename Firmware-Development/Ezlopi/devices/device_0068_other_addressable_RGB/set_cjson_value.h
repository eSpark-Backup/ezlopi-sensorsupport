#ifndef SET_CJSON_VALUE_FILE_H
#define SET_CJSON_VALUE_FILE_H

uint8_t segment_index;

typedef struct {
    const char* itemName;
    void (*itemHandler)(void* arg);
} ItemHandler;

void processItem(const char* itemName, void *arg);
void process_item_get(const char* itemName, void *arg);


#endif // SET_CJSON_VALUE_FILE_H
