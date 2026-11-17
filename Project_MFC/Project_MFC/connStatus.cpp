#include "pch.h"
#include "framework.h"
#include "Project_MFC.h"
#include "Project_MFCDlg.h"
#include "Defs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 장비 연결 상태 로그 행열
void CProjectMFCDlg::ConnStateLog() {
	// 장비 연결 상태 로그 행열
	connStatus.SetBkColor(m_clrPanel);
	connStatus.SetTextBkColor(m_clrPanel);
	connStatus.SetTextColor(m_clrText);
	SetWindowTheme(connStatus.GetSafeHwnd(), L"", L"");

	connStatus.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	connStatus.InsertColumn(0, _T("장비"), LVCFMT_LEFT, 170);
	connStatus.InsertColumn(1, _T("역할"), LVCFMT_LEFT, 115);
	connStatus.InsertColumn(2, _T("연결"), LVCFMT_LEFT, 83);
	connStatus.InsertColumn(3, _T("IP"), LVCFMT_CENTER, 150);
	connStatus.InsertColumn(4, _T("세부"), LVCFMT_LEFT, 120);

	int r1 = connStatus.InsertItem(0, _T("Omron NX102-1020"));
	connStatus.SetItemText(r1, 1, _T("서버"));
	connStatus.SetItemText(r1, 2, _T("미연결"));
	connStatus.SetItemText(r1, 3, _T("192.168.137.1"));
	connStatus.SetItemText(r1, 4, _T("PLC"));

	int r2 = connStatus.InsertItem(1, _T("Omron NA5-9W001B"));
	connStatus.SetItemText(r2, 1, _T("클라이언트"));
	connStatus.SetItemText(r2, 2, _T("미연결"));
	connStatus.SetItemText(r2, 3, _T("192.168.137.2"));
	connStatus.SetItemText(r2, 4, _T("HMI (UI)"));

	int r3 = connStatus.InsertItem(2, _T("Omron FZ-SQ050F"));
	connStatus.SetItemText(r3, 1, _T("클라이언트"));
	connStatus.SetItemText(r3, 2, _T("미연결"));
	connStatus.SetItemText(r3, 3, _T("192.168.137.10"));
	connStatus.SetItemText(r3, 4, _T("Vision Camera"));

	int r4 = connStatus.InsertItem(3, _T("Doosan Robotics A0509"));
	connStatus.SetItemText(r4, 1, _T("클라이언트"));
	connStatus.SetItemText(r4, 2, _T("미연결"));
	connStatus.SetItemText(r4, 3, _T("192.168.137.100"));
	connStatus.SetItemText(r4, 4, _T("Cobot"));

	int r5 = connStatus.InsertItem(4, _T("Arduino IoT-ModLink"));
	connStatus.SetItemText(r5, 1, _T("클라이언트"));
	connStatus.SetItemText(r5, 2, _T("미연결"));
	connStatus.SetItemText(r5, 3, _T("192.168.137.60"));
	connStatus.SetItemText(r5, 4, _T("Sensor"));
}

// 장비 연결 상태 로그 커스텀
void CProjectMFCDlg::OnCustomdrawConn(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVCUSTOMDRAW pLV = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
	*pResult = 0;

	switch (pLV->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
		pLV->clrTextBk = m_clrPanel;
		pLV->clrText = m_clrText;
		if (pLV->iSubItem == 2)
		{
			int row = (int)pLV->nmcd.dwItemSpec;
			CString s = connStatus.GetItemText(row, 2);

			COLORREF bg = RGB(0x2A, 0x35, 0x47), fg = RGB(0xA9, 0xB4, 0xC4);   // 정보
			if (s == _T("에러")) { bg = RGB(0x4A, 0x1D, 0x1B); fg = RGB(0xFF, 0x8A, 0x83); }
			else if (s == _T("정상")) { bg = RGB(0x12, 0x36, 0x1F); fg = RGB(0x3D, 0xDC, 0x84); }

			CDC* pDC = CDC::FromHandle(pLV->nmcd.hdc);
			CRect rc;
			connStatus.GetSubItemRect(row, 2, LVIR_LABEL, rc);
			pDC->FillSolidRect(rc, m_clrPanel);

			CFont* pOldFont = pDC->SelectObject(connStatus.GetFont());
			CSize sz = pDC->GetTextExtent(s);
			CRect chip(rc.left + 8, rc.CenterPoint().y - 12, rc.left + 8 + sz.cx + 20, rc.CenterPoint().y + 12);

			CBrush br(bg);
			CBrush* pOldBr = pDC->SelectObject(&br);
			CPen* pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
			pDC->RoundRect(chip, CPoint(4, 4));		// border radius 6px

			pDC->SetBkMode(TRANSPARENT);
			pDC->SetTextColor(fg);
			pDC->DrawText(s, chip, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBr);
			pDC->SelectObject(pOldFont);

			*pResult = CDRF_SKIPDEFAULT;	// 기본 그리기 생략
		}
		break;
	}
}