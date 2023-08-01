/*
 * Helper.h
 *
 *  Created on: Oct 28, 2017
 *      Author: vovan
 */

#ifndef SYSTEM_HELPER_H_
#define SYSTEM_HELPER_H_

#include "stm32f7xx_hal.h"

void DoGreenBlinking();

void Console_WriteLn(const char* str);

void DoWatchDogRefresh();

void SetIsNetUp(uint8_t bool);

#endif /* SYSTEM_HELPER_H_ */
