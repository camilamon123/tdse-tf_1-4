/*
 * nv_store.c
 *
 *  Created on: Jan 26, 2026
 *      Author: feder
 */

#include "app/nv_store.h"

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_flash_ex.h"

/* ===================== Config ===================== */

#define NV_MAGIC       0x4D534F52UL  /* 'MSOR' */
#define NV_VERSION     1U

/* STM32F103RB: página de 1 KB (densidad media) */
#define FLASH_PAGE_SIZE_BYTES  0x400U

/* Layout en Flash (halfwords):
 *  [0] magic LSW
 *  [1] magic MSW
 *  [2] version
 *  [3] th_low
 *  [4] th_high
 *  [5] tu_ms
 *  [6] crc (xor de [0..5])
 */
#define NV_WORDS 7U

static uint16_t nv_crc_xor(const uint16_t *w, uint32_t n)
{
  uint16_t crc = 0;
  for (uint32_t i = 0; i < n; i++) crc ^= w[i];
  return crc;
}

static uint32_t nv_flash_size_bytes(void)
{
  /* FLASHSIZE_BASE viene definido por el header del device */
  uint16_t kb = *(uint16_t *)FLASHSIZE_BASE;
  return (uint32_t)kb * 1024U;
}

static uint32_t nv_base_addr(void)
{
  uint32_t size = nv_flash_size_bytes();
  return FLASH_BASE + size - FLASH_PAGE_SIZE_BYTES;
}

int NvStore_Clear(void)
{
  uint32_t page_addr = nv_base_addr();

  HAL_FLASH_Unlock();

  FLASH_EraseInitTypeDef erase;
  uint32_t page_error = 0;

  erase.TypeErase   = FLASH_TYPEERASE_PAGES;
  erase.PageAddress = page_addr;
  erase.NbPages     = 1;

  HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&erase, &page_error);

  HAL_FLASH_Lock();

  return (st == HAL_OK) ? 0 : -1;
}

int NvStore_Save(const nv_cfg_t *cfg)
{
  if (cfg == NULL) return -1;

  /* Validaciones básicas */
  if (cfg->th_low >= cfg->th_high) return -1;
  if (cfg->tu_ms < 80U || cfg->tu_ms > 2000U) return -1;

  uint16_t w[NV_WORDS];

  uint32_t magic = NV_MAGIC;
  w[0] = (uint16_t)(magic & 0xFFFFU);
  w[1] = (uint16_t)((magic >> 16) & 0xFFFFU);
  w[2] = (uint16_t)NV_VERSION;
  w[3] = cfg->th_low;
  w[4] = cfg->th_high;
  w[5] = cfg->tu_ms;
  w[6] = nv_crc_xor(w, NV_WORDS - 1U);

  /* Borramos página y programamos */
  if (NvStore_Clear() != 0) return -1;

  uint32_t addr = nv_base_addr();

  HAL_FLASH_Unlock();

  for (uint32_t i = 0; i < NV_WORDS; i++)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, w[i]) != HAL_OK)
    {
      HAL_FLASH_Lock();
      return -1;
    }
    addr += 2U;
  }

  HAL_FLASH_Lock();
  return 0;
}

int NvStore_Load(nv_cfg_t *cfg)
{
  if (cfg == NULL) return -1;

  uint32_t addr = nv_base_addr();

  uint16_t w[NV_WORDS];
  for (uint32_t i = 0; i < NV_WORDS; i++)
  {
    w[i] = *(volatile uint16_t *)(addr + 2U * i);
  }

  uint32_t magic = ((uint32_t)w[1] << 16) | (uint32_t)w[0];
  if (magic != NV_MAGIC) return -1;

  if (w[2] != (uint16_t)NV_VERSION) return -1;

  uint16_t crc = nv_crc_xor(w, NV_WORDS - 1U);
  if (crc != w[6]) return -1;

  nv_cfg_t out;
  out.th_low  = w[3];
  out.th_high = w[4];
  out.tu_ms   = w[5];

  /* Validaciones */
  if (out.th_low >= out.th_high) return -1;
  if (out.tu_ms < 80U || out.tu_ms > 2000U) return -1;

  *cfg = out;
  return 0;
}
