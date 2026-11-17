#include "pch.h"
#include "framework.h"
#include "Project_MFC.h"
#include "Project_MFCDlg.h"
#include "Defs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 경과 시간은 PLC가 MFC_HR[51]~[56]에 배터리 ID별로 넣어 줌 (초)
//   빌려 간 배터리만 0보다 큰 값, 슬롯에 꽂혀 있는 배터리는 0
//   → 대여 중 여부와 시간은 이 값만 보고 판단
// 알림 로그는 PLC의 대여·반납 횟수(MFC_HR[13], [15])가 바뀌는 순간에 남김
static WORD s_rentSec[BATTERY_COUNT + 1] = {};		// 이번에 받은 경과 시간 (초)
static WORD s_lastSec[BATTERY_COUNT + 1] = {};		// 0이 되기 직전 마지막 경과 시간 (반납 로그용)
static WORD s_prevRentSeq = 0;						// 직전에 읽은 대여 횟수
static WORD s_prevReturnSeq = 0;					// 직전에 읽은 반납 횟수
static bool s_firstRead = true;

// 오늘 대여 / 반납 수 (하루 단위, 레지스트리에 저장해서 재시작해도 유지)
static int s_todayRent = 0, s_todayReturn = 0;
static int s_today = 0;								// 기준 날짜 (yyyymmdd, 0 = 아직 안 불러옴)

static int TodayKey() {
	CTime t = CTime::GetCurrentTime();
	return t.GetYear() * 10000 + t.GetMonth() * 100 + t.GetDay();
}

static void SaveTodayCount() {
	CWinApp* app = AfxGetApp();
	app->WriteProfileInt(_T("Count"), _T("Date"), s_today);
	app->WriteProfileInt(_T("Count"), _T("Rent"), s_todayRent);
	app->WriteProfileInt(_T("Count"), _T("Return"), s_todayReturn);
}

// 초 → "[ 시 : 분 : 초 ]"
static CString FormatSec(int sec) {
	CString str;
	str.Format(_T("[ %02d : %02d : %02d ]"), sec / 3600, (sec % 3600) / 60, sec % 60);
	return str;
}

// 타이머 로그
void CProjectMFCDlg::TimeCntList() {
	timeCount.SetBkColor(m_clrPanel);
	timeCount.SetTextBkColor(m_clrPanel);
	timeCount.SetTextColor(m_clrText);
	SetWindowTheme(timeCount.GetSafeHwnd(), L"", L"");

	timeCount.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	timeCount.InsertColumn(0, _T(""), LVCFMT_LEFT, 0);
	timeCount.InsertColumn(1, _T("배터리 ID"), LVCFMT_CENTER, 120);
	timeCount.InsertColumn(2, _T("경과 시간"), LVCFMT_CENTER, 150);

	int r1 = timeCount.InsertItem(0, _T(""));
	timeCount.SetItemText(r1, 1, _T("pika_B01"));
	timeCount.SetItemText(r1, 2, _T("[ 00 : 00 : 00 ]"));

	int r2 = timeCount.InsertItem(1, _T(""));
	timeCount.SetItemText(r2, 1, _T("pika_B02"));
	timeCount.SetItemText(r2, 2, _T("[ 00 : 00 : 00 ]"));

	int r3 = timeCount.InsertItem(2, _T(""));
	timeCount.SetItemText(r3, 1, _T("pika_B03"));
	timeCount.SetItemText(r3, 2, _T("[ 00 : 00 : 00 ]"));

	int r4 = timeCount.InsertItem(3, _T(""));
	timeCount.SetItemText(r4, 1, _T("pika_B04"));
	timeCount.SetItemText(r4, 2, _T("[ 00 : 00 : 00 ]"));

	int r5 = timeCount.InsertItem(4, _T(""));
	timeCount.SetItemText(r5, 1, _T("pika_B05"));
	timeCount.SetItemText(r5, 2, _T("[ 00 : 00 : 00 ]"));

	int r6 = timeCount.InsertItem(5, _T(""));
	timeCount.SetItemText(r6, 1, _T("pika_B06"));
	timeCount.SetItemText(r6, 2, _T("[ 00 : 00 : 00 ]"));
}


void CProjectMFCDlg::OnCustomTimer(NMHDR* pNMHDR, LRESULT* pResult)
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
		break;
	}
}

// 대여 경과 시간 표시 (PLC 값이 0이면 00:00:00)
void CProjectMFCDlg::CountTimer(WORD id) {
	if (id < 1 || id > BATTERY_COUNT) return;

	int row = id - 1;
	CString str = FormatSec(s_rentSec[id]);

	if (timeCount.GetItemText(row, 2) != str)
		timeCount.SetItemText(row, 2, str);
}

// PLC 경과 시간 받기 + 대여·반납 횟수 변화로 알림 로그
void CProjectMFCDlg::CheckRentChange(const WORD* reg) {
	// 오늘 대여 / 반납 : 날짜 확인 + 화면 표시
	if (s_today == 0) {												// 처음 한 번 : 저장된 값 불러오기
		CWinApp* app = AfxGetApp();
		s_today = app->GetProfileInt(_T("Count"), _T("Date"), 0);
		s_todayRent = app->GetProfileInt(_T("Count"), _T("Rent"), 0);
		s_todayReturn = app->GetProfileInt(_T("Count"), _T("Return"), 0);
	}
	int today = TodayKey();
	if (today != s_today) {											// 날짜가 바뀌면 0부터
		s_today = today;
		s_todayRent = 0;
		s_todayReturn = 0;
		SaveTodayCount();
	}

	auto show = [](CStatic& wnd, int v) {
		CString s, cur;
		s.Format(_T("%d"), v);
		wnd.GetWindowText(cur);
		if (cur != s) wnd.SetWindowText(s);
		};
	show(CRental, s_todayRent);										// 오늘 대여 패널
	show(CReturn, s_todayReturn);									// 오늘 반납 패널

	// 반납 로그용 : 0이 되기 전 마지막 값 보관
	for (WORD id = 1; id <= BATTERY_COUNT; id++) {
		WORD sec = reg[RENT_SEC_IDX + id - 1];
		if (sec > 0) s_lastSec[id] = sec;
		s_rentSec[id] = sec;
	}

	WORD rentSeq = reg[RENT_SEQ_IDX];
	WORD returnSeq = reg[RETURN_SEQ_IDX];

	if (s_firstRead) {									// 프로그램 시작 시 : 횟수는 기준으로만 저장
		s_prevRentSeq = rentSeq;
		s_prevReturnSeq = returnSeq;
		s_firstRead = false;
		return;
	}

	// 대여 완료 : 대여 횟수가 바뀜
	if (rentSeq != s_prevRentSeq) {
		if (rentSeq > s_prevRentSeq) {								// 늘어난 만큼 더함 (PLC 재시작으로 줄어든 경우는 제외)
			s_todayRent += rentSeq - s_prevRentSeq;
			SaveTodayCount();
		}

		WORD id = reg[RENT_BATID_IDX];
		WORD slot = reg[RENT_SLOT_IDX];

		CString logLine;
		if (id >= 1 && id <= BATTERY_COUNT) {
			logLine.Format(_T("[대여] %s 대여 시작 (슬롯 %d)"),
				timeCount.GetItemText(id - 1, 1).GetString(), slot);
			AddAlarm(_T("정보"), logLine);
		}
		else {
			logLine.Format(_T("[대여 오류] 배터리 ID 확인 불가 (슬롯 %d, ID %d)"), slot, id);
			AddAlarm(_T("경고"), logLine);
		}
		s_prevRentSeq = rentSeq;
	}

	// 반납 완료 : 반납 횟수가 바뀜 → 반납 슬롯의 배터리 ID로 마지막 대여 시간 기록
	if (returnSeq != s_prevReturnSeq) {
		if (returnSeq > s_prevReturnSeq) {
			s_todayReturn += returnSeq - s_prevReturnSeq;
			SaveTodayCount();
		}

		WORD slot = reg[RETURN_SLOT_IDX];
		WORD id = (slot >= SLOT_FIRST && slot <= SLOT_LAST) ? GetSlotBatteryId(reg, slot) : 0;

		CString logLine;
		if (id >= 1 && id <= BATTERY_COUNT) {
			logLine.Format(_T("[반납] %s / 대여 시간 : %s"),
				timeCount.GetItemText(id - 1, 1).GetString(),
				FormatSec(s_lastSec[id]).GetString());
			AddAlarm(_T("정보"), logLine);

			s_lastSec[id] = 0;
		}
		else {
			logLine.Format(_T("[반납 오류] 배터리 ID 확인 불가 (슬롯 %d)"), slot);
			AddAlarm(_T("경고"), logLine);
		}
		s_prevReturnSeq = returnSeq;
	}
}

// 경과 시간 표시 갱신 (OnTimer에서 호출)
void CProjectMFCDlg::UpdateRentTimers() {
	for (WORD id = 1; id <= BATTERY_COUNT; id++)
		CountTimer(id);
}

// 대여 중인 배터리인지 확인 (PLC 경과 시간이 0보다 크면 대여 중)
bool CProjectMFCDlg::IsRenting(WORD id) {
	return (id >= 1 && id <= BATTERY_COUNT && s_rentSec[id] > 0);
}