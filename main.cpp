/* ----------------------------------------------------------------------
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 *
* $Date:         17. January 2013
* $Revision:     V1.4.0
*
* Project:       CMSIS DSP Library
 * Title:        arm_fir_example_f32.c
 *
 * Description:  Example code demonstrating how an FIR filter can be used
 *               as a low pass filter.
 *
 * Target Processor: Cortex-M4/Cortex-M3
 *
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*   - Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   - Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.
*   - Neither the name of ARM LIMITED nor the names of its contributors
*     may be used to endorse or promote products derived from this
*     software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
 * -------------------------------------------------------------------- */

/**
 * @ingroup groupExamples
 */

/**
 * @defgroup FIRLPF FIR Lowpass Filter Example
 *
 * \par Description:
 * \par
 * Removes high frequency signal components from the input using an FIR lowpass filter.
 * The example demonstrates how to configure an FIR filter and then pass data through
 * it in a block-by-block fashion.
 * \image html FIRLPF_signalflow.gif
 *
 * \par Algorithm:
 * \par
 * The input signal is a sum of two sine waves:  1 kHz and 15 kHz.
 * This is processed by an FIR lowpass filter with cutoff frequency 6 kHz.
 * The lowpass filter eliminates the 15 kHz signal leaving only the 1 kHz sine wave at the output.
 * \par
 * The lowpass filter was designed using MATLAB with a sample rate of 48 kHz and
 * a length of 29 points.
 * The MATLAB code to generate the filter coefficients is shown below:
 * <pre>
 *     h = fir1(28, 6/24);
 * </pre>
 * The first argument is the "order" of the filter and is always one less than the desired length.
 * The second argument is the normalized cutoff frequency.  This is in the range 0 (DC) to 1.0 (Nyquist).
 * A 6 kHz cutoff with a Nyquist frequency of 24 kHz lies at a normalized frequency of 6/24 = 0.25.
 * The CMSIS FIR filter function requires the coefficients to be in time reversed order.
 * <pre>
 *     fliplr(h)
 * </pre>
 * The resulting filter coefficients and are shown below.
 * Note that the filter is symmetric (a property of linear phase FIR filters)
 * and the point of symmetry is sample 14.  Thus the filter will have a delay of
 * 14 samples for all frequencies.
 * \par
 * \image html FIRLPF_coeffs.gif
 * \par
 * The frequency response of the filter is shown next.
 * The passband gain of the filter is 1.0 and it reaches 0.5 at the cutoff frequency 6 kHz.
 * \par
 * \image html FIRLPF_response.gif
 * \par
 * The input signal is shown below.
 * The left hand side shows the signal in the time domain while the right hand side is a frequency domain representation.
 * The two sine wave components can be clearly seen.
 * \par
 * \image html FIRLPF_input.gif
 * \par
 * The output of the filter is shown below.  The 15 kHz component has been eliminated.
 * \par
 * \image html FIRLPF_output.gif
 *
 * \par Variables Description:
 * \par
 * \li \c testInput_f32_1kHz_15kHz points to the input data
 * \li \c refOutput points to the reference output data
 * \li \c testOutput points to the test output data
 * \li \c firStateF32 points to state buffer
 * \li \c firCoeffs32 points to coefficient buffer
 * \li \c blockSize number of samples processed at a time
 * \li \c numBlocks number of frames
 *
 * \par CMSIS DSP Software Library Functions Used:
 * \par
 * - arm_fir_init_f32()
 * - arm_fir_f32()
 *
 * <b> Refer  </b>
 * \link arm_fir_example_f32.c \endlink
 *
 */


/** \example arm_fir_example_f32.c
 */

/* ----------------------------------------------------------------------
** Include Files
** ------------------------------------------------------------------- */

#include "ThisThread.h"
#define SEMIHOSTING
#include "mbed.h"
#include "arm_math.h"
#include "math_helper.h"

#include "stm32l475e_iot01_magneto.h"
#include "stm32l475e_iot01_gyro.h"
#include "stm32l475e_iot01_accelero.h"

#if defined(SEMIHOSTING)
#include <stdio.h>
#endif


/* ----------------------------------------------------------------------
** Macro Defines
** ------------------------------------------------------------------- */

#define TEST_LENGTH_SAMPLES  320
/*
This SNR is a bit small. Need to understand why
this example is not giving better SNR ...
*/
#define SNR_THRESHOLD_F32    75.0f
#define BLOCK_SIZE            32

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
/* Must be a multiple of 16 */
#define NUM_TAPS_ARRAY_SIZE              32
#else
#define NUM_TAPS_ARRAY_SIZE              29
#endif

#define NUM_TAPS              29

/* -------------------------------------------------------------------
 * The input signal and reference output (computed with MATLAB)
 * are defined externally in arm_fir_lpf_data.c.
 * ------------------------------------------------------------------- */

extern float32_t testInput_f32_1kHz_15kHz[TEST_LENGTH_SAMPLES];
extern float32_t refOutput[TEST_LENGTH_SAMPLES];

/* -------------------------------------------------------------------
 * Declare Test output buffer
 * ------------------------------------------------------------------- */

static float32_t testOutput[TEST_LENGTH_SAMPLES];

/* -------------------------------------------------------------------
 * Declare State buffer of size (numTaps + blockSize - 1)
 * ------------------------------------------------------------------- */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
static float32_t firStateF32[2 * BLOCK_SIZE + NUM_TAPS - 1];
#else
static float32_t firStateF32[BLOCK_SIZE + NUM_TAPS - 1];
#endif 

/* ----------------------------------------------------------------------
** FIR Coefficients buffer generated using fir1() MATLAB function.
** fir1(28, 6/24)
** ------------------------------------------------------------------- */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
const float32_t firCoeffs32[NUM_TAPS_ARRAY_SIZE] = {
  -0.0018225230f, -0.0015879294f, +0.0000000000f, +0.0036977508f, +0.0080754303f, +0.0085302217f, -0.0000000000f, -0.0173976984f,
  -0.0341458607f, -0.0333591565f, +0.0000000000f, +0.0676308395f, +0.1522061835f, +0.2229246956f, +0.2504960933f, +0.2229246956f,
  +0.1522061835f, +0.0676308395f, +0.0000000000f, -0.0333591565f, -0.0341458607f, -0.0173976984f, -0.0000000000f, +0.0085302217f,
  +0.0080754303f, +0.0036977508f, +0.0000000000f, -0.0015879294f, -0.0018225230f, 0.0f,0.0f,0.0f
};
#else
const float32_t firCoeffs32[NUM_TAPS_ARRAY_SIZE] = {
  -0.0018225230f, -0.0015879294f, +0.0000000000f, +0.0036977508f, +0.0080754303f, +0.0085302217f, -0.0000000000f, -0.0173976984f,
  -0.0341458607f, -0.0333591565f, +0.0000000000f, +0.0676308395f, +0.1522061835f, +0.2229246956f, +0.2504960933f, +0.2229246956f,
  +0.1522061835f, +0.0676308395f, +0.0000000000f, -0.0333591565f, -0.0341458607f, -0.0173976984f, -0.0000000000f, +0.0085302217f,
  +0.0080754303f, +0.0036977508f, +0.0000000000f, -0.0015879294f, -0.0018225230f
};
#endif

/* ------------------------------------------------------------------
 * Global variables for FIR LPF Example
 * ------------------------------------------------------------------- */

uint32_t blockSize = BLOCK_SIZE;
uint32_t numBlocks = TEST_LENGTH_SAMPLES/BLOCK_SIZE;

float32_t  snr;

/* ----------------------------------------------------------------------
 * FIR LPF Example
 * ------------------------------------------------------------------- */

float32_t Input_GYRO_X[TEST_LENGTH_SAMPLES];
float32_t Input_GYRO_Y[TEST_LENGTH_SAMPLES];
float32_t Input_GYRO_Z[TEST_LENGTH_SAMPLES];

float32_t Input_ACC_X[TEST_LENGTH_SAMPLES];
float32_t Input_ACC_Y[TEST_LENGTH_SAMPLES];
float32_t Input_ACC_Z[TEST_LENGTH_SAMPLES];

static float32_t Output_GYRO_X[TEST_LENGTH_SAMPLES];
static float32_t Output_GYRO_Y[TEST_LENGTH_SAMPLES];
static float32_t Output_GYRO_Z[TEST_LENGTH_SAMPLES];
static float32_t Output_ACC_X[TEST_LENGTH_SAMPLES];
static float32_t Output_ACC_Y[TEST_LENGTH_SAMPLES];
static float32_t Output_ACC_Z[TEST_LENGTH_SAMPLES];


int32_t main(void)
{


    BSP_MAGNETO_Init();
    BSP_GYRO_Init();
    BSP_ACCELERO_Init();
    
    float pGyroDataXYZ[3] = {0};
    int16_t pDataXYZ[3] = {0};


    uint32_t i;
    /* Get acc data */

    printf("Getting data...\n");
    for(i=0; i < TEST_LENGTH_SAMPLES; i++){
        BSP_GYRO_GetXYZ(pGyroDataXYZ);
        // printf("\nGYRO_X = %.2f\n", pGyroDataXYZ[0]);
        // printf("GYRO_Y = %.2f\n", pGyroDataXYZ[1]);
        // printf("GYRO_Z = %.2f\n", pGyroDataXYZ[2]);

        BSP_ACCELERO_AccGetXYZ(pDataXYZ);
        // printf("\nACCELERO_X = %d\n", pDataXYZ[0]);
        // printf("ACCELERO_Y = %d\n", pDataXYZ[1]);
        // printf("ACCELERO_Z = %d\n", pDataXYZ[2]);
        
        Input_GYRO_X[i] = pGyroDataXYZ[0];
        Input_GYRO_Y[i] = pGyroDataXYZ[1];
        Input_GYRO_Z[i] = pGyroDataXYZ[2];
        
        Input_ACC_X[i] = pDataXYZ[0];
        Input_ACC_Y[i] = pDataXYZ[1];
        Input_ACC_Z[i] = pDataXYZ[2];

        ThisThread::sleep_for(208.3);
    }

    
    arm_fir_instance_f32 S;
    arm_status status;
    float32_t  *inputF32, *outputF32;

    /* Initialize input and output buffer pointers */
    inputF32 = &testInput_f32_1kHz_15kHz[0];
    outputF32 = &testOutput[0];

    /* Call FIR init function to initialize the instance structure. */
    arm_fir_init_f32(&S, NUM_TAPS, (float32_t *)&firCoeffs32[0], &firStateF32[0], blockSize);

    /* ----------------------------------------------------------------------
    ** Call the FIR process function for every blockSize samples
    ** ------------------------------------------------------------------- */

    for(i=0; i < numBlocks; i++)
    {
        arm_fir_f32(&S, inputF32 + (i * blockSize), outputF32 + (i * blockSize), blockSize);
    }


    /* ----------------------------------------------------------------------
    ** Compare the generated output against the reference output computed
    ** in MATLAB.
    ** ------------------------------------------------------------------- */
    
    printf("\nTesting Correctness\n");
    printf("SNR THRESHOLD: %f\n", SNR_THRESHOLD_F32);
    for(i=0; i < TEST_LENGTH_SAMPLES; i++){
        printf("input: %f,  ref: %f,  test: %f\n", testInput_f32_1kHz_15kHz[i], (refOutput[i]), (testOutput[i]));
    }
    
    snr = arm_snr_f32(&refOutput[0], &testOutput[0], TEST_LENGTH_SAMPLES);
    printf("snr_threshold: %f. snr: %f\n", SNR_THRESHOLD_F32, snr);
    status = (snr < SNR_THRESHOLD_F32) ? ARM_MATH_TEST_FAILURE : ARM_MATH_SUCCESS;
  
    if (status != ARM_MATH_SUCCESS)
    {
        #if defined (SEMIHOSTING)
            printf("FAILURE\n");
            return 0;
        #else
            while (1);                             /* main function does not return */
        #endif
    }
    else
    {
        #if defined (SEMIHOSTING)
            printf("SUCCESS\n");
        #else
            while (1);                             /* main function does not return */
        #endif
    }

    /* Filter our input data */

    float32_t  *input_GYRO_X, *output_GYRO_X; 
    float32_t  *input_GYRO_Y, *output_GYRO_Y; 
    float32_t  *input_GYRO_Z, *output_GYRO_Z;
    float32_t  *input_ACC_X, *output_ACC_X;  
    float32_t  *input_ACC_Y, *output_ACC_Y;  
    float32_t  *input_ACC_Z, *output_ACC_Z;  
    input_GYRO_X = &Input_GYRO_X[0];
    input_GYRO_Y = &Input_GYRO_Y[0];
    input_GYRO_Z = &Input_GYRO_Z[0];
    input_ACC_X = &Input_ACC_X[0];
    input_ACC_Y = &Input_ACC_Y[0];
    input_ACC_Z = &Input_ACC_Z[0];
    output_GYRO_X = &Output_GYRO_X[0];
    output_GYRO_Y = &Output_GYRO_Y[0];
    output_GYRO_Z = &Output_GYRO_Z[0];
    output_ACC_X = &Output_ACC_X[0];
    output_ACC_Y = &Output_ACC_Y[0];
    output_ACC_Z = &Output_ACC_Z[0];
    arm_fir_init_f32(&S, NUM_TAPS, (float32_t *)&firCoeffs32[0], &firStateF32[0], blockSize);
    for(i=0; i < numBlocks; i++)
    {
        arm_fir_f32(&S, input_GYRO_X + (i * blockSize), output_GYRO_X + (i * blockSize), blockSize);
        arm_fir_f32(&S, input_GYRO_Y + (i * blockSize), output_GYRO_Y + (i * blockSize), blockSize);
        arm_fir_f32(&S, input_GYRO_Z + (i * blockSize), output_GYRO_Z + (i * blockSize), blockSize);
        arm_fir_f32(&S, input_ACC_X + (i * blockSize), output_ACC_X + (i * blockSize), blockSize);
        arm_fir_f32(&S, input_ACC_Y + (i * blockSize), output_ACC_Y + (i * blockSize), blockSize);
        arm_fir_f32(&S, input_ACC_Z + (i * blockSize), output_ACC_Z + (i * blockSize), blockSize);
    }

    /* Output Filtered data */


    // printf("The Filtered GYRO/ACC data: \n");
    // for(i=0; i < TEST_LENGTH_SAMPLES; i++){
    //     printf("GYRO input: (%f, %f, %f), output: (%f, %f, %f)\n", Input_GYRO_X[i], Input_GYRO_Y[i], Input_GYRO_Z[i], Output_GYRO_X[i], Output_GYRO_Y[i], Output_GYRO_Z[i]);
    // }
    // for(i=0; i < TEST_LENGTH_SAMPLES; i++){
    //     printf("ACC input: (%f, %f, %f), output: (%f, %f, %f)\n", Input_ACC_X[i], Input_ACC_Y[i], Input_ACC_Z[i], Output_ACC_X[i], Output_ACC_Y[i], Output_ACC_Z[i]);
    // }


    /* Print full array */
    printf("GYRO_X = [ %f", Input_GYRO_X[0]);
    for(i=1; i < TEST_LENGTH_SAMPLES; i++){
        printf(" %f", Input_GYRO_X[i]);
    }
    printf("]\n");

    printf("GYRO_Y = [ %f", Input_GYRO_Y[0]);
    for(i=1; i < TEST_LENGTH_SAMPLES; i++){
        printf(" %f", Input_GYRO_Y[i]);
    }
    printf("]\n");

    printf("GYRO_Z = [ %f", Input_GYRO_Z[0]);
    for(i=1; i < TEST_LENGTH_SAMPLES; i++){
        printf(" %f", Input_GYRO_Z[i]);
    }
    printf("]\n");

    printf("ACC_X = [ %f", Input_ACC_X[0]);
    for(i=1; i < TEST_LENGTH_SAMPLES; i++){
        printf(" %f", Input_ACC_X[i]);
    }
    printf("]\n");

    printf("ACC_Y = [ %f", Input_ACC_Y[0]);
    for(i=1; i < TEST_LENGTH_SAMPLES; i++){
        printf(" %f", Input_ACC_Y[i]);
    }
    printf("]\n");

    printf("ACC_Z = [ %f", Input_ACC_Z[0]);
    for(i=1; i < TEST_LENGTH_SAMPLES; i++){
        printf(" %f", Input_ACC_Z[i]);
    }
    printf("]\n");

    printf("GYRO_OUTPUT_X = [ %f", Output_GYRO_X[0]);
    for(i=1; i < TEST_LENGTH_SAMPLES; i++){
        printf(" %f", Output_GYRO_X[i]);
    }
    printf("]\n");

    printf("GYRO_OUTPUT_Y = [ %f", Output_GYRO_Y[0]);
    for(i=1; i < TEST_LENGTH_SAMPLES; i++){
        printf(" %f", Output_GYRO_Y[i]);
    }
    printf("]\n");

    printf("GYRO_OUTPUT_Z = [ %f", Output_GYRO_Z[0]);
    for(i=1; i < TEST_LENGTH_SAMPLES; i++){
        printf(" %f", Output_GYRO_Z[i]);
    }
    printf("]\n");

    printf("ACC_OUTPUT_X = [ %f", Output_ACC_X[0]);
    for(i=1; i < TEST_LENGTH_SAMPLES; i++){
        printf(" %f", Output_ACC_X[i]);
    }
    printf("]\n");

    printf("ACC_OUTPUT_Y = [ %f", Output_ACC_Y[0]);
    for(i=1; i < TEST_LENGTH_SAMPLES; i++){
        printf(" %f", Output_ACC_Y[i]);
    }
    printf("]\n");

    printf("ACC_OUTPUT_Z = [ %f", Output_ACC_Z[0]);
    for(i=1; i < TEST_LENGTH_SAMPLES; i++){
        printf(" %f", Output_ACC_Z[i]);
    }
    printf("]\n");


}

/** \endlink */