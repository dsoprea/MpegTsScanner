
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <dvbpsi/dvbpsi.h>
#include <dvbpsi/pat.h>

// This is a derived function from the dvbpsi_DeletePAT is actually a macro.
void tsscanner_PatchedDeletePAT(dvbpsi_pat_t* p_pat)
{
    dvbpsi_EmptyPAT(p_pat);
    free(p_pat);
}

