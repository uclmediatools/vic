/*
 */
#include "../config.h"
#include "DataTypes.h"
#include "structs.h"

typedef struct h263streamheader { 
	u_int	syncword:17; 	/* must be 1 */
	u_int	gobnr:5;	/* must be 0 */
	u_int	tr:8;		/* temporal ref */
	/* ptype */
	
		u_int	reserved0:1;	/* must be 1 */
		u_int	reserved1:1;	/* must be 0 */

		u_int	splitscreen:1;
		u_int	doccamera:1;
		u_int	freeze_picture_release:1;
		u_int	srcformat:3;
		u_int	picture_coding_type:1;
		u_int	unrestricted_motion_vector:1;
		u_int	syntax_based_arithmetic:1;
		u_int	advanced_prediction:1;
		u_int	pb_frames:1;
	u_int	pquant:5;
	u_int	cpm:1;

	/* ... optional fields ... not used by this code ... */
	u_int	psbi:2;
	u_int	trb:3;
	u_int	dbq:2;
} h263streamheader;

typedef struct h263rtpheader_A {
	u_int	ftype:1;	/* 0 for mode A */
	u_int	pbframes:1;	/* 0 for mode A */
	u_int	sbit:3;
	u_int	ebit:3;
	u_int	srcformat:3;
	u_int	picture_coding_type:1;	
	u_int	unrestricted_motion_vector:1;
	u_int	syntax_based_arithmetic:1;
	u_int	advanced_prediction:1;
	u_int	reserved:4;
	u_int	dbq:2;
	u_int	trb:3;
	u_int	tr:8;
} h263rtpheader_A;

typedef struct h263rtpheader_B {
	u_int	ftype:1;	/* 1 for mode B */
	u_int	pbframes:1;	/* 0 for mode B */
	u_int	sbit:3;
	u_int	ebit:3;
	u_int	srcformat:3;
	u_int	quant:5;
	u_int	gobn:5;
	u_int	mba:9;
	u_int	reserved:2;
	/* second 32 bit word */
	u_int	picture_coding_type:1;
	u_int	unrestricted_motion_vector:1;
	u_int	syntax_based_arithmetic:1;
	u_int	advanced_prediction:1;
	u_int	hmv1:7;
	u_int	vmv1:7;
	u_int	hmv2:7;
	u_int	vmv2:7;
} h263rtpheader_B;

typedef struct h263_rtp_packet {
	union {
		h263rtpheader_A h263a;
		h263rtpheader_B h263b;
	} header;
	int	headersize; /* could be dedicated from the h263a header */
	u_char	*data;
	int	 datasize;
} h263_rtp_packet;


#ifdef __cplusplus
extern "C" {
#endif
extern int split_and_send_h263stream(
        u_char *bs,             /* bitstream [in] */
        u_int bitcount,         /* bitstream length [in] */
        u_int gobs_per_frame,   /* gobs per frame [in] */
        u_int mbs_per_gob,      /* mbs per gob [in] */
        int *mbind,             /* macroblock indices [in] */
        int *mbquant,           /* quantifiers [in] */
        MVField mvfield,        /* motion vectors [in] */
        u_int ts_,              /* timestamp [in] */
        int headersize,         /* headerlength [in] */
        h263_rtp_packet  **packets/* rtp packets [out] */
);
#ifdef __cplusplus
}
#endif
