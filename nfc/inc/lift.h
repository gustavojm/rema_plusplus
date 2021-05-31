#ifndef LIFT_H_
#define LIFT_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIFT_DIRECTION_CHANGE_DELAY_MS	500

enum lift_direction {
	LIFT_DIRECTION_UP = 0, LIFT_DIRECTION_DOWN = 1,
};

enum lift_type {
	LIFT_TYPE_UP = 0, LIFT_TYPE_DOWN = 1, LIFT_TYPE_STOP
};

/**
 * @struct 	lift_msg
 * @brief	messages to lift_task.
 */
struct lift_msg {
	enum lift_type type;
};

/**
 * @struct 	lift
 * @brief	lift task status.
 */
struct lift {
	enum lift_type type;
	volatile bool upLimit;
	volatile bool downLimit;
};

void lift_init();

struct lift *lift_status_get(void);

#ifdef __cplusplus
}
#endif

#endif /* LIFT_H_ */
