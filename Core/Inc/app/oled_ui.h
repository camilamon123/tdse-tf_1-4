/*
 * oled_ui.h
 *
 *  Created on: Feb 10, 2026
 *      Author: feder
 */

#ifndef APP_OLED_UI_H_
#define APP_OLED_UI_H_

#include "stm32f1xx_hal.h"
#include "app/app.h"   /* app_mode_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa el módulo OLED UI (usa SSD1306 SPI).
 * Llamar una vez al boot, después de MX_SPI1_Init y GPIO init.
 */
void OledUI_Init(SPI_HandleTypeDef *hspi);

/* Snapshot: lo llama App_Task (o main) para pasarle “qué mostrar”.
 * OledUI_Task se encarga de dibujarlo con rate-limit.
 */
void OledUI_SetSnapshot(app_mode_t mode,
                        uint8_t bt_connected,
                        uint32_t tu_ms,
                        uint16_t th_low,
                        uint16_t th_high,
                        uint8_t ldr_pct,
                        const char *sym_dbg,
                        const char *last_phrase);

/* Tarea no bloqueante (rate-limit de refresh) */
void OledUI_Task(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* APP_OLED_UI_H_ */
