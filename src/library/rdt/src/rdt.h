#ifndef	_RDT_H
#define	_RDT_H

SEXP RdtTrace(SEXP s_filename, SEXP disabled_probes);
SEXP RdtTraceBlock(SEXP rho, SEXP s_filename, SEXP disabled_probes);
SEXP RdtNoop();

#endif /* _RDT_H */