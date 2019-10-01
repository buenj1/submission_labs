#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h" // includes interrupt assignments for the Tiva C
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h" // includes interrupt functions
#include "driverlib/adc.h" // includes adc functions
#include "driverlib/timer.h" // includes timer functions

//GLOBAL VARIABLES
    uint32_t ui32ADC0Value[4];  // array to store samples of ADC with 4 steps
    volatile uint32_t ui32TempAvg;  // stores avg temp
    volatile uint32_t ui32TempValueC;   // stores temp in Celsius
    volatile uint32_t ui32TempValueF;   // stores temp in Fahrenheit


//Timer 1 Delay Function
void timer1A_delaySec(int ttime) {
    int i;

    SYSCTL_RCGCTIMER_R |= 2;
    TIMER1_CTL_R = 0;
    TIMER1_CFG_R = 0x04;
    TIMER1_TAMR_R = 0x02;
    TIMER1_TAILR_R = 64000 - 1;
    TIMER1_TAPR_R = 250 - 1;
    TIMER1_ICR_R = 0x1;
    TIMER1_CTL_R |= 0x01;
    for (i=0; i < ttime; i++) {
        while ((TIMER1_RIS_R & 0x1) == 0);
        TIMER1_ICR_R = 0x1;
    }
}


int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ); // using 40MHz clock source

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); //enable ADC0 peripheral
    ADCHardwareOversampleConfigure(ADC0_BASE, 32); // introducing hardware averaging with 32 samples to be averaged


//Configure ADC
    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0); // using ADC sample sequencer 1 (SS1), set as the highest priority, and processor will trigger ADC
    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS); // ADC sample step 0
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS); // ADC sample step 1
    ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS); // ADC sample step 2
    ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END); //ADC sample step 3, set ADC interrupt flag, end sampling
    ADCSequenceEnable(ADC0_BASE, 1);    // enable ADC0

//configure and enable Timer1 and Interrupt for Timer1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1); // enable timer1 peripheral
    IntMasterEnable(); // enable all interrupts
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC); // config timer 1 to periodic mode
    TimerLoadSet(TIMER1_BASE, TIMER_A, 120000000); // initial delay for timer
    IntEnable(INT_TIMER1A); // enable timer1A interrupt on vector table
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT); // trigger timer 1 int at TIMEOUT
    TimerEnable(TIMER1_BASE, TIMER_A); // enable timer1A


    while(1)
    {
        // wait for timer 1 interrupt to occur...
    }
}


void timer1A_ISR (void) {
    TimerIntClear(TIMER1_BASE, TIMER_A); // clear int for timer 1

    // call timer1a delay function
    timer1A_delaySec(0.5); // delay for 0.5s

    // perform ADC conversion
    ADCIntClear(ADC0_BASE, 1);   // clear ADC interrupt
    ADCProcessorTrigger(ADC0_BASE, 1); // processor begins to trigger ADC

    while(!ADCIntStatus(ADC0_BASE, 1, false)) // wait for ADC conversion..
    {
    }

    ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value); // get ADC value from samples

    //calculations of temperatures
    ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2)/4;
    ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10;
    ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;

}





