// UI 디자인

#include "pch.h"
#include "framework.h"
#include "Project_MFC.h"
#include "Project_MFCDlg.h"
#include "Defs.h"
#include <initializer_list>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 화면 배율 조정
void CProjectMFCDlg::FullScreen()
{
	if (!m_bFullScreen)
	{
		GetWindowRect(&m_rcRestore);
		m_dwRestoreStyle = GetStyle();

		HMONITOR hMon = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		::GetMonitorInfo(hMon, &mi);

		ModifyStyle(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU, 0);

		const CRect rc = mi.rcWork;
		SetWindowPos(&wndTop, rc.left, rc.top, rc.Width(), rc.Height(),
			SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}
	else
	{
		ModifyStyle(0, m_dwRestoreStyle & (WS_CAPTION | WS_THICKFRAME | WS_SYSMENU));
		SetWindowPos(nullptr, m_rcRestore.left, m_rcRestore.top,
			m_rcRestore.Width(), m_rcRestore.Height(),
			SWP_NOZORDER | SWP_FRAMECHANGED);
	}
	m_bFullScreen = !m_bFullScreen;
}

// 배경 컬러 설정
HBRUSH CProjectMFCDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  여기서 DC의 특성을 변경합니다.
	if (nCtlColor == CTLCOLOR_DLG)
		return (HBRUSH)m_brBg.GetSafeHandle();

	if (nCtlColor == CTLCOLOR_EDIT)
	{
		pDC->SetTextColor(m_clrText);
		pDC->SetBkColor(m_clrPanel);
		pDC->SetBkMode(OPAQUE);
		return (HBRUSH)m_brPanel.GetSafeHandle();
	}

	if (nCtlColor == CTLCOLOR_STATIC)
	{
		COLORREF clrText = m_clrText;		// 기본 글자색
		COLORREF clrBg = m_clrPanel;		// 기본: 패널 안 라벨
		HBRUSH   hbrBg = (HBRUSH)m_brPanel.GetSafeHandle();

		// 폰트 색 설정
		switch (pWnd->GetDlgCtrlID()) {
		case IDC_TITLE:				// 컨트롤 타워 타이틀
			clrBg = m_clrBg;
			hbrBg = (HBRUSH)m_brBg.GetSafeHandle();
			break;
		case IDC_RENT: clrText = RGB(0x3D, 0xDC, 0x84); break;
		case IDC_CHARGING: clrText = RGB(0xF5, 0xB9, 0x42); break;
		case IDC_FAULTY: clrText = RGB(0xFF, 0x5A, 0x52); break;
		case IDC_STEPNAME: clrText = m_clrStepName; clrBg = m_clrLight; hbrBg = (HBRUSH)m_brLight.GetSafeHandle(); break;
		case IDC_STEPINFO: clrText = RGB(0xA9, 0xB4, 0xC4); clrBg = m_clrLight; hbrBg = (HBRUSH)m_brLight.GetSafeHandle(); break;
		case IDC_botStatus: clrText = m_clrBotState; clrBg = m_clrLight; hbrBg = (HBRUSH)m_brLight.GetSafeHandle(); break;
		}

		pDC->SetTextColor(clrText);
		pDC->SetBkColor(clrBg);
		pDC->SetBkMode(TRANSPARENT);
		return hbrBg;
	}

	// TODO:  기본값이 적당하지 않으면 다른 브러시를 반환합니다.
	return hbr;
}

// 패널 배경 삭제
BOOL CProjectMFCDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CRect rc; GetClientRect(&rc);
	pDC->FillSolidRect(rc, RGB(0x0F, 0x16, 0x20));
	CBrush br(RGB(0x17, 0x21, 0x31));
	CPen pen(PS_SOLID, 1, RGB(0x2A, 0x35, 0x47));
	CBrush* ob = pDC->SelectObject(&br);
	CPen* op = pDC->SelectObject(&pen);
	for (UINT id : { IDC_PRENT, IDC_PCHARGING, IDC_PFAULTY, IDC_PRENTAL, IDC_PRETURN, IDC_PRUNNING, IDC_BSTATUS, IDC_ALARM, IDC_CONN, IDC_BTIMER, IDC_BOTSTEP })
	{
		CRect r; GetDlgItem(id)->GetWindowRect(&r); ScreenToClient(&r);
		pDC->RoundRect(r, CPoint(12, 12));
	}

	for (UINT id : { IDC_PAUTO, IDC_PPLC, IDC_PVISION, IDC_PROBOT, IDC_PARDUINO, IDC_STEPINFO, IDC_STEPNAME })
	{
		CRect r; GetDlgItem(id)->GetWindowRect(&r); ScreenToClient(&r);
		pDC->RoundRect(r, CPoint(40, 40));
	}

	pDC->SelectObject(&m_brLight);
	for (UINT id : { IDC_NOW, IDC_BOTNOW })
	{
		CRect r; GetDlgItem(id)->GetWindowRect(&r); ScreenToClient(&r);
		pDC->RoundRect(r, CPoint(12, 12));
	}
	pDC->SelectObject(&br);

	// 상태 LED 그리기
	CPen penNone(PS_NULL, 0, RGB(0, 0, 0));
	pDC->SelectObject(&penNone);
	for (int i = 0; i < LED_COUNT; i++)
	{
		CRect r; GetDlgItem(kLedIds[i])->GetWindowRect(&r); ScreenToClient(&r);
		CBrush brLed(m_clrLed[i]);
		CBrush* prev = pDC->SelectObject(&brLed);
		pDC->Ellipse(r);
		pDC->SelectObject(prev);          // 브러시를 삭제하기 전에 되돌림
	}

	pDC->SelectObject(ob); pDC->SelectObject(op);

	DrawRobotSteps(pDC);

	return TRUE;

	return CDialogEx::OnEraseBkgnd(pDC);
}

// 폰트 크기 설정
void CProjectMFCDlg::SetFontSize() {
	// Label 폰트 설정
	m_title.CreateFont(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("맑은 고딕"));

	GetDlgItem(IDC_TITLE)->SetFont(&m_title);

	m_Pannel.CreateFont(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("맑은 고딕"));

	GetDlgItem(IDC_STEPNAME)->SetFont(&m_Pannel);
	GetDlgItem(IDC_botStatus)->SetFont(&m_Pannel);

	// Label 폰트 설정
	m_header.CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("맑은 고딕"));

	GetDlgItem(IDC_Trent)->SetFont(&m_header);
	GetDlgItem(IDC_Tcharging)->SetFont(&m_header);
	GetDlgItem(IDC_Tfaulty)->SetFont(&m_header);
	GetDlgItem(IDC_Trental)->SetFont(&m_header);
	GetDlgItem(IDC_Treturn)->SetFont(&m_header);
	GetDlgItem(IDC_Trunning)->SetFont(&m_header);
	GetDlgItem(IDC_TBStatus)->SetFont(&m_header);
	GetDlgItem(IDC_TAstatus)->SetFont(&m_header);
	GetDlgItem(IDC_TCstatus)->SetFont(&m_header);
	GetDlgItem(IDC_TimeCnt)->SetFont(&m_header);
	GetDlgItem(IDC_TBotStatus)->SetFont(&m_header);

	// 상태 라벨 폰트 설정
	m_status.CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("맑은 고딕"));

	GetDlgItem(IDC_AUTO)->SetFont(&m_status);
	GetDlgItem(IDC_PLC)->SetFont(&m_status);
	GetDlgItem(IDC_VISION)->SetFont(&m_status);
	GetDlgItem(IDC_ROBOT)->SetFont(&m_status);
	GetDlgItem(IDC_ARDUINO)->SetFont(&m_status);
	GetDlgItem(IDC_STATE)->SetFont(&m_status);
	GetDlgItem(IDC_ConnState)->SetFont(&m_status);
	GetDlgItem(IDC_LogList)->SetFont(&m_status);
	GetDlgItem(IDC_Battery)->SetFont(&m_status);
	GetDlgItem(IDC_Timer)->SetFont(&m_status);
	GetDlgItem(IDC_BOTID)->SetFont(&m_status);
	GetDlgItem(IDC_STEPINFO)->SetFont(&m_status);

	// Static Text 폰트 설정
	m_fontSize.CreateFont(
		50, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("맑은 고딕"));

	GetDlgItem(IDC_RENT)->SetFont(&m_fontSize);
	GetDlgItem(IDC_CHARGING)->SetFont(&m_fontSize);
	GetDlgItem(IDC_FAULTY)->SetFont(&m_fontSize);
	GetDlgItem(IDC_RENTAL)->SetFont(&m_fontSize);
	GetDlgItem(IDC_RETURN)->SetFont(&m_fontSize);
	GetDlgItem(IDC_RUNNINGT)->SetFont(&m_fontSize);

	CRent.SetWindowText(_T("0"));
	CCharging.SetWindowText(_T("0"));
	CFaulty.SetWindowText(_T("0"));
	CRental.SetWindowText(_T("0"));
	CReturn.SetWindowText(_T("0"));
	CRunningT.SetWindowText(_T("00:00:00"));
}

// 가동 시간
void CProjectMFCDlg::UpdateRunningTime() {
	CString str = CTime::GetCurrentTime().Format(_T("%H:%M:%S"));

	CString cur;
	CRunningT.GetWindowText(cur);
	if (cur != str)
		CRunningT.SetWindowText(str);
}

// 스크롤 바 숨김
void CProjectMFCDlg::HideScrollBar(CListCtrl& lc) {
	CRect rc;
	lc.GetWindowRect(&rc);

	int cx = GetSystemMetrics(SM_CXVSCROLL);
	int cy = GetSystemMetrics(SM_CXVSCROLL);
	lc.SetWindowPos(nullptr, 0, 0, rc.Width() + cx, rc.Height(), SWP_NOMOVE | SWP_NOZORDER);
	lc.SetWindowPos(nullptr, 0, 0, rc.Width() + cy, rc.Height(), SWP_NOMOVE | SWP_NOZORDER);

	CRgn rgn;
	rgn.CreateRectRgn(0, 0, rc.Width(), rc.Height());
	lc.SetWindowRgn((HRGN)rgn.Detach(), TRUE);
}