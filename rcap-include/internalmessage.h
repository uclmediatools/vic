/*
 * internalmesage.h
 * Anindo banerjea (banerjea@tenet)
 */
#ifndef INTERNALMESSAGE_H
#define INTERNALMESSAGE_H
#include "internal.h"

/* Note about Dependencies in the data structures in this file and in
 * the file "internal.h".
 * 
 * Every time you add an ER you must change
 * 1. RcapInternalMessage.messageLength
 * 2. RcapHeader.rcap_message_length
 * 3. The local copy of this with the host byte ordering
 * 4. RcapInternetNSR.level_length (and the local copy)
 * 5. RcapInternetNSR.er_count (and the local copy)
 * 
 * Every time you add an NSR you must change
 * 1. RcapInternalMessage.messageLength
 * 2. RcapHeader.rcap_message_length ( and local copy)
 * 3. RmtpHeaderRecord.level_count (and local copies )
 * 
 */

/*
 * RcapInfo
 *
 * A machine-dependent representation of portions of an RCAP control
 * message.
 */

typedef struct RcapInfo 
{
    unsigned char message_type;		/* message type */
    unsigned char *messageP;		/* pointer to RCAP control message */
    RcapHeader localHeader ;	/* Rcap header copy with host byte
    				 * ordering.
				 */
    unsigned char *headerP;		/* pointer to RCAP header */

    union {

	struct establish_request {
	    unsigned char *headerRecordP; /* Pointer to header Record
					   */
	    RmtpHeaderRecord localHeaderRecord;
					/* Local copy of header record
					 * Correct host byte ordering
					 */
	    unsigned char *nsrP;	/* pointer to current NSR */
	    RcapInternetNSR localNSR;	/* Current NSR with host byte
	    				 * ordering. This is a local
					 * copy so changes made have
					 * to be explicitly copied
					 * back. with hton conversions
					 */
	    unsigned char *erP;		/* pointer to current ER */
	    RcapInternetER localER;	/* Current ER with host byte
	    				 * ordering (local copy)
					 */
	    unsigned char *previous_erP;	/* pointer to last node's ER */
	    RcapInternetER previous_ER;	/* last node's ER with host byte
	    				 * ordering (local copy)
					 */
	} establish_request ;

	struct establish_denied {
	    unsigned char *establishDeniedP;
	    RcapEstablishDenied localEstablishDenied;
	} establish_denied ;

	struct establish_accept {
	    unsigned char *headerRecordP; /* Pointer to header Record
					   */
	    RmtpHeaderRecord localHeaderRecord;
					/* Local copy of header record
					 * Correct host byte ordering
					 */
	    unsigned char *nsrP;	/* pointer to current NSR */
	    RcapInternetNSR localNSR;	/* Current NSR with host byte
	    				 * ordering. This is a local
					 * copy so changes made have
					 * to be explicitly copied
					 * back. with hton conversions
					 */
	    unsigned char *erP;		/* pointer to current ER */
	    RcapInternetER localER;	/* Current ER with host byte
	    				 * ordering (local copy)
					 */
	    unsigned char *previous_erP;	/* pointer to last node's ER */
	    RcapInternetER previous_ER;	/* last node's ER with host byte
	    				 * ordering (local copy)
					 */
	} establish_accept ;

	struct close_request{
	    unsigned char *closeRequestP;
	    RcapCloseRequestMess localCloseRequest;
	} close_request;

	struct status_request {

	    unsigned char *srrP;	/* pointer to current status
	    				 * request record */
	    RcapInternetSRR localSRR;
	} status_request ;

	struct status_report {
	    unsigned char *srrP;	/* pointer to current status
	    				 * request record */
	    RcapInternetSRR localSRR;

	} status_report ;

    } variant;

} RcapInfo;

/*
 * RcapInternalMessage
 *
 * Internal message format used for communication between RCAP
 * modules on a single node.  This consists of the original
 * RCAP message along with some parsed fields.  The use of this
 * type of structure is purely a performance hack.
 */
typedef struct RcapInternalMessage 
{

    RcapInfo info;		/* info structure as defined above */

    unsigned char *message;		/* original RCAP message */
    unsigned int messageLength;		/* length of original RCAP message 
					 *
					 * Warning: dependency with
					 * RcapHeader.rcap_message_length
					 * and level_length in the
					 * last NSR
					 *
					 */

} RcapInternalMessage;

/*
 * Function prototypes
 */
extern int
InitializeRcapInternalMessage(RcapInternalMessage *theInternalMessage,
			      unsigned char *message, int size);

extern void
InitializeInternetER(RcapInternalMessage *theInternalMessage,  int er_number, RcapInternetER *local_ER, unsigned char **bufferP);

extern void
AddInternetER(RcapInternalMessage *theInternalMessage);

extern void
RemoveInternetER(RcapInternalMessage *theInternalMessage);

extern void
InitializeInternetSRR(RcapInternalMessage *theInternalMessage);

extern void
AddInternetSRR(RcapInternalMessage *theInternalMessage);

extern void
MakeEstablishDenied(RcapInternalMessage *theInternalMessage, unsigned short cause);

extern void
MakeCloseRequest(RcapInternalMessage *theInternalMessage, unsigned short cause, unsigned char message_type);

extern void
SendEstablishDenied(RcapInternalMessage *theInternalMessage, unsigned short upLine,  unsigned short downLine, unsigned short testId, unsigned short cause);

extern void
SendStatusReport(RcapInternalMessage *theInternalMessage, unsigned short outLine, unsigned short state);

#endif INTERNALMESSAGE_H
