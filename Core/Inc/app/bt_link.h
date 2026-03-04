/*
 * bt_link.h
 *
 *  Created on: Jan 31, 2026
 *      Author: feder
 */

#ifndef APP_BT_LINK_H
#define APP_BT_LINK_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa el link Bluetooth (HC-05) sobre un UART.
 * - huart puede ser NULL: deja el link deshabilitado.
 */
void BtLink_Init(UART_HandleTypeDef *huart);

void BtLink_SetEnabled(uint8_t en);
uint8_t BtLink_IsEnabled(void);

/* Envia datos por Bluetooth (si está habilitado).
 * Nota: esta función filtra/normaliza ciertas salidas para que en apps tipo
 * "Serial Bluetooth Terminal" se vean bien
 */
void BtLink_Write(const uint8_t *data, uint16_t len);
void BtLink_WriteStr(const char *s);

/* --- Recepción (para usar la misma consola/CLI desde Bluetooth) --- */

/* Registra el callback que recibe bytes (fuera de interrupción) */
void BtLink_AttachCliRx(void (*on_byte)(uint8_t b));

/* Debe llamarse desde HAL_UART_RxCpltCallback().
 * Devuelve 1 si el evento pertenece al UART de Bluetooth y fue consumido.
 */
uint8_t BtLink_OnUartRxCplt(UART_HandleTypeDef *huart);

/* Llamar periódicamente (por ejemplo desde App_Task) para entregar bytes
 * recibidos al callback registrado.
 */
void BtLink_Task(void);

#ifdef __cplusplus
}
#endif

#endif

