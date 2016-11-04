#ifndef	_RDTRACE_H
#define	_RDTRACE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Defn.h>
#include <rdtrace_probes.h>

#define RDT_IS_ENABLED(name) (rdt_curr_handler->name != NULL)
#define RDT_FIRE_PROBE(name, ...) (rdt_curr_handler->name(__VA_ARGS__))

void rdt_start(const rdt_handler *handler, const SEXP options);
void rdt_stop();

extern const rdt_handler *rdt_curr_handler;

#endif	/* _RDTRACE_H */
