#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED
#include "types.h"

#define CMD_SHUTDOWN 0
#define CMD_RAM_RUN 1
#define CMD_EEPROM 2
#define CMD_EEPROM_RUN 3

void prop_action(const char* device, u32 cmd);

#endif // LOADER_H_INCLUDED
