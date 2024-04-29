/*
 * user.c
 *
 *  Created on: 2023. 12. 20.
 *      Author: GAERON
 */
#include "user.h"
//#include "usb_device.h"
//#include "usbd_cdc_if.h"

char str[512];

unsigned int sec1ms=0;

//USB
//extern USBD_HandleTypeDef hUsbDeviceFS;
unsigned char USB_Step=0;
unsigned char USB_Conn_EN=0;
unsigned int USB_sec1ms=0, USB_CON_sec1ms=0;

void User_Thread(void){

//	LED1_OFF();
//	LED2_ON();
//
//	BGT60TR13C_Thread();
//	while(1){
//		Uart1_Thread();
//		Uart6_Thread();
//
//		if(sec1ms>=100){
//			sec1ms=0;
//			LED1_TGG();
//		}
//		HAL_Delay(1);
//	}
}

//void USB_Thread(void){
//	switch(USB_Step){
//	case 0:
//		if(USB_Conn_EN==1){
//			++USB_Step;
//		}
//		break;
//
//	case 1:
//		if(USB_sec1ms>=100){
//			USB_sec1ms=0;
//			USB_PutDataRadar();
//		}
//		break;
//	}
//
//	if(USB_CON_sec1ms>500){
//		USB_CON_sec1ms=0;
//		if(hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED){
//			USB_Conn_EN=1;
//		}
//		else {
//			USB_Conn_EN=0;
//			USB_Step=0;
//			USB_sec1ms=0;
//			Uart6_Str("USB Dis!\n");
//		}
//	}
//}


//void USB_PutDataRadar(void){
//	unsigned char buff[32]={0x55, 0xAA, 0x08, };
//	unsigned char chksum=0;
//	static unsigned char rev1=0;
//
//	HumanDetct_Flag=1;
//	HeartRate=0x45;
//	Raspiration=0x20;
//	HumanDistance=0x17;
//
//	buff[3]=HumanDetct_Flag;
//	buff[4]=HeartRate;
//	buff[5]=Raspiration;
//	buff[6]=HumanDistance;
//	buff[7]=rev1++;
//	buff[8]=0;
//	chksum=buff[0]^buff[1]^buff[2]^buff[3]^buff[4]^buff[5]^buff[6]^buff[7]^buff[8];
//	buff[9]=chksum;
//
//	//Uart6_Str_Size(buff, 10);
//	CDC_Transmit_FS((uint8_t *)buff, 10);
//}

