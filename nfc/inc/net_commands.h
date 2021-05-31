#ifndef NET_COMMANDS_H_
#define NET_COMMANDS_H_

#include <stdint.h>
#include <stdbool.h>

#include "parson.h"

#ifdef __cplusplus
extern "C" {
#endif

JSON_Value * cmd_execute(char const *cmd, JSON_Value const *pars);

#ifdef __cplusplus
}
#endif

#endif /* NET_COMMANDS_H_ */
