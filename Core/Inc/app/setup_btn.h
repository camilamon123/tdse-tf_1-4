/*
 * setup_btn.h
 *
 *  Created on: Jan 26, 2026
 *      Author: feder
 */

#ifndef SETUP_BTN_H_
#define SETUP_BTN_H_

#include <stdint.h>
#include "stm32f1xx_hal.h"

/* Inicializa el módulo (solo para imprimir por UART) */
void SetupBtn_Init(UART_HandleTypeDef *huart);

/* Arranca el proceso de calibración por botón */
void SetupBtn_Start(uint32_t now);

/* Se ejecuta en el super-loop (no bloqueante) */
void SetupBtn_Task(uint32_t now);

#endif /* SETUP_BTN_H_ */
