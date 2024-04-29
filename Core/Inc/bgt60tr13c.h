/*
 * bgt60tr13c.h
 *
 *  Created on: Feb 21, 2024
 *      Author: GAERON
 */

#ifndef INC_BGT60TR13C_H_
#define INC_BGT60TR13C_H_

#include "stm32f7xx_hal.h"
#include "main.h"

//User PinConfig
#define BGT60TR13C_RST(X)	(X!=0?(GPIOA->ODR|=GPIO_PIN_3):(GPIOA->ODR&=~GPIO_PIN_3))
#define BGT60TR13C_CS(X)	(X!=0?(GPIOA->ODR|=GPIO_PIN_4):(GPIOA->ODR&=~GPIO_PIN_4))

//Global Status :
//7,6,5,4 : Reserved
//3:FIFO OverFlow
//2:SPI High Speed Active
//1:Burst R/W Error
//0:SPI Spec Clock Error

//Register MAP
#define MAIN_REG	0x00
#define MADC0_REG	0x01
#define CHIPID_REG	0x02	//default digiID=0x3D, RFID=0x3D
#define STAT1_REG	0x03

void BGT60TR13C_Thread(void);
void BGT60TR13C_Config(void);
void BGT60TR13C_Read_REG(unsigned char add, unsigned char *read_data);

#endif /* INC_BGT60TR13C_H_ */
