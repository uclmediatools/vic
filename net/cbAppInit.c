/*
 * tkAppInit.c --
 *
 *      Provides a default version of the Tcl_AppInit procedure for
 *      use in wish and similar Tk-based applications.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef lint
static const char sccsid[] = "@(#) tkAppInit.c 1.15 95/06/28 13:14:28";
#endif /* not lint */

#ifdef WIN32^M
#include <winsock.h>^M
#endif

#include <tk.h> //SV

/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */

//extern int matherr();
//int *tclDummyMathPtr = (int *) matherr;


/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *      This procedure performs application-specific initialization.
 *      Most applications, especially those that incorporate additional
 *      packages, will have their own version of this procedure.
 *
 * Results:
 *      Returns a standard Tcl completion code, and leaves an error
 *      message in interp->result if an error occurs.
 *
 * Side effects:
 *      Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppInit(Tcl_Interp *interp)
//interp;               /* Interpreter for application. */
{
    if (Tcl_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tk_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }

    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */

    if (Cb_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */

    /*
     * Specify a user-specific startup file to invoke if the application
     * is run interactively.  Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.  If this line is deleted
     * then no user-specific startup file will be run under any conditions.
     */

    /*tcl_RcFileName = "~/.cbrc";*/
    Tcl_SetVar(interp, "tcl_RcFileName", "~/.cbrc", TCL_GLOBAL_ONLY);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *      This is the main program for the application.
 *
 * Results:
 *      None: Tk_Main never returns here, so this procedure never
 *      returns either.
 *
 * Side effects:
 *      Whatever the application does.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;                   /* Number of command-line arguments. */
    char **argv;                /* Values of command-line arguments. */
{

#ifdef WIN32
        WSADATA wsaData;

        /*
         * Setup Winsock 1.1
         */
        WSAStartup(0x0101, &wsaData);

        /* Confirm that the WinSock DLL supports 1.1.        */
        /* Note that if the DLL supports versions greater    */
        /* than 1.1 in addition to 1.1, it will still return */
        /* 1.1 in wVersion since that is the version we      */
        /* requested.                                        */
        if (LOBYTE(wsaData.wVersion) != 1 ||
        HIBYTE(wsaData.wVersion) != 1 ) {
                /* Couldn't find an acceptable WinSock DLL       */
                WSACleanup();

                fprintf(stderr, "%s: failed to find Winsock version 1.1 or better\n", 
                                argv[0]);

                exit(1);
        }

        /* The WinSock DLL is acceptable. Proceed...         */
        //fprintf(stderr, "%s found Winsock version 1.1 or better\n", argv[0]);
#endif



    Tk_Main(argc, argv, Tcl_AppInit);



#ifdef WIN32
        /*
         * Cleanup Winsock
         */
        WSACleanup();
#endif


    return 0;                   /* Needed only to prevent compiler warning. */
}

