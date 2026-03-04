/*
 * buzzer.h
 *
 *  Created on: Jan 25, 2026
 *      Author: feder
 */

#pragma once
#include <stdint.h>
#include "stm32f1xx_hal.h"

/* Inicializa el buzzer usando un pin GPIO */
void Buzzer_Init(GPIO_TypeDef *port, uint16_t pin);

/* Pedir un beep no bloqueante */
void Buzzer_Beep(uint32_t ms);

/* Mantener no bloqueante (apagar cuando cumpla el tiempo) */
void Buzzer_Task(uint32_t now);
