/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * XXXX for now have this here.
 */
#include <assert.h>

#include <hal/flash_map.h>
#include <hal/hal_bsp.h>

#include <os/os_dev.h>
#include <uart/uart.h>
#include <uart_hal/uart_hal.h>

#include "bsp/bsp.h"
#include "syscfg/syscfg.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_adc.h"
#include <adc_stm32f4/adc_stm32f4.h>

static struct flash_area bsp_flash_areas[] = {
    [FLASH_AREA_BOOTLOADER] = {
        .fa_flash_id = 0,	/* internal flash */
        .fa_off = 0x08000000,	/* beginning */
        .fa_size = (32 * 1024)
    },
    /* 2 * 16K and 1*64K sectors here */
    [FLASH_AREA_IMAGE_0] = {
        .fa_flash_id = 0,
        .fa_off = 0x08020000,
        .fa_size = (384 * 1024)
    },
    [FLASH_AREA_IMAGE_1] = {
        .fa_flash_id = 0,
        .fa_off = 0x08080000,
        .fa_size = (384 * 1024)
    },
    [FLASH_AREA_IMAGE_SCRATCH] = {
        .fa_flash_id = 0,
        .fa_off = 0x080e0000,
        .fa_size = (128 * 1024)
    },
    [FLASH_AREA_NFFS] = {
        .fa_flash_id = 0,
        .fa_off = 0x08008000,
        .fa_size = (32 * 1024)
    }
};
static struct uart_dev hal_uart0;

/* XXX should not be here */

#if MYNEWT_VAL(ADC_1)
struct adc_dev my_dev_adc1;
#endif

struct stm32f4_uart_cfg;
#if MYNEWT_VAL(ADC_3)
struct adc_dev my_dev_adc3;
#endif

extern struct stm32f4_uart_cfg *bsp_uart_config(int port);

/*****************ADC3 Config ***************/

#if MYNEWT_VAL(ADC_3)

ADC_HandleTypeDef adc_handle;

#define STM32F4_DEFAULT_DMA_HANDLE21 {\
    .Instance = DMA2_Stream1,\
    .Init.Channel = DMA_CHANNEL_2,\
    .Init.Direction = DMA_PERIPH_TO_MEMORY,\
    .Init.PeriphInc = DMA_PINC_DISABLE,\
    .Init.MemInc = DMA_MINC_ENABLE,\
    .Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD,\
    .Init.MemDataAlignment = DMA_MDATAALIGN_WORD,\
    .Init.Mode = DMA_CIRCULAR,\
    .Init.Priority = DMA_PRIORITY_HIGH,\
    .Init.FIFOMode = DMA_FIFOMODE_DISABLE,\
    .Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL,\
    .Init.MemBurst = DMA_MBURST_SINGLE,\
    .Init.PeriphBurst = DMA_PBURST_SINGLE,\
    .Parent = &adc_handle,\
}
#endif

#if MYNEWT_VAL(ADC_1)
/*
 * adc_handle is defined earlier because the DMA handle's
 * parent needs to be pointing to the adc_handle
 */
ADC_HandleTypeDef adc_handle120;

DMA_HandleTypeDef adc_dma_handle21 = STM32F4_DEFAULT_DMA_HANDLE21;

#define STM32F4_DEFAULT_DMA_HANDLE20 {\
    .Instance = DMA2_Stream0,\
    .Init.Channel = DMA_CHANNEL_0,\
    .Init.Direction = DMA_PERIPH_TO_MEMORY,\
    .Init.PeriphInc = DMA_PINC_DISABLE,\
    .Init.MemInc = DMA_MINC_ENABLE,\
    .Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD,\
    .Init.MemDataAlignment = DMA_MDATAALIGN_WORD,\
    .Init.Mode = DMA_CIRCULAR,\
    .Init.Priority = DMA_PRIORITY_HIGH,\
    .Init.FIFOMode = DMA_FIFOMODE_DISABLE,\
    .Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL,\
    .Init.MemBurst = DMA_MBURST_SINGLE,\
    .Init.PeriphBurst = DMA_PBURST_SINGLE,\
    .Parent = &adc_handle120,\
}

DMA_HandleTypeDef adc_dma_handle20 = STM32F4_DEFAULT_DMA_HANDLE20;
#endif


#define STM32F4_ADC_DEFAULT_INIT_TD {\
    .ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2,\
    .Resolution = ADC_RESOLUTION12b,\
    .DataAlign = ADC_DATAALIGN_RIGHT,\
    .ScanConvMode = DISABLE,\
    .EOCSelection = DISABLE,\
    .ContinuousConvMode = ENABLE,\
    .NbrOfConversion = 2,\
    .DiscontinuousConvMode = DISABLE,\
    .NbrOfDiscConversion = 0,\
    .ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1,\
    .ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE,\
    .DMAContinuousRequests = ENABLE\
}


#if MYNEWT_VAL(ADC_3)

#define STM32F4_DEFAULT_ADC_HANDLE {\
    .Init = STM32F4_ADC_DEFAULT_INIT_TD,\
    .Instance = ADC3,\
    .NbrOfCurrentConversionRank = 0,\
    .DMA_Handle = &adc_dma_handle21,\
    .Lock = HAL_UNLOCKED,\
    .State = 0,\
    .ErrorCode = 0\
}

ADC_HandleTypeDef adc_handle = STM32F4_DEFAULT_ADC_HANDLE;

#define STM32F4_ADC_DEFAULT_SAC_CHAN {\
    .c_refmv = 3300,\
    .c_res   = 12,\
    .c_configured = 1,\
    .c_cnum = ADC_CHANNEL_4\
}

struct adc_chan_config def_sac_chan = STM32F4_ADC_DEFAULT_SAC_CHAN;

#define STM32F4_ADC_DEFAULT_CONFIG {\
    .sac_chan_count = 16,\
    .sac_chans = &def_sac_chan,\
    .sac_adc_handle = &adc_handle,\
}

struct stm32f4_adc_dev_cfg adc_config = STM32F4_ADC_DEFAULT_CONFIG;
#endif


#if MYNEWT_VAL(ADC_1)

/*****************ADC1 Config ***************/
#define STM32F4_DEFAULT_ADC_HANDLE120 {\
    .Init = STM32F4_ADC_DEFAULT_INIT_TD,\
    .Instance = ADC1,\
    .NbrOfCurrentConversionRank = 0,\
    .DMA_Handle = &adc_dma_handle20,\
    .Lock = HAL_UNLOCKED,\
    .State = 0,\
    .ErrorCode = 0\
}

ADC_HandleTypeDef adc_handle120 = STM32F4_DEFAULT_ADC_HANDLE120;

#define STM32F4_ADC_DEFAULT_SAC_CHAN10 {\
    .c_refmv = 3300,\
    .c_res   = 12,\
    .c_configured = 1,\
    .c_cnum = ADC_CHANNEL_10\
}

struct adc_chan_config def_sac_chan10 = STM32F4_ADC_DEFAULT_SAC_CHAN10;

#define STM32F4_ADC_DEFAULT_CONFIG_1_2_0_10 {\
    .sac_chan_count = 16,\
    .sac_chans = &def_sac_chan10,\
    .sac_adc_handle = &adc_handle120,\
}

struct stm32f4_adc_dev_cfg adc_config12010 = STM32F4_ADC_DEFAULT_CONFIG_1_2_0_10;
/*********************************************/
#endif

void _close(int fd);

/*
 * Returns the flash map slot where the currently active image is located.
 * If executing from internal flash from fixed location, that slot would
 * be easy to find.
 * If images are in external flash, and copied to RAM for execution, then
 * this routine would have to figure out which one of those slots is being
 * used.
 */
int
bsp_imgr_current_slot(void)
{
    return FLASH_AREA_IMAGE_0;
}

void
bsp_init(void)
{
    int rc;

    /*
     * XXX this reference is here to keep this function in.
     */
    _sbrk(0);
    _close(0);
    flash_area_init(bsp_flash_areas,
                    sizeof(bsp_flash_areas) / sizeof(bsp_flash_areas[0]));

    rc = os_dev_create((struct os_dev *) &hal_uart0, CONSOLE_UART,
      OS_DEV_INIT_PRIMARY, 0, uart_hal_init, (void *)bsp_uart_config(0));
    assert(rc == 0);
#if MYNEWT_VAL(ADC_3)
    rc = os_dev_create((struct os_dev *) &my_dev_adc3, "adc3",
            OS_DEV_INIT_KERNEL, OS_DEV_INIT_PRIO_DEFAULT,
            stm32f4_adc_dev_init, &adc_config);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(ADC_1)
    rc = os_dev_create((struct os_dev *) &my_dev_adc1, "adc1",
            OS_DEV_INIT_KERNEL, OS_DEV_INIT_PRIO_DEFAULT,
            stm32f4_adc_dev_init, &adc_config12010);
    assert(rc == 0);
#endif
}

