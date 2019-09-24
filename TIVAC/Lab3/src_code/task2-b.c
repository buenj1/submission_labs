#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

uint8_t ui8PinData=2; //start with red LED

int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

    while(1)
    {
         GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, ui8PinData);
        SysCtlDelay(2000000);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x00);
        SysCtlDelay(2000000);
        if (ui8PinData==14) { //if LED = RGB
            ui8PinData = 2; //resets back to R
        }
        else if (ui8PinData == 8) {
            ui8PinData = 10; //LED = RG
        }
        else if (ui8PinData == 10) {
            ui8PinData = 6; //LED = RB
        }
        else if (ui8PinData == 6) {
            ui8PinData = 12; //LED = GB
        }
        else if (ui8PinData == 12) {
            ui8PinData = 14; //LED = RGB
        }
        else {
            ui8PinData *= 2; //pattern sequence will reset to R, B, G (single pattern)
        }
    }
}


