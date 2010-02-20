/*
 * Copyright (c) 1993-1995 The Regents of the University of California.
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tcl.h>
#ifndef NO_TK
#include <tk.h>
#endif
#undef Status
#include "vic_tcl.h"
#include <sys/types.h>
#ifdef USE_ZVFS
#include "zvfs.h"
extern "C" {
	char *TclSetPreInitScript(char *string);
}
#endif

Tcl Tcl::instance_;

Tcl::Tcl() :
	tcl_(0),
	tkmain_(0),
	application_(0)
{
	bp_ = buffer_;
}

void Tcl::init(const char* application)
{
	init(Tcl_CreateInterp(), application);
}

void Tcl::init(Tcl_Interp* tcl, const char* application)
{
	instance_.tcl_ = tcl;
	instance_.application_ = application;

#ifdef USE_ZVFS
	Zvfs_Init(tcl);
	Zvfs_Mount(tcl, (char *)Tcl_GetNameOfExecutable(), "/zvfs");
	Tcl_SetVar2(tcl, "env", "TCL_LIBRARY", "/zvfs/tcl", TCL_GLOBAL_ONLY);
	Tcl_SetVar2(tcl, "env", "TK_LIBRARY", "/zvfs/tk", TCL_GLOBAL_ONLY);

	Tcl_SetVar(tcl, "auto_path", "/zvfs/tcl /zvfs/tk /zvfs/vic", TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl, "tcl_libPath", "/zvfs/tcl /zvfs/tk /zvfs/vic", TCL_GLOBAL_ONLY);
	TclSetPreInitScript("\n"
"proc tclInit {} {\n"
"  global tcl_libPath tcl_library env\n"
"  rename tclInit {}\n"
"  set tcl_library [set env(TCL_LIBRARY)]\n"
"  set tclfile [file join $tcl_library init.tcl]\n"
"  if {[file exists $tclfile]} {\n"
"    set errors {}\n"
"    if {[catch {uplevel #0 [list source $tclfile]} msg opts]} {\n"
"      append errors \"$tclfile: $msg\n\"\n"
"      append errors \"[dict get $opts -errorinfo]\n\"\n"
"      set msg \"Can't find a usable init.tcl in the following location: \n\"\n"
"      append msg \"$errors\n\n\"\n"
"      append msg \"This probably means that VIC wasn't built properly.\n\"\n"
"      error $msg\n"
"    }\n"
"  }\n"
"}\n"
"tclInit");
#endif
	//Tk_InitConsoleChannels(tcl);
	Tcl_Init(tcl);

#ifdef USE_ZVFS
	Tcl_SetVar(tcl, "auto_path", "/zvfs/tcl /zvfs/tk /zvfs/vic", TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl, "tcl_libPath", "/zvfs/tcl /zvfs/tk /zvfs/vic", TCL_GLOBAL_ONLY);
#endif
}

void Tcl::evalc(const char* s)
{
	u_int n = strlen(s) + 1;
	if (n < sizeof(buffer_) - (bp_ - buffer_)) {
		char* const p = bp_;
		bp_ += n;
		strcpy(p, s);
		eval(p);
		bp_ = p;
	} else {
		char* p = new char[n + 1];
		strcpy(p, s);
		eval(p);
		delete[] p; //SV-XXX: Debian
	}
}

void Tcl::eval(char* s)
{
	int st = Tcl_GlobalEval(tcl_, s);
	if (st != TCL_OK) {
		int n = strlen(application_) + strlen(s);
		char* wrk = new char[n + 80];
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION < 5)
		sprintf(wrk, "tkerror \"%s: %s\"", application_, s);
#else
		sprintf(wrk, "bgerror \"%s: %s\"", application_, s);
#endif
		Tcl_GlobalEval(tcl_, wrk);
		delete[] wrk; //SV-XXX: Debian
		//exit(1);
	}
}

void Tcl::eval()
{
	char* p = bp_;
	bp_ = p + strlen(p) + 1;
	/*XXX*/
	if (bp_ >= &buffer_[1024]) {
		fprintf(stderr, "bailing in Tcl::eval\n");
		abort();
	}
	eval(p);
	bp_ = p;
}

void Tcl::error(const char* s)
{
	fprintf(stderr, "%s: \"%s\": %s\n", application_, s, tcl_->result);
	exit(1);
}

#ifndef NO_TK
void Tcl::add_option(const char* name, const char* value)
{
	bp_[0] = toupper(application_[0]);
	sprintf(&bp_[1], "%s.%s", application_ + 1, name);
	Tk_AddOption(tkmain_, bp_, (char*)value, TK_USER_DEFAULT_PRIO + 1);
}

void Tcl::add_default(const char* name, const char* value)
{
	bp_[0] = toupper(application_[0]);
	sprintf(&bp_[1], "%s.%s", application_ + 1, name);
	Tk_AddOption(tkmain_, bp_, (char*)value, TK_STARTUP_FILE_PRIO + 1);
}

const char* Tcl::attr(const char* attr) const
{
	bp_[0] = toupper(application_[0]);
	strcpy(&bp_[1], application_ + 1);
	const char* cp = Tk_GetOption(tkmain_, (char*)attr, bp_);
	if (cp != 0 && *cp == 0)
		cp = 0;
	return (cp);
}
#endif

TclObject* TclObject::all_;
int TclObject::id_;

TclObject::TclObject(const char* name) : name_(0), class_name_(0)
{
	char wrk[32];
	if (name == 0) {
		sprintf(wrk, "_o%d", id_++);
		name = wrk;
	}
	setproc(name);
	next_ = all_;
	all_ = this;
}

TclObject::~TclObject()
{
	Tcl& tcl = Tcl::instance();
	if (!tcl.dark())
		tcl.DeleteCommand(name_);
	if (name_ != 0)
		delete[] name_; //SV-XXX: Debian
	TclObject** p;
	for (p = &all_; *p != this; p = &(*p)->next_)
		;
	*p = (*p)->next_;
	delete[] class_name_; //SV-XXX: Debian
}

/*
 * go through all the objects and make sure they are defines
 * i.e., this should be run at startup to initialize all the
 * statically defined object classes.  it's okay if we create
 * a command twice in the tcl interpreter -- we'll just "override"
 * the value that we already put there.
 */
void TclObject::define()
{
	Tcl& tcl = Tcl::instance();
	for (TclObject* p = all_; p != 0; p = p->next_) {
		tcl.CreateCommand(p->name(), callback, (ClientData)p, 0);
		p->inception();
	}
}

/*
 * Called when object is hooked into tcl, which may be when the
 * object is created (if Tcl has been initialized), or later, when
 * TclObject::define() is called.
 */
void TclObject::inception()
{
}

void TclObject::setproc(const char* s)
{
	Tcl& tcl = Tcl::instance();
	if (name_ != 0) {
		if (!tcl.dark())
			tcl.DeleteCommand(name_);
		delete[] name_; //SV-XXX: Debian
	}
	int n = strlen(s);
	name_ = new char[n + 1];
	strcpy(name_, s);
	if (!tcl.dark()) {
		tcl.CreateCommand(name_, callback, (ClientData)this, 0);
		inception();
	}
}

#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION == 0)
int TclObject::callback(ClientData cd, Tcl_Interp*, int ac, char** av)
#else
int TclObject::callback(ClientData cd, Tcl_Interp*, int ac, const char** av)
#endif
{
	TclObject* tc = (TclObject*)cd;
	return (tc->command(ac, (const char*const*)av));
}

int TclObject::command(int argc, const char*const* argv)
{
	Tcl& t = Tcl::instance();
	char* cp = t.buffer();
	sprintf(cp, "%s: ", t.application());
	cp += strlen(cp);
	const char* cmd = argv[0];
	if (cmd[0] == '_' && cmd[1] == 'o' && class_name_ != 0)
		sprintf(cp, "\"%s\" (%s): ", class_name_, cmd);
	else
		sprintf(cp, "%s: ", cmd);
	cp += strlen(cp);
	if (argc >= 2)
		sprintf(cp, "no such method (%s)", argv[1]);
	else
		sprintf(cp, "requires additional args");

	t.result(t.buffer());
	return (TCL_ERROR);
}

void TclObject::reset()
{
}

void TclObject::reset_all()
{
	for (TclObject* p = all_; p != 0; p = p->next_)
		p->reset();
}

TclObject* TclObject::lookup(const char* name)
{
	TclObject* p;
	for (p = all_; p != 0; p = p->next_) {
		if (strcmp(p->name_, name) == 0)
			break;
	}
	return (p);
}

void TclObject::class_name(const char* s)
{
	delete[] class_name_; //SV-XXX: Debian
	class_name_ = new char[strlen(s) + 1];
	strcpy(class_name_, s);
}

/*
 * delete command - can be used to delete any tcl object
 * (i.e., since ~TclObject is virtual)
 */
class DeleteCommand : public TclObject {
public:
	DeleteCommand() : TclObject("delete") {}
	int command(int argc, const char*const* argv);
} cmd_delete;

int DeleteCommand::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc != 2) {
		tcl.result("delete: bad args");
		return (TCL_ERROR);
	}
	TclObject* p = TclObject::lookup(argv[1]);
	if (p == 0) {
		tcl.result("delete: no such object");
		return (TCL_ERROR);
	}
	delete p;
	return (TCL_OK);
}

/*
 * create command - can be used to create any tcl object
 * create $classname $id
 */
class CreateCommand : public TclObject {
public:
	CreateCommand() : TclObject("new") {}
	int command(int argc, const char*const* argv);
} cmd_create;

int CreateCommand::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	const char* id;
	if (argc == 2)
		id = 0;
	else if (argc == 3)
		id = argv[2];
	else {
		tcl.result("create: bad args");
		return (TCL_ERROR);
	}
	TclObject* p = Matcher::lookup(argv[1], id);
	if (p != 0)
		tcl.result(p->name());
	return (TCL_OK);
}

Matcher* Matcher::all_;

Matcher::Matcher(const char* classname) : classname_(classname)
{
	next_ = all_;
	all_ = this;
}

TclObject* Matcher::lookup(const char* classname, const char* id)
{
	for (Matcher* p = all_; p != 0; p = p->next_) {
		if (strcasecmp(classname, p->classname_) != 0)
			continue;
		TclObject* o = p->match(id);
		if (o != 0) {
			/* remember an id for error messages */
			if (id != 0) {
				char wrk[80];
				sprintf(wrk, "%s/%s", classname, id);
				o->class_name(wrk);
			} else
				o->class_name(classname);
			return (o);
		}
	}
#ifdef DEBUG
    debug_msg("Failed Matcher::lookup on %s\n", classname);
#endif /* DEBUG */
	return (0);
}

EmbeddedTcl* EmbeddedTcl::all_;

EmbeddedTcl::EmbeddedTcl(int pass, const char* code) : code_(code), pass_(pass)
{
	next_ = all_;
	all_ = this;
}

int EmbeddedTcl::makepass(int pass)
{
	int done = 1;
	Tcl& tcl = Tcl::instance();
	for (EmbeddedTcl* p = all_; p != 0; p = p->next_) {
		if (p->pass_ > pass)
			done = 0;
		else if (p->pass_ == pass)
			tcl.evalc(p->code_);
	}
	return (!done);
}

void EmbeddedTcl::init()
{
	/* make sure all static commands are defined */
	TclObject::define();

	int pass = 0;
	while (makepass(pass))
		++pass;
}
