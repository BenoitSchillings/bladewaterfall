// this source code was started from the nuand bladerf sample codes
//
/*
 * This file is part of the bladeRF project:
 *   http://www.github.com/nuand/bladeRF
 *
 * Copyright (C) 2013 Nuand LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */



#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <host_config.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <libbladeRF.h>
#include <pthread.h>
#include <sys/time.h>
#include "minmax.h"
#include "blde.h"
#include <unistd.h>
#include <math.h>


static bool shutdown_stream = false;

//---------------------------------------------------------------
#define uchar unsigned char
#define ushort unsigned short
pthread_mutex_t g_dev_lock;
struct  bladerf *g_dev;
float   g_frequency = 0;
//---------------------------------------------------------------
#define LSIZE	(15)
#define	SIZE	(1<<LSIZE)
#define SIZE_S  (SIZE/2)
#define PIXELS  1024 
//---------------------------------------------------------------
#define DEFAULT_SAMPLERATE      24000000
#define DEFAULT_FREQUENCY       2450 * 1000000
#define DEFAULT_STREAM_XFERS    4
#define DEFAULT_STREAM_BUFFERS  4
#define SYNC_TIMEOUT_MS         500
#define FAKE                    0
//---------------------------------------------------------------
#include "fft.c"
//---------------------------------------------------------------


void set_frequency(float frequency)
{
    pthread_mutex_lock(&g_dev_lock);
    bladerf_set_frequency(g_dev, BLADERF_MODULE_RX, frequency);
    g_frequency = frequency;
    pthread_mutex_unlock(&g_dev_lock);

}

//---------------------------------------------------------------

float get_frequency()
{
    return g_frequency;
}

//---------------------------------------------------------------

void retune(float delta)
{
    set_frequency(get_frequency() + delta);
}

//---------------------------------------------------------------

static int init_module(struct bladerf *dev, struct test_params *p, bladerf_module m)
{
    const char *m_str = "RX";
    int status;
    unsigned int samplerate_actual;
    unsigned int frequency_actual;
    unsigned int bw_actual;
    
    status = bladerf_set_sample_rate(dev, m, p->samplerate, &samplerate_actual);
    status = bladerf_set_frequency(dev, m, p->frequency);
    status = bladerf_get_frequency(dev, m, &frequency_actual);
    status = bladerf_set_loopback(dev, BLADERF_LB_NONE);
    status = bladerf_set_lna_gain(dev,   BLADERF_LNA_GAIN_MID);
    status = bladerf_set_lpf_mode(dev,  m,BLADERF_LPF_NORMAL);
    status = bladerf_set_bandwidth(dev,  m,p->samplerate, &bw_actual);
    bladerf_set_rxvga1(dev, p->gain);
    bladerf_set_rxvga2(dev, p->gain);
    g_frequency = p->frequency;
    printf("%s Frequency = %u, %s Samplerate = %u actual bw=%u\n", m_str, frequency_actual, m_str, samplerate_actual, bw_actual);
    
    return status;
}

//---------------------------------------------------------------

static struct bladerf * initialize_device(struct test_params *p)
{
    struct bladerf *dev;
    int fpga_loaded;
    
    if (FAKE)
        return dev;
    
    int status = bladerf_open(&dev, p->device_str);
    if (status != 0) {
        printf("Failed to open device: %s\n", bladerf_strerror(status));
        exit(0);
    }
    
    fpga_loaded = bladerf_is_fpga_configured(dev);
    if (fpga_loaded < 0) {
        printf("Failed to check FPGA state: %s\n", bladerf_strerror(fpga_loaded));
        status = -1;
        exit(0);
    } else if (fpga_loaded == 0) {
        printf("The device's FPGA is not loaded.\n");
        status = -1;
        exit(0);
    }
    
    status = init_module(dev, p, BLADERF_MODULE_RX);
    if (status != 0) {
        printf("Failed to init RX module: %s\n", bladerf_strerror(status));
        exit(0);
    }
    
    status = bladerf_set_loopback(dev, BLADERF_LB_NONE);
    
    
    return dev;
}

//---------------------------------------------------------------

void handler(int signal)
{
    if (signal == SIGTERM || signal == SIGINT) {
        shutdown_stream = true;
        printf("Caught signal, canceling transfers\n");
        fflush(stdout);
    }
}


//---------------------------------------------------------------

void clean_fft(short *data)
{
    data[0] = 0;            //clear dc
}


//---------------------------------------------------------------

int comp (const void * elem1, const void * elem2)
{
    short f = *((short*)elem1);
    short s = *((short*)elem2); 
    if (f > s) return  1;
    if (f < s) return -1;
    return 0;
}
//---------------------------------------------------------------

float offset = 0;
float gain = 4.45;

void fft_to_uchar(short *fft_val, FILE *p_out)
{
    uchar *tmp;
    int  overflow;
    int  underflow;
    float temp_array[PIXELS*2];
    float temp_array1[PIXELS*2];
    uchar output[PIXELS*2];
    
    tmp = &output[0];
    
    overflow = 0;
    underflow = 0;
    
    int i, j, k;
    
    j = 0;
    
     for (i = 0; i < PIXELS; i++) {
        float sum = 0;
        for (k = 0; k < (SIZE_S/PIXELS); k++) {
	    float v1 = fft_val[j];
 	    float v2 = fft_val[j+1];
            sum = sum + v1 * v1 + v2 * v2;
            j+=2;
        }
        temp_array[i] = sum;
        temp_array1[i] = (sum - offset) * gain;
    }

    for (i = 0; i < PIXELS; i++) {
        float v = ((temp_array[i] - offset) * gain);
        if (v < 0) {
            v = 0;
        }
        
        
        if (v > 255) {
            overflow++;
            v = 255;
        }
        
        *tmp++ = v;
        
    }
  
  
    float adapt = 1.01;
  

    
    if (overflow > 2)
        gain /= adapt;
    else
        gain *= adapt;

    
    fwrite (output, sizeof(uchar), PIXELS, p_out);

}

//---------------------------------------------------------------

void *rx_task(void *args)
{
    int             status;
    uint8_t         *samples;
    unsigned int    to_rx;
    struct task_args *task = (struct task_args*) args;
    struct test_params *p = task->p;
    bool done = false;
    size_t          n;
    int             line_count = 0;
    
    
    if (FAKE) {
        short *cvt = (short*)malloc(p->block_size * 2 * sizeof(int16_t));
        FILE *p_out = fopen("./buffer.bin", "wb");
        uchar output[PIXELS];

        while (!done) {
	    int i;

            for (i = 0; i < 1024; i++) {
                output[i] = sin(line_count/50.0) * cos(i) * 255;
            }
            fwrite (output, sizeof(uchar), PIXELS, p_out);
            line_count++;
            sleep(3);
        }
    }
    
    
    samples = (uint8_t *)malloc(p->block_size *2 * sizeof(int16_t));
    	
    status = bladerf_sync_config(task->dev,
                                 BLADERF_MODULE_RX,
                                 BLADERF_FORMAT_SC16_Q11,
                                 p->stream_buffer_count,
                                 p->stream_buffer_size,
                                 2,
                                 SYNC_TIMEOUT_MS);
   
    if (status != 0) {
        printf("Failed to initialize RX sync handle: %s\n", bladerf_strerror(status));
        exit(0);
    }
    
    status = bladerf_enable_module(task->dev, BLADERF_MODULE_RX, true);
    if (status != 0) {
        printf("Failed to enable RX module: %s\n", bladerf_strerror(status));
        exit(0);
    }
   
    short *cvt = (short *)calloc(p->block_size, 2 * 2 * sizeof(int16_t));
    short *mag = (short *)calloc(p->block_size, 1 * 2 * sizeof(int16_t));
   FILE *p_out = fopen("buffer.bin", "wb");
    //FILE *p_raw = fopen("raw.bin", "wb"); 
 
    while (!task->quit) {
        to_rx = (unsigned int) u64_min(p->block_size, p->rx_count);
	pthread_mutex_lock(&g_dev_lock);
        status = bladerf_sync_rx(task->dev, samples, to_rx, NULL, SYNC_TIMEOUT_MS);
	pthread_mutex_unlock(&g_dev_lock);
        if (status != 0) {
            printf("RX failed: %s\n", bladerf_strerror(status));
            exit(0);
            done = true;
        } else {
 
	    iq_to_unsigned((short*)samples, cvt);
	    //iq_to_mag((short*)samples, mag);
	    //c16_to_8((short*)samples, (char*)mag); 
	    //fwrite (cvt, 2, SIZE, p_raw);
 
            fix_fft(cvt, LSIZE);
            clean_fft(cvt);
            fft_to_uchar(cvt, p_out);

            line_count++;
        }
    }
    
    free(samples);
    status = bladerf_enable_module(task->dev, BLADERF_MODULE_RX, false);
    
    return NULL;
}


//---------------------------------------------------------------
struct timeval t;
struct timezone z;

double nanotime() {
    
	gettimeofday(&t, &z);
    
	return t.tv_usec;
}


//---------------------------------------------------------------
struct test_params p;
//---------------------------------------------------------------
#include <unistd.h>

int init_fft(int argc, char **argv) {
    
    int     line = 0;
    int     opt;
    int     status;
    
    
    sine_table(LSIZE);
 
    pthread_mutex_init(&g_dev_lock, NULL);
    p.frequency = DEFAULT_FREQUENCY;
    p.samplerate = DEFAULT_SAMPLERATE;
    p.rx_count = 1000000000;
    p.block_size = SIZE;
    p.device_str = NULL;
    p.stream_buffer_size = SIZE * 2;
    p.stream_buffer_count = 4;
    p.gain = 30;
    p.bandwidth = 10 * 1000000.0;


    while ((opt = getopt(argc, argv, "d:f:g:s:b:n:S::")) != -1) {
		switch (opt) {
            case 'f':
                p.frequency = atof(optarg);
                break;
            case 'g':
		p.gain = atof(optarg);
                break;
	    case 'b':
	        p.samplerate = atof(optarg) * 1000000.0;
                break;
            default:
                break;
		}
	}
  
    g_dev = initialize_device(&p);
    if (g_dev == NULL) {
        return -1;
    }
   
    rx_args.dev = g_dev;
    rx_args.p = &p;
    rx_args.status = 0;
    rx_args.quit = false;
    
    if (pthread_create(&rx_args.thread, NULL, rx_task, &rx_args) != 0) {
    }
    return 0;
}

