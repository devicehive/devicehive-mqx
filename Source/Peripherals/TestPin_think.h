#pragma once

#include "CommonTypes.h"

////////////////////////////////////////////////////////////////////////////////////
void InitLED_FW(void);
/* diese Prozedur muss einmal vor der erten Benutzung des Moduls aufgerufen werden. */

////////////////////////////////////////////////////////////////////////////////////
void SetLED_FW(void);
void ClearLED_FW(void);
void ToggleLED_FW(void);


void SetLED_Err(void);
void ClearLED_Err(void);
void ToggleLED_Err(void);

void InitPeripherals(void);
Bool GetPushButton(void);
// return true, if push button is pressed.

