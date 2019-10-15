#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

//global variables
uint32_t ui32ADC0Value[4];  // array to store samples of ADC with 4 steps
volatile uint32_t ui32TempAvg;  // stores avg temp
volatile uint32_t ui32TempValueC;   // stores temp in Celsius
volatile uint32_t ui32TempValueF;   // stores temp in Fahrenheit
char str_temp[10]; // variable used to store temp value in string
char command; //character representing the command to perform

//print function
void print_string(char * str) {
    while(*str != '\0')
    {
        UARTCharPut(UART0_BASE,*str); // print temp value on terminal
        ++str;
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

    //reverse the temperature string (swap the 2 numbers)
    char temp;
    int i1 = 0;
    int i2 = i-1; //length of string - 1
    temp = str[i1];
    str[i1] = str[i2];
    str[i2] = temp;

    return str;
}

void prompt(void) {
    UARTCharPut(UART0_BASE, 'R');
    UARTCharPut(UART0_BASE, ':');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, 'r');
    UARTCharPut(UART0_BASE, 'e');
    UARTCharPut(UART0_BASE, 'd');
    UARTCharPut(UART0_BASE, ',');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, 'G');
    UARTCharPut(UART0_BASE, ':');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, 'g');
    UARTCharPut(UART0_BASE, 'r');
    UARTCharPut(UART0_BASE, 'e');
    UARTCharPut(UART0_BASE, 'e');
    UARTCharPut(UART0_BASE, 'n');
    UARTCharPut(UART0_BASE, ',');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, 'B');
    UARTCharPut(UART0_BASE, ':');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, 'b');
    UARTCharPut(UART0_BASE, 'l');
    UARTCharPut(UART0_BASE, 'u');
    UARTCharPut(UART0_BASE, 'e');
    UARTCharPut(UART0_BASE, ',');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, 'T');
    UARTCharPut(UART0_BASE, ':');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, 't');
    UARTCharPut(UART0_BASE, 'e');
    UARTCharPut(UART0_BASE, 'm');
    UARTCharPut(UART0_BASE, 'p');
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');
}


void UARTIntHandler(void)
{
    uint32_t ui32Status;

    ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status - using RX and TX interrupts only

    UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts

    while(UARTCharsAvail(UART0_BASE)) //loop while there are chars
    {
        command = UARTCharGet(UART0_BASE); //obtain&store char command
        UARTCharPut(UART0_BASE, command); //display the char command

        switch(command) {
        case 'R':
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0); //turn off previous LED
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1); //turn on Red LED
            UARTCharPut(UART0_BASE, '\n');
            break;
        case 'B':
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0); //turn off previous LED
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2); //turn on Blue LED
            UARTCharPut(UART0_BASE, '\n');
            break;
        case 'G':
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0); //turn off previous LED
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3); //turn on Green LED
            UARTCharPut(UART0_BASE, '\n');
            break;
        case 'T':
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

            UARTCharPut(UART0_BASE, '\n');
            itoa(ui32TempValueF, str_temp, 10);
            print_string(str_temp);
            print_string("F\n"); //carriage return and line feed to current and previous temp values
            break;
        default:
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0); //turn off previous LED
            UARTCharPut(UART0_BASE, '\n');
        }

        prompt();
    }
}

int main(void) {

    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); //UART pins located on GPIO PORTA
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); //enable ADC0 peripheral

    GPIOPinConfigure(GPIO_PA0_U0RX); //configure receiver
    GPIOPinConfigure(GPIO_PA1_U0TX); //configure transmitter
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1); //configure pins as UART

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //enable GPIO port for LED
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3); //enable pin as output for all LEDs

    //Configure ADC
    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0); // using ADC sample sequencer 1 (SS1), set as the highest priority, and processor will trigger ADC
    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS); // ADC sample step 0
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS); // ADC sample step 1
    ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS); // ADC sample step 2
    ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END); //ADC sample step 3, set ADC interrupt flag, end sampling
    ADCSequenceEnable(ADC0_BASE, 1);    // enable ADC0

    //set up UART: using system clk, baud rate: 115200, 8 data bits, 1 stop bit, and no parity bits
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    IntMasterEnable(); //enable processor interrupts
    IntEnable(INT_UART0); //enable the UART interrupt
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT); //only enable RX and TX interrupts

    //prompt "R: red, G: green, B: blue, T: temperature:"
    prompt();

    while (1) //let interrupt handler do the UART echo function
    {
//      if (UARTCharsAvail(UART0_BASE)) UARTCharPut(UART0_BASE, UARTCharGet(UART0_BASE));
    }

}





