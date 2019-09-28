#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h" //definitions for interrupt and register assignments on Tiva C
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h" //Defines and macros for interrupts, functions -> IntEnable() and IntPrioritySet()
#include "driverlib/gpio.h"
#include "driverlib/timer.h" //Defines and macros for timers, functions -> TimerConfigure(), TimerLoadSet()

//GLOBAL VARIABLES
uint32_t timerDelayHigh; //variable to store delay for high time (43% of DC)
uint32_t timerDelayLow; //variable to store delay for low time (57% of DC)


int main (void){


    //Clock setup - using 40MHz clock
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    //GPIO setup
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //system control - enable peripheral PORTF
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3); //set PORTF pins 1-3 as OUTPUT

    //Timer setup
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0); //system control - enable peripheral TIMER0
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC); //32-bit timer configured in periodic mode
                                                     //timer0 = timer0A(16-bits) and timer0B(16-bits)

    //Calculate Delay
    timerDelayHigh = (SysCtlClockGet() / 10) * .43; //delay that contributes to the 43% duty cycle ON time of LED
    timerDelayLow = (SysCtlClockGet() / 10) * .57;  //delay that contributes to the OFF time of LED (57% low)

    //initialize timer0A to begin with the delay time of when the LED is on
    TimerLoadSet(TIMER0_BASE, TIMER_A, timerDelayHigh -1);

    //Interrupt Enable
    IntEnable(INT_TIMER0A); //enables timer0A interrupt in the interrupt vector table
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT); //interrupt is triggered at TIMEOUT of timer0A
    IntMasterEnable(); //enable ALL interrupts

    //Timer Enable
    TimerEnable(TIMER0_BASE, TIMER_A);


    while(1) {
        //wait for interrupt to occur...
    }
}


void Timer0IntHandler(void)
{
// Clear the timer interrupt
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

// Read the current state of the GPIO pin and
// write back the opposite state

    if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2)) //if GPIO_PIN_2 (blue LED) is on, turn LED off
    {
        TimerLoadSet(TIMER0_BASE, TIMER_A, timerDelayLow - 1); //delay for 57ms (57% of DC - OFF time)
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0); //turn off LED
    }

    else //if LED is off, turn LED on
    {
        TimerLoadSet(TIMER0_BASE, TIMER_A, timerDelayHigh - 1); //delay for 43ms (43% of DC - ON time)
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 4); //turn on LED (blue)
    }
}



