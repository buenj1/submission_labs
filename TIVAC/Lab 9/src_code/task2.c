#include <stdint.h>
#include <stdbool.h>
#include <math.h>   // included to use sinf() & cosf() functions
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/fpu.h"  // support for Floating Point Unit
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"

//  Define pi value (3.14....)
#ifndef M_PI
#define M_PI                    3.14159265358979323846
#endif


//  Define the length (size) of array containing floating point values
#define SERIES_LENGTH 1000


float gSeriesData[SERIES_LENGTH]; // array of floating point values

int32_t i32DataCount = 0; // index/counter of floating point array (gSeriesData[])


int main(void)
{
    float fRadians; // variable used to store and calculate the sine value

    ROM_FPULazyStackingEnable(); // turn on Lazy Stacking
    ROM_FPUEnable(); // turn on FPU (it is off by default)

    //  set up system clock for 50MHz
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    fRadians = ((2 * M_PI) / SERIES_LENGTH); // a full sine wave cycle is 2pi radians.
                                             // Divide 2pi by the depth of the array (SERIES_LENGTH)


    //  using the sine function (sinf()) and cosine function (cosf()) to calculate the equation
    while(i32DataCount < SERIES_LENGTH)
    {
        gSeriesData[i32DataCount] = 1.5 + sinf(fRadians * (50*i32DataCount)) + 0.5*cosf(fRadians*(200*i32DataCount));

        i32DataCount++; // increment array index
    }


    //  loop forever..
    while(1)
    {
    }
}


