/* Copyright 1998 Marcus Meissner
 */

/* H263Encoder Klasse */
class H263Encoder : public TransmitterModule {
 public:
	H263Encoder();
	~H263Encoder();
	virtual int consume(const VideoFrame*);
	virtual int command(int argc,const char *const *argv);
 protected:
	Picture	oldpic_;	/* vorheriges dekodiertes Bild */
	Picture	oldorigpic_;	/* vorheriges Bild */
	Picture	decpict_;	/* jetziges dekodiertes Bild */
	Bitstr	bitstr_;	/* Bitstream */
	MVField	mvfield_;	/* MotionVectorfield */
	int	*mbind_;	/* MacroBlock startindizes */
	int	*mbquant_;	/* MacroBlock Quantisierer */
	u_int	maxrate_;	/* maximale bitzahl per Frame */
	u_int	lastw_,lasth_;	/* letzte Groesse des Frames */
	u_int	nexttype_;	/* naechster Frametyp */
	int	maycheck_;	/* Anzahl an Frames seit letzten Zeitcheck */
	int	codingtime_;	/* derzeitige Codingtime */
	int	q_;		/* derzeitiger Quantisierer */
};

/*  H263EncoderMatcher, kreiert H263Encoder auf Anforderung des Tcl Codes
 */
static class H263EncoderMatcher : public Matcher {
public:
	H263EncoderMatcher() : Matcher("module") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "h263") == 0)
			return (new H263Encoder);
		return (0);
	}
} encoder_matcher_h263;
