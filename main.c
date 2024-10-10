#include "json.h"

int main(){
    JsonObject *json = createJsonFromFile("json");

    printJson(json);

    free_json(json);
}