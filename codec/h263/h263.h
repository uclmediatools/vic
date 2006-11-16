/*
 */
#include "../../config.h"
#include "DataTypes.h"
#include "structs.h"

typedef struct h263streamheader { 
	u_int	syncword; 	/* must be 1 */
	u_int	gobnr;	/* must be 0 */
	u_int	tr;		/* temporal ref */
	/* ptype */
	
		u_int	reserved0;	/* must be 1 */
		u_int	reserved1;	/* must be 0 */

		u_int	splitscreen;
		u_int	doccamera;
		u_int	freeze_picture_release;
		u_int	srcformat;
		u_int	picture_coding_type;
		u_int	unrestricted_motion_vector;
		u_int	syntax_based_arithmetic;
		u_int	advanced_prediction;
		u_int	pb_frames;
	u_int	pquant;
	u_int	cpm;

	/* ... optional fields ... not used by this code ... */
	u_int	psbi;
	u_int	trb;
	u_int	dbq;
} h263streamheader;

typedef struct h263rtpheader_A {
	u_int	ftype;	/* 0 for mode A */
	u_int	pbframes;	/* 0 for mode A */
	u_int	sbit;
	u_int	ebit;
	u_int	srcformat;
	u_int	picture_coding_type;	
	u_int	unrestricted_motion_vector;
	u_int	syntax_based_arithmetic;
	u_int	advanced_prediction;
	u_int	reserved;
	u_int	dbq;
	u_int	trb;
	u_int	tr;
} h263rtpheader_A;

typedef struct h263rtpheader_B {
	u_int	ftype;	/* 1 for mode B */
	u_int	pbframes;	/* 0 for mode B */
	u_int	sbit;
	u_int	ebit;
	u_int	srcformat;
	u_int	quant;
	u_int	gobn;
	u_int	mba;
	u_int	reserved;
	/* second 32 bit word */
	u_int	picture_coding_type;
	u_int	unrestricted_motion_vector;
	u_int	syntax_based_arithmetic;
	u_int	advanced_prediction;
	u_int	hmv1;
	u_int	vmv1;
	u_int	hmv2;
	u_int	vmv2;
} h263rtpheader_B;

typedef struct h263_rtp_packet {
	u_char	header[8];
	int	headersize;
	u_char	*data;
	int	datasize;
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
