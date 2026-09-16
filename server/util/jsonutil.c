#include "jsonutil.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdbool.h>

bool parseJSONValue(char* buffer, char* key, char* value){
  cJSON* json = cJSON_Parse(buffer);
  // Boilerplate
  if (json == NULL) {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL) {
      printf("Error en respuesta: %s\n", error_ptr);
    }
    cJSON_Delete(json);
    return false;
  }
  cJSON *data = cJSON_GetObjectItemCaseSensitive(json, key);
  if (cJSON_IsString(data) && (data->valuestring != NULL)) {
    value = data->valuestring;
    printf("%s: %s\n", key, value);
    return true;
  }
  cJSON_Delete(json);
  return false;
}

bool validJSON(char* buffer) {
  cJSON* json = cJSON_Parse(buffer);
  // Boilerplate
  if (json == NULL) {
    cJSON_Delete(json);
    return false;
  }
  return true;
}
