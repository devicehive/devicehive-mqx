#include "TestPin_think.h"

#include "MK70F12.h"
#include <mqx.h>
#include <bsp.h>

#define PORTNOFW  4

#define PORTNOERR 5  // Error LED

//-----------------------------------------------------------------------------------------
static LWGPIO_STRUCT pushButton;

//-----------------------------------------------------------------------------------------
void InitPeripherals(void)
//-----------------------------------------------------------------------------------------
{
   lwgpio_init(&pushButton, (GPIO_PORT_A | GPIO_PIN24), LWGPIO_DIR_INPUT, LWGPIO_VALUE_LOW);
   lwgpio_set_functionality(&pushButton, 1); // Pin-Function Alternate 1, GPIO
}

//-----------------------------------------------------------------------------------------
Bool GetPushButton(void)
//-----------------------------------------------------------------------------------------
{
   return !lwgpio_get_value(&pushButton);
}

//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
void InitLED_FW(void)
//-----------------------------------------------------------------------------------------
{
    // Alt1 ist GPIO, high drive strength, low slew rate
    PORTA_PCR4 = PORT_PCR_MUX(1) | PORT_PCR_SRE_MASK | PORT_PCR_DSE_MASK;
    GPIOA_PDDR |= 1 << PORTNOFW;       // Direction Output

    // Hier wird auch die Error-LED initilaisiert:
    PORTB_PCR5 = PORT_PCR_MUX(1) | PORT_PCR_SRE_MASK | PORT_PCR_DSE_MASK;
    GPIOB_PDDR |= 1 << PORTNOERR;       // Direction Output

}

//-----------------------------------------------------------------------------------------
void SetLED_FW(void)
//-----------------------------------------------------------------------------------------
{
   GPIOA_PCOR  = (1 << PORTNOFW);
}

//-----------------------------------------------------------------------------------------
void ClearLED_FW(void)
//-----------------------------------------------------------------------------------------
{
   GPIOA_PSOR  =  (1 << PORTNOFW);
}

//-----------------------------------------------------------------------------------------
void ToggleLED_FW(void)
//-----------------------------------------------------------------------------------------
{
   if (GPIOA_PDIR & (1 << PORTNOFW))
   {
      SetLED_FW();
   }
   else
   {
      ClearLED_FW();
   }
}

//-----------------------------------------------------------------------------------------
void SetLED_Err(void)
//-----------------------------------------------------------------------------------------
{
   GPIOB_PCOR  = (1 << PORTNOERR);
}

//-----------------------------------------------------------------------------------------
void ClearLED_Err(void)
//-----------------------------------------------------------------------------------------
{
   GPIOB_PSOR  =  (1 << PORTNOERR);
}


//-----------------------------------------------------------------------------------------
void ToggleLED_Err(void)
//-----------------------------------------------------------------------------------------
{
   if (GPIOB_PDIR & (1 << PORTNOERR))
   {
      SetLED_Err();
   }
   else
   {
      ClearLED_Err();
   }
}

//-----------------------------------------------------------------------------------------
