/*
 * Helper.c
 *
 *  Created on: Oct 28, 2017
 *      Author: vovan
 */

#include "Helper.h"
#include "string.h"
#include "lwip/netif.h"

extern UART_HandleTypeDef huart3;

uint32_t lastGreenBlink = 0;
void DoGreenBlinking()
{
	if (HAL_GetTick() - lastGreenBlink > 1000)
	{
		HAL_GPIO_TogglePin(LD1_Green_GPIO_Port, LD1_Green_Pin);
		lastGreenBlink = HAL_GetTick();
	}
}

void Console_WriteLn(const char* str)
{
	HAL_UART_Transmit(&huart3, (uint8_t*)str, strlen(str), 1000);
	HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 1000);
}

uint8_t previousIsNetUp;
extern struct netif gnetif;
void SetIsNetUp(uint8_t bool)
{
	previousIsNetUp = bool;
	if (bool && netif_is_link_up(&gnetif))
		HAL_GPIO_WritePin(LD3_Red_GPIO_Port, LD3_Red_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(LD3_Red_GPIO_Port, LD3_Red_Pin, GPIO_PIN_RESET);
}

extern ETH_HandleTypeDef heth;
extern IWDG_HandleTypeDef hiwdg;
uint32_t lastWatchDogRefresh = 0;
uint8_t restartRequired = 0;
void DoWatchDogRefresh()
{
	if (HAL_GetTick() - lastWatchDogRefresh > 20000)
	{
		lastWatchDogRefresh = HAL_GetTick();

		uint32_t phyreg;
		HAL_ETH_ReadPHYRegister(&heth, PHY_BSR, &phyreg);
		if ((phyreg & PHY_LINKED_STATUS) != 0)
		{
			if (!previousIsNetUp)
			{
				SetIsNetUp(1);
//				Console_WriteLn("Up");
				restartRequired = 1;
			}
		}
		else
		{
			if (previousIsNetUp)
			{
				SetIsNetUp(0);
//				Console_WriteLn("Down");
			}
		}

		if (!restartRequired)
			HAL_IWDG_Refresh(&hiwdg);
	}
}
