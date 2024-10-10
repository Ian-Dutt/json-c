#ifndef __JSON__
#define __JSON__

typedef enum {
    STR,
    JSON,
    NUMBER
} JsonType;

typedef struct _json_obj{
    JsonType type;
    char *key;
    int size;
    // void *value;
    struct {
        struct _json_obj **children;
        double number;
        char *string;
    };
} JsonObject;

JsonObject *createJsonFromFile(const char *file_name);

JsonObject *parseJsonString(char *json);

void printJson(JsonObject *json);

void free_json(JsonObject *json);

#endif