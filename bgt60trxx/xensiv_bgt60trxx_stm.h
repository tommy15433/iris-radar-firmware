#pragma once
/***********************************************************************************************//**
 * \file xensiv_bgt60trxx_stm.h
 *
 * \brief
 * This file contains the MTB platform functions declarations
 * for interacting with the XENSIV(TM) BGT60TRxx 60GHz FMCW radar sensors.
 *
 ***************************************************************************************************
 * \copyright
 * Copyright 2024 Unique Facturing
 * SPDX-License-Identifier: 
 **************************************************************************************************/


/**
 * bgt60trxx driver for stm HAL
*/


#include "stm32f730xx.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_spi.h"
#include "stm32f7xx_hal_gpio.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "xensiv_bgt60trxx.h"


/**
 * \addtogroup group_board_libs_mtb XENSIV(TM) BGT60TRxx Radar Sensor ModusToolBox(TM) Interface
 * \{
 * Provides the ModusToolbox(TM) interface to the XENSIV(TM) BGT60TRxx 60GHz FMCW radar sensors
 * library and the implementation of the platform functions using the PSoC(TM) 6 HAL.
 *
 * \note The library uses delays while waiting for the sensor. If the RTOS_AWARE component is set
 * or CY_RTOS_AWARE is defined, the driver will defer to the RTOS for delays. Because of this, it is
 * not safe to call any functions until after the RTOS scheduler has started.
 *
 * \section subsection_board_libs_snippets Code snippets
 * \subsection subsection_board_libs_snippet_1 Snippet 1: Initialization.
 * The following snippet initializes an SPI instance and the BGT60TRxx.
 * \snippet snippet/main.c snippet_xensiv_bgt60trxx_init
 *
 * \subsection subsection_board_libs_snippet_2 Snippet 2: Reading from the sensor.
 * The following snippet demonstrates how to read data from the sensor.
 * \snippet snippet/main.c snippet_xensiv_bgt60trxx_get_fifo_data
 * \image html example-terminal.png
 *
 */


/************************************** Macros *******************************************/

#ifndef CY_RSLT_MODULE_BOARD_HARDWARE_XENSIV_BGT60TRXX
/** Module identifier for the XENSIV(TM) BGT60TRxx radar sensor library.
    Asset(s): (sensor-XENSIV(TM)-bgt60trxx) */
#define CY_RSLT_MODULE_BOARD_HARDWARE_XENSIV_BGT60TRXX 0x01CC
#endif

/** Result code indicating a communication error. */
#define XENSIV_BGT60TRXX_RSLT_ERR_COMM\
    (CY_RSLT_CREATE(stm_result_tYPE_ERROR, CY_RSLT_MODULE_BOARD_HARDWARE_XENSIV_BGT60TRXX, XENSIV_BGT60TRXX_STATUS_COM_ERROR))

/** Result code indicating an unsupported device error. */
#define XENSIV_BGT60TRXX_RSLT_ERR_UNKNOWN_DEVICE\
    (CY_RSLT_CREATE(stm_result_tYPE_ERROR, CY_RSLT_MODULE_BOARD_HARDWARE_XENSIV_BGT60TRXX, XENSIV_BGT60TRXX_STATUS_DEV_ERROR))

/** An attempt was made to reconfigure the interrupt pin */
//#define XENSIV_BGT60TRXX_RSLT_ERR_INTPIN_INUSE
//    (CY_RSLT_CREATE(stm_result_tYPE_ERROR, CY_RSLT_MODULE_BOARD_HARDWARE_XENSIV_BGT60TRXX, 0x100))




/** 
 * redefining cy macro
*/
#define CY_ASSERT(obj)  \
    if((obj) == false) { printf("error\r\n"); }

#define XENSIV_BGT60TRXX_RSLT_ERR_INTPIN_INUSE stm_result_intpin_inuse

#define XENSIV_BGT60TRXX_ERROR(res) ((res) == 0) ? 0 : 1

/******************************** Type definitions ****************************************/

/**
 * Structure holding the XENSIV(TM) BGT60TRxx ModusToolbox(TM) interface.
 *
 * Application code should not rely on the specific content of this struct.
 * They are considered an implementation detail which is subject to change
 * between platforms and/or library releases.
 */

typedef enum stm_result_t_{
    stm_result_ok = 0,
    stm_result_intpin_inuse,


    stm_result_fail
} stm_result_t;

typedef struct 
{
    GPIO_TypeDef* gpio;
    uint16_t pin;
} stm_gpio_t;

typedef struct 
{
    GPIO_TypeDef* gpio;
    uint16_t pin;
} stm_gpio_int_t;

typedef void (*cyhal_gpio_event_callback_t)(void);

typedef struct
{
    SPI_HandleTypeDef* spi;
    stm_gpio_t selpin;
    stm_gpio_t rstpin;
    stm_gpio_int_t irqpin;
} xensiv_bgt60trxx_mtb_iface_t;


/**
 * Structure holding the XENSIV(TM) BGT60TRxx ModusToolbox(TM) object.
 * Content initialized using \ref xensiv_bgt60trxx_mtb_init
 *
 */
typedef struct
{
    xensiv_bgt60trxx_t dev; /**< sensor object */
    xensiv_bgt60trxx_mtb_iface_t iface; /**< interface object for communication */
} xensiv_bgt60trxx_mtb_t;

/******************************* Function prototypes *************************************/

#ifdef __cplusplus
extern "C" {
#endif

/** Initializes the XENSIV(TM) BGT60TRxx sensor.
 * The provided pins will be initialized by this function.
 * If the reset pin is used (\p rstpin) the function will generate a hardware reset sequence.
 * The SPI slave select pin (\p selpin) is expected to be controlled by this driver,
 * not the SPI block itself. This allows the driver to issue multiple read/write requests
 * back to back.
 *
 * The function initializes a sensor object using the configuration register list.
 *
 * Refer \ref subsection_board_libs_snippets for more information.
 *
 * @param[inout] obj       Pointer to the BGT60TRxx ModusToolbox(TM) object. The caller must
 * allocate the memory for this object but the init function will initialize its contents.
 * @param[in]    spi       Pointer to an initialized SPI HAL object using Standard Motorola SPI
 *                         Mode 0 (CPOL=0, CPHA=0) with MSB first
 * @param[in]    selpin    Pin connected to the SPI_CSN pin of the sensor.
 *                         @note This pin cannot be NC
 * @param[in]    rstpin    Pin connected to the SPI_DIO3 pin of the sensor.
 *                         @note This pin can be NC
 * @param[in]    regs      Pointer to the configuration registers list.
 * @param[in]    len       Length of the configuration registers list.
 * @return CY_RSLT_SUCCESS if properly initialized; else an error indicating what went wrong.
 */
stm_result_t xensiv_bgt60trxx_mtb_init(xensiv_bgt60trxx_mtb_t* obj,
                                    SPI_HandleTypeDef* spi,
                                    stm_gpio_t selpin,
                                    stm_gpio_t rstpin,
                                    const uint32_t* regs,
                                    size_t len);
int xensiv_bgt60trxx_mtb_init2(xensiv_bgt60trxx_mtb_t* obj, uint16_t fifo_limit, const uint32_t* regs, size_t len);

/** Configures a GPIO pin as an interrupt for the XENSIV(TM) BGT60TRxx.
 * Initializes and configures the pin (\p irqpin) as an interrupt input.
 * Configures the XENSIV(TM) BGT60TRxx to trigger an interrupt after the number of fifo_limit words
 * are stored in the BGT60TRxx FIFO. Each FIFO word corresponds to two ADC samples.
 * @note BGT60TRxx pointer must be initialized using \ref xensiv_bgt60trxx_mtb_init() before
 * calling this function.
 *
 * Refer \ref subsection_board_libs_snippets for more information.
 *
 * @param[inout] obj               Pointer to the BGT60TRxx ModusToolbox(TM) object.
 * @param[in]    fifo_limit        Number of words stored in FIFO that will trigger an interrupt.
 * @param[in]    irqpin            Pin connected to the IRQ pin of the sensor.
 * @param[in]    intr_priority     The priority for NVIC interrupt events.
 * @param[in]    callback          The function to call when the specified event happens. Pass NULL
 * to unregister the handler.
 * @param[in]    callback_arg      Generic argument that will be provided to the callback when
 * called, can be NULL
 * @return CY_RSLT_SUCCESS if interrupt was successfully enabled; else an error occurred while
 * initializing the pin.
 */
stm_result_t xensiv_bgt60trxx_mtb_interrupt_init(xensiv_bgt60trxx_mtb_t* obj,
                                              uint16_t fifo_limit,
                                              stm_gpio_t irqpin,
                                              uint8_t intr_priority,
                                              cyhal_gpio_event_callback_t callback,
                                              void* callback_arg);

/**
 * Frees up any resources allocated by the XENSIV(TM) BGT60TRxx as part of
 * \ref xensiv_bgt60trxx_mtb_init()
 * @param[in] obj  Pointer to the BGT60TRxx ModusToolbox(TM) object.
 */
void xensiv_bgt60trxx_mtb_free(xensiv_bgt60trxx_mtb_t* obj);

#ifdef __cplusplus
}
#endif
