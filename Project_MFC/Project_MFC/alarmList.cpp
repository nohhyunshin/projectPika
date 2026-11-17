#include "pch.h"
#include "framework.h"
#include "Project_MFC.h"
#include "Project_MFCDlg.h"
#include "Defs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 알람 로그
void CProjectMFCDlg::AlarmLogList() {
	alarmLog.SetBkColor(m_clrPanel);
	alarmLog.SetTextBkColor(m_clrPanel);
	alarmLog.SetTextColor(m_clrText);
	SetWindowTheme(alarmLog.GetSafeHwnd(), L"", L"");

	alarmLog.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	alarmLog.InsertColumn(0, _T("등급"), LVCFMT_LEFT, 71);
	alarmLog.InsertColumn(1, _T("시각"), LVCFMT_CENTER, 120);
	alarmLog.InsertColumn(2, _T("내용"), LVCFMT_LEFT, 346);
	alarmLog.InsertColumn(3, _T("확인 여부"), LVCFMT_CENTER, 100);

	strTime = CTime::GetCurrentTime().Format(_T("[ %H:%M:%S ]"));
}

void CProjectMFCDlg::OnDrawLoglist(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVCUSTOMDRAW pLV = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
	*pResult = CDRF_DODEFAULT;

	switch (pLV->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
		pLV->nmcd.uItemState &= ~CDIS_SELECTED;
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
		pLV->clrTextBk = m_clrPanel;
		pLV->clrText = m_clrText;
		if (pLV->iSubItem == 0)
		{
			int row = (int)pLV->nmcd.dwItemSpec;
			CString s = alarmLog.GetItemText(row, 0);

			COLORREF bg = RGB(0x2A, 0x35, 0x47), fg = RGB(0xA9, 0xB4, 0xC4);
			if (s == _T("위험")) { bg = RGB(0x4A, 0x1D, 0x1B); fg = RGB(0xFF, 0x8A, 0x83); }
			else if (s == _T("경고")) { bg = RGB(0x4A, 0x3A, 0x12); fg = RGB(0xF5, 0xB9, 0x42); }

			CDC* pDC = CDC::FromHandle(pLV->nmcd.hdc);
			CRect rc;
			alarmLog.GetSubItemRect(row, 0, LVIR_LABEL, rc);
			pDC->FillSolidRect(rc, m_clrPanel);

			CFont* pOldFont = pDC->SelectObject(alarmLog.GetFont());
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
		else if (pLV->iSubItem == 3)            // 확인 칸
		{
			int row = (int)pLV->nmcd.dwItemSpec;
			CString chip = alarmLog.GetItemText(row, 0);
			CString s = alarmLog.GetItemText(row, 3);
			bool check = (chip == _T("위험") || chip == _T("경고"));

			if (!check) {
				CDC* pDC = CDC::FromHandle(pLV->nmcd.hdc);
				CRect rc;
				alarmLog.GetSubItemRect(row, 3, LVIR_BOUNDS, rc);
				pDC->FillSolidRect(rc, m_clrPanel);

				*pResult = CDRF_SKIPDEFAULT;
			} 
			else if (s == _T("확인"))                 // 아직 미확인: 버튼 모양으로 직접 그림
			{
				CDC* pDC = CDC::FromHandle(pLV->nmcd.hdc);
				CRect rc;
				alarmLog.GetSubItemRect(row, 3, LVIR_BOUNDS, rc);
				pDC->FillSolidRect(rc, m_clrPanel);

				CRect btn(rc.CenterPoint().x - 30, rc.CenterPoint().y - 14, rc.CenterPoint().x + 30, rc.CenterPoint().y + 14);

				CBrush br(RGB(0x22, 0x30, 0x49));
				CPen pen(PS_SOLID, 1, RGB(0x3A, 0x4A, 0x63));
				CBrush* pOldBr = pDC->SelectObject(&br);
				CPen* pOldPen = pDC->SelectObject(&pen);
				pDC->RoundRect(btn, CPoint(8, 8));

				CFont* pOldFont = pDC->SelectObject(alarmLog.GetFont());
				pDC->SetBkMode(TRANSPARENT);
				pDC->SetTextColor(m_clrText);
				pDC->DrawText(s, btn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				pDC->SelectObject(pOldFont);
				pDC->SelectObject(pOldPen);
				pDC->SelectObject(pOldBr);

				*pResult = CDRF_SKIPDEFAULT;
			}
			else if (s == _T("확인됨"))
			{
				pLV->clrText = RGB(0x3D, 0xDC, 0x84);
			}
		}
		break;
	}
}

void CProjectMFCDlg::OnNMClickLoglist(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	auto* p = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
	*pResult = 0;

	LVHITTESTINFO ht = {};
	ht.pt = p->ptAction;
	alarmLog.SubItemHitTest(&ht);

	if (ht.iItem >= 0 && ht.iSubItem == 3)						// 3번 칸 = 확인 칸
	{
		CString chip = alarmLog.GetItemText(ht.iItem, 0);
		if (chip != _T("위험") && chip != _T("경고")) return;

		if (alarmLog.GetItemText(ht.iItem, 3) == _T("확인"))
		{
			alarmLog.SetItemText(ht.iItem, 3, _T("확인됨"));   // 처리 후 글자만 바꿈
			// 여기서 알람 확인 처리
		}
	}
	*pResult = 0;
}

void CProjectMFCDlg::AddAlarm(LPCTSTR level, const CString& msg) {
	const int MAX_ALARM = 100;

	CString time = CTime::GetCurrentTime().Format(_T("[ %H:%M:%S ]"));

	int r = alarmLog.InsertItem(0, level);				// 최신 알람이 맨 위
	alarmLog.SetItemText(r, 1, time);
	alarmLog.SetItemText(r, 2, msg);
	alarmLog.SetItemText(r, 3, _T("확인"));

	while (alarmLog.GetItemCount() > MAX_ALARM)			// 오래된 것부터 삭제
		alarmLog.DeleteItem(alarmLog.GetItemCount() - 1);
}