#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"


//global variables
    uint32_t ui32ADC0Value[4];  // array to store samples of ADC with 4 steps
    volatile uint32_t ui32TempAvg;  // stores avg temp
    volatile uint32_t ui32TempValueC;   // stores temp in Celsius
    volatile uint32_t ui32TempValueF;   // stores temp in Fahrenheit
    char str_temp[10]; // variable used to store temp value in string

//print function
void print_string(char * str) {
    while(*str != '\0')
    {
        UARTCharPut(UART0_BASE,*str); // print temp value on terminal
        ++str;
    }
}

//reverse() function in association with itoa function
//reverses string
void reverse(char str[], int len)
{
    int start, end;
    char temp;
    for(start=0, end=len-1; start < end; start++, end--) {
        temp = *(str+start);
        *(str+start) = *(str+end);
        *(str+end) = temp;
    }
}

//itoa() function to convert from int to string to display on terminal
//returns the converted temp value in string
char* itoa(int num, char* str, int base)
{
    int i = 0;
    bool isNegative = false;

    /* A zero is same "0" string in all base */
    if (num == 0) {
        str[i] = '0';
        str[i + 1] = '\0';
        return str;
    }

    /* negative numbers are only handled if base is 10
       otherwise considered unsigned number */
    if (num < 0 && base == 10) {
        isNegative = true;
        num = -num;
    }

    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'A' : rem + '0';
        num = num/base;
    }

    /* Append negative sign for negative numbers */
    if (isNegative){
        str[i++] = '-';
    }

    str[i] = '\0';
    reverse(str, i);
    return str;
}



int main(void) {

    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0); //enable uart0 peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); //UART pins located on GPIO PORTA
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); //enable ADC0 peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1); //enable timer1 peripheral

    GPIOPinConfigure(GPIO_PA0_U0RX); //configure receiver
    GPIOPinConfigure(GPIO_PA1_U0TX); //configure transmitter
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1); //configure pins as UART

    //set up UART: using system clk, baud rate: 115200, 8 data bits, 1 stop bit, and no parity bits
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    //Configure ADC
    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0); // using ADC sample sequencer 1 (SS1), set as the highest priority, and processor will trigger ADC
    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS); // ADC sample step 0
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS); // ADC sample step 1
    ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS); // ADC sample step 2
    ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END); //ADC sample step 3, set ADC interrupt flag, end sampling
    ADCSequenceEnable(ADC0_BASE, 1);    // enable ADC0

    //configure global interrupt and timer1 interrupt
    IntMasterEnable(); //enable processor interrupts
    IntEnable(INT_TIMER1A); //enables timer1A interrupt in the interrupt vector table
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT); //interrupt is triggered at TIMEOUT of timer1A

    //Configure Timer1
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerEnable(TIMER1_BASE, TIMER_A);
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet()/2); //period (duty cycle) of 0.5s


    while (1) //let interrupt handler do the UART echo function
    {
        //wait for interrupt
    }

}


void Timer1IntHandler(void) {
    // Clear the timer interrupt
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

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

    //convert temperature to string and display on terminal (only in Fahrenheit)
    print_string(itoa(ui32TempValueF, str_temp, 10));
    print_string("\r\n"); //carriage return and line feed to current and previous temp values
}





