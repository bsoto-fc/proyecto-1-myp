#ifndef JSONUTIL_H_
#define JSONUTIL_H_

#include <cjson/cJSON.h>
#include <stdbool.h>

bool parseJSONValue(cJSON* json, char* key, char* value, size_t valueSize);

bool validJSON(cJSON* json); 

#endif // JSONUTIL_H_
