#include "gdbpacket.hxx"

#include <stdint.h>
#include <QRegExp>

QPair<QByteArray, QByteArray> GdbPacket::extract_packet(const QByteArray & gdb_bytestream_data)
{
QRegExp	rx("\\$(.+)#..");
QPair<QByteArray, QByteArray> res;
QByteArray unzeroed_gdb_data = gdb_bytestream_data;
int x;
	/* literal zero bytes in byte arrays do not work with qt regular expressions,
	 * so do this replacement hack as a workaround */
	rx.setMinimal(true);
	unzeroed_gdb_data.replace(0, QString("s"));
	res.first = QByteArray();
	res.second = gdb_bytestream_data;

	if ((x = rx.indexIn(unzeroed_gdb_data)) != -1)
	{
		res.first = gdb_bytestream_data.mid(x /* screw the dollar */ + 1, rx.cap(1).length());
		res.second= gdb_bytestream_data.right(gdb_bytestream_data.length() - x - rx.matchedLength());
	}
	return res;
}

QByteArray GdbPacket::make_complete_packet(const QByteArray & packet_data)
{
uint8_t cksum;
int i;
QByteArray res("$");
	for (cksum = i = 0; i < packet_data.length(); cksum += packet_data.at(i), res += packet_data.at(i ++));
	res += '#';
	res += QString("%1").arg(cksum, 2, 16, QChar('0'));
	return res;
}
