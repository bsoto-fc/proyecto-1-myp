#ifndef ARGPARSER_H_
#define ARGPARSER_H_

#include <stdint.h>

uint16_t ReadPortFromString(char* str);

uint16_t ParsePort(int argc, char* argv[]);

#endif // ARGPARSER_H_
