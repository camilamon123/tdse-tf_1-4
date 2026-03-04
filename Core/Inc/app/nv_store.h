/*
 * nv_store.h
 *
 *  Created on: Jan 26, 2026
 *      Author: feder
 */

#ifndef NV_STORE_H
#define NV_STORE_H

#include <stdint.h>

/*
 * Una memoria simple en Flash interna (STM32F103).
 * Que guarda:
 *   - th_low
 *   - th_high
 *   - tu_ms
 */

typedef struct
{
  uint16_t th_low;
  uint16_t th_high;
  uint16_t tu_ms;
} nv_cfg_t;

/* Retorna 0 si hay config válida y la carga en cfg. */
int NvStore_Load(nv_cfg_t *cfg);

/* Retorna 0 si pudo guardar cfg. */
int NvStore_Save(const nv_cfg_t *cfg);

/* Borra (erase) la configuración persistida. */
int NvStore_Clear(void);

#endif /* NV_STORE_H */

