class H263Decoder : public Decoder {
public:
	H263Decoder();
	~H263Decoder();
	int colorhist(u_int* hist) const;
	virtual void recv(const rtphdr* rh, const u_char* bp, int cc);
	virtual void info(char* wrk) const;

protected:
	int reassemble_gobs(u_char **newbp,int *newcc);
	virtual void redraw();

	/*
	 * Reassembly buffer 'slots' for resequencing & reassembling
	 * gobs split across packet boundaries.
	 */
#define H263_SLOTS	64
#define H263_SLOTMASK	(H263_SLOTS - 1)
	int	h263streamsize_;	/* max streamsize */
	u_char	*h263stream_;		/* h263stream */

	struct slot {
		u_short 	seqno;
		u_int		rtflags;
		h263rtpheader_B	h263rh;
		u_int		cc;
		u_char* 	bp;
	} slot_[H263_SLOTS];

	u_char	*frame_,*backframe_;
	u_int	srcformat_;
	u_int	inw_,inh_;
	u_int	lastknowngood_;

	H263Global *h263decoder;
};

static class H263DecoderMatcher : public Matcher {
public:
	H263DecoderMatcher() : Matcher("decoder") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "h263") == 0)
			return (new H263Decoder());
		else
			return (0);
	}
} dm_h263;
