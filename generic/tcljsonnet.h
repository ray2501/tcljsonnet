#ifndef _JSONNET
#define _JSONNET

/*
 * For C++ compilers, use extern "C"
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <tcl.h>
#include "libjsonnet.h"

/*
 * Only the _Init function is exported.
 */

extern DLLEXPORT int	Jsonnet_Init(Tcl_Interp * interp);

int  evaluateFile (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const obj[]);
int  evaluateSnippet (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const obj[]);

/*
 * end block for C++
 */

#ifdef __cplusplus
}
#endif

#endif /* _JSONNET */
