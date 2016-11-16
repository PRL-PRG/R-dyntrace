#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <R.h>
#include <Rinternals.h>

#include "rdt.h"
#include <R_ext/Rdynload.h>


#define CALLDEF(name, n)  {#name, (DL_FUNC) &name, n}

static const R_CallMethodDef CallEntries[] = {
    CALLDEF(RdtTrace, 2),
    CALLDEF(RdtTraceBlock, 3),
    CALLDEF(RdtNoop, 0)
};

void R_init_rdt(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}
