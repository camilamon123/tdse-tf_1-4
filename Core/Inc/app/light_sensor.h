/*
 * light_sensor.h
 *
 *  Created on: Jan 24, 2026
 *      Author: feder
 *
 */

#ifndef INC_APP_LIGHT_SENSOR_H_
#define INC_APP_LIGHT_SENSOR_H_

#pragma once
#include <stdint.h>

void LightSensor_SyncLevel(uint16_t value);
/* Inicializa el módulo con thresholds por defecto */
void LightSensor_Init(uint16_t th_low_default, uint16_t th_high_default);

/* Thresholds */
uint16_t LightSensor_GetThLow(void);
uint16_t LightSensor_GetThHigh(void);
int      LightSensor_SetThresholds(uint16_t th_low, uint16_t th_high);

/* Level con histéresis: 0/1 */
uint8_t  LightSensor_GetLevel(void);
uint8_t  LightSensor_UpdateLevel(uint16_t value);

/* Porcentaje normalizado (0..100) usando thresholds */
uint8_t  LightSensor_ComputePercent(uint16_t value);

/* Calibración */
void     LightSensor_CalibStart(void);
int      LightSensor_CalibSetLow(uint16_t value);
int      LightSensor_CalibSetHigh(uint16_t value);

#endif /* INC_APP_LIGHT_SENSOR_H_ */
