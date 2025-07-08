#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "PythonApplication.h"
#include "Packet.h"

using namespace network;

void CPythonNetworkStream::OffLinePhase()
{
	InputPacket packet;

	while (Recv(packet))
	{
		switch (packet.get_header<TGCHeader>())
		{
			case TGCHeader::SET_VERIFY_KEY:
				SetPacketVerifyKey(packet.get<GCSetVerifyKeyPacket>()->verify_key());
				break;

			case TGCHeader::PHASE:
				RecvPhasePacket(packet.get<GCPhasePacket>());
				return;

			default:
				RecvErrorPacket(packet);
				break;
		}
	}
}