#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "json.h"

#define MAX_SIZE 50000

void *new(size_t size){
    void *result = calloc(1, size);
    if(result == NULL){
        fprintf(stderr, "Could not allocate memory\n");
        exit(1);
    }
    return result;
}

char *readfile(FILE *file){
    size_t size;
    char *file_contents;
    fseek(file, 0L, SEEK_END);
    size = ftell(file);
    rewind(file);

    file_contents = new(size * sizeof(char));
    fread(file_contents, sizeof(char), size, file);

    return file_contents;
}

void *_grow(void *list, size_t len, size_t value_size, void *value){
    void **new_list = realloc(list, (len + 1) * value_size);
    if(new_list == NULL){
        free(list);
        exit(1);
    }

    new_list[len] = value;
    return new_list;
}

char *copy_str(char *str){
    char *copy = strdup(str);
    if(copy == NULL){
        fprintf(stderr, "Could not copy the string\n");
        exit(1);
    }

    return copy;
}

int is_number(char *str, double *num){
    char *end;
    *num = strtod(str, &end);

    if(*end != '\0'){
        fprintf(stderr, "%s could not be converted to a number\n", str);
        return 0;
    }
    return 1;
}

#define grow(list, len, val) list = _grow(list, len++, sizeof(val), val)
void _printJsonHelper(JsonObject *json, int indent){
    int i;
    printf("{\n");
    indent += 2;
    for(i = 0; i < json->size; ++i){
        if(json->children[i]->type == STR){
            printf("%*s'%s': '%s'", -indent, "", json->children[i]->key, json->children[i]->string);
        }else if(json->children[i]->type == NUMBER){
            printf("%*s'%s': %f", -indent, "", json->children[i]->key, json->children[i]->number);
        }else if(json->children[i]->type == JSON){
            printf("%*s'%s': ", -indent, "", json->children[i]->key);
            _printJsonHelper(json->children[i], indent);
        }else{
            fprintf(stderr, "Print does not hendle a type\n");
        }
        printf("%s\n", i == json->size - 1 ? "" : ",");
    }
    printf("%*s}%s", indent - 2, "", indent != 2 ? "" : "\n");
}

void printJson(JsonObject *json){
    _printJsonHelper(json, 0);
}

JsonObject *keyValToJson(char *key, char *value){
    JsonObject *json = new(sizeof(JsonObject));
    double number;

    json->key = copy_str(key);

    if(value[0] == '"'){
        value[strlen(value) - 1] = '\0';
        json->string = copy_str(value + 1);
        json->type = STR;
    }else if(is_number(value, &number)){
        json->number = number;
        json->type = NUMBER;
    }else{
        fprintf(stderr, "This is not supported\n");
        exit(1);
    }
    return json;
}

#define ignorewhitespace(str, i) for(; str[i] && isspace(str[i]); ++i)
#define validate(v, e) if(v != e){ fprintf(stderr,  "%d: %c does not equal %c\n", __LINE__, v, e); exit(1);}

JsonObject *_parseJsonString(char *jsonString, int *out_i){
    char key[MAX_SIZE];
    char value[MAX_SIZE];

    JsonObject *json = new(sizeof(JsonObject));
    json->type = JSON;

    int i = *out_i, key_i = 0, val_i = 0;
    validate(jsonString[i], '{');
    for(i = i + 1; jsonString[i]; ++i){
        ignorewhitespace(jsonString, i);
        validate(jsonString[i], '"');
        /* read the key */
        for(i = i + 1; jsonString[i] && jsonString[i] != '"'; ++i){
            key[key_i++] = jsonString[i];
        }
        key[key_i] = '\0';
        validate(jsonString[i], '"');
        i++;
        ignorewhitespace(jsonString, i);
        validate(jsonString[i], ':');
        i++;
        ignorewhitespace(jsonString, i);
        if(jsonString[i] == '"'){
            value[val_i++] = '"';
            /* read the value */
            for(i = i + 1; jsonString[i] && jsonString[i] != '"'; ++i){
                value[val_i++] = jsonString[i];
            }
            value[val_i++] = '"';
            value[val_i] = '\0';
            validate(jsonString[i], '"');
            i++;
            grow(json->children, json->size, keyValToJson(key, value));
        }else if(jsonString[i] == '{'){
            JsonObject *new_obj = _parseJsonString(jsonString, &i);
            new_obj->key = copy_str(key);
            grow(json->children, json->size, new_obj);
        }else{
            for(; jsonString[i] && !isspace(jsonString[i]) && jsonString[i] != ',' && jsonString[i] != '}'; ++i){
                value[val_i++] = jsonString[i];
            }
            value[val_i] = '\0';
            grow(json->children, json->size, keyValToJson(key, value));
        }

        val_i = 0;
        key_i = 0;
        if(jsonString[i] != ',') break;

    }
    ignorewhitespace(jsonString, i);
    validate(jsonString[i], '}');
    i++;
    *out_i = i;
    // validate(jsonString[i], '}');
    return json;
}
#undef validate
#undef ignorewhitespace

JsonObject *parseJsonString(char *jsonString){
    int i = 0;
    return _parseJsonString(jsonString, &i);
}

JsonObject *createJsonFromFile(const char *file_name){
    JsonObject *json;
    FILE *jsonFile = fopen(file_name, "r");
    if(jsonFile == NULL){
        fprintf(stderr, "Could not open file %s\n", file_name);
        exit(1);
    }
    char *json_string = readfile(jsonFile);
    json = parseJsonString(json_string);

    free(json_string);
    fclose(jsonFile);

    return json;
}

void free_json(JsonObject *json){
    int i;
    for(i = 0; i < json->size; i++){
        if(json->children[i]->type == STR){
            free(json->children[i]->key);
            free(json->children[i]->string);
            free(json->children[i]);
        }else if(json->children[i]->type == NUMBER){
            free(json->children[i]->key);
            free(json->children[i]);           
        }else if(json->children[i]->type == JSON){
            free(json->children[i]->key);
            free_json(json->children[i]);
        }else{
            fprintf(stderr, "Unsupported free operation");
            free(json->children[i]);
        }
    }
    free(json->children);
    free(json);
}