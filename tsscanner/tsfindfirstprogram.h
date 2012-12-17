#ifndef __TSFINDFIRSTPROGRAM_H
#define __TSFINDFIRSTPROGRAM_H

#include <stdint.h>

#ifndef _DVBPSI_DVBPSI_H_
#include <dvbpsi/dvbpsi.h>
#endif

#include <tsscanner/tsscanner.h>

typedef struct
{
    uint32_t i_max_bitrate;
} max_bitrate_descriptor_t;

typedef struct
{
    int b_external_clock_ref;
    uint8_t i_clock_accuracy_integer;
    uint8_t i_clock_accuracy_exponent;
} system_clock_descriptor_t;

typedef struct
{
    uint8_t i_component_tag;
} stream_identifier_descriptor_t;

typedef struct
{
    uint8_t i_iso6392_language_code[3];
    uint8_t i_subtitling_type;
    uint16_t i_composition_page_id;
    uint16_t i_ancillary_page_id;
} one_subtitle_t;

typedef struct
{
    one_subtitle_t *subtitles;
    unsigned char count;
} subtitle_descriptor_t;

typedef struct
{
    char *data;
    unsigned char length;
} unknown_descriptor_t;

typedef union
{
    max_bitrate_descriptor_t max_bitrate;
    system_clock_descriptor_t system_clock;
    stream_identifier_descriptor_t stream_identifier;
    subtitle_descriptor_t subtitle;
    unknown_descriptor_t unknown;
} descriptor_info_t;

typedef enum
{
    max_bitrate,
    system_clock,
    stream_identifier,
    subtitle,
    unknown
} descriptor_type_t;

struct descriptor_s;
typedef struct descriptor_s descriptor_t;

struct descriptor_s
{
    unsigned char i_tag;
    descriptor_type_t type;
    descriptor_info_t info;
    
    descriptor_t *next;
};

struct descriptor_es_s;
typedef struct descriptor_es_s descriptor_es_t;

// A PMT record might have many ES descriptors, where each has a LL of normal 
// descriptors.
struct descriptor_es_s
{
    uint8_t i_type; /*!< stream_type */
    char *type_name;
    uint16_t i_pid; /*!< elementary_PID */

    descriptor_t *next_child;
    descriptor_es_t *next_sibling;
};

typedef struct
{
    ts_stream_t* p_stream;
    int found;

    uint16_t pmt_program_number;
    uint8_t  pmt_version;
    uint16_t pmt_pcr_pid;
    descriptor_t *pmt_descriptor;
    descriptor_es_t *pmt_es_descriptor;
} scan_state_t;

extern scan_state_t *find_first_program(const char *filename);

#endif

