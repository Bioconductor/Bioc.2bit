#include <R_ext/Rdynload.h>

void R_init_Bioc_2bit(DllInfo *info)
{
	//R_registerRoutines(info, NULL, callMethods, NULL, NULL);
	R_useDynamicSymbols(info, 0);
	return;
}

