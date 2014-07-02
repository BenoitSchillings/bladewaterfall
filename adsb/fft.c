#include <math.h>


// FFT code fragment from the different rtl_sdr code
/*
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Hoernchen <la@tfc-server.de>
 * Copyright (C) 2012 by Kyle Keen <keenerd@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


//---------------------------------------------------------------

int16_t* Sinewave;
int N_WAVE, LOG2_N_WAVE;
int16_t *fft_buf;


/* FFT based on fix_fft.c by Roberts, Slaney and Bouras
   http://www.jjj.de/fft/fftpage.html
  16 bit ints for everything
  -32768..+32768 maps to -1.0..+1.0
*/



//---------------------------------------------------------------

void sine_table(int size) {
	int i;
	double d;
	LOG2_N_WAVE = size;
	N_WAVE = 1 << LOG2_N_WAVE;
	Sinewave = (int16_t*)malloc(sizeof(int16_t) * N_WAVE * 3 / 4);
	for (i = 0; i < N_WAVE * 3 / 4; i++) {
		d = (double) i * 2.0 * M_PI / N_WAVE;
		Sinewave[i] = (int) round(32767 * sin(d));
	}
}

//---------------------------------------------------------------


inline int16_t FIX_MPY(int16_t a, int16_t b) {
	int c = ((int) a * (int) b) >> 14;
	b = c & 0x01;
	return (c >> 1) + b;
}

//---------------------------------------------------------------

int fix_fft(int16_t iq[], int m)
/* interleaved iq[], 0 <= n < 2**m, changes in place */
{
	int mr, nn, i, j, l, k, istep, n, shift;
	int16_t qr, qi, tr, ti, wr, wi;
	n = 1 << m;
	if (n > N_WAVE) {
		return -1;
	}
	mr = 0;
	nn = n - 1;
	/* decimation in time - re-order data */
	for (m = 1; m <= nn; ++m) {
		l = n;
		do {
			l >>= 1;
		} while (mr + l > nn);
		mr = (mr & (l - 1)) + l;
		if (mr <= m) {
			continue;
		}
		// real = 2*m, imag = 2*m+1
		tr = iq[2 * m];
		iq[2 * m] = iq[2 * mr];
		iq[2 * mr] = tr;
		ti = iq[2 * m + 1];
		iq[2 * m + 1] = iq[2 * mr + 1];
		iq[2 * mr + 1] = ti;
	}
	l = 1;
	k = LOG2_N_WAVE - 1;
	while (l < n) {
		shift = 1;
		istep = l << 1;
		for (m = 0; m < l; ++m) {
			j = m << k;
			wr = Sinewave[j + N_WAVE / 4];
			wi = -Sinewave[j];
			if (shift) {
				wr >>= 1;
				wi >>= 1;
			}
			for (i = m; i < n; i += istep) {
				j = i + l;
				tr = FIX_MPY(wr, iq[2 * j]) - FIX_MPY(wi, iq[2 * j + 1]);
				ti = FIX_MPY(wr, iq[2 * j + 1]) + FIX_MPY(wi, iq[2 * j]);
				qr = iq[2 * i];
				qi = iq[2 * i + 1];
				if (shift) {
					qr >>= 1;
					qi >>= 1;
				}
				iq[2 * j] = qr - tr;
				iq[2 * j + 1] = qi - ti;
				iq[2 * i] = qr + tr;
				iq[2 * i + 1] = qi + ti;
			}
		}
		--k;
		l = istep;
	}
	return 0;
}


//---------------------------------------------------------------
float v_i,v_q;

void calc_iq(short *in_buffer)
{
	float	s1, s2;
	int	i;


	s1 = 0;
	s2 = 0;

	for (i = 0; i < SIZE; i++) {
		s1 += in_buffer[i*2];
		s2 += in_buffer[1+i*2];
	}
	s1 /= (SIZE*1.0);
	s2 /= (SIZE*1.0);	
	v_i = s1;
	v_q = s2; 

}

void iq_to_unsigned(short *in_buffer, short *cvt)
{
    int i;

    calc_iq(in_buffer);

    for (i = 0; i < SIZE; i++) {
        float   v;
        float   v1;
        
        v = in_buffer[i * 2];
        v1 = in_buffer[1 + i * 2];

	v -= v_i;
	v1 -= v_q;

        cvt[i * 2] = 16 * v;
        cvt[i * 2 + 1] = 16 * v1; 
    }
}

void c16_to_8(short *in_buffer, char *out)
{
	int	i;

	calc_iq(in_buffer);

	for (i = 0; i < SIZE; i++) {

		*out++ = 0x80+(*in_buffer++ - v_i) / 8;
		*out++ = 0x80+(*in_buffer++ - v_q)  / 8;
	}

} 


void iq_to_mag(short *in_buffer, short *mag)
{
    int i;

    for (i = 0; i < SIZE; i++) {
        float   v;
        float   v1;
        
        v = in_buffer[i * 2];
        v1 = in_buffer[1 + i * 2];

	v = (1.0*sqrt(v*v+v1*v1));
	if (v > 32000) v = 32000;
	if (v < -32000) v = -32000;
        mag[i] = v;
    }
}

