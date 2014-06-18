/********************************************************************
	created:	2006/08/07
	filename: 	P2PClient.h
	author:		�
                http://www.cppblog.com/converse/

	purpose:	
*********************************************************************/

#ifndef __P2P_CLIENT_H__
#define __P2P_CLIENT_H__

#include <WinSock2.h>
#include "../CommonDefine.h"

class P2PClient
{
public:
	P2PClient();
	~P2PClient();

	bool Login(char *pszUserName, char *pszServerIP);
	bool Logout();
	bool GetUserList();
	bool SendText(char *pszUserName, char* pszText, int nTextLen);
	void DisplayUserList();

private:
	bool Initialize();
	static DWORD WINAPI RecvThreadProc(LPVOID lpParam);

	// ���������Ϣ�ĺ���
	bool ProcUserLogAckMsg(MSGDef::TMSG_HEADER *pMsgHeader);
	bool ProcGetUserList(MSGDef::TMSG_HEADER *pMsgHeader);
	bool ProcUserListCmpMsg();
	bool ProcP2PMsg(MSGDef::TMSG_HEADER *pMsgHeader, const sockaddr_in& sockAddr);
	bool ProcP2PConnectMsg(MSGDef::TMSG_HEADER *pMsgHeader, const sockaddr_in& sockAddr);
	bool ProcP2PConnectAckMsg(MSGDef::TMSG_HEADER *pMsgHeader, const sockaddr_in& sockAddr);
	bool ProcUserActiveQueryMsg(MSGDef::TMSG_HEADER *pMsgHeader, const sockaddr_in& sockAddr);

private:
	SOCKET				m_sSocket;				// �׽���
	Peer_Info			m_PeerInfo;				// ������Ϣ
	PeerList			m_PeerList;				// �뱾�������Ľڵ�����
	HANDLE				m_hThread;				// �߳̾��
	DWORD				m_dwServerIP;			// server IP��ַ
	WSAOVERLAPPED		m_ol;					// ���ڵȴ������¼����ص��ṹ
	CRITICAL_SECTION	m_PeerListLock;			// ���ڶ�ȡm_PeerList���ٽ�������
	bool				m_bExitThread;			// �Ƿ��˳��߳�
	bool				m_bLogin;				// �Ƿ��Ѿ���½��������
	bool				m_bUserListCmp;			// �Ƿ�õ����û��б�
	bool				m_bMessageACK;			// �Ƿ���յ���ϢӦ��
};

#endif // __P2P_CLIENT_H__
