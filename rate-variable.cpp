/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include <stdlib.h>
#include "sys-time.h"
#include "vic_tcl.h"

/*
 * The following command & the associate service procedure
 * creates a `rate variable'.  Reading a rate variable returns
 * the time rate of change of its value.  Writing a rate variable
 * updates the rate of change estimate.  A rate variable can be
 * either simple or an array element.  It must be global.  This
 * routine is called as:
 *	rate_variable varname filter_const
 * where 'varname' is the variable name (e.g., "foo" or "foo(bar)")
 * and 'filter_const' is a smoothing constant for the rate estimate.
 * It should be >0 and <1 (a value of 1 will result in no smoothing).
 */
struct rv_data {
	double rate;
	double filter;
	timeval lastupdate;
	int lastval;
	char format[16];
};

class RateVariable : public TclObject {
public:
	RateVariable(const char* name) : TclObject(name) { }
protected:
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION == 0)
	static char* update_rate_var(ClientData, Tcl_Interp*, char* name1,
				     char* name2, int flags);
#else
	static char* update_rate_var(ClientData, Tcl_Interp*, const char* name1,
				     const char* name2, int flags);
#endif
	int command(int argc, const char*const* argv);
} rate_variable_cmd("rate_variable");

#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION == 0)
char* RateVariable::update_rate_var(ClientData clientData, Tcl_Interp* tcl,
		 char* name1, char* name2, int flags) 
#else
char* RateVariable::update_rate_var(ClientData clientData, Tcl_Interp* tcl,
		 const char* name1, const char* name2, int flags) 
#endif
{
	rv_data* rv = (rv_data*)clientData;
	if (rv == NULL)
		return ("no clientdata for rate var");
	if (flags & TCL_TRACE_WRITES) {
		/*
		 * the variable has been written with a new value.
		 * compute the rate of change & make that its vale
		 * for subsequent reads.
		 */
		char res[128];
		flags &= TCL_GLOBAL_ONLY;
		const char* cv = Tcl_GetVar2(tcl, name1, name2, flags); 
		
		if (cv == NULL)
			return (tcl->result);
		int curval = atoi(cv);
		double rate = 0.;
		timeval tv;
		gettimeofday(&tv, 0);
		if (rv->lastupdate.tv_sec != 0) {
			/* not first time through */
			double dt = double(tv.tv_sec - rv->lastupdate.tv_sec) +
				    double(tv.tv_usec -
					rv->lastupdate.tv_usec) * 1e-6;
			if (dt <= 0.)
				/* have to wait until next clock tick */
				return NULL;
			double dv = double(curval - rv->lastval);
			if (dv >= 0.) {
				rate = rv->rate;
				rate += (dv/dt - rate) * rv->filter;
				/* work around tcl f.p. precision bug */
				if (rate < 1e-12)
					rate = 0.;
			}
		}
		rv->rate = rate;
		rv->lastupdate = tv;
		rv->lastval = curval;
		sprintf(res, rv->format, rate);
		Tcl_SetVar2(tcl, name1, name2, res, flags);
	} else if (flags & (TCL_TRACE_DESTROYED|TCL_INTERP_DESTROYED)) {
		delete rv;
	}
	return NULL;
}

int RateVariable::command(int argc, const char *const*argv)
{
	Tcl& tcl = Tcl::instance();
	const char* fmt;
	if (argc == 4)
		fmt = argv[3];
	else if (argc == 3)
		fmt = "%g";
	else {
		tcl.result("usage: rate_variable varname filter_const");
		return (TCL_ERROR);
	}
	double fconst = atof(argv[2]);
	if (fconst <= 0. || fconst > 1.) {
		tcl.result("rate_variable: invalid filter constant");
		return (TCL_ERROR);
	}
	rv_data* rv = new rv_data;
	rv->filter = fconst;
	rv->rate = 0.;
	rv->lastupdate.tv_sec = 0;
	rv->lastval = 0;
	strcpy(rv->format, fmt);
	int sts = Tcl_TraceVar(tcl.interp(), (char*)argv[1],
			TCL_TRACE_WRITES|TCL_TRACE_UNSETS|TCL_GLOBAL_ONLY,
			update_rate_var, (ClientData)rv);
	if (sts != TCL_OK)
		delete rv;
	return (sts);
}
