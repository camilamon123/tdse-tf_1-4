/*
 * ldr.h
 *
 *  Created on: Jan 21, 2026
 *      Author: feder
 */

#ifndef INC_DRIVERS_LDR_H_
#define INC_DRIVERS_LDR_H_

#include <stdint.h>

void     LDR_Init(void);

/* lee ADC real (1 conversión) */
uint16_t LDR_ReadRaw(void);

/* lee ADC real (1 conversión) + actualiza filtro EMA + devuelve filtrado */
uint16_t LDR_ReadFiltered(void);

/* Devuelve el último RAW leído */
uint16_t LDR_GetLastRaw(void);

#endif /* INC_DRIVERS_LDR_H_ */

