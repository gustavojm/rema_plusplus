#ifndef NET_COMMANDS_H_
#define NET_COMMANDS_H_

#include <cstdint>

#include "parson.h"

JSON_Value * cmd_execute(char const *cmd, JSON_Value const *pars);

#endif /* NET_COMMANDS_H_ */
