#include "jsonutil.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

bool parseJSONValue(cJSON* json, char* key, char* value, size_t valueSize){
  if(json == NULL || key == NULL || value == NULL || valueSize == 0)
    return false;
  cJSON *data = cJSON_GetObjectItemCaseSensitive(json, key);
  if (cJSON_IsString(data) && (data->valuestring != NULL)) {
    size_t length = strlen(data->valuestring);
    if(length>=valueSize) {
        cJSON_Delete(json);
        return false;
    }
    strcpy(value, data->valuestring);
    printf("[JSON]:Key: \"%s\": \"%s\"\n", key, value);
    return true;
  }
  cJSON_Delete(json);
  return false;
}

bool validJSON(cJSON* json) {
  if (json == NULL) {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL) {
      printf("[JSON]: JSON Inválido: \"%s\"\n", error_ptr);
    }
    cJSON_Delete(json);
    return false;
  }
  return true;
}
