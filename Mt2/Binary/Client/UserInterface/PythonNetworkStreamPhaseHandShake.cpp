#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "PythonApplication.h"
#include "Packet.h"
#include "../eterpack/EterPackManager.h"
#include "../EterPack/mtrand.h"

using namespace network;

inline void mb_codeAuth(const DWORD sz, const DWORD sy, const DWORD *master, DWORD *dest, int lenght)
{
	unsigned int y = sy, z = sz, sum = 0;
	unsigned int		n = 4 * lenght;

	while (n-- > 0)
	{
		y += ((z << 4 ^ z >> 5) + z) ^ (sum + master[sum & (lenght - 1)]);
		sum += 0x9B737699;
		z += ((y << 4 ^ y >> 5) + y) ^ (sum + master[sum >> 11 & (lenght - 1)]);
	}

	*(dest++) = y;
	*dest = z;
}

inline int mb_encryptAuth(DWORD *dest, const DWORD *src, const DWORD key, int size)
{
	int		i;
	int		resize;

	if (size % 8 != 0)
	{
		resize = size + 8 - (size % 8);
		memset((char *)src + size, 0, resize - size);
	}
	else
		resize = size;

	static DWORD master[] = {
		0x93821DFC,
		0x879163AC,
		0xCA34E2D3,
		0x0AFECD69,
		0x00,
	};

	master[4] = key;

	for (i = 0; i < resize >> 3; i++, dest += 2, src += 2)
		mb_codeAuth(*(src + 1), *src, master, dest, 5);

	return resize;
}

// HandShake ---------------------------------------------------------------------------
void CPythonNetworkStream::HandShakePhase()
{
	InputPacket packet;

	while (Recv(packet))
	{
		switch (packet.get_header<TGCHeader>())
		{
			case TGCHeader::PHASE:
				RecvPhasePacket(packet.get<GCPhasePacket>());
				break;

			case TGCHeader::HANDSHAKE:
				{
					auto pack = packet.get<GCHandshakePacket>();

	#ifdef _USE_LOG_FILE
					Tracenf("HANDSHAKE RECV %u", pack->time());
	#endif

					ELTimer_SetServerMSec(pack->time());

					//DWORD dwBaseServerTime = kPacketHandshake.dwTime+ kPacketHandshake.lDelta;
					//DWORD dwBaseClientTime = ELTimer_GetMSec();

					network::CGOutputPacket<network::CGHandshakePacket> p;
					p->set_handshake(pack->handshake());

					MTRandom rnd = MTRandom(pack->crypt_key());
					DWORD keys[8];
					for (size_t i = 0; i < 8; i++)
						keys[i] = rnd.next();

					mb_encryptAuth(keys, keys, pack->crypt_key(), sizeof(keys));
					p->set_crypt_data(keys, sizeof(keys));

	#ifdef _USE_LOG_FILE
					Tracenf("HANDSHAKE SEND %u", pack->time());
	#endif

					if (!Send(p))
					{
	#ifdef _USE_LOG_FILE
						Tracen(" CAccountConnector::__AuthState_RecvHandshake - SendHandshake Error");
	#endif
					}
				}
				break;

			case TGCHeader::PING:
				RecvPingPacket();
				return;
				break;

	#ifdef _IMPROVED_PACKET_ENCRYPTION_
			case HEADER_GC_KEY_AGREEMENT:
				RecvKeyAgreementPacket();
				return;
				break;

			case HEADER_GC_KEY_AGREEMENT_COMPLETED:
				RecvKeyAgreementCompletedPacket();
				return;
				break;
	#endif

			default:
				GamePhase();
				return;
				break;
		}
	}
}

void CPythonNetworkStream::SetHandShakePhase()
{
	if ("HandShake"!=m_strPhase)
		m_phaseLeaveFunc.Run();

#ifdef _USE_LOG_FILE
	Tracen("");
#endif
#ifdef _USE_LOG_FILE
	Tracen("## Network - Hand Shake Phase ##");
#endif
#ifdef _USE_LOG_FILE
	Tracen("");
#endif

	m_strPhase = "HandShake";

	m_dwChangingPhaseTime = ELTimer_GetMSec();
	m_phaseProcessFunc.Set(this, &CPythonNetworkStream::HandShakePhase);
	m_phaseLeaveFunc.Set(this, &CPythonNetworkStream::__LeaveHandshakePhase);

	SetGameOnline();

	if (__DirectEnterMode_IsSet())
	{
		// None
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnHandShake", Py_BuildValue("()"));
	}
}

bool CPythonNetworkStream::RecvHandshakePacket(std::unique_ptr<GCHandshakePacket> pack)
{
#ifdef _USE_LOG_FILE
	Tracenf("HANDSHAKE RECV %u", pack->time());
#endif

	ELTimer_SetServerMSec(pack->time());

	//DWORD dwBaseServerTime = kPacketHandshake.dwTime+ kPacketHandshake.lDelta;
	//DWORD dwBaseClientTime = ELTimer_GetMSec();

	network::CGOutputPacket<network::CGHandshakePacket> p;
	p->set_handshake(pack->handshake());

	MTRandom rnd = MTRandom(pack->crypt_key());

	DWORD crypt_data[8];
	for (size_t i = 0; i < 8; ++i)
		crypt_data[i] = rnd.next();

	mb_encryptAuth(crypt_data, crypt_data, pack->crypt_key(), sizeof(crypt_data));
	p->set_crypt_data(crypt_data, sizeof(crypt_data));

#ifdef _USE_LOG_FILE
	Tracenf("HANDSHAKE SEND %u", pack->time());
#endif

	if (!Send(p))
	{
#ifdef _USE_LOG_FILE
		Tracen(" CAccountConnector::__AuthState_RecvHandshake - SendHandshake Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::RecvHandshakeOKPacket()
{
	DWORD dwDelta=ELTimer_GetMSec()-m_kServerTimeSync.m_dwChangeClientTime;
	ELTimer_SetServerMSec(m_kServerTimeSync.m_dwChangeServerTime+dwDelta);

#ifdef _USE_LOG_FILE
	Tracenf("HANDSHAKE OK RECV %u %u", m_kServerTimeSync.m_dwChangeServerTime, dwDelta);
#endif

	return true;
}

#ifdef _IMPROVED_PACKET_ENCRYPTION_
bool CPythonNetworkStream::RecvKeyAgreementPacket()
{
	TPacketKeyAgreement packet;
	if (!Recv(&packet))
	{
		return false;
	}

#ifdef _USE_LOG_FILE
	Tracenf("KEY_AGREEMENT RECV %u", packet.wDataLength);
#endif

	TPacketKeyAgreement packetToSend;
	size_t dataLength = TPacketKeyAgreement::MAX_DATA_LEN;
	size_t agreedLength = Prepare(packetToSend.data, &dataLength);
	if (agreedLength == 0)
	{
		// 초기화 실패
		Disconnect();
		return false;
	}
	assert(dataLength <= TPacketKeyAgreement::MAX_DATA_LEN);

	if (Activate(packet.wAgreedLength, packet.data, packet.wDataLength))
	{
		// Key agreement 성공, 응답 전송
		packetToSend.bHeader = HEADER_CG_KEY_AGREEMENT;
		packetToSend.wAgreedLength = (WORD)agreedLength;
		packetToSend.wDataLength = (WORD)dataLength;

		if (!Send(packetToSend))
		{
			assert(!"Failed Sending KeyAgreement");
			return false;
		}
#ifdef _USE_LOG_FILE
		Tracenf("KEY_AGREEMENT SEND %u", packetToSend.wDataLength);
#endif
	}
	else
	{
		// 키 협상 실패
		Disconnect();
		return false;
	}
	return true;
}

bool CPythonNetworkStream::RecvKeyAgreementCompletedPacket()
{
	TPacketKeyAgreementCompleted packet;
	if (!Recv(&packet))
	{
		return false;
	}

#ifdef _USE_LOG_FILE
	Tracenf("KEY_AGREEMENT_COMPLETED RECV");
#endif

	ActivateCipher();

	return true;
}
#endif // _IMPROVED_PACKET_ENCRYPTION_

