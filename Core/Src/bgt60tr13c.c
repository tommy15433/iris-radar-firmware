///*
// * bgt60tr13c.c
// *
// *  Created on: Feb 21, 2024
// *      Author: GAERON
// */
//#include "bgt60tr13c.h"
//
//unsigned char BGT60TR13C_MainStep=0;
//unsigned char BGT60TR13C_REG_Buff[64] = {0, };
//
//unsigned int BGT60TR13C_DIGI_ID_Value=0, BGT60TR13C_RF_ID_Value=0;
//
//
//void BGT60TR13C_Thread(void){
//	BGT60TR13C_Config();
//
//	while(1){
//		BGT60TR13C_Read_REG(CHIPID_REG,BGT60TR13C_REG_Buff);
//
//		sprintf(str, "ID1=0x%02X\n", BGT60TR13C_REG_Buff[0]);
//		Uart1_Str(str);
//		sprintf(str, "ID2=0x%02X\n", BGT60TR13C_REG_Buff[1]);
//		Uart1_Str(str);
//		sprintf(str, "ID3=0x%02X\n", BGT60TR13C_REG_Buff[2]);
//		Uart1_Str(str);
//		sprintf(str, "ID4=0x%02X\n", BGT60TR13C_REG_Buff[3]);
//		Uart1_Str(str);
//		LED1_TGG();
//		HAL_Delay(500);
//
//	}
//}
//
//void BGT60TR13C_Config(void){
//	BGT60TR13C_RST(0);
//	HAL_Delay(100);
//	BGT60TR13C_RST(1);
//}
//
//void BGT60TR13C_Read_REG(unsigned char add, unsigned char *read_data){
//	unsigned char buff[8] = {add<<1, 0 };
//
//	BGT60TR13C_CS(0);
//	HAL_SPI_TransmitReceive(&hspi1, buff, read_data, 4, 10);
//	BGT60TR13C_CS(1);
//}
//
////BGT60TR13C_Read_REG(CHIPID_REG,BGT60TR13C_REG_Buff);
////
////sprintf(str, "ID1=0x%02X\n", BGT60TR13C_REG_Buff[0]);
////Uart1_Str(str);
////sprintf(str, "ID2=0x%02X\n", BGT60TR13C_REG_Buff[1]);
////Uart1_Str(str);
////sprintf(str, "ID3=0x%02X\n", BGT60TR13C_REG_Buff[2]);
////Uart1_Str(str);
////sprintf(str, "ID4=0x%02X\n", BGT60TR13C_REG_Buff[3]);
////Uart1_Str(str);
////HAL_Delay(500);
//
