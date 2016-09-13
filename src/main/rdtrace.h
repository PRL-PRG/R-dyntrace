#ifndef	_RDTRACE_H
#define	_RDTRACE_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Defn.h>
#include <rdtrace_probes.h>

void rdtrace_function_entry(SEXP call, SEXP op, SEXP rho);

#endif	/* _RDTRACE_H */
