#define HUFF_DECODE_PVH_RC(nbb, bb, bs, result) \
	HUFF_DECODE(htd_pvh_rc, 5, nbb, bb, bs, result)

extern const unsigned short htd_pvh_rc[];
extern const struct huffent hte_pvh_rc[];
extern const unsigned char hts_pvh_rc[];
#define HUFF_DECODE_PVH_TC(nbb, bb, bs, result) \
	HUFF_DECODE(htd_pvh_tc, 14, nbb, bb, bs, result)

extern const unsigned short htd_pvh_tc[];
extern const struct huffent hte_pvh_tc[];
extern const unsigned char hts_pvh_tc[];
