#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h" // includes adc functions

int main(void)
{
    uint32_t ui32ADC0Value[4];  // array to store samples of ADC with 4 steps
    volatile uint32_t ui32TempAvg;  // stores avg temp
    volatile uint32_t ui32TempValueC;   // stores temp in Celsius
    volatile uint32_t ui32TempValueF;   // stores temp in Fahrenheit

    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ); // using 40MHz clock source

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); //enable ADC0 peripheral


//Configure ADC
    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0); // using ADC sample sequencer 1 (SS1), set as the highest priority, and processor will trigger ADC
    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS); // ADC sample step 0
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS); // ADC sample step 1
    ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS); // ADC sample step 2
    ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END); //ADC sample step 3, set ADC interrupt flag, end sampling
    ADCSequenceEnable(ADC0_BASE, 1);    // enable ADC0

    while(1)
    {
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
}
