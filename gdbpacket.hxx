#ifndef GDBPACKET_HXX
#define GDBPACKET_HXX

#include <QString>
#include <QPair>

class GdbPacket
{
public:
	/* extracts a gdb packet out of a gdb data bytestream received so far */
	static QPair<QByteArray /* extracted packet */, QByteArray /* packet after extraction */>
		extract_packet(const QByteArray & gdb_bytestream_data);
};

#endif // GDBPACKET_HXX
