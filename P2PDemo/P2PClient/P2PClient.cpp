/********************************************************************
	created:	2006/08/07
	filename: 	P2PClient.cpp
	author:		�
                http://www.cppblog.com/converse/

	purpose:	
*********************************************************************/

#include <assert.h>
#include <stdio.h>
#include "P2PClient.h"

P2PClient::P2PClient()
	: m_sSocket(INVALID_SOCKET)
	, m_bExitThread(false)
	, m_bLogin(false)
	, m_bUserListCmp(false)
	, m_bMessageACK(false)
{
	Initialize();
}

P2PClient::~P2PClient()
{
	Logout();

	// ֪ͨ�����߳��˳�
	if(m_hThread != NULL)
	{
		m_bExitThread = TRUE;
		::WSASetEvent(m_ol.hEvent);
		::WaitForSingleObject(m_hThread, 300);
		::CloseHandle(m_hThread);
	}

	if(INVALID_SOCKET != m_sSocket)
	{
		::closesocket(m_sSocket);
	}

	::WSACloseEvent(m_ol.hEvent);

	::DeleteCriticalSection(&m_PeerListLock);
	::WSACleanup();
}

bool P2PClient::Initialize()
{
	if (INVALID_SOCKET != m_sSocket)
	{
		printf("Error: Socket Already Been Initialized!\n");
		return false;
	}

	// ��ʼ��WS2_32.dll
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	if(::WSAStartup(sockVersion, &wsaData) != 0)
	{
		printf("Error: Initialize WS2_32.dll Failed!\n");
		exit(-1);
	}

	// ��ʼ���ٽ���
	::InitializeCriticalSection(&m_PeerListLock);

	// 
	memset(&m_ol, 0, sizeof(m_ol));
	m_ol.hEvent = ::WSACreateEvent();

	// ����һ���ص�I/O socket
	m_sSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_sSocket)
	{
		printf("Error: Initialize Socket Failed!\n");
		return false;
	}

	// ��socket
	// ע����������ַ��INADDR_ANY,��ʾ��ϵͳ
	// ����������������һ���˿ں�
	sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(0);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	int nAddrLen = sizeof(addr);
	if (SOCKET_ERROR == ::bind(m_sSocket, (sockaddr*)(&addr), nAddrLen))
	{
		printf("Error: Bind Socket Failed!\n");
		::closesocket(m_sSocket);
		return false;
	}

	// �õ���Ч�Ķ˿ں�
	::getsockname(m_sSocket, (sockaddr*)(&addr), &nAddrLen);
	unsigned short usPort = ntohs(addr.sin_port);

	// �õ�������IP��ַ
	char szHost[256];
	::gethostname(szHost, 256);
	hostent* pHost = ::gethostbyname(szHost);

	// �õ�����������������IP��ַ�Ͷ˿ں�,��Щ����˽�е�ַ/�˿ں�
	char *pIP;
	for (int i = 0; i < MAX_ADDNUM - 1; ++i)
	{
		if (NULL == (pIP = pHost->h_addr_list[i]))
		{
			break;
		}
		
		memcpy(&(m_PeerInfo.IPAddr[i].dwIP), pIP, pHost->h_length);
		m_PeerInfo.IPAddr[i].usPort = usPort;
		++m_PeerInfo.nAddrNum;
	}

	// �������ս���
	if (NULL == (::CreateThread(NULL, 0, RecvThreadProc, this, 0, NULL)))
	{
		printf("Error: Create Thread Failed!\n");
		return false;
	}

	return true;
}

bool P2PClient::Login(char *pszUserName, char *pszServerIP)
{
	assert(NULL != pszUserName);
	assert(NULL != pszServerIP);

	int nUserNameLen;
	if ((nUserNameLen = (int)strlen(pszUserName)) > MAX_USERNAME - 1)
	{
		printf("Error: Input User Name Too Large!\n");
		return false;
	}

	if (true == m_bLogin)
	{
		printf("Error: Already Login Server!\n");
		return false;
	}

	// ����server��IP��ַ���û���
	m_dwServerIP = ::inet_addr(pszServerIP);
	memcpy(m_PeerInfo.szUserName, pszUserName, nUserNameLen);

	// 
	sockaddr_in serverAddr = {0};
	serverAddr.sin_addr.S_un.S_addr = m_dwServerIP;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);

	// ��server���͵�½��Ϣ
	MSGDef::TMSG_USERLOGIN tUserLogin(m_PeerInfo);
	for(int i = 0; i< MAX_TRY_NUMBER; ++i)
	{
		::sendto(m_sSocket, (char*)(&tUserLogin), sizeof(MSGDef::TMSG_USERLOGIN), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

		// �ȴ���½�ɹ����˳�
		for(int j = 0; j < 10; j++)
		{
			if(m_bLogin)
			{
				return true;
			}
			::Sleep(300);
		}
	}

	printf("Error: Login Server Failed!\n");
	return false;
}

bool P2PClient::Logout()
{
	if (true == m_bLogin)
	{
		sockaddr_in serverAddr = {0};
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.S_un.S_addr = m_dwServerIP;
		serverAddr.sin_port = htons(SERVER_PORT);

		MSGDef::TMSG_USERLOGOUT tUserLogout(m_PeerInfo);
		::sendto(m_sSocket, (char*)(&tUserLogout), sizeof(tUserLogout), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
		m_bLogin = false;

		return true;
	}
	else
	{
		return false;
	}
}

bool P2PClient::GetUserList()
{
	sockaddr_in serverAddr = {0};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = m_dwServerIP;
	serverAddr.sin_port = ntohs(SERVER_PORT);

	MSGDef::TMSG_GETUSERLIST tMsgGetUserList(m_PeerInfo);

	// 
	::EnterCriticalSection(&m_PeerListLock);
	m_bUserListCmp = false;
	m_PeerList.DeleteAllPeer();
	::LeaveCriticalSection(&m_PeerListLock);

	for (int i= 0; i < MAX_TRY_NUMBER; ++i)
	{
		::sendto(m_sSocket, (char*)(&tMsgGetUserList), sizeof(tMsgGetUserList), 0, (sockaddr*)(&serverAddr), sizeof(sockaddr_in));

		// �ȴ��û��б�������
		for (int j = 0; j < 10; ++j)
		{
			if (true == m_bUserListCmp)
			{
				return true;
			}
			::Sleep(300);
		}
	}

	return false;
}

void P2PClient::DisplayUserList()
{
	int nCurrentSize = m_PeerList.GetCurrentSize();
	printf("Have %d Users Logined Server: \n", nCurrentSize);
	int nAddrNum;
	for(int i = 0; i< nCurrentSize; i++)
	{
		Peer_Info* pPeerInfo = m_PeerList[i];
		nAddrNum = pPeerInfo->nAddrNum;
		printf("User Name: %s(%s:%ld) \n", pPeerInfo->szUserName, 
			::inet_ntoa(*((in_addr*)&pPeerInfo->IPAddr[nAddrNum - 1].dwIP)), 
			pPeerInfo->IPAddr[nAddrNum - 1].usPort);
	}
}

bool P2PClient::SendText(char *pszUserName, char* pszText, int nTextLen)
{
	if (NULL == pszUserName || NULL == pszText 
		|| strlen(pszUserName) > MAX_USERNAME
		|| nTextLen > MAX_PACKET_SIZE- sizeof(Peer_Info) - sizeof(MSGDef::TMSG_HEADER))
	{
		return false;
	}

	MSGDef::TMSG_P2PMSG tP2PMsg(m_PeerInfo);
	strcpy(tP2PMsg.szMsg, pszText);
	m_bMessageACK = false;
	Peer_Info* pPeerInfo;
	int j;
	for (int i = 0; i < MAX_TRY_NUMBER; ++i)
	{
		pPeerInfo = m_PeerList.GetAPeer(pszUserName);
		if (NULL == pPeerInfo)
		{
			return false;
		}

		// ����Է�P2P��ַ��Ϊ0������ͼ����ΪĿ�ĵ�ַ�������ݣ�
		// �������ʧ�ܣ�����Ϊ��P2P��ַ��Ч
		if (0 != pPeerInfo->P2PAddr.dwIP)
		{
			sockaddr_in peerAddr = {0};
			peerAddr.sin_family = AF_INET;
			peerAddr.sin_port = ntohs(pPeerInfo->P2PAddr.usPort);
			peerAddr.sin_addr.S_un.S_addr = pPeerInfo->P2PAddr.dwIP;

			::sendto(m_sSocket, (char*)(&tP2PMsg), sizeof(tP2PMsg), 0, (sockaddr*)(&peerAddr), sizeof(peerAddr));

			// �ȴ�һ��ʱ�俴���ͳɹ���û��
			for (int j = 0; i < 10; ++j)
			{
				if (true == m_bMessageACK)
				{
					return true;
				}
				::Sleep(300);
			}
		}

		// ����򶴣�������������P2P��ַ
		pPeerInfo->P2PAddr.dwIP = 0;

		// ����P2P�򶴷��
		MSGDef::TMSG_P2PCONNECT tP2PConnect(m_PeerInfo);
		strcpy(tP2PConnect.szUserName, pszUserName);

		// ����ֱ�ӷ���Ŀ��,��Ŀ��ڵ���������������ʹ���Ϣ
		sockaddr_in peerAddr = { 0 };
		peerAddr.sin_family = AF_INET;
		int nAddrNum = pPeerInfo->nAddrNum;
		for(j = 0; j < nAddrNum; ++j)
		{
			peerAddr.sin_addr.S_un.S_addr = pPeerInfo->IPAddr[j].dwIP;
			peerAddr.sin_port = htons(pPeerInfo->IPAddr[j].usPort);
			::sendto(m_sSocket, (char*)(&tP2PConnect), sizeof(tP2PConnect), 0, (sockaddr*)&peerAddr, sizeof(peerAddr));
		}

		// Ȼ��ͨ��������ת��������Է����Լ���
		sockaddr_in serverAddr = { 0 };
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.S_un.S_addr = m_dwServerIP;
		serverAddr.sin_port = htons(SERVER_PORT);
		::sendto(m_sSocket, (char*)(&tP2PConnect), sizeof(tP2PConnect), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

		// �ȴ��Է���P2PCONNECTACK��Ϣ
		for(j = 0; j < 10; ++j)
		{
			if (0 != pPeerInfo->P2PAddr.dwIP)
				break;
			::Sleep(300);
		}	

	}

	return false;
}

DWORD WINAPI P2PClient::RecvThreadProc(LPVOID lpParam)
{
	P2PClient* pThis = (P2PClient*)lpParam;
	char szBuff[MAX_PACKET_SIZE];
	int nRet;
	sockaddr_in remoteAddr = {0};
	int nAddrLen = sizeof(sockaddr_in);
	WSABUF wsaBuff;
	wsaBuff.buf = szBuff;
	wsaBuff.len = MAX_PACKET_SIZE;
	DWORD dwRecv, dwFlags = 0;
	MSGDef::TMSG_HEADER *pMsgHeader;

	while (true)
	{
		nRet = ::WSARecvFrom(pThis->m_sSocket, &wsaBuff, 1, &dwRecv, &dwFlags, 
							(sockaddr*)(&remoteAddr), &nAddrLen, &pThis->m_ol, NULL);
		
		// 
		if(SOCKET_ERROR == nRet && ::WSAGetLastError() == WSA_IO_PENDING)
		{
			::WSAGetOverlappedResult(pThis->m_sSocket, &pThis->m_ol, &dwRecv, TRUE, &dwFlags);
		}

		// ���Ȳ鿴�Ƿ�Ҫ�˳�
		if(pThis->m_bExitThread)
			break;

		// ������ͬ����Ϣ���д���
		pMsgHeader = (MSGDef::TMSG_HEADER*)szBuff;
		switch (pMsgHeader->cMsgID)
		{
		case MSG_USERLOGACK:			// ���յ������������ĵ�½ȷ��
			{
				pThis->ProcUserLogAckMsg(pMsgHeader);
			}
			break;
		case MSG_GETUSERLIST:			// �����û��б�
			{
				pThis->ProcGetUserList(pMsgHeader);
			}
			break;
		case MSG_USERLISTCMP:			// �����û��б����
			{
				pThis->ProcUserListCmpMsg();
			}
			break;
		case MSG_P2PMSG:				// ��һ���ڵ������Ƿ�����Ϣ
			{
				pThis->ProcP2PMsg(pMsgHeader, remoteAddr);
			}
			break;
		case MSG_P2PMSGACK:				// ��ĳ���ڵ㷢����Ϣ֮��Ļظ�
			{
				pThis->m_bMessageACK = true;
			}
			break;
		case MSG_P2PCONNECT:			// �����
			{
				pThis->ProcP2PConnectMsg(pMsgHeader, remoteAddr);
			}
			break;
		case MSG_P2PCONNECTACK:			// ����򶴵Ļظ�
			{
				pThis->ProcP2PConnectAckMsg(pMsgHeader, remoteAddr);
			}
			break;
		case MSG_USERACTIVEQUERY:
			{
				pThis->ProcUserActiveQueryMsg(pMsgHeader, remoteAddr);
			}
			break;
		}
	}

	return 0;
}

// ���յ������������ĵ�½ȷ��
bool P2PClient::ProcUserLogAckMsg(MSGDef::TMSG_HEADER *pMsgHeader)
{
	MSGDef::TMSG_USERLOGACK *pUserLogAckMsg = (MSGDef::TMSG_USERLOGACK *)pMsgHeader;

	memcpy(&m_PeerInfo, &pUserLogAckMsg->PeerInfo, sizeof(Peer_Info));
	int nAddrNum =  pUserLogAckMsg->PeerInfo.nAddrNum;
	in_addr LoginAddr;
	LoginAddr.S_un.S_addr = pUserLogAckMsg->PeerInfo.IPAddr[nAddrNum].dwIP;
	printf("Login IP: %s\n", ::inet_ntoa(LoginAddr));
	printf("Login port: %ld\n", ntohs(pUserLogAckMsg->PeerInfo.IPAddr[nAddrNum].usPort));

	m_bLogin = true;

	printf("Login P2P Server Success!\n");
	return true;
}

// �����û��б�
bool P2PClient::ProcGetUserList(MSGDef::TMSG_HEADER *pMsgHeader)
{
	MSGDef::TMSG_GETUSERLIST* pMsgGetUserList = (MSGDef::TMSG_GETUSERLIST*)pMsgHeader;

	pMsgGetUserList->PeerInfo.P2PAddr.dwIP = 0;

	::EnterCriticalSection(&m_PeerListLock);
	m_PeerList.AddPeer(pMsgGetUserList->PeerInfo);
	::LeaveCriticalSection(&m_PeerListLock);

	return true;
}

// �����û��б����
bool P2PClient::ProcUserListCmpMsg()
{
	m_bUserListCmp = true;

	return true;
}

// ��һ���ڵ������Ƿ�����Ϣ
bool P2PClient::ProcP2PMsg(MSGDef::TMSG_HEADER *pMsgHeader, const sockaddr_in& sockAddr)
{
	MSGDef::TMSG_P2PMSG *pP2PMsg = (MSGDef::TMSG_P2PMSG *)pMsgHeader;

	// �����յ���Ϣ��ȷ����Ϣ
	MSGDef::TMSG_P2PMSGACK tP2PMsgAck(m_PeerInfo);
	::sendto(m_sSocket, (char*)(&tP2PMsgAck), sizeof(tP2PMsgAck), 0, (sockaddr*)(&sockAddr), sizeof(sockAddr));

	printf("Receive a Message from %s :  %s \n", pP2PMsg->PeerInfo.szUserName, pP2PMsg->szMsg);

	return true;
}

// һ���ڵ�������P2P���ӣ��򶴣��������Ƿ����������ģ�Ҳ�����������ڵ㷢����
bool P2PClient::ProcP2PConnectMsg(MSGDef::TMSG_HEADER *pMsgHeader, const sockaddr_in& sockAddr)
{
	MSGDef::TMSG_P2PCONNECT *pP2PConnect = (MSGDef::TMSG_P2PCONNECT *)pMsgHeader;
	MSGDef::TMSG_P2PCONNECTACK tP2PConnectAck(m_PeerInfo);

	if (sockAddr.sin_addr.S_un.S_addr != m_dwServerIP)	// �ڵ㷢��������Ϣ
	{
		::EnterCriticalSection(&m_PeerListLock);
		Peer_Info *pPeerInfo = m_PeerList.GetAPeer( pP2PConnect->PeerInfo.szUserName);

		if (NULL != pPeerInfo)
		{
			if (0 == pPeerInfo->P2PAddr.dwIP)
			{
				// ���¸ýڵ�����P2Pͨ�ŵ�IP��ַ�Ͷ˿ں�
				pPeerInfo->P2PAddr.dwIP = sockAddr.sin_addr.S_un.S_addr;
				pPeerInfo->P2PAddr.usPort = ntohs(sockAddr.sin_port);
				printf("Set P2P Address For %s -> %s:%ld \n", pPeerInfo->szUserName, 
					::inet_ntoa(sockAddr.sin_addr), ntohs(sockAddr.sin_port));
			}
		}
		::LeaveCriticalSection(&m_PeerListLock);
		::sendto(m_sSocket, (char*)(&tP2PConnectAck), sizeof(tP2PConnectAck), 0, (sockaddr*)(&sockAddr), sizeof(sockAddr));
	}
	else												// ������ת������Ϣ
	{
		// ��ڵ�������ն˷��ʹ���Ϣ
		sockaddr_in peerAddr = { 0 };
		peerAddr.sin_family = AF_INET;
		for(int i = 0, nAddrNum = pP2PConnect->PeerInfo.nAddrNum; i < nAddrNum; ++i)
		{
			peerAddr.sin_addr.S_un.S_addr = pP2PConnect->PeerInfo.IPAddr[i].dwIP;
			peerAddr.sin_port = htons(pP2PConnect->PeerInfo.IPAddr[i].usPort);
			::sendto(m_sSocket, (char*)(&tP2PConnectAck), sizeof(tP2PConnectAck), 0, (sockaddr*)(&peerAddr), sizeof(peerAddr));
		}
	}

	return true;
}

// ���յ��ڵ�Ĵ���Ϣ�����������øýڵ��P2Pͨ�ŵ�ַ
bool P2PClient::ProcP2PConnectAckMsg(MSGDef::TMSG_HEADER *pMsgHeader, const sockaddr_in& sockAddr)
{
	MSGDef::TMSG_P2PCONNECTACK* pP2PConnectAck = (MSGDef::TMSG_P2PCONNECTACK*)pMsgHeader;

	::EnterCriticalSection(&m_PeerListLock);
	Peer_Info* pPeerInfo = m_PeerList.GetAPeer(pP2PConnectAck->PeerInfo.szUserName);

	if (NULL != pPeerInfo)
	{
		if (0 == pPeerInfo->P2PAddr.dwIP)
		{
			pPeerInfo->P2PAddr.dwIP = sockAddr.sin_addr.S_un.S_addr;
			pPeerInfo->P2PAddr.usPort = ntohs(sockAddr.sin_port);

			printf("Set P2P address for %s -> %s:%ld \n", pP2PConnectAck->PeerInfo.szUserName, 
				::inet_ntoa(sockAddr.sin_addr), ntohs(sockAddr.sin_port));
		}
	}
	::LeaveCriticalSection(&m_PeerListLock);

	return true;
}

// ������ѯ���Ƿ���
bool P2PClient::ProcUserActiveQueryMsg(MSGDef::TMSG_HEADER *pMsgHeader, const sockaddr_in& sockAddr)
{
	MSGDef::TMSG_USERACTIVEQUERY tUserActiveQuery(m_PeerInfo);
	::sendto(m_sSocket, (char*)(&tUserActiveQuery), sizeof(tUserActiveQuery), 0, (sockaddr*)&sockAddr, sizeof(sockAddr));

	return true;
}
