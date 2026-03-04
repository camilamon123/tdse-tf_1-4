/*
 * button_input.h
 *
 *  Created on: Jan 21, 2026
 *      Author: feder
 */

#ifndef INC_APP_BUTTON_INPUT_H_
#define INC_APP_BUTTON_INPUT_H_

#pragma once
#include <stdint.h>

void ButtonInput_Init(void);
uint8_t ButtonInput_GetStable(void); // 1=presionado(ON), 0=suelto(OFF)

#endif /* INC_APP_BUTTON_INPUT_H_ */
