#include <stdio.h>
#include <tsscanner/tsfindfirstprogram.h>

int main(int argc, char *argv[])
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

    dump_state_info(state);

    free_state(state);
}

