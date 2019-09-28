#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h" //definitions for interrupt and register assignments on Tiva C
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h" //Defines and macros for interrupts, functions -> IntEnable() and IntPrioritySet()
#include "driverlib/gpio.h"
#include "driverlib/timer.h" //Defines and macros for timers, functions -> TimerConfigure(), TimerLoadSet()


//GLOBAL VARIABLES
uint32_t timerDelayHigh; //variable to store delay for high time (43% of DC)
uint32_t timerDelayLow; //variable to store delay for low time (57% of DC)


//FUNCTION DECLARATION
void timer1A_delaySec(int ttime) {
//Defined variables are found in "inc/tm4c123gh6pm.h" header file
//This function is used to delay for a number of seconds for timer 1


    int i; //used to traverse through for loop

    SYSCTL_RCGCTIMER_R |= 2;        // enable clock to Timer Block 1
    TIMER1_CTL_R = 0;               // disable timer before initialization
    TIMER1_CFG_R = 0x04;            // 16-bit option
    TIMER1_TAMR_R = 0x02;           // periodic mode and down-counter
    TIMER1_TAILR_R = 64000 - 1;     // TimerA interval load value reg
    TIMER1_TAPR_R = 250 - 1;        // TimerA prescaler 16MHz/250 = 64000Hz
    TIMER1_ICR_R = 0x1;             // clear the TimerA timeout flag
    TIMER1_CTL_R |= 0x01;           // enable TimerA after initialization
    for (i=0; i < ttime; i++) {
        while ((TIMER1_RIS_R & 0x1) == 0);   // wait for TimerA timeout flag
        TIMER1_ICR_R = 0x1;                  // clear the TimerA timeout flag
    }
}


int main (void){

    //Clock setup - using 40MHz clock
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    //GPIO setup
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //system control - enable peripheral PORTF
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3); //set PORTF pins 1-3 as OUTPUT

    //Timer0 setup
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



//UNLOCK PIN 0 (SW2)
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= GPIO_PIN_0;

//GPIO INTERRUPT SETUP
    //Set SW2 (GPIO_PIN_0) as input and pad configuration
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0); //enable SW2 to be an input
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
        //Pin 0 will drive around 2mA of current and has pad type of "push-pull with weak pull-up"

    //Setup GPIO interrupt
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_0); //enable interrupt for pin 0 (SW2)
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_INT_PIN_0, GPIO_RISING_EDGE); //trigger interrupt on rising edge
    IntEnable(INT_GPIOF); //enable PORTF interrupt on vector table


//TIMER1 SETUP
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerEnable(TIMER1_BASE, TIMER_A);



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


void PortFPin0IntHandler(void) {
//Interrupt Subroutine for Pin 0 (SW2)

    //clear interrupt for GPIO
    GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_0);

    //turn on LED
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 4);

    //perform delay of 1s using timer1 - call timer1A delay function
    timer1A_delaySec(1); //delay for 1s

    //turn off LED
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

}


