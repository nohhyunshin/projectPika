#include "pch.h"
#include "framework.h"
#include "Project_MFC.h"
#include "Project_MFCDlg.h"
#include "Defs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void CProjectMFCDlg::ErrQuit(wchar_t* msg) {
	LPVOID IpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&IpMsgBuf, 0, NULL);

	MessageBoxW((LPCWSTR)IpMsgBuf, (LPCWSTR)msg, MB_OK | MB_ICONERROR);
	LocalFree(IpMsgBuf);
	exit(-1);
}

void CProjectMFCDlg::ErrDisplay(wchar_t* msg) {
	LPVOID IpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&IpMsgBuf, 0, NULL);
	MessageBoxW((LPCWSTR)IpMsgBuf, (LPCWSTR)msg, MB_OK | MB_ICONERROR);
	LocalFree(IpMsgBuf);
}

// PLC에 접속
int CProjectMFCDlg::ConnectPLC() {
	client_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (client_sock == INVALID_SOCKET) {
		ErrDisplay(L"socket()");
		SetState(false);
		return -1;
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(502);								// PLC Port
	inet_pton(AF_INET, "192.168.137.1", &serveraddr.sin_addr);		// PLC IP

	retval = connect(client_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		closesocket(client_sock);
		client_sock = INVALID_SOCKET;
		ConnectionFlag = 1;
		SetState(false);
		return -1;
	}

	u_long mode = 1;
	ioctlsocket(client_sock, FIONBIO, &mode);

	ConnectionFlag = 0;
	SetState(true);

	ReadHoldingRegs();

	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &serveraddr.sin_addr, ip, sizeof(ip));
	CString ipStr(ip);

	CString PLClogLine;
	PLClogLine.Format(L"[TCP 서버 접속] IP 주소 : %s / Port No : %d \r\n", ipStr, ntohs(serveraddr.sin_port));
	m_state += PLClogLine;

	UpdateData(FALSE);
	Invalidate();
	return 0;
}

// PLC 연결 상태 감시
void CProjectMFCDlg::ConnectionStatus() {
	if (ConnectionFlag == 1) {
		ConnectPLC();
		return;
	}

	// 연결 상태 감지 - 끊겼을 때
	char buf[1];
	int ret = recv(client_sock, buf, 1, MSG_PEEK);
	if (ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)) {
		closesocket(client_sock);
		client_sock = INVALID_SOCKET;
		ConnectionFlag = 1;
		SetState(false);
		UpdateData(FALSE);
		Invalidate();
	}
}

// 장비 연결 상태 표시 LED
void CProjectMFCDlg::SetLed(int idx, COLORREF clr) {
	if (m_clrLed[idx] == clr) return;
	COLORREF prev = m_clrLed[idx];
	m_clrLed[idx] = clr;
	CRect r; GetDlgItem(kLedIds[idx])->GetWindowRect(&r); ScreenToClient(&r);
	InvalidateRect(r, TRUE);

	// 장비 통신 상태 로그 창 변경
	bool ok = (clr == RGB(0x3D, 0xDC, 0x84));
	CString status = ok ? _T("정상") : _T("에러");
	if (connStatus.GetItemText(kLEDRow[idx], 2) != status)
		connStatus.SetItemText(kLEDRow[idx], 2, status);

	// HMI : PLC 연결 → HMI 정상, PLC 끊김 → HMI 에러
	if (idx == LED_PLC && connStatus.GetItemText(1, 2) != status)
		connStatus.SetItemText(1, 2, status);

	bool progInit = (prev == RGB(0x6B, 0x77, 0x8A));
	CString dev = connStatus.GetItemText(kLEDRow[idx], 0);
	CString detail = connStatus.GetItemText(kLEDRow[idx], 4);

	CString msg;
	if (!ok) {
		msg.Format(_T("%s (%s) %s"), dev.GetString(), detail.GetString(), progInit ? _T("미연결") : _T("연결 끊김"));
		AddAlarm(_T("위험"), msg);
	}
	else if (!progInit) {
		msg.Format(_T("%s (%s) 연결 복구"), dev.GetString(), detail.GetString());
		AddAlarm(_T("정보"), msg);
	}
}

void CProjectMFCDlg::SetState(bool bOn) {
	COLORREF clr = bOn ? RGB(0x3D, 0xDC, 0x84) : RGB(0xFF, 0x5A, 0x52);
	if (m_clrLed[LED_PLC] == clr) return;        // 상태가 같으면 아무것도 안 함
	SetLed(LED_PLC, clr);
	SetLed(LED_VISION, clr);                     // 비전은 PLC 연결 상태를 따라감
}

void CProjectMFCDlg::ReadHoldingRegs() {
	if (ConnectionFlag == 1) return;

	BYTE request[] = {
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x06,
		0x01,
		0x03,
		0x00, 0x00,
		0x00, REG_COUNT
	};

	send(client_sock, (char*)request, sizeof(request), 0);

	// 응답 올 때까지 최대 1초 대기
	fd_set rset; FD_ZERO(&rset); FD_SET(client_sock, &rset);
	timeval tv = { 1, 0 };
	if (select(0, &rset, NULL, NULL, &tv) <= 0) return;

	BYTE response[128];
	int receivedData = recv(client_sock, (char*)response, sizeof(response), 0);

	if (receivedData == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAEWOULDBLOCK) return;
		return;
	}
	if (receivedData < 9 + REG_COUNT * 2) return;		// 길이 확인

	if (response[7] != 0x03) return;					// 응답 코드 확인

	WORD reg[REG_COUNT];
	for (int i = 0; i < REG_COUNT; i++) {
		reg[i] = (response[9 + i * 2] << 8) | response[10 + i * 2];		// MFC_HR[i]
	}

	CheckRentChange(reg);

	WORD arduinoStatus = reg[ARDUINO_STATE_IDX] & 0x00FF;		// 아두이노 상태 레지스터
	bool arduinoOn = (arduinoStatus == ARDUINO_STATE_TARGET);
	SetLed(LED_ARDUINO, arduinoOn ? RGB(0x3D, 0xDC, 0x84) : RGB(0xFF, 0x5A, 0x52));

	WORD robotStatus = reg[ROBOT_STATE_IDX] & 0x00FF;			// 로봇 상태 레지스터
	bool robotOn = (robotStatus == ROBOT_STATE_TARGET || robotStatus == ROBOT_TASK_TARGET);
	SetLed(LED_ROBOT, robotOn ? RGB(0x3D, 0xDC, 0x84) : RGB(0xFF, 0x5A, 0x52));

	CheckRentChange(reg);
	UpdateSlotTable(reg);
	UpdateRobotStatus(reg);

	UpdateData(FALSE);
}