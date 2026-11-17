// Project_MFCDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "Project_MFC.h"
#include "Project_MFCDlg.h"
#include "afxdialogex.h"
#include "Defs.h"
#include <initializer_list>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CProjectMFCDlg 대화 상자
CProjectMFCDlg::CProjectMFCDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PROJECT_MFC_DIALOG, pParent)
	, m_state(_T(""))
	, m_msg(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CProjectMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RENT, CRent);
	DDX_Control(pDX, IDC_CHARGING, CCharging);
	DDX_Control(pDX, IDC_FAULTY, CFaulty);
	DDX_Control(pDX, IDC_RENTAL, CRental);
	DDX_Control(pDX, IDC_RETURN, CReturn);
	DDX_Control(pDX, IDC_RUNNINGT, CRunningT);
	DDX_Text(pDX, IDC_STATE, m_state);
	DDX_Control(pDX, IDC_ConnState, connStatus);
	DDX_Control(pDX, IDC_LogList, alarmLog);
	DDX_Control(pDX, IDC_Battery, batteryLog);
	DDX_Control(pDX, IDC_Timer, timeCount);
}

BEGIN_MESSAGE_MAP(CProjectMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_ConnState, &CProjectMFCDlg::OnCustomdrawConn)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LogList, &CProjectMFCDlg::OnDrawLoglist)
	ON_NOTIFY(NM_CLICK, IDC_LogList, &CProjectMFCDlg::OnNMClickLoglist)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_Battery, &CProjectMFCDlg::OnDrawBattery)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_Timer, &CProjectMFCDlg::OnCustomTimer)
END_MESSAGE_MAP()


// CProjectMFCDlg 메시지 처리기
BOOL CProjectMFCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	Gdiplus::GdiplusStartupInput gdiplusInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusInput, nullptr);

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	FullScreen();

	m_clrBg = RGB(0x0F, 0x16, 0x20);
	m_clrPanel = RGB(0x17, 0x21, 0x31);
	m_clrText = RGB(0xD4, 0xD4, 0xD4);
	m_brBg.CreateSolidBrush(m_clrBg);
	m_brPanel.CreateSolidBrush(m_clrPanel);
	m_brLight.CreateSolidBrush(m_clrLight);
	SetBackgroundColor(m_clrBg);

	m_imgRow.Create(2, 43, ILC_COLOR32, 1, 0);
	connStatus.SetImageList(&m_imgRow, LVSIL_SMALL);
	m_connHeader.SubclassWindow(connStatus.GetHeaderCtrl()->GetSafeHwnd());
	alarmLog.SetImageList(&m_imgRow, LVSIL_SMALL);
	m_logHeader.SubclassWindow(alarmLog.GetHeaderCtrl()->GetSafeHwnd());
	batteryLog.SetImageList(&m_imgRow, LVSIL_SMALL);
	m_batteryHeader.SubclassWindow(batteryLog.GetHeaderCtrl()->GetSafeHwnd());
	timeCount.SetImageList(&m_imgRow, LVSIL_SMALL);
	m_timerHeader.SubclassWindow(timeCount.GetHeaderCtrl()->GetSafeHwnd());
	ConnStateLog();
	AlarmLogList();
	BatteryLogList();
	TimeCntList();

	HideScrollBar(alarmLog);

	for (auto& c : m_clrLed) c = RGB(0x6B, 0x77, 0x8A);		// 장비 연결 상태 LED 초기 값
	ConnectionFlag = 1;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	ConnectPLC();
	m_startTick = GetTickCount64();				// 가동 시간 기준점
	SetTimer(TIMER_ID, READ_INTERVAL, NULL);	// 타이머 시작

	GetDlgItem(IDC_STEPS)->ShowWindow(SW_HIDE);
	SetFontSize();
	UpdateStepText();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CProjectMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CProjectMFCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CProjectMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CProjectMFCDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == 1) {
		ConnectionStatus();
		ReadHoldingRegs();
		UpdateRunningTime();
		UpdateRentTimers();
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CProjectMFCDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	KillTimer(TIMER_ID);
	Gdiplus::GdiplusShutdown(m_gdiplusToken);
}

