/*
Author Name: John Patrick Buen
Midterm 1 Code
Date: 10/31/2019



*/


/*******************************************************

Goal: Interface with MPU6050 IMU module using I2C given
      various tasks.

Steps:

         Task 1: Interface the given MPU6050 IMU using I2C protocol to TivaC. Print all accelerometer
                 and gyro values on to the serial terminal.
         Task 2: Interface the given MPU6050 IMU using I2C protocol to TivaC. Plot all accelerometer
                 and gyro values on to a Graph (you can use any graphing tool).
         Task 3: Implement a complementary filter to filter the raw accelerometer and gyro values. Print all
                 raw and filtered accelerometer and gyro values on to the serial terminal. Implement the filter using IQMath Library.
         Task 4: Implement a complementary filter to filter the raw accelerometer and gyro values. Plot all
                 raw and filtered accelerometer and gyro values on to a Graph (you can use any graphing tool).
******************************************************
Chip type               : ARM TM4C123GH6PM (Tiva C)
Program type            : Firmware
Core Clock frequency    : 80.000000 MHz
*******************************************************/



#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_sysctl.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "sensorlib/i2cm_drv.c"
#include "sensorlib/hw_mpu6050.h"
#include "sensorlib/mpu6050.h"
#include "uartstdio.h"

//global variables
volatile bool g_bMPU6050Done;
tI2CMInstance sI2CInst;

//interrupt subroutine for I2C
void I2CIntHandler(void)
{
    I2CMIntHandler(&sI2CInst);
}


void InitI2C0(void)
{
    //enable I2C module 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

    //reset module
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);

    //enable GPIO peripheral that contains I2C 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // Configure the pin muxing for I2C0 functions on port B2 and B3.
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);

    // Select the I2C function for these pins.
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    // Enable and initialize the I2C0 master module.  Use the system clock for
    // the I2C0 module. The last parameter sets the I2C data transfer rate.
    // If false, the data rate is set to 100kbps and if true the data rate will
    // be set to 400kbps.
    I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), true);

    //clear I2C FIFOs
    HWREG(I2C0_BASE + I2C_O_FIFOCTL) = 80008000;

    // Initialize the I2C master driver.
    I2CMInit(&sI2CInst, I2C0_BASE, INT_I2C0, 0xff, 0xff, SysCtlClockGet());

}


void UART_Init_Config(void)
{
    //using UART module 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0); //enable UART0 peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); //enable GPIOA peripheral

    //Configure pins Rx and Tx for UART
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTStdioConfig(0, 115200, 16000000); //using baudrate of 115200
}


void MPU6050Callback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    // See if an error occurred.
    if (ui8Status != I2CM_STATUS_SUCCESS)
    {
        // An error occurred, so handle it here if required.
    }
    // Indicate that the MPU6050 transaction has completed.
    g_bMPU6050Done = true;
}

//MPU6050 Example
void MPU6050Example(void)
{
    float fAccel[3], fGyro[3];

    tMPU6050 sMPU6050;

    // Initialize the MPU6050. This code assumes that the I2C master instance
    // has already been initialized.
    g_bMPU6050Done = false;
    MPU6050Init(&sMPU6050, &sI2CInst, 0x68, MPU6050Callback, &sMPU6050);
    while (!g_bMPU6050Done)
    {
    }

    // Configure the MPU6050 for +/- 4 g accelerometer range.
    g_bMPU6050Done = false;
    MPU6050ReadModifyWrite(&sMPU6050, MPU6050_O_ACCEL_CONFIG, ~MPU6050_ACCEL_CONFIG_AFS_SEL_M,
                           MPU6050_ACCEL_CONFIG_AFS_SEL_4G, MPU6050Callback, &sMPU6050);
    while (!g_bMPU6050Done)
    {
    }

    // turn on the MPU6050 (power on)
    g_bMPU6050Done = false;
    MPU6050ReadModifyWrite(&sMPU6050, MPU6050_O_PWR_MGMT_1, 0x00, 0x00, MPU6050Callback, &sMPU6050);
    while (!g_bMPU6050Done)
    {
    }


    // Loop forever keep reading data from MPU6050
    while (1)
    {
        // Request another reading from the MPU6050.
        g_bMPU6050Done = false;
        MPU6050DataRead(&sMPU6050, MPU6050Callback, &sMPU6050);
        while (!g_bMPU6050Done)
        {
        }

        // Get the new accelerometer and gyroscope readings.
        MPU6050DataAccelGetFloat(&sMPU6050, &fAccel[0], &fAccel[1], &fAccel[2]);
        MPU6050DataGyroGetFloat(&sMPU6050, &fGyro[0], &fGyro[1], &fGyro[2]);

        //display Accel and Gyro values on terminal
        UARTprintf("ACCEL: X: %d,\t   Y: %d,\t   Z: %d\n", (int)fAccel[0], (int)fAccel[1], (int)fAccel[2]);
        UARTprintf("GYRO:  X: %d,\t   Y: %d,\t   Z: %d\n", (int)fGyro[0], (int)fGyro[1], (int)fGyro[2]);

        SysCtlDelay(SysCtlClockGet()/10);
    }
}



int main(void)
{

    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_PLL | SYSCTL_OSC_INT | SYSCTL_XTAL_16MHZ);

    //call InitI2C0
    InitI2C0();

    //call UART_Init_Config
    UART_Init_Config();

    //call MPU6050Example
    MPU6050Example();

    return 0;
}
