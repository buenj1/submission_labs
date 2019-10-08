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
    volatile uint8_t ui8Adjust;
    ui8Adjust = 83; // value represents 1.5ms of PWM ON time (servo motor is centered)

    ROM_SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
    ROM_SysCtlPWMClockSet(SYSCTL_PWMDIV_64); // PWM clock is set to 625kHz after div by 64

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1); // enable PWM 1 peripheral
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // enable GPIO PORTD to use as output for servo motor
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // enable GPIO PORTF for SW1 and SW2

    ROM_GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0); // set PORTD as a PWM output pin
    ROM_GPIOPinConfigure(GPIO_PD0_M1PWM0); // use PWM motion control module 1

    //unlock SW2
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;


    ROM_GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_DIR_MODE_IN); // set SW1 and SW2 as inputs
    ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); // use switches as pull-up

    //setup of PWM period
    ui32PWMClock = SysCtlClockGet() / 64;
    ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
    PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, ui32Load);

    //setup of PWM ON width size
    ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, ui8Adjust * ui32Load / 1000); // PWM at 1.5ms (center position)
    ROM_PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true); // set generator 0 as output
    ROM_PWMGenEnable(PWM1_BASE, PWM_GEN_0); // enable pwm and generator 0

    while(1)
    {

        //if SW1 is pressed, decrement ui8Adjust towards PWM ON time of 1ms
        if(ROM_GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00)
        {
            ui8Adjust--;
            if (ui8Adjust < 35)
            {
                ui8Adjust = 35;
            }
            ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, ui8Adjust * ui32Load / 1000);
        }

        //if SW2 is pressed, increment ui8Adjust towards PWM ON time of 2ms
        if(ROM_GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)==0x00)
        {
            ui8Adjust++;
            if (ui8Adjust > 135)
            {
                ui8Adjust = 135;
            }
            ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, ui8Adjust * ui32Load / 1000);
        }

        ROM_SysCtlDelay(100000); // speed of servo motor

    }

}
