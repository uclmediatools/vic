#ifndef mash_pkttbl_h
#define mash_pkttbl_h

//#include "tclcl.h"
#include "pktbuf.h"
#include "config.h"

class PacketTable : public TclObject {
public:
	PacketTable();
	~PacketTable();
	virtual int command(int argc, const char*const* argv);
	int addpkt(const u_char* p, int len);
	u_char* getpkt(int pktid);
	void delpkt(int pktid);
protected:
	virtual int get_field(const u_char* pkt, const char* name, 
			      u_char** fp) = 0;
private:
	void grow();
	pktbuf** table_;
	int size_;
	BufferPool* pool_;
	int last_;
};

#endif
