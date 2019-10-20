#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/adc.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup ssi_examples_list
//! <h1>SPI Master (spi_master)</h1>
//!
//! This example shows how to configure the SSI0 as SPI Master.  The code will
//! send the temperature values on the master Tx then polls the receive FIFO until
//! the temperature values are received on the master Rx.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - SSI0 peripheral
//! - GPIO Port A peripheral (for SSI0 pins)
//! - SSI0Clk - PA2
//! - SSI0Fss - PA3
//! - SSI0Rx  - PA4
//! - SSI0Tx  - PA5
//!
//! The following UART signals are configured only for displaying console
//! messages for this example.  These are not required for operation of SSI0.
//! - UART0 peripheral
//! - GPIO Port A peripheral (for UART0 pins)
//! - UART0RX - PA0
//! - UART0TX - PA1
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - None.
//
//*****************************************************************************

//*****************************************************************************
//
// Number of bytes to send and receive.
//
//*****************************************************************************


//*****************************************************************************
//
// This function sets up UART0 to be used for a console to display information
// as the example is running.
//
//*****************************************************************************
void
InitConsole(void)
{
    //
    // Enable GPIO port A which is used for UART0 pins.
    // TODO: change this to whichever GPIO port you are using.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Configure the pin muxing for UART0 functions on port A0 and A1.
    // This step is not necessary if your part does not support pin muxing.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    //
    // Enable UART0 so that we can configure the clock.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Select the alternate (UART) function for these pins.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000); // UART 0, baud rate 115200, clock 16MHz
}

// Function to initialize ADC module for
// Internal Temperature sensor
void InitADC (void) {

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); //enable ADC0 peripheral

    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0); // using ADC sample sequencer 1 (SS1), set as the highest priority, and processor will trigger ADC
    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS); // ADC sample step 0
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS); // ADC sample step 1
    ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS); // ADC sample step 2
    ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END); //ADC sample step 3, set ADC interrupt flag, end sampling
    ADCSequenceEnable(ADC0_BASE, 1);    // enable ADC0

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

// Function to convert integer to string
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

//*****************************************************************************
//
// Configure SSI0 in master Freescale (SPI) mode.  This example will send out
// the temp value, then wait for the temp value to come in.  This will all be
// done using the polling method.
//
//*****************************************************************************
int main(void)
{

    uint32_t pui32DataTx;
    uint32_t pui32DataRx;

    // global variables for ADC
    uint32_t ui32ADC0Value[4];  // array to store samples of ADC with 4 steps
    volatile uint32_t ui32TempAvg;  // stores avg temp
    volatile uint32_t ui32TempValueC;   // stores temp in Celsius
    volatile uint32_t ui32TempValueF;   // stores temp in Fahrenheit
    char tempstring[10]; // variable used to store string to display temp using UART

    //
    // Set the clocking to run directly from the external crystal/oscillator.
    // TODO: The SYSCTL_XTAL_ value must be changed to match the value of the
    // crystal on your board.
    //

    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ); // clock is 50MHz

    //
    // Set up the serial console to use for displaying messages.  This is
    // just for this example program and is not needed for SSI operation.
    //
    InitConsole(); // initialize UART for displaying onto console

    InitADC(); // initialize ADC (call function)

    //
    // Display the setup on the console. (prompt)
    //
    UARTprintf("SSI ->\n");
    UARTprintf("  Mode: SPI\n");
    UARTprintf("  Data: 8-bit\n\n");

    //
    // The SSI0 peripheral must be enabled for use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

    //
    // For this example SSI0 is used with PortA[5:2].  The actual port and pins
    // used may be different on your part, consult the data sheet for more
    // information.  GPIO port A needs to be enabled so these pins can be used.
    // TODO: change this to whichever GPIO port you are using.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Configure the pin muxing for SSI0 functions on port A2, A3, A4, and A5.
    // This step is not necessary if your part does not support pin muxing.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);

    //
    // Configure the GPIO settings for the SSI pins.  This function also gives
    // control of these pins to the SSI hardware.  Consult the data sheet to
    // see which functions are allocated per pin.
    // The pins are assigned as follows:
    //      PA5 - SSI0Tx
    //      PA4 - SSI0Rx
    //      PA3 - SSI0Fss
    //      PA2 - SSI0CLK
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 |
                   GPIO_PIN_2);

    GPIOPinWrite(GPIO_PORTA_BASE,GPIO_PIN_4,GPIO_PIN_4); // Write Rx value

    //
    // Configure and enable the SSI port for SPI master mode.  Use SSI0,
    // system clock supply, idle clock level low and active low clock in
    // freescale SPI mode, master mode, 1MHz SSI frequency, and 8-bit data.
    // For SPI mode, you can set the polarity of the SSI clock when the SSI
    // unit is idle.  You can also configure what clock edge you want to
    // capture data on.  Please reference the datasheet for more information on
    // the different SPI modes.
    //

    SSIClockSourceSet(SSI0_BASE, SSI_CLOCK_SYSTEM);

    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
                       SSI_MODE_MASTER, 1000000, 8);


    //
    // Enable the SSI0 module.
    //
    SSIEnable(SSI0_BASE);

    while(1){
    //
    // Read any residual data from the SSI port.  This makes sure the receive
    // FIFOs are empty, so we don't read any unwanted junk.  This is done here
    // because the SPI SSI mode is full-duplex, which allows you to send and
    // receive at the same time.  The SSIDataGetNonBlocking function returns
    // "true" when data was returned, and "false" when no data was returned.
    // The "non-blocking" function checks if there is any data in the receive
    // FIFO and does not "hang" if there isn't.
    //

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

    pui32DataTx = ui32TempValueF;

    while(SSIDataGetNonBlocking(SSI0_BASE, &pui32DataRx))
    {
    }


    //
    // Display indication that the SSI is transmitting data.
    //
    UARTprintf("Sent:\n  ");

    itoa(pui32DataTx, tempstring, 10);

    UARTprintf("%s ", tempstring);
    SSIDataPut(SSI0_BASE, pui32DataTx);


    //
    // Wait until SSI0 is done transferring all the data in the transmit FIFO.
    //
    while(SSIBusy(SSI0_BASE))
    {
    }

    //
    // Display indication that the SSI is receiving data.
    //
    UARTprintf("\nReceived:\n  ");


    //
    // Receive the data using the "blocking" Get function. This function
    // will wait until there is data in the receive FIFO before returning.
    //
    SSIDataGet(SSI0_BASE, &pui32DataRx);


    //
    // Display the data that SSI0 received.
    //
    itoa(pui32DataRx, tempstring, 10);
    UARTprintf("%s \n", tempstring);

    }


}



