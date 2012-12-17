
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include <dvbpsi/dvbpsi.h>
#include <dvbpsi/pat.h>

#include <dvbpsi/psi.h>
#include <dvbpsi/descriptor.h>
#include <dvbpsi/pmt.h>
#include <dvbpsi/dr.h>

#include "tsscanner.h"
#include "tsfindfirstprogram.h"

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static void pmt_received(void* p_data, dvbpsi_pmt_t* p_pmt);

// Describes list of programs on channel.
static void pat_received(void* p_data, dvbpsi_pat_t* p_pat)
{
    dvbpsi_pat_program_t* p_program = p_pat->p_first_program;

    scan_state_t* scan_state = (scan_state_t*) p_data;
    ts_stream_t* p_stream = scan_state->p_stream;

    p_stream->pat.i_pat_version = p_pat->i_version;
    p_stream->pat.i_ts_id = p_pat->i_ts_id;

    while( p_program )
    {
        p_stream->i_pmt++;
        p_stream->pmt.i_number = p_program->i_number;
        p_stream->pmt.pid_pmt = &p_stream->pid[p_program->i_pid];
        p_stream->pmt.pid_pmt->i_pid = p_program->i_pid;

        p_stream->pmt.handle = dvbpsi_AttachPMT( p_program->i_number, pmt_received, scan_state );

        p_program = p_program->p_next;
    }

    dvbpsi_DeletePAT(p_pat);
}

static char *get_type_name(uint8_t type)
{
    switch (type)
    {
        case 0x00: return "Reserved";
        case 0x01: return "ISO/IEC 11172 Video";
        case 0x02: return "ISO/IEC 13818-2 Video";
        case 0x03: return "ISO/IEC 11172 Audio";
        case 0x04: return "ISO/IEC 13818-3 Audio";
        case 0x05: return "ISO/IEC 13818-1 Private Section";
        case 0x06: return "ISO/IEC 13818-1 Private PES data packets";
        case 0x07: return "ISO/IEC 13522 MHEG";
        case 0x08: return "ISO/IEC 13818-1 Annex A DSM CC";
        case 0x09: return "H222.1";
        case 0x0A: return "ISO/IEC 13818-6 type A";
        case 0x0B: return "ISO/IEC 13818-6 type B";
        case 0x0C: return "ISO/IEC 13818-6 type C";
        case 0x0D: return "ISO/IEC 13818-6 type D";
        case 0x0E: return "ISO/IEC 13818-1 auxillary";

        default:
            if (type < 0x80)
                return "ISO/IEC 13818-1 reserved";
            else
                return "User Private";
    }
}

static int set_descriptor_primitive(dvbpsi_descriptor_t* p_descriptor, descriptor_t *descriptor)
{
    int a, i;

    dvbpsi_system_clock_dr_t *p_clock_descriptor;
    dvbpsi_max_bitrate_dr_t *bitrate_descriptor;
    dvbpsi_stream_identifier_dr_t *p_si_descriptor;
    dvbpsi_subtitling_dr_t *p_subtitle_descriptor;
    one_subtitle_t *subtitles_raw;

    switch (p_descriptor->i_tag)
    {
        case SYSTEM_CLOCK_DR:
            
            descriptor->type = system_clock;
            p_clock_descriptor = dvbpsi_DecodeSystemClockDr(p_descriptor);

            descriptor->info.system_clock.b_external_clock_ref      = p_clock_descriptor->b_external_clock_ref;
            descriptor->info.system_clock.i_clock_accuracy_integer  = p_clock_descriptor->i_clock_accuracy_integer;
            descriptor->info.system_clock.i_clock_accuracy_exponent = p_clock_descriptor->i_clock_accuracy_exponent;
            
            break;

        case MAX_BITRATE_DR:

            descriptor->type = max_bitrate;
            bitrate_descriptor = dvbpsi_DecodeMaxBitrateDr(p_descriptor);

            descriptor->info.max_bitrate.i_max_bitrate = bitrate_descriptor->i_max_bitrate;

            break;

        case STREAM_IDENTIFIER_DR:

            descriptor->type = stream_identifier;
            p_si_descriptor = dvbpsi_DecodeStreamIdentifierDr(p_descriptor);

            descriptor->info.stream_identifier.i_component_tag = p_si_descriptor->i_component_tag;

            break;

        case SUBTITLING_DR:

            descriptor->type = subtitle;
            p_subtitle_descriptor = dvbpsi_DecodeSubtitlingDr(p_descriptor);

            subtitles_raw = (one_subtitle_t *)calloc(p_subtitle_descriptor->i_subtitles_number, 
                                                     sizeof(one_subtitle_t));

            descriptor->info.subtitle.subtitles = subtitles_raw;
            descriptor->info.subtitle.count     = p_subtitle_descriptor->i_subtitles_number;

            dvbpsi_subtitle_t subtitle;
            for (a = 0; a < p_subtitle_descriptor->i_subtitles_number; ++a)
            {
                subtitle = p_subtitle_descriptor->p_subtitle[a];
            
                subtitles_raw[a].i_iso6392_language_code[0] = subtitle.i_iso6392_language_code[0];
                subtitles_raw[a].i_iso6392_language_code[1] = subtitle.i_iso6392_language_code[1];
                subtitles_raw[a].i_iso6392_language_code[2] = subtitle.i_iso6392_language_code[2];
                subtitles_raw[a].i_subtitling_type          = subtitle.i_subtitling_type;
                subtitles_raw[a].i_composition_page_id      = subtitle.i_composition_page_id;
                subtitles_raw[a].i_ancillary_page_id        = subtitle.i_ancillary_page_id;
            }

            break;

        default:

            descriptor->type = unknown;
            descriptor->info.unknown.data = (uint8_t *)malloc(p_descriptor->i_length);
            
            i = 0;
            while(i < p_descriptor->i_length)
            {
                descriptor->info.unknown.data[i] = p_descriptor->p_data[i];
                i++;
            }
            
            descriptor->info.unknown.length = p_descriptor->i_length;
    }
    
    return 0;
}

static int set_descriptors(descriptor_t **descriptor_ptr, dvbpsi_descriptor_t* p_descriptor)
{
    descriptor_t *descriptor;
    descriptor_t *last_descriptor = NULL;

    int i = 0;
    while(p_descriptor)
    {
        descriptor = (descriptor_t *)malloc(sizeof(descriptor_t));
        descriptor->i_tag = p_descriptor->i_tag;
        descriptor->next = NULL;

        if(set_descriptor_primitive(p_descriptor, descriptor) != 0)
            return -1;

        // Hook the first descriptor to the pointer-of-pointers that we were
        // given.
        if(*descriptor_ptr == NULL)
            *descriptor_ptr = descriptor;
        
        // Chain this descriptor to the last, if there is one.
        if(last_descriptor != NULL)
            last_descriptor->next = descriptor;
        
        last_descriptor = descriptor;

        p_descriptor = p_descriptor->p_next;
        i++;
    }
    
    return 0;
}

static void pmt_received(void* p_data, dvbpsi_pmt_t* p_pmt)
{

    scan_state_t *state = (scan_state_t *)p_data;

    state->found = 1;

    state->pmt_program_number = p_pmt->i_program_number;
    state->pmt_version        = p_pmt->i_version;
    state->pmt_pcr_pid        = p_pmt->i_pcr_pid;

    set_descriptors(&state->pmt_descriptor, p_pmt->p_first_descriptor);

    dvbpsi_pmt_es_t* p_es = p_pmt->p_first_es;

    descriptor_es_t **descriptor_es = &state->pmt_es_descriptor;
    *descriptor_es = NULL;
    
    descriptor_es_t *descriptor_es_temp;
    
    while(p_es)
    {
        descriptor_es_temp = (descriptor_es_t *)malloc(sizeof(descriptor_es_t));
        *descriptor_es = descriptor_es_temp;

        descriptor_es_temp->i_pid        = p_es->i_pid;
        descriptor_es_temp->i_type       = p_es->i_type;
        descriptor_es_temp->type_name    = get_type_name(p_es->i_type);
        descriptor_es_temp->next_sibling = NULL;

        set_descriptors(&descriptor_es_temp->next_child, p_es->p_first_descriptor);

        p_es = p_es->p_next;
        descriptor_es = &descriptor_es_temp->next_sibling;
    }

    dvbpsi_DeletePMT(p_pmt);
}

static void free_descriptor(descriptor_t *descriptor)
{
    if(!descriptor)
        return;

    free_descriptor(descriptor->next);
    
    // The only descriptor property that is malloc'd is stored on a subtitle 
    // descriptor.    
    if(descriptor->type == subtitle)
        free(descriptor->info.subtitle.subtitles);
    
    else if(descriptor->type == unknown)
        free(descriptor->info.unknown.data);
}

static void free_es_descriptor(descriptor_es_t *es_descriptor)
{
    if(!es_descriptor)
        return;

    free_es_descriptor(es_descriptor->next_sibling);
    free_descriptor(es_descriptor->next_child);
}

void free_state(scan_state_t *state)
{
    if(!state)
        return;
    
    if(state->pmt_descriptor)
        free_descriptor(state->pmt_descriptor);
        
    if(state->pmt_es_descriptor)
        free_es_descriptor(state->pmt_es_descriptor);
    
    free(state);
}

static int pre_call(ts_stream_t *p_stream, void *state_)
{
    scan_state_t *state = (scan_state_t *)state_;

    state->p_stream = p_stream;
    p_stream->pat.handle = dvbpsi_AttachPAT( pat_received, state );

    return 0;
}

static void post_call(void *state_)
{
    scan_state_t *state = (scan_state_t *)state_;

    dvbpsi_DetachPAT( state->p_stream->pat.handle );

    state->p_stream->pat.handle = NULL;
}

// Keep processing data until we've found PMT information.
static int continuation_call(void *state_)
{
    scan_state_t *state = (scan_state_t *)state_;

    return (state->found == 0 ? 0 : 1);
}

static void dump_supplemental_type_info(descriptor_type_t type, descriptor_info_t info, const char *indent)
{
    unsigned char i;
    char *temp;

    printf("\n");

    switch(type)
    {
        case max_bitrate:
        
            printf("%s('max_bitrate' descriptor info)\n\n", indent);
            printf("%si_max_bitrate: %u\n",                 indent, info.max_bitrate.i_max_bitrate);
        
            break;

        case system_clock:
        
            printf("%s('system_clock' descriptor info)\n\n", indent);
            printf("%s     b_external_clock_ref: %d\n",           indent, info.system_clock.b_external_clock_ref);
            printf("%s i_clock_accuracy_integer: %u\n",       indent, info.system_clock.i_clock_accuracy_integer);
            printf("%si_clock_accuracy_exponent: %u\n",      indent, info.system_clock.i_clock_accuracy_exponent);
            
            break;

        case stream_identifier:
        
            printf("%s('stream_identifier' descriptor info)\n\n", indent);
            printf("%si_component_tag: %u\n",                     indent, info.stream_identifier.i_component_tag);
            
            break;
        
        case subtitle:
        
            printf("%s('subtitle' descriptor info)\n\n", indent);
            printf("%scount: %u\n",                      indent, info.subtitle.count);
            
            if(info.subtitle.count)
            {
                printf("\n");

                i = 0;                
                while(i < info.subtitle.count)
                {
                    printf("%si_iso6392_language_code: %u, %u, %u\n", 
                            indent, 
                            info.subtitle.subtitles[i].i_iso6392_language_code[0], 
                            info.subtitle.subtitles[i].i_iso6392_language_code[1], 
                            info.subtitle.subtitles[i].i_iso6392_language_code[2]);
                    
                    printf("%s      i_subtitling_type: %u\n", 
                            indent, 
                            info.subtitle.subtitles[i].i_subtitling_type);

                    printf("%s  i_composition_page_id: %u\n", 
                            indent, 
                            info.subtitle.subtitles[i].i_composition_page_id);

                    printf("%s    i_ancillary_page_id: %u\n", 
                            indent, 
                            info.subtitle.subtitles[i].i_ancillary_page_id);

                    printf("\n");
                    
                    i++;
                }
            }
            
            break;

        case unknown:

            temp = strndup((char *)info.unknown.data, info.unknown.length);

            printf("%s('unknown' descriptor info)\n\n", indent);
            printf("%s  data: %s\n",                    indent, temp);
            printf("%slength: %u\n",                    indent, info.unknown.length);

            free(temp);

            break;
            
        default:
        
            printf("%s(invalid descriptor type (%u))\n\n", indent, type);
    }
}

static void dump_descriptors(descriptor_t *current, const char* indent)
{
    const char *indent_proper = (indent ? indent : "");

    unsigned char i = 0;
    while(current)
    {
        printf("%s%u\n",               indent_proper, i);
        printf("%s---\n",              indent_proper);
        printf("%s       i_tag: %u\n", indent_proper, current->i_tag); // Library's type.
        printf("%stype (local): %u\n", indent_proper, current->type);  // Our type.

        dump_supplemental_type_info(current->type, current->info, indent);
    
        printf("\n");
    
        current = current->next;
        i++;
    }
}

void dump_state_info(scan_state_t *state)
{
    const char *indent = "  ";

    if(state->found == 0)
    {
        printf("No PMT packets were found.\n\n");
        return;
    }

    printf("General\n");
    printf("=======\n");

    printf("Program number: %u\n", state->pmt_program_number);
    printf("       Version: %u\n", state->pmt_version);
    printf("       PCR PID: %u\n", state->pmt_pcr_pid);
    
    printf("\n");
    
    descriptor_t *current = state->pmt_descriptor;
    
    if(current)
    {
        printf("Descriptors (Regular)\n");
        printf("=====================\n");
        printf("\n");

        dump_descriptors(current, "");
    }

    descriptor_es_t *es_current = state->pmt_es_descriptor;

    if(es_current)
    {
        printf("Descriptors (ES)\n");
        printf("================\n");
        printf("\n");

        while(es_current)
        {
            printf("%s(New ES section)\n", indent);
            printf("\n");

            printf("%s   i_type: %u\n", indent, es_current->i_type);
            printf("%stype_name: %s\n", indent, es_current->type_name);
            printf("%s    i_pid: %u\n", indent, es_current->i_pid);

            printf("\n");

            if(es_current->next_child)
            {
                printf("%sDescriptors (Regular)\n", indent);
                printf("%s=====================\n", indent);

                dump_descriptors(es_current->next_child, indent);

                printf("\n");
            }

            es_current = es_current->next_sibling;
        }
    }
}

scan_state_t *find_first_program(const char *filename)
{
    int result;
    scan_state_t *state = (scan_state_t *)malloc(sizeof(scan_state_t));
    memset(state, 0, sizeof(scan_state_t));

    result = scan_file((char *)filename,
                       pre_call, 
                       post_call,
                       continuation_call,
                       state);

    if(result != 0)
    {
        free(state);
        return NULL;
    }
    
    return state;
}

