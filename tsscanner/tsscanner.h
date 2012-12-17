#ifndef __MPEG_SCANNER_H
#define __MPEG_SCANNER_H

#ifndef _DVBPSI_DVBPSI_H_
#include <dvbpsi/dvbpsi.h>
#endif

#define SYSTEM_CLOCK_DR 0x0B
#define MAX_BITRATE_DR 0x0E
#define STREAM_IDENTIFIER_DR 0x52
#define SUBTITLING_DR 0x59

/*****************************************************************************
 * General typdefs
 *****************************************************************************/
typedef int vlc_bool_t;
#define VLC_FALSE 0
#define VLC_TRUE  1

typedef int64_t mtime_t;

/*****************************************************************************
 * TS stream structures
 *----------------------------------------------------------------------------
 * PAT pid=0
 * - refers to N PMT's with pids known PAT
 *  PMT 0 pid=X stream_type
 *  PMT 1 pid=Y stream_type
 *  PMT 2 pid=Z stream_type
 *  - a PMT refers to N program_streams with pids known from PMT
 *   PID A type audio
 *   PID B type audio
 *   PID C type audio .. etc
 *   PID D type video
 *   PID E type teletext
 *****************************************************************************/

typedef struct
{
    dvbpsi_handle handle;

    int i_pat_version;
    int i_ts_id;
} ts_pat_t;

typedef struct ts_pid_s
{
    int         i_pid;

    vlc_bool_t  b_seen;
    int         i_cc;   /* countinuity counter */

    vlc_bool_t  b_pcr;  /* this PID is the PCR_PID */
    mtime_t     i_pcr;  /* last know PCR value */
} ts_pid_t;

typedef struct ts_pmt_s
{
    dvbpsi_handle handle;

    int         i_number; /* i_number = 0 is actually a NIT */
    int         i_pmt_version;
    ts_pid_t    *pid_pmt;
    ts_pid_t    *pid_pcr;
} ts_pmt_t;

typedef struct
{
    ts_pat_t    pat;

    int         i_pmt;
    ts_pmt_t    pmt;

    ts_pid_t    pid[8192];
} ts_stream_t;

typedef int (*pre_callback_t)(ts_stream_t *p_stream, void *state);
typedef void (*post_callback_t)(void *state);
typedef int (*continuation_check_callback_t)(void *state);

extern int scan_file(char *filename, 
                     pre_callback_t pre_callback, 
                     post_callback_t post_callback, 
                     continuation_check_callback_t continuation_callback, 
                     void *state);

#endif

