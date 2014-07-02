#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#define S  8 
#define S2 4 
#define uchar unsigned char

#define MODES_LONG_MSG_BITS 112
#define MODES_SHORT_MSG_BITS 56
#define MODES_LONG_MSG_BYTES (112/8)
#define MODES_SHORT_MSG_BYTES (56/8)

long good = 0;

//-------------------------------------------------------------------

int modesMessageLenByType(int type) {
    if (type == 16 || type == 17 ||
        type == 19 || type == 20 ||
        type == 21)
        return MODES_LONG_MSG_BITS;
    else
        return MODES_SHORT_MSG_BITS;
}

//-------------------------------------------------------------------

ushort inverse[256*256*256];
float test_var = 0;

//-------------------------------------------------------------------


uint32_t modes_checksum_table[112] = {
0x3935ea, 0x1c9af5, 0xf1b77e, 0x78dbbf, 0xc397db, 0x9e31e9, 0xb0e2f0, 0x587178,
0x2c38bc, 0x161c5e, 0x0b0e2f, 0xfa7d13, 0x82c48d, 0xbe9842, 0x5f4c21, 0xd05c14,
0x682e0a, 0x341705, 0xe5f186, 0x72f8c3, 0xc68665, 0x9cb936, 0x4e5c9b, 0xd8d449,
0x939020, 0x49c810, 0x24e408, 0x127204, 0x093902, 0x049c81, 0xfdb444, 0x7eda22,
0x3f6d11, 0xe04c8c, 0x702646, 0x381323, 0xe3f395, 0x8e03ce, 0x4701e7, 0xdc7af7,
0x91c77f, 0xb719bb, 0xa476d9, 0xadc168, 0x56e0b4, 0x2b705a, 0x15b82d, 0xf52612,
0x7a9309, 0xc2b380, 0x6159c0, 0x30ace0, 0x185670, 0x0c2b38, 0x06159c, 0x030ace,
0x018567, 0xff38b7, 0x80665f, 0xbfc92b, 0xa01e91, 0xaff54c, 0x57faa6, 0x2bfd53,
0xea04ad, 0x8af852, 0x457c29, 0xdd4410, 0x6ea208, 0x375104, 0x1ba882, 0x0dd441,
0xf91024, 0x7c8812, 0x3e4409, 0xe0d800, 0x706c00, 0x383600, 0x1c1b00, 0x0e0d80,
0x0706c0, 0x038360, 0x01c1b0, 0x00e0d8, 0x00706c, 0x003836, 0x001c1b, 0xfff409,
0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000
};


//-------------------------------------------------------------------

void init_inverse()
{
	ulong	i, j;

	printf("init\n");
	for (i = 0; i < 256*256*256; i++)
		inverse[i] = 0xffff;
	

	for (i = 0;i < 112; i++) {
		for (j = i + 1; j < 112;j++) {
			ulong v = modes_checksum_table[i] ^ modes_checksum_table[j];
			if (inverse[v] != 0xffff) {
				//printf("collision %d %d\n", i, j);
			}	
			inverse[v] = (i << 8) | j;
		}
	}
}

//-------------------------------------------------------------------

uint32_t modesChecksum(unsigned char *msg, int bits) {
    uint32_t crc = 0;
    int offset = (bits == 112) ? 0 : (112-56);
    int j;

    for(j = 0; j < bits; j++) {
        int byte = j/8;
        int bit = j%8;
        int bitmask = 1 << (7-bit);

        /* If bit is set, xor with corresponding table entry. */
        if (msg[byte] & bitmask)
            crc ^= modes_checksum_table[j+offset];
    }
    return crc; /* 24 bit checksum. */
}

//-------------------------------------------------------------------

uint32_t fix2BitErrors(unsigned char *msg, int bits) {
        uint32_t crc1, crc2, diff;
	ushort	 c_bits;
 

	crc1 = ((uint32_t)msg[(bits/8)-3] << 16) |
               ((uint32_t)msg[(bits/8)-2] << 8) |
                (uint32_t)msg[(bits/8)-1];
        crc2 = modesChecksum(msg,bits);

	diff = (crc1 ^ crc2);

	c_bits = inverse[diff];
	if (c_bits == 0xffff) {
		return(0);
	}
	ushort	b1;
	ushort	b2;

	b1 = c_bits >> 8;
	b2 = c_bits & 0xff;

	msg[b1/8] ^= 1<<(7-(b1%8));
	msg[b2/8] ^= 1<<(7-(b2%8));

	crc2 = modesChecksum(msg, bits);
	//printf("crc1 2 %x %x\n", crc1, crc2);
}


uint32_t fixSingleBitErrors(unsigned char *msg, int bits) {
    
    int j;
    unsigned char aux[MODES_LONG_MSG_BITS/8];

    for (j = 0; j < bits; j++) {
        int byte = j/8;
        int bitmask = 1 << (7-(j%8));
        uint32_t crc1, crc2;

        memcpy(aux,msg,bits/8);
        aux[byte] ^= bitmask;

        crc1 = ((uint32_t)aux[(bits/8)-3] << 16) |
               ((uint32_t)aux[(bits/8)-2] << 8) |
                (uint32_t)aux[(bits/8)-1];
        crc2 = modesChecksum(aux,bits);

        if (crc1 == crc2) {
            memcpy(msg,aux,bits/8);
            return crc1;
        }
    } 
    
    //return -1; 
    return(fix2BitErrors(msg, bits)); 
}


//-------------------------------------------------------------------

float bi = 0.0;
float bq = 0.0;
float fgain = 2.0;

//-------------------------------------------------------------------

float mag(short *array, int i)
{
    float vi;
    float vq;

    vi = array[i];
    vq = array[i+1];

    bi = (bi * 5000.0) + vi;
    bq = (bq * 5000.0) + vq;
    bi /= 5001.0;
    bq /= 5001.0;

    vi -= bi;
    vq -= bq;

    float p = (vi*vi + vq*vq);
   
    p = sqrt(p); 
    //if (p > 32000)
	//fgain += 0.04;

    return p/fgain;
}

//-------------------------------------------------------------------

#define MODES_PREAMBLE_US 8       /* microseconds */
#define MODES_FULL_LEN (MODES_PREAMBLE_US+MODES_LONG_MSG_BITS)

char detectModeS(short *m, int pos) {
    uchar bits[MODES_LONG_MSG_BITS];
    uchar msg[MODES_LONG_MSG_BITS/2];
    int j;
    int use_correction = 0;
    
    /* The Mode S preamble is made of impulses of 0.5 microseconds at
     * the following time offsets:
     *
     * 0   - 0.5 usec: first impulse.
     * 1.0 - 1.5 usec: second impulse.
     * 3.5 - 4   usec: third impulse.
     * 4.5 - 5   usec: last impulse.
     *
     * Since we are sampling at 2 Mhz every sample in our magnitude vector
     * is 0.5 usec, so the preamble will look like this, assuming there is
     * an impulse at offset 0 in the array:
     *
     * 0   -----------------
     * 1   -
     * 2   ------------------
     * 3   --
     * 4   -
     * 5   --
     * 6   -
     * 7   ------------------
     * 8   --
     * 9   -------------------
     */
    j = pos;
    {
        int low, high, delta, i, errors;
        int good_message = 0;
        
        
        /* First check of relations between the first 10 samples
         * representing a valid preamble. We don't even investigate further
         * if this simple test is not passed. */
        if (!(m[j] > m[j+1*S2] &&
              m[j+1*S2] < m[j+2*S2] &&
              m[j+2*S2] > m[j+3*S2] &&
              m[j+3*S2] < m[j] &&
              m[j+4*S2] < m[j] &&
              m[j+5*S2] < m[j] &&
              m[j+6*S2] < m[j] &&
              m[j+7*S2] > m[j+8*S2] &&
              m[j+8*S2] < m[j+9*S2] &&
              m[j+9*S2] > m[j+6*S2]))
        {
            return 0;
        }
        
        /* The samples between the two spikes must be < than the average
         * of the high spikes level. We don't test bits too near to
         * the high levels as signals can be out of phase so part of the
         * energy can be in the near samples. */
        high = (m[j]+m[j+2*S2]+m[j+7*S2]+m[j+9*S2])/5;
        if (m[j+4*S2] >= high ||
            m[j+5*S2] >= high)
        {
             return 0;
        }
        
        /* Similarly samples in the range 11-14 must be low, as it is the
         * space between the preamble and real data. Again we don't test
         * bits too near to high levels, see above. */
        if (m[j+11*S2] >= high ||
            m[j+12*S2] >= high ||
            m[j+13*S2] >= high ||
            m[j+14*S2] >= high)
        {
            return 0;
        }
        
    good_preamble:
        /* If the previous attempt with this message failed, retry using
         * magnitude correction. */
        
        /* Decode all the next 112 bits, regardless of the actual message
         * size. We'll check the actual message type later. */
        errors = 0;
        for (i = 0; i < MODES_LONG_MSG_BITS*S; i += S) {
            low = m[j+i+S*MODES_PREAMBLE_US];
            high = m[j+i+S*MODES_PREAMBLE_US+S2];
            delta = low-high;
            if (delta < 0) delta = -delta;
            //printf("%d\n", delta);
            if (i > 0 && delta < 9) {
                bits[i/S] = bits[i/S-1];
            } else if (low == high) {
                /* Checking if two adiacent samples have the same magnitude
                 * is an effective way to detect if it's just random noise
                 * that was detected as a valid preamble. */
                bits[i/S] = 2; /* error */
                if (i < MODES_SHORT_MSG_BITS*S) errors++;
            } else if (low > high) {
                bits[i/S] = 1;
            } else {
                /* (low < high) for exclusion  */
                bits[i/S] = 0;
            }
        }
        
        
        /* Pack bits into bytes */
        for (i = 0; i < MODES_LONG_MSG_BITS; i += 8) {
            msg[i/8] =
            bits[i]<<7 |
            bits[i+1]<<6 |
            bits[i+2]<<5 |
            bits[i+3]<<4 |
            bits[i+4]<<3 |
            bits[i+5]<<2 |
            bits[i+6]<<1 |
            bits[i+7];
        }
        
	int msgtype = msg[0]>>3;
	int len = modesMessageLenByType(msgtype); 
    	
        uint32_t crc1;
	uint32_t crc2;

	crc1  = ((uint32_t)msg[(len/8)-3] << 16) |
                ((uint32_t)msg[(len/8)-2] << 8) |
                (uint32_t)msg[(len/8)-1];
   
	if (crc1 == 0)
		return 0;
 
	crc2 = modesChecksum(msg, len);

	if (crc1 != crc2) {
		crc2 = fixSingleBitErrors(msg, len);
	}
	if (crc1 != crc2) {
		return 0;
	}
	good++;
	
	printf("*");
	for (i = 0; i < len ; i += 8) {
		printf("%02x", msg[i/8]);
	}	
	printf(";\n");	
	fflush(stdout); 	
	//printf("  %x %x\n", crc1, crc2);
	 
     }

     return 1;
}

#define N 320000000

short   mags[N/2];


void proc(short *array, int cnt)
{
	int	i;

	for (i = 2 ;i < cnt;i += 2) {
		  mags[i/2] = (1.4 * mags[(i/2)-1] + mag(array, i))/1.89;
	}

	i = 0;	
	while(i < cnt/2) {	
		if (detectModeS(&mags[0], i)) 
			i += 10;
		i++;
	}

	//printf("ba %f %f\n", bi, bq);
}


short array[N];

void xmain()
{
	FILE	*f = fopen("./raw.bin", "rb");
	init_inverse();

 	fread(array, 2, N, f); 
	
        fclose(f);

	int	i, j;
	float	s;
	float	k;

	//for (k = 0.9; k < 1.1; k +=0.01) {
		test_var = k;
		for (i = 2; i < N; i+=2) {
			mags[i/2] = (1.4 * mags[(i/2)-1] + mag(array, i))/1.89;
		}

    		good = 0;
    		for (i = 0; i < (N/2);i++) {
        		if (detectModeS(&mags[0], i)) i += 10;
    		}
    		printf("%f %d\n", k, good); 
	//}
}

