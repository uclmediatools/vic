/*
 * $Header$
 *
 * nethost.h
 * Bruce A. Mah (bmah@tenet)
 *
 * $Log$
 * Revision 1.1  1998/02/18 18:04:00  ucacsva
 * Initial revision
 *
 * Revision 1.6  1992/07/29  20:46:30  bmah
 * Added a couple of #includes for HP/UX compatability.
 *
 * Revision 1.5  1992/06/02  03:03:22  bmah
 * Added RCAP_CLOSE_REQUEST_SIZE.
 *
 * Revision 1.4  1992/05/21  03:35:30  bmah
 * Modified internet ER and SRR in accordance with new parameters for
 * Tenet scheme.
 *
 * Revision 1.3  1992/05/12  20:59:05  bmah
 * Fixed typos, it now works.  Checkpoint before making changes to NSR
 * and SRR structure to accomodate new channel establishment parameters
 * for buffer allocation.
 *
 * Revision 1.2  1992/02/01  02:48:43  bmah
 * Added pseudo-control messages.
 *
 * Revision 1.1  1992/01/18  22:46:23  bmah
 * Initial revision
 *
 *
 * Header file for network<->host byte order conversions.
 */
#ifndef NETHOST_H
#define NETHOST_H

#include "internal.h"
#include "internalmessage.h"
#include <sys/types.h>
#include <netinet/in.h>

/*
 * Primitive net<->host conversions.  These conversions are similar to
 * ntohl() & Co. but they read/write the value in question from/to a
 * character array (e.g. representation of an RCAP control message).
 * They depend on having ntohl(), et al.
 */
extern unsigned long nhl;
extern unsigned short nhs;
extern unsigned char nhb;
extern unsigned long nhip;

#define NetToHostLong(out, in) (out = ntohl(* (unsigned long *) \
					    (in)))
#define NetToHostShort(out, in) (out = ntohs(* (unsigned short *) \
					     (in)))
#define NetToHostIp(out, in) (out = * (unsigned long *) (in))
#define NetToHostByte(out, in) (out = * (unsigned char *) (in))

#define HostToNetLong(out, in) (* (unsigned long *) (out) = htonl(in))
#define HostToNetShort(out, in) (* (unsigned short *) (out) = htons(in))
#define HostToNetIp(out, in) (* (unsigned long *) (out) = in)
#define HostToNetByte(out, in) (* (unsigned char *) (out) = in)

/*
 * Structure conversions.
 */
extern void NetToHostRcapHeader(RcapHeader *theRcapHeader,
				unsigned char *theBytes);
extern void NetToHostRmtpHeaderRecord(RmtpHeaderRecord *theRmtpHeaderRecord,
				      unsigned char *theBytes);
extern void NetToHostRcapInternetNSR(RcapInternetNSR *theRcapInternetNSR,
				     unsigned char *theBytes);
extern void NetToHostRcapInternetER(RcapInternetER *theRcapInternetER,
				    unsigned char *theBytes);
extern void
  NetToHostRcapEstablishDenied(RcapEstablishDenied *theEstablishDenied,
			       unsigned char *theBytes);
extern void 
  NetToHostRcapCloseRequest(RcapCloseRequestMess *theRcapCloseRequest,
			    unsigned char *theBytes);
extern void
  NetToHostRcapInternetSRR(RcapInternetSRR *theRcapInternetSRR,
			   unsigned char *theBytes);
extern void
  NetToHostRcapReturn(RcapReturn *theRcapReturn,
		      unsigned char *theBytes);
extern void
  NetToHostRcapRegister(RcapRegisterMess *theRcapRegister,
			unsigned char *theBytes);
extern void
  NetToHostRcapUnregister(RcapUnregisterMess *theRcapUnregister,
			  unsigned char *theBytes);
extern void
  NetToHostRcapReceiveRequest(RcapReceiveRequestMess *theRcapReceiveRequest,
			      unsigned char *theBytes);
extern void
  NetToHostRcapReturnParms(RcapReturnParmsMess *theRcapReturnParms,
			   unsigned char *theBytes);
extern void
  NetToHostRcapEstablishReturnMess(RcapEstablishReturnMess *theRcapEstablishReturn,
				   unsigned char *theBytes);

extern void HostToNetRcapHeader(unsigned char *theBytes,
				RcapHeader *theRcapHeader);
extern void HostToNetRmtpHeaderRecord(unsigned char *theBytes,
				      RmtpHeaderRecord *theRmtpHeaderRecord);
extern void HostToNetRcapInternetNSR(unsigned char *theBytes,
				     RcapInternetNSR *theRcapInternetNSR);
extern void HostToNetRcapInternetER(unsigned char *theBytes,
				    RcapInternetER *theRcapInternetER);
extern void HostToNetRcapEstablishDenied(unsigned char *theBytes,
					 RcapEstablishDenied
					 *theRcapEstablishDenied);
extern void HostToNetRcapCloseRequest(unsigned char *theBytes,
				      RcapCloseRequestMess
				      *theRcapCloseRequestMess);
extern void HostToNetRcapInternetSRR(unsigned char *theBytes,
				     RcapInternetSRR *theRcapInternetSRR);
extern void HostToNetRcapReturn(unsigned char *theBytes,
				RcapReturn *theRcapReturn);
extern void HostToNetRcapRegister(unsigned char *theBytes,
				  RcapRegisterMess *theRcapRegister);
extern void HostToNetRcapUnregister(unsigned char *theBytes,
				    RcapUnregisterMess *theRcapUnregister);
extern void HostToNetRcapReceiveRequest(unsigned char *theBytes,
					RcapReceiveRequestMess *theRcapReceiveRequest);
extern void HostToNetRcapReturnParms(unsigned char *theBytes,
				     RcapReturnParmsMess *theRcapReturnParms);
extern void HostToNetRcapEstablishReturnMess(char *theBytes,
					     RcapEstablishReturnMess *theRcapEstablishReturn);

extern void htonRcapEstablishRequest
  (struct establish_request *establish_request_p);
extern void htonRcapEstablishAccept
  (struct establish_accept *establish_accept_p);

/*
 * Sizes of RCAP control message components (fixed parts only).
 */
#define RCAP_HEADER_SIZE			28
#define RMTP_HEADER_RECORD_SIZE			48
#define RCAP_INTERNET_NSR_SIZE			56
#define RCAP_INTERNET_ER_SIZE			68
#define RCAP_INTERNET_SRR_SIZE                  44
#define RCAP_ESTABLISH_DENIED_SIZE              4
#define RCAP_CLOSE_REQUEST_SIZE                 4
#define RCAP_RETURN_SIZE                        4
#define RCAP_REGISTER_SIZE                      12
#define RCAP_UNREGISTER_SIZE                    4
#define RCAP_RECEIVE_REQUEST_SIZE               4
#define RCAP_RETURN_PARMS_SIZE                  4
#define RCAP_ESTABLISH_RETURN_SIZE              8

/*
 * Define external representation of RCAP control message components.
 * The #defines here are offsets into the structures.
 */
#define RCAP_HEADER_RCAP_ID 			0
#define RCAP_HEADER_MESSAGE_TYPE		1
#define RCAP_HEADER_SEQUENCE_NUMBER		2
#define RCAP_HEADER_ORIGINATOR_ADDRESS		4
#define RCAP_HEADER_SOURCE_ADDRESS		8
#define RCAP_HEADER_SOURCE_LCID			12
#define RCAP_HEADER_DESTINATION_ADDRESS		16
#define RCAP_HEADER_DESTINATION_PORT		20
#define RCAP_HEADER_RCAP_MESSAGE_LENGTH		22
#define RCAP_HEADER_HEADER_CHECKSUM		24
#define RCAP_HEADER_MESSAGE_CHECKSUM		26

#define RMTP_HEADER_RECORD_HR_LENGTH		0
#define RMTP_HEADER_RECORD_LEVEL_COUNT		2
#define RMTP_HEADER_RECORD_PROTOCOL		4
#define RMTP_HEADER_RECORD_SUBPROTOCOL		5
#define RMTP_HEADER_RECORD_RTIP_OPTIONS		6
#define RMTP_HEADER_RECORD_XMIN			8
#define RMTP_HEADER_RECORD_XAVE			12
#define RMTP_HEADER_RECORD_I			16
#define RMTP_HEADER_RECORD_SMAX			20
#define RMTP_HEADER_RECORD_D			24
#define RMTP_HEADER_RECORD_J			28
#define RMTP_HEADER_RECORD_Z			32
#define RMTP_HEADER_RECORD_W			36
#define RMTP_HEADER_RECORD_U			40	
#define RMTP_HEADER_RECORD_TYPE			44
#define RMTP_HEADER_RECORD_RMTP_OPTIONS		45
#define RMTP_HEADER_RECORD_USER_CONTROL_LENGTH	46
#define RMTP_HEADER_RECORD_USER_CONTROL		48

#define RCAP_INTERNET_NSR_LEVEL_LENGTH		0
#define RCAP_INTERNET_NSR_ER_COUNT		2
#define RCAP_INTERNET_NSR_NSR_LENGTH		4
#define RCAP_INTERNET_NSR_LCID			6
#define RCAP_INTERNET_NSR_XMIN			8
#define RCAP_INTERNET_NSR_XAVE			12
#define RCAP_INTERNET_NSR_I			16
#define RCAP_INTERNET_NSR_SMAX			20
#define RCAP_INTERNET_NSR_D			24
#define RCAP_INTERNET_NSR_J			28
#define RCAP_INTERNET_NSR_Z			32
#define RCAP_INTERNET_NSR_W			36
#define RCAP_INTERNET_NSR_U			40
#define RCAP_INTERNET_NSR_TYPE			44
#define RCAP_INTERNET_NSR_DCUMUL		48
#define RCAP_INTERNET_NSR_DRELAX		52

#define RCAP_INTERNET_ER_ER_LENGTH		0
#define RCAP_INTERNET_ER_ER_NUMBER		2
#define RCAP_INTERNET_ER_NODE_ADDRESS		4
#define RCAP_INTERNET_ER_D_FIXED_MIN		8
#define RCAP_INTERNET_ER_D_FIXED_MAX		12
#define RCAP_INTERNET_ER_D_VAR_MIN		16
#define RCAP_INTERNET_ER_D_VAR_MAX		20
#define RCAP_INTERNET_ER_Z			24
#define RCAP_INTERNET_ER_W			28
#define RCAP_INTERNET_ER_U			32
#define RCAP_INTERNET_ER_B			36
#define RCAP_INTERNET_ER_BIG_B			40
#define RCAP_INTERNET_ER_F_IP			44
#define RCAP_INTERNET_ER_F_LCID			48
#define RCAP_INTERNET_ER_F_IFN			50
#define RCAP_INTERNET_ER_B_IP			52
#define RCAP_INTERNET_ER_B_LCID			56
#define RCAP_INTERNET_ER_B_IFN			58
#define RCAP_INTERNET_ER_DCUMUL			60
#define RCAP_INTERNET_ER_DRELAX_NUMERATOR       64
#define RCAP_INTERNET_ER_DRELAX_DENOMINATOR     66

#define RCAP_ESTABLISH_DENIED_REASON_CODE       0

#define RCAP_CLOSE_REQUEST_MESS_REASON_CODE     0

#define RCAP_INTERNET_SRR_SRR_LENGTH            0
#define RCAP_INTERNET_SRR_SRR_TYPE              2
#define RCAP_INTERNET_SRR_NODE_ADDRESS          4
#define RCAP_INTERNET_SRR_D_FIXED_MIN           8
#define RCAP_INTERNET_SRR_D_FIXED_MAX           12
#define RCAP_INTERNET_SRR_D_VAR_MIN             16
#define RCAP_INTERNET_SRR_D_VAR_MAX             20
#define RCAP_INTERNET_SRR_Z                     24
#define RCAP_INTERNET_SRR_W                     28
#define RCAP_INTERNET_SRR_U                     32
#define RCAP_INTERNET_SRR_BUFFERS               36
#define RCAP_INTERNET_SRR_STATE                 40

#define RCAP_RETURN_RETURN_CODE                 0

#define RCAP_REGISTER_PORT                      0
#define RCAP_REGISTER_QUEUE_LENGTH              4
#define RCAP_REGISTER_PROTOCOL                  8
#define RCAP_REGISTER_SUBPROTOCOL               10

#define RCAP_UNREGISTER_PORT                    0

#define RCAP_RECEIVE_REQUEST_PORT               0

#define RCAP_RETURN_PARMS_LCID                  0

#define RCAP_ESTABLISH_RETURN_LCID              0
#define RCAP_ESTABLISH_RETURN_RESULT            2
#define RCAP_ESTABLISH_RETURN_REASON_CODE       4
#define RCAP_ESTABLISH_RETURN_USER_CONTROL_LENGTH 6

#endif
