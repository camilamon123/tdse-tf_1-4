/*
 * app.h
 *
 *  Created on: Jan 22, 2026
 *      Author: feder
 */

#ifndef INC_APP_APP_H_
#define INC_APP_APP_H_

#include <stdint.h>
#include "stm32f1xx_hal.h"

/* ====== MODOS ====== */
typedef enum {
  MODE_MORSE = 0,
  MODE_LDR_SIM,     // monitor LDR (raw/filt/pct/LEVEL)
  MODE_CALIB_SIM,   // placeholder
  MODE_LIGHT_MORSE,  // morse usando LEVEL del LDR
  MODE_SETUP_BTN,
  MODE_NONE         // sin modo activo (idle)
} app_mode_t;

void App_Init(UART_HandleTypeDef *huart);
void App_Task(uint32_t now);
void App_OnUartRxByte(uint8_t b);

/* ====== GETTERS para UI (OLED / debug) ====== */
app_mode_t  App_GetMode(void);
uint32_t    App_GetTuMs(void);
const char* App_GetSymbolDbg(void);
const char* App_GetLastPhrase(void);
uint8_t     App_GetBtConnected(void);
uint8_t     App_GetLdrPct(void);


#endif /* INC_APP_APP_H_ */
