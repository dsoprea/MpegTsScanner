#include <stdio.h>
#include <tsscanner/tsfindfirstprogram.h>

void main(int argc, char *argv[])
{

    if(argc < 2)
    {
        printf("Please provide the file-path of an MPEG-TS file.\n\n");
        return -1;
    }

    char *filename = argv[1];

    scan_state_t *state = find_first_program(filename);

    if(state == NULL)
    {
        printf("Could not do scan.\n\n");
        return -2;
    }

/*
    printf("RESULT= (%d)\n", result);

//    printf("Found PMT:\n");
//    DisplayPMT(state.p_stream, state.p_pmt);

    if(state.found == 0)
        printf("PMT not found.\n");
//    else
//        DisplayPMT(state->p_stream, state->p_pmt);
*/
    free_state(state);
}

