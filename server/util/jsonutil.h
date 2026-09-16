#ifndef JSONUTIL_H_
#define JSONUTIL_H_

#include <cjson/cJSON.h>
#include <stdbool.h>

bool parseJSONValue(char* buffer, char* key, char* value);

#endif // JSONUTIL_H_
