#ifndef RCAP_CONSTS_H
#define RCAP_CONSTS_H
/*
 * $Header$
 * consts.h
 * Anindo Banerjea (banerjea@tenet)
 *
 * This file contains constants like
 * The RCAP message types
 * Size of Records of Rcap Messages
 * Protocol types
 * Reasons for failure, error or SUCCESS
 * FSM states
 *
 * $Log$
 * Revision 1.1  1998/02/18 18:03:59  ucacsva
 * Initial revision
 *
 * Revision 1.5  1993/02/02  19:14:34  banerjea
 * Added a constant MIN_BUFFER_RESERVATION which lower bounds the
 * number of buffers which are reserved on any channel. this makes sure
 * that rtip gets enough buffers to work correctly.
 *
 * Revision 1.4  1992/08/17  17:45:01  bmah
 * Added copyright stuff to keep the lawyers happy.
 *
 * Revision 1.3  1992/07/12  17:49:36  bmah
 * First RTIP-compatible daemon.  Some error codes added and fine-tuned.
 *
 * Copyright (c) 1992 Regents of the University of California
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for educational, research, and non-profit purposes and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of the University of California not be used in advertising
 * or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Permission to incorporate this
 * software into commercial products can be obtained from the Campus
 * Software Office, 295 Evans Hall, University of California, Berkeley,
 * Ca., 94720 provided only that the the requestor give the University
 * of California a free licence to any derived software for educational
 * and research purposes.  The University of California makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

/* Some magic numbers */
#define WORST_CASE_UTIL  1.5 /* Used in the Buffer Calc and in det_test*/
#define MAX_DET_UTIL 0.5     /* Used in the det_test */
#define MAX_UTIL 1.0         /* Used in delay bound test */
#define MAX_NODE_UTIL 0.8    /* Used for the processor utilization cap */
#define MIN_BUFFER_RESERVATION 2 /* Used in buffer test to ensure atleast 2
				  * buffers are allocated in RTIP */

/* Identifying protocol constant for RCAP*/
#define RCAP_ID 101

/* 
 * RCAP message types
 */
#define RCAP_ESTABLISH_REQUEST 102
#define RCAP_ESTABLISH_DENIED  103
#define RCAP_ESTABLISH_ACCEPT  104
#define RCAP_CLOSE_REQUEST_FORWARD 105
#define RCAP_CLOSE_REQUEST_REVERSE 106
#define RCAP_STATUS_REQUEST 109
#define RCAP_STATUS_REPORT 110

/* 
 * Size of records of RCAP messages in bytes
 */
#define SIZE_OF_RCAP_HEADER (7*4)
#define MIN_SIZE_OF_RMTP_HEADER_RECORD (12*4)
#define MIN_SIZE_OF_CMTP_HEADER_RECORD (11*4)
#define SIZE_OF_INTERNET_NSR (14*4)
#define SIZE_OF_INTERNET_ER (17*4)
#define SIZE_OF_INTERNET_SRR (11*4)
#define SIZE_OF_ESTABLISH_DENIED_BODY (1*4)
#define SIZE_OF_CLOSE_REQUEST_BODY (1*4)

/*
 * Protocol types
 */
#define RTIPPROTO_RMTP 111
#define RTIPPROTO_CMTP 112

/*
 * Reasons for failure, error or SUCCESS
 */
#define RCAP_SUCCESS 1
#define RCAP_FAIL_NO_AVAILABLE_VCIS 113
#define RCAP_REASON_USER 115
#define RCAP_REASON_NETFAIL 116
#define RCAP_FAIL_DET        117
#define RCAP_FAIL_STAT       118
#define RCAP_FAIL_DELAY      119
#define RCAP_FAIL_BUF        120
#define RCAP_ERROR_UNKNOWN_GLOBAL_ID	121
#define RCAP_ERROR_DUPLICATE_GLOBAL_ID	122
#define RCAP_ERROR_RELAX_FAILED	124

#define RCAP_ERROR_UNKNOWN_LCID 125
#define RCAP_ERROR_DUPLICATE_LCID 126
#define RCAP_FAIL_NODE_SATURATION  127
#define RCAP_ERROR_ROUTING_TABLE 128

#define RCAP_ERROR_NO_PORT 160
#define RCAP_ERROR_PORT_IN_USE 161
#define RCAP_ERROR_UNREGISTERED_PORT 162
#define RCAP_ERROR_NO_MEMORY 163
#define RCAP_ERROR_PROTOCOL 164
#define RCAP_ERROR_RECEIVER 165
#define RCAP_ERROR_TRANSPORT 166
#define RCAP_ERROR_RELEASE_FAILED 167
#define RCAP_ERROR_INIT 168

/* Finite State Machine States */
#define RCAP_FSM_INITIAL_STATE 130
#define RCAP_FSM_TENTATIVE_STATE 131
#define RCAP_FSM_PERMANENT_STATE 132
#define RCAP_FSM_ERROR_STATE 133

/* SRR types */
#define RCAP_SRR_TYPE_INET 140
#define RCAP_SRR_TYPE_ATM 141

/* Network and subnetwork Levels */
#define RCAP_FDDI 150
#define RCAP_XUNET 151

/* Local communication constants */

#define RCAP_LOCAL_RETURN 200
#define RCAP_LOCAL_REGISTER 201
#define RCAP_LOCAL_UNREGISTER 202
#define RCAP_LOCAL_RECEIVE_REQUEST 203
#define RCAP_LOCAL_RETURN_PARMS 204
#define RCAP_LOCAL_ESTABLISH_RETURN 205

#endif
