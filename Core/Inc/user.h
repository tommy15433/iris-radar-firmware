/*
 * user.h
 *
 *  Created on: 2023. 12. 20.
 *      Author: GAERON
 */

#ifndef INC_USER_H_
#define INC_USER_H_

#include "stm32f7xx_hal.h"
#include "main.h"

#define LED1_ON()	(GPIOB->ODR |= GPIO_PIN_10)
#define LED1_OFF()	(GPIOB->ODR &= ~GPIO_PIN_10)
#define LED1_TGG()	(GPIOB->ODR ^= GPIO_PIN_10)

#define LED2_ON()	(GPIOB->ODR |= GPIO_PIN_11)
#define LED2_OFF()	(GPIOB->ODR &= ~GPIO_PIN_11)
#define LED2_TGG()	(GPIOB->ODR ^= GPIO_PIN_11)

void User_Thread(void);
void USB_Thread(void);
void USB_PutDataRadar(void);

extern char str[512];
extern unsigned int sec1ms;

//USB
extern unsigned int USB_sec1ms, USB_CON_sec1ms;

#endif /* INC_USER_H_ */
