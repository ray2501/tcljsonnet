/*
 * Copyright 2015 Danilo Chang. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif
#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include "tcljsonnet.h"
#ifdef __cplusplus
}
#endif


int evaluateFile (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST obj[])
{
    char *file_path = NULL;
    int path_len = 0;
    int error;
    char *output;
    struct JsonnetVm *vm;
    Tcl_Obj *pResultStr = NULL;
    Tcl_DString translatedFilename;

    if(objc != 2)
    {
        Tcl_WrongNumArgs(interp, 1, obj, "file_name");
        return TCL_ERROR;
    }

    file_path = (char *) Tcl_GetStringFromObj(obj[1], &path_len);
    if( !file_path || path_len < 1 ){
       return TCL_ERROR;
    }

    file_path = Tcl_TranslateFileName(interp, file_path, &translatedFilename);

    vm = jsonnet_make();
    output = jsonnet_evaluate_file(vm, file_path, &error);
    Tcl_DStringFree(&translatedFilename);

    if (error) {
          jsonnet_realloc(vm, output, 0);
          jsonnet_destroy(vm);

          Tcl_AppendResult(interp, "jsonnet_evaluate_file fail", (char*)0);
          return TCL_ERROR;
    }

    pResultStr = Tcl_NewStringObj( output, -1 );
    jsonnet_realloc(vm, output, 0);
    jsonnet_destroy(vm);
    Tcl_SetObjResult(interp, pResultStr);

    return TCL_OK;
}


int evaluateSnippet (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST obj[])
{
    char *snippet_string = NULL;
    int snippet_len = 0;
    int error;
    char *output;
    struct JsonnetVm *vm;
    Tcl_Obj *pResultStr = NULL;

    if(objc != 2)
    {
        Tcl_WrongNumArgs(interp, 1, obj, "snippet_string");
        return TCL_ERROR;
    }

    snippet_string = (char *) Tcl_GetStringFromObj(obj[1], &snippet_len);
    if( !snippet_string || snippet_len < 1 ){
       return TCL_ERROR;
    }

    vm = jsonnet_make();
    output = jsonnet_evaluate_snippet(vm, "snippet", snippet_string, &error);
    if (error) {
          jsonnet_realloc(vm, output, 0);
          jsonnet_destroy(vm);

          Tcl_AppendResult(interp, "jsonnet_evaluate_snippet fail", (char*)0);
          return TCL_ERROR;
    }

    pResultStr = Tcl_NewStringObj( output, -1 );
    jsonnet_realloc(vm, output, 0);
    jsonnet_destroy(vm);
    Tcl_SetObjResult(interp, pResultStr);

    return TCL_OK;
}


int Jsonnet_Init(Tcl_Interp *interp)
{
    if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_CreateObjCommand(interp, "jsonnet::evaluateFile", (Tcl_ObjCmdProc *) evaluateFile,
	   (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, "jsonnet::evaluateSnippet", (Tcl_ObjCmdProc *) evaluateSnippet,
       (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}
