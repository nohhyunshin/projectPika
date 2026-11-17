#include "pch.h"
#include "framework.h"
#include "Project_MFC.h"
#include "Project_MFCDlg.h"
#include "Defs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static WORD s_prevStatus[SLOT_COUNT] = {};

// 오늘 불량 수 (하루 단위, 레지스트리에 저장해서 재시작해도 유지)
static int  s_todayFaulty = 0;
static int  s_faultyDate = 0;			// 기준 날짜 (yyyymmdd, 0 = 아직 안 불러옴)
static bool s_faultyFirst = true;		// MFC 켜고 처음 읽는 중인지

static int FaultyTodayKey() {
	CTime t = CTime::GetCurrentTime();
	return t.GetYear() * 10000 + t.GetMonth() * 100 + t.GetDay();
}

static void SaveFaultyCount() {
	CWinApp* app = AfxGetApp();
	app->WriteProfileInt(_T("Count"), _T("FaultyDate"), s_faultyDate);
	app->WriteProfileInt(_T("Count"), _T("Faulty"), s_todayFaulty);
}

// 배터리 상태 로그
void CProjectMFCDlg::BatteryLogList() {
	batteryLog.SetBkColor(m_clrPanel);
	batteryLog.SetTextBkColor(m_clrPanel);
	batteryLog.SetTextColor(m_clrText);
	SetWindowTheme(batteryLog.GetSafeHwnd(), L"", L"");

	batteryLog.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	batteryLog.InsertColumn(0, _T(""), LVCFMT_LEFT, 0);
	batteryLog.InsertColumn(1, _T("슬롯"), LVCFMT_CENTER, 70);
	batteryLog.InsertColumn(2, _T("배터리 ID"), LVCFMT_CENTER, 120);
	batteryLog.InsertColumn(3, _T("위치"), LVCFMT_CENTER, 110);
	batteryLog.InsertColumn(4, _T("상태"), LVCFMT_LEFT, 110);
	batteryLog.InsertColumn(5, _T("잔여 전력량"), LVCFMT_CENTER, 200);
	batteryLog.InsertColumn(6, _T("HMI 표시"), LVCFMT_CENTER, 110);

	int r1 = batteryLog.InsertItem(0, _T(""));
	batteryLog.SetItemText(r1, 1, _T("1"));
	batteryLog.SetItemText(r1, 2, _T(""));
	batteryLog.SetItemText(r1, 3, _T("충전함"));
	batteryLog.SetItemText(r1, 4, _T("충전 완료"));
	batteryLog.SetItemText(r1, 5, _T("92%"));
	batteryLog.SetItemText(r1, 6, _T("즉시 대여"));
	batteryLog.SetItemText(r1, 7, _T("[ 00 : 00 ]"));

	int r2 = batteryLog.InsertItem(1, _T(""));
	batteryLog.SetItemText(r2, 1, _T("2"));
	batteryLog.SetItemText(r2, 2, _T(""));
	batteryLog.SetItemText(r2, 3, _T("충전함"));
	batteryLog.SetItemText(r2, 4, _T("충전 중"));
	batteryLog.SetItemText(r2, 5, _T("40%"));
	batteryLog.SetItemText(r2, 6, _T("조건부 대여"));
	batteryLog.SetItemText(r2, 7, _T("[ 00 : 00 ]"));

	int r3 = batteryLog.InsertItem(2, _T(""));
	batteryLog.SetItemText(r3, 1, _T("3"));
	batteryLog.SetItemText(r3, 2, _T(""));
	batteryLog.SetItemText(r3, 3, _T("충전함"));
	batteryLog.SetItemText(r3, 4, _T("불량"));
	batteryLog.SetItemText(r3, 5, _T(""));
	batteryLog.SetItemText(r3, 6, _T("표시 안 함"));
	batteryLog.SetItemText(r3, 7, _T("[ 00 : 00 ]"));

	int r4 = batteryLog.InsertItem(3, _T(""));
	batteryLog.SetItemText(r4, 1, _T("4"));
	batteryLog.SetItemText(r4, 2, _T(""));
	batteryLog.SetItemText(r4, 3, _T("충전함"));
	batteryLog.SetItemText(r4, 4, _T("충전 완료"));
	batteryLog.SetItemText(r4, 5, _T("88%"));
	batteryLog.SetItemText(r4, 6, _T("즉시 대여"));
	batteryLog.SetItemText(r4, 7, _T("[ 00 : 00 ]"));

	int r5 = batteryLog.InsertItem(4, _T(""));
	batteryLog.SetItemText(r5, 1, _T("5"));
	batteryLog.SetItemText(r5, 2, _T(""));
	batteryLog.SetItemText(r5, 3, _T("충전함"));
	batteryLog.SetItemText(r5, 4, _T("충전 중"));
	batteryLog.SetItemText(r5, 5, _T("37%"));
	batteryLog.SetItemText(r5, 6, _T("조건부 대여"));
	batteryLog.SetItemText(r5, 7, _T("[ 00 : 00 ]"));

	int r6 = batteryLog.InsertItem(5, _T(""));
	batteryLog.SetItemText(r6, 1, _T("6"));
	batteryLog.SetItemText(r6, 2, _T(""));
	batteryLog.SetItemText(r6, 3, _T("충전함"));
	batteryLog.SetItemText(r6, 4, _T("비어 있음"));
	batteryLog.SetItemText(r6, 5, _T(""));
	batteryLog.SetItemText(r6, 6, _T("표시 안 함"));
	batteryLog.SetItemText(r6, 7, _T("[ 01 : 30 ]"));
}

void CProjectMFCDlg::OnDrawBattery(NMHDR* pNMHDR, LRESULT* pResult)
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
		if (pLV->iSubItem == 4)									// 상태 열
		{
			int row = (int)pLV->nmcd.dwItemSpec;
			CString s = batteryLog.GetItemText(row, 4);

			COLORREF bg = RGB(0x2A, 0x35, 0x47), fg = RGB(0xA9, 0xB4, 0xC4);
			if (s == _T("충전 완료")) { bg = RGB(0x12, 0x36, 0x1F); fg = RGB(0x3D, 0xDC, 0x84); }
			else if (s == _T("충전 중")) { bg = RGB(0x4A, 0x3A, 0x12); fg = RGB(0xF5, 0xB9, 0x42); }
			else if (s == _T("불량")) { bg = RGB(0x4A, 0x1D, 0x1B); fg = RGB(0xFF, 0x8A, 0x83); }

			CDC* pDC = CDC::FromHandle(pLV->nmcd.hdc);
			CRect rc;
			batteryLog.GetSubItemRect(row, 4, LVIR_LABEL, rc);
			pDC->FillSolidRect(rc, m_clrPanel);

			CFont* pOldFont = pDC->SelectObject(batteryLog.GetFont());
			CSize sz = pDC->GetTextExtent(s);
			CRect chip(rc.left + 8, rc.CenterPoint().y - 12, rc.left + 8 + sz.cx + 20, rc.CenterPoint().y + 12);

			CBrush br(bg);
			CBrush* pOldBr = pDC->SelectObject(&br);
			CPen* pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
			pDC->RoundRect(chip, CPoint(4, 4));					// border radius 6px

			pDC->SetBkMode(TRANSPARENT);
			pDC->SetTextColor(fg);
			pDC->DrawText(s, chip, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBr);
			pDC->SelectObject(pOldFont);

			*pResult = CDRF_SKIPDEFAULT;						// 기본 그리기 생략
		}
		else if (pLV->iSubItem == 5) {							// 잔여 전력량 progress bar
			int row = (int)pLV->nmcd.dwItemSpec;
			CString s = batteryLog.GetItemText(row, 5);
			int pct = _ttoi(s);

			CDC* pDC = CDC::FromHandle(pLV->nmcd.hdc);
			CRect rc;
			batteryLog.GetSubItemRect(row, 5, LVIR_LABEL, rc);
			pDC->FillSolidRect(rc, m_clrPanel);

			if (!s.IsEmpty()) {
				COLORREF fill = (pct >= 60) ? RGB(0x3D, 0xDC, 0x84) : RGB(0xF5, 0xB9, 0x42);

				CRect track(rc.left + 15, rc.CenterPoint().y - 4, rc.left + 15 + 120, rc.CenterPoint().y + 5);
				CRect bar = track;
				bar.right = track.left + track.Width() * pct / 100;

				CPen* pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);

				CBrush brTrack(RGB(0x2A, 0x35, 0x47));
				CBrush* pOldBr = pDC->SelectObject(&brTrack);
				pDC->RoundRect(track, CPoint(5, 5));

				if (pct > 0)
				{
					CBrush brFill(fill);
					pDC->SelectObject(&brFill);
					pDC->RoundRect(bar, CPoint(5, 5));
				}

				// 퍼센트 글자
				CRect txt(track.right + 8, rc.top, rc.right, rc.bottom);
				pDC->SelectObject(batteryLog.GetFont());
				pDC->SetBkMode(TRANSPARENT);
				pDC->SetTextColor(m_clrText);
				pDC->DrawText(s, txt, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_CENTER);

				pDC->SelectObject(pOldBr);
				pDC->SelectObject(pOldPen);
			}
			*pResult = CDRF_SKIPDEFAULT;					// 기본 그리기 생략
		}
		else if (pLV->iSubItem == 6) {						// HMI 표시 데이터 열
			int row = (int)pLV->nmcd.dwItemSpec;
			CString s = batteryLog.GetItemText(row, 6);

			if (s == _T("즉시 대여")) pLV->clrText = RGB(0x3D, 0xDC, 0x84);
			else if (s == _T("조건부 대여")) pLV->clrText = RGB(0xF5, 0xB9, 0x42);
			else pLV->clrText = RGB(0xA9, 0xB4, 0xC4);
		}
		break;
	}
}

// 슬롯 ST[1] ~ ST[6]에 꽂힌 배터리 ID (빈방이면 0)
WORD CProjectMFCDlg::GetSlotBatteryId(const WORD* reg, int s) {
	if (reg[SLOT_REG(s, SLOT_STATUS_OFS)] == SLOT_EMPTY) return 0;
	return reg[BATTERY_ID_IDX + s - 1];
}

void CProjectMFCDlg::UpdateSlotTable(const WORD* reg) {
	auto setIfChanged = [](CListCtrl& lc, int row, int col, const CString& text) {
		if (lc.GetItemText(row, col) != text) lc.SetItemText(row, col, text);
		};
	auto setWndIfChanged = [](CWnd& wnd, int value) {
		CString str, cur;
		str.Format(_T("%d"), value);
		wnd.GetWindowText(cur);
		if (cur != str) wnd.SetWindowText(str);
		};

	// 오늘 불량 수 : 처음 한 번 레지스트리에서 불러오고, 날짜가 바뀌면 0으로
	int today = FaultyTodayKey();
	if (s_faultyDate == 0) {
		CWinApp* app = AfxGetApp();
		int saved = app->GetProfileInt(_T("Count"), _T("FaultyDate"), 0);
		s_todayFaulty = (saved == today) ? app->GetProfileInt(_T("Count"), _T("Faulty"), 0) : 0;
		s_faultyDate = today;
	}
	else if (s_faultyDate != today) {
		s_faultyDate = today;
		s_todayFaulty = 0;
		SaveFaultyCount();
	}

	int rentCnt = 0, chargeCnt = 0;

	for (int s = SLOT_FIRST; s <= SLOT_LAST; s++) {				// ST[1]~[6]
		int  row = s - 1;										// 슬롯 1 = 행 0
		WORD status = reg[SLOT_REG(s, SLOT_STATUS_OFS)];
		WORD soc = reg[SLOT_REG(s, SLOT_SOC_OFS)];
		WORD batId = GetSlotBatteryId(reg, s);
		if (soc == SOC_FAULT) status = SLOT_FAULTY;

		if (status == SLOT_FAULTY && s_prevStatus[s] != SLOT_FAULTY) {
			CString msg;
			msg.Format(_T("pika-B%02d 배터리 불량 감지"), batId);
			AddAlarm(_T("경고"), msg);

			if (!s_faultyFirst) {								// 켜자마자 이미 불량인 건 다시 세지 않음
				s_todayFaulty++;
				SaveFaultyCount();
			}
		}
		s_prevStatus[s] = status;

		if (IsRenting(batId)) { status = SLOT_EMPTY; batId = 0; }

		CString name, state, socStr, hmi;
		if (batId != 0) name.Format(_T("pika-B%02d"), batId);

		if (status == SLOT_FAULTY) {										// 2 : 불량
			state = _T("불량");
			hmi = _T("표시 안 함");
		}
		else if (status == SLOT_CHARGING) {									// 1 : 충전 중
			socStr.Format(_T("%d%%"), soc);
			state = (soc >= 100) ? _T("충전 완료") : _T("충전 중");
			if (soc >= RENT_PCT_MIN) { hmi = _T("즉시 대여");   rentCnt++; }
			else { hmi = _T("조건부 대여"); chargeCnt++; }
		}
		else {																// 0 : 비어 있음
			state = _T("비어 있음");
			hmi = _T("표시 안 함");
		}

		setIfChanged(batteryLog, row, 2, name);						// 배터리 ID
		setIfChanged(batteryLog, row, 4, state);					// 상태
		setIfChanged(batteryLog, row, 5, socStr);					// 잔여 전력량
		setIfChanged(batteryLog, row, 6, hmi);						// HMI 표시
	}
	s_faultyFirst = false;

	setWndIfChanged(CRent, rentCnt);
	setWndIfChanged(CCharging, chargeCnt);
	setWndIfChanged(CFaulty, s_todayFaulty);					// 불량 수거 : 오늘 누적
}