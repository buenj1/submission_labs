#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/rom.h"


#define PWM_FREQUENCY 55 // PWM frequency at 55Hz

int main(void)
{
    volatile uint32_t ui32Load;
    volatile uint32_t ui32PWMClock;
    volatile uint32_t ui32Adjust;
    ui32Adjust = 900; // value represents PWM ON time (duty cycle of LED - 90%)

    ROM_SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
    ROM_SysCtlPWMClockSet(SYSCTL_PWMDIV_64); // PWM clock is set to 625kHz after div by 64

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1); // enable PWM 1 peripheral
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // enable GPIO PORTD for PWM
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // enable GPIO PORTF for LED pin

    ROM_GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0); // set PORTD as a PWM output pin
    ROM_GPIOPinConfigure(GPIO_PD0_M1PWM0); // use PWM motion control module 1

    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1); // set PORTF as output


    //setup of PWM period
    ui32PWMClock = SysCtlClockGet() / 64;
    ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
    PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, ui32Load);

    //setup of PWM ON width size
    ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, ui32Adjust * ui32Load / 1000);
    ROM_PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true); // set generator 0 as output
    ROM_PWMGenEnable(PWM1_BASE, PWM_GEN_0); // enable pwm and generator 0



    while(1)
    {

        // if PWM is low, turn off LED
        if(ROM_GPIOPinRead(GPIO_PORTD_BASE,GPIO_PIN_0)==0x00)
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
        }

        // else turn on LED during the ON time of the PWM
        else {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 2);
        }


    }

}



