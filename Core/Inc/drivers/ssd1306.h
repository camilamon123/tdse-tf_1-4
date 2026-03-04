/*
 * ssd1306.h
 *
 *  Created on: Feb 10, 2026
 *      Author: feder
 */

#ifndef DRIVERS_SSD1306_H_
#define DRIVERS_SSD1306_H_

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Tamaño típico */
#ifndef SSD1306_WIDTH
#define SSD1306_WIDTH   128u
#endif

#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT   64u
#endif

#define SSD1306_PAGES   (SSD1306_HEIGHT / 8u)

typedef struct
{
  SPI_HandleTypeDef *hspi;

  GPIO_TypeDef *cs_port;
  uint16_t      cs_pin;

  GPIO_TypeDef *dc_port;
  uint16_t      dc_pin;

  GPIO_TypeDef *res_port;
  uint16_t      res_pin;

  uint8_t width;
  uint8_t height;
  uint8_t pages;

  uint8_t x_offset;

  /* cursor para texto */
  uint8_t cursor_x;
  uint8_t cursor_y;

  /* Framebuffer: 128*64/8 = 1024 bytes */
  uint8_t buf[SSD1306_WIDTH * SSD1306_PAGES];

} ssd1306_t;

/* ====== API ====== */

/**
 * @brief Inicializa el display SSD1306 por SPI.
 * @param d       puntero a struct display
 * @param hspi    handle SPI (ej: &hspi1)
 * @param cs_port/cs_pin   GPIO CS
 * @param dc_port/dc_pin   GPIO DC
 * @param res_port/res_pin GPIO RES
 * @param addr7   (NO usado en SPI; se deja para compat si venías de I2C) -> pasar 0
 *
 * @return HAL_OK si pudo inicializar
 */
HAL_StatusTypeDef SSD1306_Init(ssd1306_t *d,
                               SPI_HandleTypeDef *hspi,
                               GPIO_TypeDef *cs_port, uint16_t cs_pin,
                               GPIO_TypeDef *dc_port, uint16_t dc_pin,
                               GPIO_TypeDef *res_port, uint16_t res_pin,
                               uint8_t addr7);

/* Limpia el framebuffer (no actualiza display hasta Update) */
void SSD1306_Clear(ssd1306_t *d);

/* Rellena el framebuffer con 0 (negro) o 1 (blanco) */
void SSD1306_Fill(ssd1306_t *d, uint8_t color);

/* Envía framebuffer completo al display */
HAL_StatusTypeDef SSD1306_Update(ssd1306_t *d);

/* Primitivas */
void SSD1306_DrawPixel(ssd1306_t *d, uint8_t x, uint8_t y, uint8_t color);

/* Texto 5x7 */
void SSD1306_SetCursor(ssd1306_t *d, uint8_t x, uint8_t y);
void SSD1306_DrawChar5x7(ssd1306_t *d, char c);
void SSD1306_DrawString5x7(ssd1306_t *d, const char *s);

/* dibuja string en (x,y) sin depender del cursor previo */
static inline void SSD1306_DrawString5x7At(ssd1306_t *d, uint8_t x, uint8_t y, const char *s)
{
  SSD1306_SetCursor(d, x, y);
  SSD1306_DrawString5x7(d, s);
}

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_SSD1306_H_ */


