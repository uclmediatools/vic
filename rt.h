/* Revision: %W%	%G%	Hui Zhang */

#ifndef _SYS_RT_INCLUDED
#define _SYS_RT_INCLUDED

struct ChannelID {
        struct in_addr ci_srcIP;   /* source IP address */
        u_short        ci_srcLCID;   /* source channel ID */
};

struct TrafficSpec{
        u_long ts_xmin;
        u_long ts_xave;
        u_long ts_I;
        u_long ts_smax;
};



struct PerfSpec{
    u_long ps_d;           /* local delay bound (variable delay in node) */
    u_long ps_adn;         /* accumul total delay */
    u_long ps_j;           /* local jitter bound */
    u_long ps_z;           /* statistical delay probability */
    u_long ps_w;           /* statistical drop probability */
    u_long ps_u;           /* statistical jitter probability */
    u_char ps_type;        /* channel type */
};

struct BufferSpec {
    u_long bs_bufferSize;
    u_long bs_bufferNumber; 
};

struct RtipSpec {
    u_short            rs_lcid;          /* lcid of this connection */
    u_short            rs_downLcid;      /* downstream lcid */
    u_long             rs_jitterControl; /* 1 if jitter control, 0 otherwise */
#define DESTINATION_LCID 0              
    struct sockaddr_in rs_downName;      /* downstream IP address */
    struct TrafficSpec rs_trafficSpec;   /* traffic specification */
    struct PerfSpec    rs_perfSpec;      /* performance specification */
    struct BufferSpec  rs_bufferSpec;    /* buffer specification */
};


struct TimeMeasure {
    int    tm_packetSeq;
    u_long tm_curTime;
    u_long tm_curTimeDif;
    u_long tm_idealTime;
    u_long tm_expArr;
    u_long tm_expArrDif;
};

#define FZA_TRANS_MAX 200              /* used in measurement of FDDI */
#define TIME_MEASURE_MAX        500

#define IPPROTO_RTIP 5
#define IPPROTO_RMTP 7

/* used for "setsockopt" or "getsockopt" */

#define RTIP_SPEC    1
#define RTIP_ASSOC   2
#define RTIP_RELEASE 3
#define RTIP_DESTINATION_ASSOC 4
#define RTIP_MEASURE_RESET 5
#define RTIP_MEASURE_ON    6
#define RTIP_MEASURE_OFF   7
#define RTIP_MEASURE_DUMP  8
#define RTIP_MEASURE_INDEX 9

/* channel types */ 

#define CT_DETERMINISTIC 0
#define CT_STATISTICAL 1
#define CT_NORMAL      2

/* RMTP options  */

#define RMTP_CHECKSUM 0x1
#define RMTP_TIMESTAMP 0x2


#endif /* _SYS_RT_INCLUDED */











