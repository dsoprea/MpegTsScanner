/*****************************************************************************
 * decode_mpeg.c: MPEG decoder example
 *----------------------------------------------------------------------------
 * Copyright (C) 2001-2010 VideoLAN
 * $Id: decode_mpeg.c 104 2005-03-21 13:38:56Z massiot $
 *
 * Authors: Jean-Paul Saman <jpsaman #_at_# m2x dot nl>
 *          Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *----------------------------------------------------------------------------
 *
 *****************************************************************************/

//#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <inttypes.h>

#include <dvbpsi/descriptor.h>
#include <dvbpsi/dvbpsi.h>
#include <dvbpsi/pat.h>
#include <dvbpsi/pmt.h>

#include "tsscanner.h"

static int read_packet( int i_fd, uint8_t* p_dst )
{
    int i = 187;
    int i_rc = 1;

    p_dst[0] = 0;

    while((p_dst[0] != 0x47) && (i_rc > 0))
    {
        i_rc = read(i_fd, p_dst, 1);
    }

    while((i != 0) && (i_rc > 0))
    {
        i_rc = read(i_fd, p_dst + 188 - i, i);
        if(i_rc >= 0)
            i -= i_rc;
    }
    return (i_rc <= 0) ? i_rc : 188;
}

int scan_file(char *filename, 
              pre_callback_t pre_callback, 
              post_callback_t post_callback, 
              continuation_check_callback_t continuation_callback, 
              void *state)
{
//    int next_option = 0;

    // LIBDVBPSI likes to spit stuff out on STDERR. Squelch it.
    freopen("/dev/null", "w", stderr);

    int i_fd = -1;
//    int i_mtu = 1316; /* (7 * 188) = 1316 < 1500 network MTU */

//    mtime_t  time_prev = 0;
//    mtime_t  time_base = 0;

//    mtime_t  i_prev_pcr = 0;  /* 33 bits */
//    int      i_old_cc = -1;
    uint32_t i_bytes = 0; /* bytes transmitted between PCR's */

    uint8_t *p_data = NULL;
    ts_stream_t *p_stream = NULL;
    int i_len = 0;

    struct stat stat_buf;
    if(stat(filename, &stat_buf) != 0)
        return -1;

    i_fd = open( filename, 0 );
    p_data = (uint8_t *) malloc( sizeof( uint8_t ) * 188 );
    if( !p_data )
        return -2;

    p_stream = (ts_stream_t *) malloc( sizeof(ts_stream_t) );
    if( !p_stream )
        return -3;

    memset( p_stream, 0, sizeof(ts_stream_t) );

    i_len = read_packet( i_fd, p_data );

    if(pre_callback(p_stream, state) != 0)
        return -4;

    // We expect the pre-callback to assign a PAT handler. This is required for
    // the process (it also would be useless without it).
    if( p_stream->pat.handle == NULL )
        return -5;

    while( i_len > 0)
    {
        if(continuation_callback(state) != 0)
            break;
    
        int i = 0;
//        vlc_bool_t b_first = VLC_FALSE;

        i_bytes += i_len;
        for( i = 0; i < i_len; i += 188 )
        {
            uint8_t   *p_tmp = &p_data[i];
            uint16_t   i_pid = ((uint16_t)(p_tmp[1] & 0x1f) << 8) + p_tmp[2];
            int        i_cc = (p_tmp[3] & 0x0f);
            vlc_bool_t b_adaptation = (p_tmp[3] & 0x20); /* adaptation field */
            vlc_bool_t b_discontinuity_seen = VLC_FALSE;

//            long int pos = lseek(i_fd, 0, 1);

            if( i_pid == 0x0 )
                dvbpsi_PushPacket(p_stream->pat.handle, p_tmp);

            else if( p_stream->pmt.pid_pmt && i_pid == p_stream->pmt.pid_pmt->i_pid )
                dvbpsi_PushPacket(p_stream->pmt.handle, p_tmp);

            /* Remember PID */
            if( !p_stream->pid[i_pid].b_seen )
            {
                p_stream->pid[i_pid].b_seen = VLC_TRUE;
//                i_old_cc = i_cc;
                p_stream->pid[i_pid].i_cc = i_cc;
            }
            else
            {
                /* Check continuity counter */
                int i_diff = 0;

                i_diff = i_cc - (p_stream->pid[i_pid].i_cc+1)%16;
                b_discontinuity_seen = ( i_diff != 0 );

                /* Update CC */
//                i_old_cc = p_stream->pid[i_pid].i_cc;
                p_stream->pid[i_pid].i_cc = i_cc;
            }

            /* Handle discontinuities if they occurred,
             * according to ISO/IEC 13818-1: DIS pages 20-22 */
            if( b_adaptation )
            {
//                vlc_bool_t b_discontinuity_indicator = (p_tmp[5]&0x80);
//                vlc_bool_t b_random_access_indicator = (p_tmp[5]&0x40);
                vlc_bool_t b_pcr = (p_tmp[5]&0x10);  /* PCR flag */

//                if( b_discontinuity_indicator )
//                    fprintf( stderr, "Discontinuity indicator (pid %d)\n", i_pid );
//                if( b_random_access_indicator )
//                    fprintf( stderr, "Random access indicator (pid %d)\n", i_pid );

                /* Dump PCR */
                if( b_pcr && (p_tmp[4] >= 7) )
                {
                    mtime_t i_pcr;  /* 33 bits */

                    i_pcr = ( ( (mtime_t)p_tmp[6] << 25 ) |
                              ( (mtime_t)p_tmp[7] << 17 ) |
                              ( (mtime_t)p_tmp[8] << 9 ) |
                              ( (mtime_t)p_tmp[9] << 1 ) |
                              ( (mtime_t)p_tmp[10] >> 7 ) ) / 90;
//                    i_prev_pcr = p_stream->pid[i_pid].i_pcr;
                    p_stream->pid[i_pid].i_pcr = i_pcr;

                    i_bytes = 0; /* reset byte counter */

                    if( b_discontinuity_seen )
                    {
                        /* cc discontinuity is expected */
//                        fprintf( stderr, "Server signalled the continuity counter discontinuity\n" );
                        /* Discontinuity has been handled */
                        b_discontinuity_seen = VLC_FALSE;
                    }
                }
            }

            if( b_discontinuity_seen )
            {
//                fprintf( stderr, "Continuity counter discontinuity (pid %d found %d expected %d)\n",
//                    i_pid, p_stream->pid[i_pid].i_cc, i_old_cc+1 );
                /* Discontinuity has been handled */
                b_discontinuity_seen = VLC_FALSE;
            }
        }

        /* Read next packet */
        if( filename )
            i_len = read_packet( i_fd, p_data );
    }

    post_callback(state);

    // It must be cleared, from the post-callback.
    if( p_stream->pat.handle != NULL )
        return -6;

    if( p_stream->pmt.handle )
        dvbpsi_DetachPMT( p_stream->pmt.handle );

    if( p_stream->pat.handle )
        dvbpsi_DetachPAT( p_stream->pat.handle );

    close( i_fd );

    if( p_data )
        free( p_data );

    if( p_stream )
        free( p_stream );

    return 0;
}

