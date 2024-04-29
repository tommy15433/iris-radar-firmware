/***********************************************************************************************//**
 * \file xensiv_bgt60trxx_mtb.c
 *
 * \brief
 * This file contains the MTB platform functions implementation
 * for interacting with the XENSIV(TM) BGT60TRxx 60GHz FMCW radar sensors.
 *
 ***************************************************************************************************
 * \copyright
 * Copyright 2022 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************************************/


// #include "cyhal_system.h"
#include "xensiv_bgt60trxx_stm.h"
#include "xensiv_bgt60trxx_platform.h"
#include <stdint.h>

/*******************************************************************************
* Macros
*******************************************************************************/
// #define XENSIV_BGT60TRXX_ERROR(x)           (((x) == XENSIV_BGT60TRXX_STATUS_OK) ? stm_result_ok :\
//                                              CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_BOARD_HARDWARE_XENSIV_BGT60TRXX, (x)))

static stm_gpio_t NC = {
    .gpio = 0,
    .pin = 0
};

/*******************************************************************************
 * Function Prototypes
 ********************************************************************************/
static inline bool pins_equal(stm_gpio_int_t ref_pin, stm_gpio_t pin);

static inline void set_pin(stm_gpio_int_t* ref_pin, stm_gpio_t pin);

static inline void free_pin(stm_gpio_int_t ref_pin);

static stm_result_t config_int(stm_gpio_int_t* intpin,
                            stm_gpio_t pin,
                            bool init,
                            uint8_t intr_priority,
                            cyhal_gpio_event_callback_t callback,
                            void* callback_arg);




/*******************************************************************************
 * redirecting cyhal -> stm hal
 ********************************************************************************/
static void cyhal_gpio_write(stm_gpio_t io, int level)
{
    HAL_GPIO_WritePin(io.gpio, io.pin, level);
}
static void cyhal_system_delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}
#define cyhal_gpio_read(gpio) HAL_GPIO_ReadPin(gpio.gpio, gpio.pin)
/*******************************************************************************
 * Public interface implementation
 ********************************************************************************/
stm_result_t xensiv_bgt60trxx_mtb_init(xensiv_bgt60trxx_mtb_t* obj,
                                    SPI_HandleTypeDef* spi,
                                    stm_gpio_t selpin,
                                    stm_gpio_t rstpin,
                                    const uint32_t* regs,
                                    size_t len)
{
    // CY_ASSERT(obj != NULL);
    // CY_ASSERT(spi != NULL);
    // CY_ASSERT(selpin != NC);
    // CY_ASSERT(rstpin != NC);
    // CY_ASSERT(regs != NULL);

	stm_result_t rslt = stm_result_ok;
    
    xensiv_bgt60trxx_mtb_iface_t* iface = &obj->iface;

    iface->spi = spi;
    iface->selpin = selpin;
    iface->rstpin = rstpin;
    set_pin(&(iface->irqpin), NC);

    // gpio should be initialized in stm main.c by cubemx
    
    // stm_result_t rslt = cyhal_gpio_init(selpin,
    //                                  CYHAL_GPIO_DIR_OUTPUT,
    //                                  CYHAL_GPIO_DRIVE_STRONG,
    //                                  true);

    // if (stm_result_ok == rslt)
    // {
    //     rslt = cyhal_gpio_init(rstpin,
    //                            CYHAL_GPIO_DIR_OUTPUT,
    //                            CYHAL_GPIO_DRIVE_STRONG,
    //                            true);
    // }


    xensiv_bgt60trxx_t* dev = &obj->dev;

    /* perform device hard reset before beginning init via SPI */
    xensiv_bgt60trxx_platform_rst_set(iface, true);
    xensiv_bgt60trxx_platform_spi_cs_set(iface, true);
    xensiv_bgt60trxx_platform_delay(1U);
    xensiv_bgt60trxx_platform_rst_set(iface, false);
    xensiv_bgt60trxx_platform_delay(1U);
    xensiv_bgt60trxx_platform_rst_set(iface, true);
    xensiv_bgt60trxx_platform_delay(1U);

    if (stm_result_ok == rslt)
    {
        int32_t res = xensiv_bgt60trxx_init(dev, iface, false);
        rslt = XENSIV_BGT60TRXX_ERROR(res);
    }

    if (stm_result_ok == rslt)
    {
        int32_t res = xensiv_bgt60trxx_config(dev, regs, len);
        rslt = XENSIV_BGT60TRXX_ERROR(res);
    }

    return rslt;
}


stm_result_t xensiv_bgt60trxx_mtb_interrupt_init(xensiv_bgt60trxx_mtb_t* obj,
                                              uint16_t fifo_limit,
                                              stm_gpio_t irqpin,
                                              uint8_t intr_priority,
                                              cyhal_gpio_event_callback_t callback,
                                              void* callback_arg)
{

    // unused. code generator will do will take care of it

    // CY_ASSERT(obj != NULL);

    // xensiv_bgt60trxx_t* dev = &obj->dev;
    // xensiv_bgt60trxx_mtb_iface_t* iface = &obj->iface;

    // stm_result_t result;

    // if (pins_equal(iface->irqpin, irqpin))
    // {
    //     result = config_int(&(iface->irqpin), irqpin, false, intr_priority, callback, callback_arg);
    // }
    // else if (pins_equal(iface->irqpin, NC))
    // {
    //     result = config_int(&(iface->irqpin), irqpin, true, intr_priority, callback, callback_arg);
    // }
    // else
    // {
    //     result = XENSIV_BGT60TRXX_RSLT_ERR_INTPIN_INUSE;
    // }

    // if (result == stm_result_ok)
    // {
    //     int32_t res = xensiv_bgt60trxx_set_fifo_limit(dev, fifo_limit);
    //     result = XENSIV_BGT60TRXX_ERROR(res);
    // }

    // return result;
}


void xensiv_bgt60trxx_mtb_free(xensiv_bgt60trxx_mtb_t* obj)
{
    // CY_ASSERT(obj != NULL);

    // xensiv_bgt60trxx_mtb_iface_t* iface = &obj->iface;

    // if (iface->selpin != NC)
    // {
    //     cyhal_gpio_free(iface->selpin);
    // }

    // if (iface->rstpin != NC)
    // {
    //     cyhal_gpio_free(iface->rstpin);
    // }

    // if (!pins_equal(iface->irqpin, NC))
    // {
    //     free_pin(iface->irqpin);
    // }
}


/*******************************************************************************
 * Platform functions implementation
 ********************************************************************************/
static void spi_set_data_width(SPI_HandleTypeDef* base, uint32_t data_width)
{
    // no need to set data width?

    // CY_ASSERT(CY_SCB_SPI_IS_DATA_WIDTH_VALID(data_width));

    // CY_REG32_CLR_SET(SCB_TX_CTRL(base),
    //                  SCB_TX_CTRL_DATA_WIDTH,
    //                  (uint32_t)data_width - 1U);
    // CY_REG32_CLR_SET(SCB_RX_CTRL(base),
    //                  SCB_RX_CTRL_DATA_WIDTH,
    //                  (uint32_t)data_width - 1U);
}


int32_t xensiv_bgt60trxx_platform_spi_transfer(void* iface,
                                               uint8_t* tx_data,
                                               uint8_t* rx_data,
                                               uint32_t len)
{
    const xensiv_bgt60trxx_mtb_iface_t* mtb_iface = iface;

	int res;
    if (rx_data == NULL){
        res = HAL_SPI_Transmit(mtb_iface->spi, tx_data, len, 1000);

    }else{
    	res = HAL_SPI_TransmitReceive(mtb_iface->spi, tx_data, rx_data, len, 1000);
    }

    if (res != HAL_OK){
    	return XENSIV_BGT60TRXX_STATUS_COM_ERROR;
    }else{
    	return XENSIV_BGT60TRXX_STATUS_OK;
    }


    // CY_ASSERT(iface != NULL);
    // CY_ASSERT((tx_data != NULL) || (rx_data != NULL));

    // const xensiv_bgt60trxx_mtb_iface_t* mtb_iface = iface;

    // spi_set_data_width(mtb_iface->spi->base, 8U);
    // Cy_SCB_SetByteMode(mtb_iface->spi->base, true);
    // cy_en_scb_spi_status_t status = Cy_SCB_SPI_Transfer(mtb_iface->spi->base, tx_data, rx_data, len,
    //                                                     &(mtb_iface->spi->context));
    // if (CY_SCB_SPI_SUCCESS == status)
    // {
    //     while (0UL !=
    //            (CY_SCB_SPI_TRANSFER_ACTIVE &
    //             Cy_SCB_SPI_GetTransferStatus(mtb_iface->spi->base, &(mtb_iface->spi->context))))
    //     {
    //     }
    // }

    // return ((CY_SCB_SPI_SUCCESS == status) ?
    //         XENSIV_BGT60TRXX_STATUS_OK :
    //         XENSIV_BGT60TRXX_STATUS_COM_ERROR);
}


int32_t xensiv_bgt60trxx_platform_spi_fifo_read(void* iface,
                                                uint8_t* rx_data,
                                                uint32_t len)
{
    const xensiv_bgt60trxx_mtb_iface_t* mtb_iface = iface;
    uint8_t* tmp_buf = (uint8_t*)rx_data;
    // int res = HAL_SPI_Receive(mtb_iface->spi, tmp_buf, len * 2, 2000);
    int res = HAL_SPI_Receive(mtb_iface->spi, tmp_buf, len, 2000);

    if (res != HAL_OK){

    	return XENSIV_BGT60TRXX_STATUS_COM_ERROR_FIFOREAD;

	}else{
        // for (int i = 0; i < len; i++)
        // {
        //     rx_data[i] = rx_data[i] & 0x0FFF;   // fifo is 12bit value
        // }
		return XENSIV_BGT60TRXX_STATUS_OK;
	}


    // CY_ASSERT(iface != NULL);
    // CY_ASSERT(rx_data != NULL);

    // const xensiv_bgt60trxx_mtb_iface_t* mtb_iface = iface;

    // spi_set_data_width(mtb_iface->spi->base, 12U);
    // Cy_SCB_SetByteMode(mtb_iface->spi->base, false);
    // cy_en_scb_spi_status_t status = Cy_SCB_SPI_Transfer(mtb_iface->spi->base, NULL, rx_data, len,
    //                                                     &(mtb_iface->spi->context));
    // if (CY_SCB_SPI_SUCCESS == status)
    // {
    //     while (0UL !=
    //            (CY_SCB_SPI_TRANSFER_ACTIVE &
    //             Cy_SCB_SPI_GetTransferStatus(mtb_iface->spi->base, &(mtb_iface->spi->context))))
    //     {
    //     }
    // }

    // return ((CY_SCB_SPI_SUCCESS == status) ?
    //         XENSIV_BGT60TRXX_STATUS_OK :
    //         XENSIV_BGT60TRXX_STATUS_COM_ERROR);
}


void xensiv_bgt60trxx_platform_rst_set(const void* iface, bool val)
{
    // CY_ASSERT(iface != NULL);

    const xensiv_bgt60trxx_mtb_iface_t* mtb_iface = iface;

    // CY_ASSERT(mtb_iface->rstpin != NC);

    cyhal_gpio_write(mtb_iface->rstpin, val);
}


void xensiv_bgt60trxx_platform_spi_cs_set(const void* iface, bool val)
{
    // CY_ASSERT(iface != NULL);

    const xensiv_bgt60trxx_mtb_iface_t* mtb_iface = iface;

    // CY_ASSERT(mtb_iface->selpin != NC);

    cyhal_gpio_write(mtb_iface->selpin, val);
}


void xensiv_bgt60trxx_platform_delay(uint32_t ms)
{
    (void)cyhal_system_delay_ms(ms);
}


uint32_t xensiv_bgt60trxx_platform_word_reverse(uint32_t x)
{
//	return x;
    uint32_t tmp =
        ((x & 0xff000000) >> 24) |
		((x & 0x00ff0000) >> 8) |
		((x & 0x0000ff00) << 8) |
		((x & 0x000000ff) << 24);

    return tmp;
}


void xensiv_bgt60trxx_platform_assert(bool expr)
{
    CY_ASSERT(expr);
    (void)expr; /* make release build */
}


/*******************************************************************************
 * Static functions implementation
 ********************************************************************************/
static inline bool pins_equal(stm_gpio_int_t ref_pin, stm_gpio_t pin)
{
    return ((ref_pin.gpio == pin.gpio) && (ref_pin.pin == pin.pin));
}


static inline void set_pin(stm_gpio_int_t* ref_pin, stm_gpio_t pin)
{
    ref_pin->gpio = pin.gpio;
    ref_pin->pin = pin.pin;
}


static inline void free_pin(stm_gpio_int_t ref_pin)
{
    printf("unused function - free_pin\r\n");
    // #if defined(CYHAL_API_VERSION) && (CYHAL_API_VERSION >= 2)
    // cyhal_gpio_free(ref_pin.pin);
    // #else
    // cyhal_gpio_free(ref_pin);
    // #endif
}


static stm_result_t config_int(stm_gpio_int_t* intpin,
                            stm_gpio_t pin,
                            bool init,
                            uint8_t intr_priority,
                            cyhal_gpio_event_callback_t callback,
                            void* callback_arg)
{
    // unused config. code generator should do the thing

    // stm_result_t result = stm_result_ok;

    // if (NULL == callback)
    // {
    //     set_pin(intpin, NC);
    //     cyhal_gpio_free(pin);
    // }
    // else
    // {
    //     if (init)
    //     {
    //         result = cyhal_gpio_init(pin,
    //                                  CYHAL_GPIO_DIR_INPUT,
    //                                  CYHAL_GPIO_DRIVE_PULLDOWN,
    //                                  false);
    //     }

    //     if (stm_result_ok == result)
    //     {
    //         set_pin(intpin, pin);
    //         #if defined(CYHAL_API_VERSION) && (CYHAL_API_VERSION >= 2)
    //         intpin->callback = callback;
    //         intpin->callback_arg = callback_arg;
    //         cyhal_gpio_register_callback(pin, intpin);
    //         #else
    //         cyhal_gpio_register_callback(pin, callback, callback_arg);
    //         #endif
    //         cyhal_gpio_enable_event(pin, CYHAL_GPIO_IRQ_RISE, intr_priority, true);
    //     }
    // }

    // return result;
}



int xensiv_bgt60trxx_mtb_init2(xensiv_bgt60trxx_mtb_t* obj, uint16_t fifo_limit, const uint32_t* regs, size_t len)
{
    xensiv_bgt60trxx_mtb_iface_t* iface = &obj->iface;
    xensiv_bgt60trxx_t* dev = &obj->dev;

    /* perform device hard reset before beginning init via SPI */
    xensiv_bgt60trxx_platform_rst_set(iface, true);
    xensiv_bgt60trxx_platform_spi_cs_set(iface, true);
    xensiv_bgt60trxx_platform_delay(1U);
    xensiv_bgt60trxx_platform_rst_set(iface, false);
    xensiv_bgt60trxx_platform_delay(1U);
    xensiv_bgt60trxx_platform_rst_set(iface, true);
    xensiv_bgt60trxx_platform_delay(1U);

    xensiv_bgt60trxx_set_fifo_limit(dev, fifo_limit);

    int rslt = stm_result_ok;

    if (stm_result_ok == rslt)
    {
    	rslt = xensiv_bgt60trxx_init(dev, iface, false);
    }

    if (stm_result_ok == rslt)
    {
        rslt = xensiv_bgt60trxx_config(dev, regs, len);
    }

    return rslt;
}
