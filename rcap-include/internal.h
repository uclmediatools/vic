/* 
 * internal.h
 * Anindo Banerjea (banerjea@tenet)
 */
#ifndef INTERNAL_H
#define INTERNAL_H

/*
 * Structures which make up the internal representation of
 * the RCAP message
 */

typedef struct RcapHeader 
{
	unsigned char rcap_id;
	unsigned char message_type;
	unsigned short sequence_number;
	unsigned int originator_address;
	unsigned int source_address;
	unsigned short source_lcid;
	unsigned int destination_address;
	unsigned short destination_port;
	unsigned short rcap_message_length; 
				      /* Warning: Dependency with
				       * RcapInternalMessage.message_length
				       * And with level_length on the
				       * last NSR
				       */
	unsigned short header_checksum;
	unsigned short message_checksum;
} RcapHeader;

typedef struct RmtpHeaderRecord
{
	unsigned short hr_length;
	unsigned short level_count;
	unsigned char protocol;
	unsigned char subprotocol;
	unsigned char rtip_options;
	unsigned int xmin;
	unsigned int xave;
	unsigned int I;
	unsigned int smax;
	unsigned int D;
	unsigned int J;
	unsigned int Z;
	unsigned int W;
	unsigned int U;
	unsigned char type;
	unsigned char rmtp_options;
	unsigned short user_control_length;
	/* Variable length user field is here in the
	 * actual Rcap message but not in this structure
	 */
}RmtpHeaderRecord;

typedef struct RcapInternetNSR
{
	unsigned short level_length; /* length field */
	unsigned short er_count;
	unsigned short nsr_length;
	unsigned short lcid;
	unsigned int xmin;
	unsigned int xave;
	unsigned int I;
	unsigned int smax;
	unsigned int D;
	unsigned int J;
	unsigned int Z;
	unsigned int W;
	unsigned int U;
	unsigned char type;
	unsigned int Dcumul;
	unsigned int Drelax;
}RcapInternetNSR;

typedef struct RcapInternetER
{
	unsigned short er_length;  /* length field */
	unsigned short er_number;
	unsigned int node_address;
/*	unsigned int dl; -|
 *	unsigned int dn;   > The old values now we carry
 *	unsigned int j;  _|  the folowing four
 *
 */
	unsigned int d_fixed_min; /* Segment delay values */
	unsigned int d_fixed_max;
	unsigned int d_var_min;
	unsigned int d_var_max;
	unsigned int z;
	unsigned int w;
	unsigned int u;
	unsigned int B;
	unsigned int b;
	unsigned int f_ip;
	unsigned short f_lcid;
	unsigned short f_ifn;
	unsigned int b_ip;
	unsigned short b_lcid;
	unsigned short b_ifn;
	unsigned int dcumul;
	unsigned short d_relax_numerator;
	unsigned short d_relax_denominator;
} RcapInternetER;

typedef struct RcapInternetSRR
{

	unsigned short srr_length;
	unsigned short srr_type;
	unsigned int node_address;
	unsigned int d_fixed_min; /* node delay values */
	unsigned int d_fixed_max;
	unsigned int d_var_min;
	unsigned int d_var_max;
/*	unsigned int d; > Old values
 *	unsigned int j; > Old values
 */
	unsigned int z;
	unsigned int w;
	unsigned int u;
	unsigned int buffers;
	unsigned short state;

} RcapInternetSRR;

typedef struct RcapEstablishDenied
{
	unsigned short reason_code;
} RcapEstablishDenied;

typedef struct RcapCloseRequestMess
{
	unsigned short reason_code;
} RcapCloseRequestMess;


/*
 * Internal representations of pseudo-messages (local messages between
 * RCAP library and local RCAP daemon only).
 *
 * Note:  Reserved fields not present in these structures; recommend
 * they be removed from previously-defined structures as well.  bmah's
 * nethost module eliminates the need for these fields.
 */
typedef struct RcapReturn {
	unsigned short return_code;
} RcapReturn;

typedef struct RcapRegisterMess {
	unsigned short port;
	unsigned long queue_length;
	unsigned short protocol;
	unsigned short subprotocol;
} RcapRegisterMess;

typedef struct RcapUnregisterMess {
	unsigned short port;
} RcapUnregisterMess;

typedef struct RcapReceiveRequestMess {
	unsigned short port;
} RcapReceiveRequestMess;

typedef struct RcapReturnParmsMess {
	unsigned short lcid;
} RcapReturnParmsMess;

typedef struct RcapEstablishReturnMess {
	unsigned short lcid;
	unsigned short result;
	unsigned short reason_code;
	unsigned short user_control_length;
} RcapEstablishReturnMess;

#endif INTERNAL_H
