
// Project_MFCDlg.h: 헤더 파일
//

#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#include "DarkHeader.h"

#define BUFSIZE 512

// CProjectMFCDlg 대화 상자
class CProjectMFCDlg : public CDialogEx
{
// 생성입니다.
public:
	CProjectMFCDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROJECT_MFC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);	// 배경 색 변경
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);								// 패널 배경 삭제

	// 화면 배율 조정
	bool  m_bFullScreen = false;
	CRect m_rcRestore;					// 원복
	DWORD m_dwRestoreStyle = 0;

	void FullScreen();
	CBrush m_brBg, m_brPanel;
	COLORREF m_clrBg, m_clrText, m_clrPanel;
	COLORREF m_clrLight = RGB(0x1E, 0x2A, 0x3E);
	CBrush   m_brLight;

	// 폰트 사이즈 설정
	void SetFontSize();
	CFont m_title, m_header, m_status, m_fontSize, m_Pannel;
	CStatic CRent, CCharging, CFaulty, CRental, CReturn, CRunningT;

	// 패널 데이터 설정
	ULONGLONG m_startTick = 0;
	void UpdateRunningTime();

	// PLC 연결
	int ConnectionFlag;
	int retval;
	SOCKET client_sock;
	WSADATA wsa;

	// PLC 연결 상태 표시
	COLORREF m_clrLed[8];
	void SetLed(int idx, COLORREF clr);
	void SetState(bool bOn);

	int addrlen;
	char buf[BUFSIZE + 1];

	int ConnectPLC();
	void ConnectionStatus();
	void ErrQuit(wchar_t* msg);
	void ErrDisplay(wchar_t* msg);
	void ReadHoldingRegs();

	// 리스트 컨트롤 헤더
	DarkHeader m_logHeader;
	DarkHeader m_connHeader;
	DarkHeader m_batteryHeader;
	DarkHeader m_timerHeader;
	CImageList  m_imgRow;

	// 알람 로그
	CString strTime;
	CListCtrl alarmLog;
	void AlarmLogList();
	void HideScrollBar(CListCtrl& lc);
	void AddAlarm(LPCTSTR level, const CString& msg);
	afx_msg void OnDrawLoglist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickLoglist(NMHDR* pNMHDR, LRESULT* pResult);

	// 장비 연결 상태 로그
	CString m_state;
	CListCtrl connStatus;
	void ConnStateLog();
	afx_msg void OnCustomdrawConn(NMHDR* pNMHDR, LRESULT* pResult);

	// 배터리 상태 로그
	CListCtrl batteryLog;
	void BatteryLogList();
	void UpdateSlotTable(const WORD* reg);
	WORD GetSlotBatteryId(const WORD* reg, int s);
	afx_msg void OnDrawBattery(NMHDR* pNMHDR, LRESULT* pResult);

	// 경과 시간 로그
	CListCtrl timeCount;
	void TimeCntList();
	void CountTimer(WORD id);
	void CheckRentChange(const WORD* reg);
	void UpdateRentTimers();
	bool IsRenting(WORD id);
	afx_msg void OnCustomTimer(NMHDR* pNMHDR, LRESULT* pResult);

	// 로봇 상태 로그
	int robotKind = 0;					// 0 대기 / 1 대여 / 2 반납
	int robotStep = 0;					// 0 작업 없음 / 1~6 단계별 동작
	ULONG_PTR m_gdiplusToken = 0;
	COLORREF m_clrStepName = RGB(0xA9, 0xB4, 0xC4);
	COLORREF m_clrBotState = RGB(0xA9, 0xB4, 0xC4);
	void DrawRobotSteps(CDC* pDC);
	void InvalidateCtrl(UINT id);
	void UpdateStepText();
	void UpdateRobotChip();
	void UpdateRobotStatus(const WORD* reg);
	CString m_msg;
};
