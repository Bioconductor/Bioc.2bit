#include <R_ext/Rdynload.h>

#include "twoBit.h"

#define CALLMETHOD_DEF(fun, numArgs) {#fun, (DL_FUNC) &fun, numArgs}

static const R_CallMethodDef callMethods[] = {

/* twoBit.c */
	CALLMETHOD_DEF(TwoBits_write, 2),
	CALLMETHOD_DEF(TwoBitFile_read, 4),

	{NULL, NULL, 0}
};

void R_init_Bioc_2bit(DllInfo *info)
{
	R_registerRoutines(info, NULL, callMethods, NULL, NULL);
	R_useDynamicSymbols(info, 0);
	return;
}

