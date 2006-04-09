/*
 */

#ifndef lint
/*
static char rcsid[] =
    "@(#) $Header$ (LBL)";
 */
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "vic_tcl.h"
#include "assistor-list.h"

class AssistorList assistorlist;

int
AssistorList::command(int argc,const char *const*argv) {
	int	i;
	Tcl& tcl = Tcl::instance();
/*	fprintf(stderr,"WindowAssistorList::"__FUNCTION__"(");
	for (int i=0;i<argc;i++)
		fprintf(stderr,"%s,",argv[i]);
	fprintf(stderr,")\n");
 */
	if (argc == 2) {
		if (!strcmp(argv[1],"assistors")) {

			char	*ret;
			int	len;
			
			len = 0;
			for (i=0;i<nrofwassistors;i++)
				len+=strlen(wassistors[i].name+1);
			len+=3;
			ret = (char*)malloc(len);
			strcpy(ret,"{");
			for (i=0;i<nrofwassistors;i++) {
				if (ret[1]) strcat(ret," ");
				strcat(ret,wassistors[i].name);
			}
			strcat(ret,"}");
			tcl.result(ret);
			return (TCL_OK);
		}
	}
	if (argc == 3) {
		if (!strcmp(argv[1],"assistors")) {
			char	*ret;
			int	len;
			
			len = 0;
			for (i=0;i<nrofwassistors;i++)
				if (NULL!=strstr(wassistors[i].formats,argv[2]))
					len+=strlen(wassistors[i].name+1);
			len+=3;
			ret = (char*)malloc(len);strcpy(ret,"");
			for (i=0;i<nrofwassistors;i++) {
				if (NULL==strstr(wassistors[i].formats,argv[2]))
					continue;
				if (ret[0]) strcat(ret," ");
				strcat(ret,wassistors[i].name);
			}
			tcl.result(ret);
			return (TCL_OK);
		}
	}
	return TclObject::command(argc,argv);
}

void 
AssistorList::register_assistor(char *name,char *formats) {
	//fprintf(stderr,"register_assistor(%s,%s)\n",name,formats);
	for (int i=0;i<nrofwassistors;i++)
		if (!strcmp(wassistors[i].name,name))
			return;
	if (nrofwassistors)
		wassistors = (struct wassistor*)realloc(wassistors,sizeof(struct wassistor)*(nrofwassistors+1));
	else 
		wassistors = (struct wassistor*)malloc(sizeof(struct wassistor));
	wassistors[nrofwassistors].name = strdup(name);
	wassistors[nrofwassistors].formats = strdup(formats);
	nrofwassistors++;
}

class ALMatcher : public Matcher {
public:
	ALMatcher() : Matcher("assistorlist") { }
	class TclObject* match(const char *id) {
		UNUSED(id); //SV-XXX: unused
		return (TclObject*)&assistorlist;
	}
} almatcher;
