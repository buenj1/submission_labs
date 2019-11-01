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
#include <math.h>
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
#include "IQmath/IQmathLib.h"

// define and configurations of accelerometer
// and gyroscope modules on MPU6050 as well as
// sample rate, ratio(180deg/3.14), and pi
#define ACCELEROMETER_SENSITIVITY 8192.0
#define GYROSCOPE_SENSITIVITY 131
#define SAMPLE_RATE 0.01
#define RATIO (180/3.14)
#define M_PI 3.14

volatile bool g_bMPU6050Done;
tI2CMInstance sI2CInst;

//I2C interrupt subroutine
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
    // the I2C0 module. The last parameter sets the I2C data transfer ratioe.
    // If false, the data ratioe is set to 100kbps and if true the data ratioe will
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
    UARTStdioConfig(0, 115200, 16000000); //using baudratioe of 115200
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

// This function will be used to filter the raw accel and gyroscope
// values and display them on the serial terminal.
void ComplementaryFilter(float accData[3], float gyroData[3])
{
    _iq16 ForceMagApprx, PitchAcc, RollAcc, YawAcc, sensitivity, ratio, num1, num2;
    _iq16 Gyro[3], Acc[3];
    _iq16 Pitch = 0; //y value for gyro
    _iq16 Roll = 0; //x value for gyro
    _iq16 Yaw = 0; //z value for gyro

    //initialize values using IQ math
    ratio = _IQ16(RATIO);
    num1 = _IQ16(0.98);
    num2 = _IQ16(0.02);
    Gyro[0] = _IQ16(gyroData[0]);
    Gyro[1] = _IQ16(gyroData[1]);
    Gyro[2] = _IQ16(gyroData[2]);
    Acc[0] = _IQ16(accData[0]);
    Acc[1] = _IQ16(accData[1]);
    Acc[2] = _IQ16(accData[2]);
    sensitivity = _IQ16(GYROSCOPE_SENSITIVITY);

    //compute pitch, roll, and yaw values
    Pitch += _IQ16mpy(_IQ16div(Gyro[0],sensitivity), _IQ16(SAMPLE_RATE));
    Roll -= _IQ16mpy(_IQ16div(Gyro[1],sensitivity), _IQ16(SAMPLE_RATE));
    Yaw = _IQ16mpy(_IQ16div(Gyro[2],sensitivity), _IQ16(SAMPLE_RATE));

    //compute magnitude of force
    ForceMagApprx = _IQabs(Acc[0]) + _IQabs(Acc[1]) + _IQabs(Acc[2]);

    if(ForceMagApprx > 1411510 && ForceMagApprx < 4705028)
    {
        //compute gyro and accelerometer values based on magnitude of force
        PitchAcc = _IQ16mpy(_IQ16atan2(Acc[1],Acc[2]), ratio);
        Pitch = _IQ16mpy(Pitch,num1) + _IQ16mpy(PitchAcc,num2);
        RollAcc = _IQ16mpy(_IQ16atan2(Acc[0],Acc[2]), ratio);
        Roll = _IQ16mpy(Roll,num1) + _IQ16mpy(RollAcc,num2);
        YawAcc = _IQ16mpy(_IQ16atan2(Acc[0],Acc[1]), ratio);
        Yaw = _IQ16mpy(Yaw,num1) + _IQ16mpy(YawAcc,num2);


        //Display roll, pitch, and yaw
        UARTprintf("Roll: %d,\t   Pitch: %d,\t   Yaw: %d\n", (int)Roll, (int)Pitch, (int)Yaw);
    }
}


void MPU6050Example(void)
{
    float fAccel[3], fGyro[3];
    uint_fast16_t f_accel[3];   // variable for raw accelerometer values
    uint_fast16_t f_gyro[3];    // variable for raw gyroscope values

    float ax, ay, az, gx, gy, gz; // temp variables for displaying

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

        // Get accelerometer and gyroscope readings - FLOAT values.
        // These values will be passed into the complementary filter function
        MPU6050DataAccelGetFloat(&sMPU6050, &fAccel[0], &fAccel[1], &fAccel[2]);
        MPU6050DataGyroGetFloat(&sMPU6050, &fGyro[0], &fGyro[1], &fGyro[2]);

        // Get accelerometer and gyroscope readings - RAW values
        MPU6050DataAccelGetRaw(&sMPU6050, &f_accel[0], &f_accel[1], &f_accel[2]);
        MPU6050DataGyroGetRaw(&sMPU6050, &f_gyro[0], &f_gyro[1], &f_gyro[2]);


        //display Accel and Gyro values on terminal
        UARTprintf("///////////////RAW VALUES//////////////////////////////\n");
        UARTprintf("ACCEL:  X: %d,\t   Y: %d,\t   Z: %d\n", (uint_fast16_t)f_accel[0], (uint_fast16_t)f_accel[1], (uint_fast16_t)f_accel[2]);
        UARTprintf("GYRO:   X: %d,\t   Y: %d,\t   Z: %d\n", (uint_fast16_t)f_gyro[0], (uint_fast16_t)f_gyro[1], (uint_fast16_t)f_gyro[2]);

        //compute gyro and accel values prior to filtering
        ax = (atan2(fAccel[0], sqrt((fAccel[1]*fAccel[1]) + (fAccel[2] * fAccel[2])))*180.0)/M_PI;
        ay = (atan2(fAccel[1], sqrt((fAccel[0]*fAccel[0]) + (fAccel[2] * fAccel[2])))*180.0)/M_PI;
        az = (atan2(fAccel[2], sqrt((fAccel[1]*fAccel[1]) + (fAccel[2] * fAccel[2])))*180.0)/M_PI;
        gx = fGyro[0]*10000;
        gy = fGyro[1]*10000;
        gz = fGyro[2]*10000;


        //call the complementary filter
        ComplementaryFilter(fAccel, fGyro);

        //Display filtered values on terminal
        UARTprintf("///////////////FILTERED VALUES//////////////////////////////\n");
        UARTprintf("ACCEL: X: %d,\t   Y: %d,\t     Z: %d\n", (int)ax, (int)ay, (int)az);
        UARTprintf("GYRO:  X: %d,\t   Y: %d,\t     Z: %d\n", (int)gx, (int)gy, (int)gz);

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



