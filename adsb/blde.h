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



//---------------------------------------------------------------


struct test_data
{
    void                **buffers;      /* Transmit buffers */
    size_t              num_buffers;    /* Number of buffers */
    size_t              samples_per_buffer; /* Number of samples per buffer */
    unsigned int        idx;            /* The next one that needs to go out */
    bladerf_module      module;         /* Direction */
    ssize_t             samples_left;   /* Number of samples left */
};

//---------------------------------------------------------------

static struct task_args {
    struct bladerf      *dev;
    struct test_params  *p;
    pthread_t           thread;
    int                 status;
    bool                quit;
    bool		fail;
} rx_args;

//---------------------------------------------------------------

struct test_params {
    const char      *device_str;
    unsigned int    samplerate;
    unsigned int    frequency;
    unsigned int    gain;
    unsigned int    bandwidth;
    uint64_t        rx_count;
    unsigned int    block_size;
    unsigned int    num_xfers;
    unsigned int    stream_buffer_count;
    unsigned int    stream_buffer_size;    /* Units of samples */
};


