/********************************************************************
	created:	2006/08/10
	filename: 	CommonDefine.h
	author:		�
                http://www.cppblog.com/converse/

	purpose:	����ͻ��˺ͷ������˶���Ҫ֪����һЩ�궨��
*********************************************************************/

#ifndef __COMMON_DEFINE_H__
#define __COMMON_DEFINE_H__

#include <Windows.h>
#include "PeerList.h"

#pragma comment(lib, "WS2_32")	// ���ӵ�WS2_32.lib

#define MAX_TRY_NUMBER		10

#define SERVER_PORT			4096
#define MAX_PACKET_SIZE		1024

// ������Ϣ��ʶ��
#define INVALID_MSG			-1
#define MSG_USERLOGIN		1						// �û���½
#define MSG_USERLOGACK		2						// ����ȷ���û���½����Ϣ
#define MSG_GETUSERLIST		3						// ��ȡ�û��б�
#define MSG_USERLISTCMP		4						// �û��б������
#define MSG_P2PMSG			5						// ����P2P��Ϣ
#define MSG_P2PCONNECT		6						// ���û���������һ���û��������ʹ���Ϣ
#define MSG_P2PMSGACK		7
#define MSG_P2PCONNECTACK	8
#define MSG_USERLOGOUT		9						// ֪ͨserver�û��˳�
#define MSG_USERACTIVEQUERY	10						// ��ѯ�û��Ƿ񻹴���

class MSGDef										// ������Ϣ�Ľṹ��
{
public:
#pragma pack(1)										// ʹ�ṹ������ݰ���1�ֽ�������,ʡ�ռ�

	// ��Ϣͷ
	struct TMSG_HEADER
	{
		char    cMsgID;								// ��Ϣ��ʶ

		TMSG_HEADER(char MsgID = INVALID_MSG)
			: cMsgID(MsgID)
		{
		}
	};

	// �û���½
	struct TMSG_USERLOGIN
		: TMSG_HEADER
	{
		Peer_Info PeerInfo;

		TMSG_USERLOGIN(const Peer_Info &rPeerinfo)
			: TMSG_HEADER(MSG_USERLOGIN)
		{
			PeerInfo = rPeerinfo;
		}		
	};

	// ����ȷ���û���½����Ϣ
	struct TMSG_USERLOGACK
		: TMSG_HEADER
	{
		Peer_Info PeerInfo;

		TMSG_USERLOGACK(const Peer_Info& rPeerInfo)
			: TMSG_HEADER(MSG_USERLOGACK)
		{
			PeerInfo = rPeerInfo;
		}
	};

	// ��ȡ�û��б�
	struct TMSG_GETUSERLIST
		: TMSG_HEADER
	{
		Peer_Info PeerInfo;

		TMSG_GETUSERLIST(const Peer_Info& rPeerInfo)
			: TMSG_HEADER(MSG_GETUSERLIST)
		{
			PeerInfo = rPeerInfo;
		}
	};

	// �����û��б����
	struct TMSG_USERLISTCMP
		: TMSG_HEADER
	{
		TMSG_USERLISTCMP()
			: TMSG_HEADER(MSG_USERLISTCMP)
		{

		}
	};

	// һ��client����һ��client������Ϣ
	struct TMSG_P2PMSG
		: TMSG_HEADER
	{
		Peer_Info	PeerInfo;
		char		szMsg[MAX_PACKET_SIZE - sizeof(TMSG_HEADER) - sizeof(Peer_Info)];

		TMSG_P2PMSG(const Peer_Info& rPeerInfo)
			: TMSG_HEADER(MSG_P2PMSG)
		{
			PeerInfo = rPeerInfo;
			memset(szMsg, 0, MAX_PACKET_SIZE - sizeof(TMSG_HEADER) - sizeof(PeerInfo));
		}
	};

	// ���û���������һ���û��������ʹ���Ϣ
	struct TMSG_P2PCONNECT
		: TMSG_HEADER
	{
		Peer_Info	PeerInfo;
		char		szUserName[MAX_USERNAME];

		TMSG_P2PCONNECT(const Peer_Info& rPeerInfo)
			: TMSG_HEADER(MSG_P2PCONNECT)
		{
			PeerInfo = rPeerInfo;
		}
	};

	// client�յ���һ��client���͵���Ϣ֮���ȷ��
	struct TMSG_P2PMSGACK
		: TMSG_HEADER
	{
		Peer_Info PeerInfo;

		TMSG_P2PMSGACK(const Peer_Info& rPeerInfo)
			: TMSG_HEADER(MSG_P2PMSGACK)
		{
			PeerInfo = rPeerInfo;
		}
	};

	// ���յ��ڵ�Ĵ���Ϣ����������������P2Pͨ�ŵ�ַ
	struct TMSG_P2PCONNECTACK
		: TMSG_HEADER
	{
		Peer_Info PeerInfo;

		TMSG_P2PCONNECTACK(const Peer_Info& rPeerInfo)
			: TMSG_HEADER(MSG_P2PCONNECTACK)
		{
			PeerInfo = rPeerInfo;
		}
	};

	// ֪ͨserver�û��˳�
	struct TMSG_USERLOGOUT
		: TMSG_HEADER
	{
		Peer_Info PeerInfo;
		TMSG_USERLOGOUT(const Peer_Info& rPeerInfo)
			: TMSG_HEADER(MSG_USERLOGOUT)
		{
			PeerInfo = rPeerInfo;
		}
	};

	// ��ѯ�û��Ƿ񻹴���
	struct TMSG_USERACTIVEQUERY
		: TMSG_HEADER
	{
		Peer_Info PeerInfo;

		TMSG_USERACTIVEQUERY(const Peer_Info& rPeerInfo = Peer_Info())
			: TMSG_HEADER(MSG_USERACTIVEQUERY)
		{
			PeerInfo = rPeerInfo;
		}
	};

#pragma pack()
};

#endif // __COMMON_DEFINE_H__