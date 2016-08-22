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
	/* creates a valid gdb packet by prepending a start marker and appending an end
	 * marker and a checksum to an arbitrary gdb packet data array */
	static QByteArray make_complete_packet(const QByteArray & packet_data);
	static QByteArray format_monitor_packet(const QByteArray & monitor_data)
		{ return QByteArray("qRcmd,") + monitor_data.toHex(); }
};

#endif // GDBPACKET_HXX
