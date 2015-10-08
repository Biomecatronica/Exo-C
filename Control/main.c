//=============================================================================
// (c) Copyright 2005 Diamond Systems Corporation. Use of this source code
// is subject to the terms of Diamond Systems' Software License Agreement.
// Diamond Systems provides no warranty of proper performance if this
// source code is modified.
//
// File: DSCADScan.c   v5.9
// Desc: Sample program that demonstrates how to perform an AD scan
// Created By KL
//=============================================================================

#include <stdio.h>

#if defined(linux) || defined(__QNXNTO__) || defined(_WRS_VXWORKS_5_X)
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
// diamond driver includes
#include "dscud.h"


#ifdef _WRS_VXWORKS_5_X
#include <selectLib.h>
#define main DMM16ATDSCADScan
#else
#include <sys/time.h>
#endif

static int kbhit()
{
    struct timeval timeout;
    fd_set rfds;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    if ( select(0+1, &rfds, NULL, NULL, &timeout) > 0 )
        return getchar();

    return 0;
}
#endif

// var declarations
BYTE result;    // returned error code
DSCB dscb;      // handle used to refer to the board
DSCCB dsccb;    // structure containing board settings
DFLOAT potentiometerVoltage; // actual voltage
DFLOAT strainGaugeVoltage; // actual voltage
DSCSAMPLE* samples;          // sample readings
DSCADSCAN dscadscan;         // structure containing A/D scan settings
DSCADSETTINGS dscadsettings; // structure containing A/D conversion settings
ERRPARAMS errorParams;       // structure for returning error code and error string
int intBuff;    // temp variable of size int
int i;          // miscellaneous counter
int aux = 1;        // auxiliary variable
float pot_read = 0; // potentiometer reading
float new_pot_read; // new potentiometer reading
//=============================================================================
// Name: main()
// Desc: Starting function that calls the driver functions used
//
//       NOTE: By convention, you should capture the BYTE return value for each
//       driver API call, and check the error code.
//
//     STEPS TO FOLLOW:
//
//       I. Driver Initialization
//      II. Board Initialization
//     III. AD Settings Initialization
//      IV. AD Scan Initialization
//       V. Scanning and Output
//      VI. Cleanup
//
//=============================================================================

int main( void )
{

    //=========================================================================
    // I. DRIVER INITIALIZATION
    //
    //    Initializes the DSCUD library.
    //
    //=========================================================================

    if( dscInit( DSC_VERSION ) != DE_NONE )
    {
        dscGetLastError(&errorParams);
        fprintf( stderr, "dscInit error: %s %s\n", dscGetErrorString(errorParams.ErrCode), errorParams.errstring );
        return 0;
    }

    //=========================================================================
    // II. BOARD INITIALIZATION
    //
    //     Initialize the DMM-16-AT board. This function passes the various
    //     hardware parameters to the driver and resets the hardware.
    //
    //=========================================================================

    printf( "\nDMM16AT BOARD INITIALIZATION:\n" );

    printf("Base address (default 0x300) : ");
    dsccb.base_address = 0x300;

    dsccb.int_level = 3;

    if(dscInitBoard(DSC_DMM16AT, &dsccb, &dscb)!= DE_NONE)
    {
        dscGetLastError(&errorParams);
        fprintf( stderr, "dscInitBoard error: %s %s\n", dscGetErrorString(errorParams.ErrCode), errorParams.errstring );
        return 0;
    }

    //=========================================================================
    // III. AD SETTINGS INITIALIZATION
    //
    //      Initialize the structure containing the AD conversion settings and
    //      then pass it to the driver.
    //
    //=========================================================================

    /* PRE-FILLED EXAMPLE
    dscadsettings.range = RANGE_10;
    dscadsettings.polarity = BIPOLAR;
    dscadsettings.gain = GAIN_1;
    dscadsettings.load_cal = (BYTE)TRUE;
    dscadsettings.current_channel = 0;
    */

    printf( "\nAD SETTINGS INITIALIZATION\n" );

    memset(&dscadsettings, 0, sizeof(DSCADSETTINGS));

    printf( "Range: [1] 10V range)");
    // scanf("%d", &intBuff);
    dscadsettings.range = (BYTE) 1;

    printf( "Polarity: [1] UNIPOLAR)");
    // scanf("%d", &intBuff);
    dscadsettings.polarity = (BYTE) 1;

    printf( "Gain: [0] GAIN 1");
    // scanf("%d", &intBuff);
    dscadsettings.gain = (BYTE) 0;

    printf( "Load calibration flag: [0] for FALSE");
    // scanf("%d", &intBuff);
    dscadsettings.load_cal = (BYTE) 1;

    dscadsettings.current_channel = 0;

    if( ( result = dscADSetSettings( dscb, &dscadsettings ) ) != DE_NONE )
    {
        dscGetLastError(&errorParams);
        fprintf( stderr, "dscADSetSettings error: %s %s\n", dscGetErrorString(errorParams.ErrCode), errorParams.errstring );
        return 0;
    }

    //=========================================================================
    // IV. AD SCAN INITIALIZATION
    //
    //     Initialize the structure containing the AD scan setting and allocate
    //     memory for our buffer containing sample values.
    //
    //=========================================================================

    /* PRE-FILLED EXAMPLE
    dscadscan.low_channel = 3;
    dscadscan.high_channel = 6;
    dscadscan.gain = GAIN_1;
    */

    printf( "\nAD SCAN INITIALIZATION\n" );

    memset(&dscadscan, 0, sizeof(DSCADSCAN));

    dscadscan.low_channel = 0;
    dscadscan.high_channel = 2;

    dscadscan.gain = dscadsettings.gain;

    // allocate memory for buffer
    samples = (DSCSAMPLE*)malloc( sizeof(DSCSAMPLE) * ( dscadscan.high_channel - dscadscan.low_channel + 1 ) );

    getchar();

    //=========================================================================
    // V. SCANNING AND OUTPUT
    //
    //    Perform the actual sampling and then output the results. To calculate
    //    the actual input voltages, we must convert the sample code (which
    //    must be cast to a short to get the correct code) and then plug it
    //    into one of the formulas located in the manual for your board (under
    //    "A/D Conversion Formulas").
    //=========================================================================

    printf( "\nSCANNING AND OUTPUT\n" );

    do
    {
        while (1)
        {
        if( ( result = dscADScan( dscb, &dscadscan, samples ) ) != DE_NONE )
        {
            dscGetLastError(&errorParams);
            fprintf( stderr, "dscADScan error: %s %s\n", dscGetErrorString(errorParams.ErrCode), errorParams.errstring );
            free( samples ); // remember to deallocate malloc() memory
            return 0;
        }

        printf( "Sample readouts:" );

        for( i = 0; i < (dscadscan.high_channel - dscadscan.low_channel)+ 1; i++)
            printf( " %hd", dscadscan.sample_values[i] );

        printf( "Actual voltages:" );

        int delay;
        for(delay=0;delay<10000000;delay++);

        if( dscADCodeToVoltage(dscb, dscadsettings, dscadscan.sample_values[0], &strainGaugeVoltage) != DE_NONE)
        {
            dscGetLastError(&errorParams);
            fprintf( stderr, "dscInit error: %s %s\n", dscGetErrorString(errorParams.ErrCode), errorParams.errstring );
            return 0;
        }
        printf(" TORQUE2: %5.3lfV", strainGaugeVoltage);

        dscDAConvert(dscb, 0 , (int)4095*strainGaugeVoltage/5.0);



        if( dscADCodeToVoltage(dscb, dscadsettings, dscadscan.sample_values[1], &potentiometerVoltage) != DE_NONE)
        {
            dscGetLastError(&errorParams);
            fprintf( stderr, "dscInit error: %s %s\n", dscGetErrorString(errorParams.ErrCode), errorParams.errstring );
            free(samples);
            return 0;
        }
        printf(" POTENCIOMETRO2: %5.3lfV", potentiometerVoltage);
        // dscDAConvert(dscb, 0 , (int)4095*(potentiometerVoltage/5.0));

        printf( "\n" );
        }
        printf("Enter 'q' to exit, any key to continue\n");
    }

    while (getchar() != 'q' );


    //=========================================================================
    // VI. CLEANUP
    //
    //     Cleanup any remnants left by the program and free the resources used
    //     by the driver.
    //
    //     STEPS TO FOLLOW:
    //
    //     1. free the memory allocated for sample values
    //     2. free the driver resources
    //=========================================================================

    free(samples);

    dscFree();

    printf( "\nDSCADScan completed.\n" );

    return 0;
} // end main()


#ifdef _WIN32_WCE
int WINAPI WinMain( HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPTSTR    lpCmdLine,
                    int       nCmdShow)
{
    main();

    return 0;
}

#endif
