#ifndef __DEPARSE_H__
#define __DEPARSE_H__

#define BUFSIZE 512

#define MIN_Cutoff 20
#define DEFAULT_Cutoff 60
#define MAX_Cutoff (BUFSIZE - 12)
/* ----- MAX_Cutoff  <	BUFSIZE !! */

#include "RBufferUtils.h"

typedef R_StringBuffer DeparseBuffer;

typedef struct {
  int linenumber;
  int len; // FIXME: size_t
  int incurly;
  int inlist;
  Rboolean startline; /* = TRUE; */
  int indent;
  SEXP strvec;

  DeparseBuffer buffer;

  int cutoff;
  int backtick;
  int opts;
  int sourceable;
  int longstring;
  int maxlines;
  Rboolean active;
  int isS4;
  Rboolean fnarg; /* fn argument, so parenthesize = as assignment */
} LocalParseData;

void deparse2buff(SEXP, LocalParseData *);
#endif /* __DEPARSE_H__ */
