/*
 * Copyright (c) 1993-1994 The Regents of the University of California.
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

#ifndef vic_Tcl_h
#define vic_Tcl_h

#include "config.h"
#include <string.h>
#include <tcl.h>

struct Tk_Window_;

class Tcl {
    public:
	/* constructor should be private but SGIs C++ compiler complains*/
	Tcl();
	static void init(const char* application);
	static void init(Tcl_Interp*, const char* application);
	static inline Tcl& instance() { return (instance_); }
	inline int dark() const { return (tcl_ == 0); }
	inline Tcl_Interp* interp() const { return (tcl_); }
	inline char* result() const { return (tcl_->result); }
	inline void result(const char* p, Tcl_FreeProc* freeProc = TCL_STATIC)
	    { Tcl_SetResult(tcl_, (char *)p, freeProc); }
	void resultf(const char* fmt, ...);
	inline void CreateCommand(const char* cmd, Tcl_CmdProc* cproc,
				  ClientData cd = 0,
				  Tcl_CmdDeleteProc* dproc = 0) {
		Tcl_CreateCommand(tcl_, (char*)cmd, cproc, cd, dproc);
	}
	inline void CreateCommand(Tcl_CmdProc* cproc,
				  ClientData cd = 0,
				  Tcl_CmdDeleteProc* dproc = 0) {
		Tcl_CreateCommand(tcl_, buffer_, cproc, cd, dproc);
	}
	inline void DeleteCommand(const char* cmd) {
		Tcl_DeleteCommand(tcl_, (char*)cmd);
	}
	inline void EvalFile(const char* file) {
		if (Tcl_EvalFile(tcl_, (char*)file) != TCL_OK)
			error(file);
	}
	inline char* var(char* varname, int flags = TCL_GLOBAL_ONLY) { //SV-XXX: FreeBSD
		return ((char*)Tcl_GetVar(tcl_, (char*) varname, flags)); //SV-XXX: FreeBSD
	}
	/*
	 * Hooks for invoking the tcl interpreter:
	 *  eval(char*) - when string is in writable store
	 *  evalc() - when string is in read-only store (e.g., string consts)
	 *  evalf() - printf style formatting of command
	 * Or, write into the buffer returned by buffer() and
	 * then call eval(void).
	 */
	void eval(char* s);
	void evalc(const char* s);
	void eval();
	char* buffer() { return (bp_); }
	/*
	 * This routine used to be inlined, but SGI's C++ compiler
	 * can't hack stdarg inlining.  No big deal here.
	 */
	void evalf(const char* fmt, ...);

	inline struct Tk_Window_* tkmain() const { return (tkmain_); }
	inline void tkmain(struct Tk_Window_* w) { tkmain_ = w; }
	void add_option(const char* name, const char* value);
	void add_default(const char* name, const char* value);
	const char* attr(const char* attr) const;
	const char* application() const { return (application_); }
	inline const char* rds(const char* a, const char* fld) const {
		return (Tcl_GetVar2(tcl_, (char*)a, (char*)fld,
				    TCL_GLOBAL_ONLY));
	}
    private:
	void error(const char*);

	static Tcl instance_;
	Tcl_Interp* tcl_;
	Tk_Window_* tkmain_;
	char* bp_;
	const char* application_;
	char buffer_[4096];
};

class TclObject {
    public:
	virtual ~TclObject();
	static TclObject* lookup(const char* name);
	inline const char* name() { return (name_); }
	static void define();
	virtual void reset();
	void class_name(const char*);
	/* make this public for Sun's compiler */
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION == 0)
	static int callback(ClientData, Tcl_Interp*, int ac, char** av);
#else
	static int callback(ClientData, Tcl_Interp*, int ac, const char** av);
#endif
	static void reset_all();
    protected:
	TclObject(const char* name = 0);
	virtual int command(int argc, const char*const* argv);
	void setproc(const char* proc);
	virtual void inception();
    private:
	static int id_;
	static TclObject* all_;
	char* name_;
	char* class_name_;
	TclObject* next_;
};

class Matcher {
    public:
	static TclObject* lookup(const char* classname, const char* id);
    protected:
	Matcher(const char* classname);
	virtual TclObject* match(const char* id) = 0;
        virtual ~Matcher() {}; //SV-XXX: Solves the "missing" virtual destructor warning from gcc4
    private:
	static Matcher* all_;
	Matcher* next_;
	const char* classname_;
};

class EmbeddedTcl {
    public:
	EmbeddedTcl(int pass, const char* code);
	static void init();
    private:
	static int makepass(int pass);

	const char* code_;
	int pass_;
	EmbeddedTcl* next_;
	static EmbeddedTcl* all_;
};

#endif
