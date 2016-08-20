#include "gdbpacket.hxx"

#include <QRegExp>

QPair<QByteArray, QByteArray> GdbPacket::extract_packet(const QByteArray & gdb_bytestream_data)
{
QRegExp	rx("\\$(.+)#..");
QPair<QByteArray, QByteArray> res;
QByteArray unzeroed_gdb_data = gdb_bytestream_data;
int x;
	/* literal zero bytes in byte arrays do not work with qt regular expressions,
		 * so do this replacement hack as a workaround */
	unzeroed_gdb_data.replace(0, QString("s"));
	res.first = QByteArray();
	res.second = gdb_bytestream_data;

	if ((x = rx.indexIn(unzeroed_gdb_data)) != -1)
	{
		res.first = gdb_bytestream_data.mid(x, rx.cap(1).length());
		res.second= gdb_bytestream_data.right(gdb_bytestream_data.length() - x - rx.matchedLength());
	}
	return res;
}
