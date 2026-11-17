#include "pch.h"
#include "framework.h"
#include "Project_MFC.h"
#include "Project_MFCDlg.h"
#include "Defs.h"
#include <cmath>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 단계 이름 (단계 표시와 현재 단계 패널이 같이 사용)
static const LPCTSTR kRentSteps[6] = { _T("대기"), _T("슬롯 이동"), _T("파지"), _T("수령구 이동"), _T("배출"), _T("완료") };
static const LPCTSTR kReturnSteps[6] = { _T("대기"), _T("반납구 이동"), _T("파지"), _T("슬롯 이동"), _T("삽입"), _T("완료") };
static const LPCTSTR kErrSteps[6] = { _T("대기"), _T("슬롯 이동"), _T("파지"), _T("수거함 이동"), _T("배출"), _T("완료") };

static const ULONGLONG DONE_HOLD_MS = 3000;			// 완료 단계를 화면에 보여 줄 시간
static ULONGLONG s_doneTick = 0;					// 완료 단계를 처음 받은 시각

// 로봇 작업 단계 표시 : 1~3단계 윗줄(왼→오), 파지 다음 반원으로 꺾어 4~6단계 아랫줄(오→왼)
void CProjectMFCDlg::DrawRobotSteps(CDC* pDC) {
	using namespace Gdiplus;

	const LPCTSTR* names = (robotKind == 2) ? kReturnSteps : (robotKind == 3) ? kErrSteps : kRentSteps;

	const COLORREF gOk = RGB(0x3D, 0xDC, 0x84), gWarn = RGB(0xF5, 0xB9, 0x42);
	const COLORREF gMuted = RGB(0xA9, 0xB4, 0xC4), gOff = RGB(0x6B, 0x77, 0x8A);

	CRect area; GetDlgItem(IDC_STEPS)->GetWindowRect(&area); ScreenToClient(&area);

	const int N = 6, R = 22, PAD = 60;
	const int H = 150;										// 윗줄과 아랫줄 간격 (= 꺾이는 반원의 지름)
	bool idle = (robotKind == 0 || robotStep == 0);
	bool completed = (robotStep == N);
	int cur = (idle || completed) ? -1 : robotStep - 1;		// 진행 중인 단계 (0부터, 없으면 -1)
	auto isDone = [&](int i) { return completed || (cur >= 0 && i < cur); };

	// 3칸 × 2줄, 오른쪽에 반원 자리를 남김
	const float MAX_GAP = 200.0f;									// 원 중심 사이 최대 간격
	const int ARC_W = 220;											// 꺾이는 곡선의 폭
	float fit = (area.Width() - 2.0f * PAD - ARC_W / 2.0f) / 2;		// 영역에 꽉 채웠을 때 간격
	float gap = (fit < MAX_GAP) ? fit : MAX_GAP;
	float drawW = gap * 2 + ARC_W / 2.0f;
	float left = area.left + (area.Width() - drawW) / 2.0f;			// 영역 안에서 가로 가운데 정렬
	int content = (R + 8) + H + (R + 38);							// 윗줄 원 위 ~ 아랫줄 글자 아래
	int cy0 = area.top + (area.Height() - content) / 2 + R + 8;		// 윗줄 원 중심
	int cy1 = cy0 + H;												// 아랫줄 원 중심

	auto posOf = [&](int i) {
		int row = i / 3, col = i % 3;
		if (row == 1) col = 2 - col;						// 아랫줄은 오른쪽부터
		return CPoint((int)(left + gap * col), row == 0 ? cy0 : cy1);
		};

	// 선 + 원 + 체크 (GDI+ 안티앨리어싱)
	{
		Graphics g(pDC->GetSafeHdc());
		g.SetSmoothingMode(SmoothingModeAntiAlias);
		g.SetPixelOffsetMode(PixelOffsetModeHalf);

		Color cOk(255, 0x3D, 0xDC, 0x84), cOkBg(255, 0x12, 0x36, 0x1F);
		Color cWarn(255, 0xF5, 0xB9, 0x42), cWarnBg(255, 0x4A, 0x3A, 0x12), cHalo(45, 0xF5, 0xB9, 0x42);
		Color cTodo(255, 0x3A, 0x4A, 0x63);
		Color cPanel(255, GetRValue(m_clrPanel), GetGValue(m_clrPanel), GetBValue(m_clrPanel));

		for (int i = 0; i < N - 1; i++) {					// 연결선
			Pen line(isDone(i + 1) || i + 1 == cur ? cOk : cTodo, 4.0f);
			line.SetStartCap(LineCapRound); line.SetEndCap(LineCapRound);
			CPoint a = posOf(i), b = posOf(i + 1);

			if (i == 2) {										// 파지 → 4단계
				const float ecx = (float)a.x;					// 타원 중심 x
				const float ecy = cy0 + H / 2.0f;				// 타원 중심 y
				const float A = ARC_W / 2.0f, B = H / 2.0f;		// 가로 반지름, 세로 반지름
				const float margin = (float)(R + 8);			// 직선과 같은 여백

				PointF pts[181];
				int n = 0;
				for (int deg = -90; deg <= 90; deg++) {
					float t = deg * 3.14159265f / 180.0f;
					float x = ecx + A * cosf(t);
					float y = ecy + B * sinf(t);
					bool nearTop = hypotf(x - a.x, y - a.y) < margin;		// 3단계 원 근처
					bool nearBottom = hypotf(x - b.x, y - b.y) < margin;	// 4단계 원 근처
					if (!nearTop && !nearBottom) pts[n++] = PointF(x, y);
				}
				line.SetLineJoin(LineJoinRound);
				if (n >= 2) g.DrawLines(&line, pts, n);
			}
			else {											// 같은 줄 : 직선 (아랫줄은 오른쪽 → 왼쪽)
				int d = (b.x > a.x) ? 1 : -1;
				g.DrawLine(&line, a.x + d * (R + 8), a.y, b.x - d * (R + 8), b.y);
			}
		}

		for (int i = 0; i < N; i++) {						// 원 (선 위에 그려서 선 끝을 덮음)
			CPoint p = posOf(i);
			Color fill = cPanel, edge = cTodo;
			if (isDone(i)) { fill = cOkBg; edge = cOk; }
			else if (i == cur) {
				SolidBrush halo(cHalo);						// 진행 중인 단계만 후광
				g.FillEllipse(&halo, p.x - R - 8, p.y - R - 8, 2 * (R + 8), 2 * (R + 8));
				fill = cWarnBg; edge = cWarn;
			}
			SolidBrush br(fill);
			Pen pen(edge, 2.5f);
			g.FillEllipse(&br, p.x - R, p.y - R, 2 * R, 2 * R);
			g.DrawEllipse(&pen, p.x - R, p.y - R, 2 * R, 2 * R);

			if (isDone(i)) {								// 체크 표시
				Pen chk(cOk, 3.0f);
				chk.SetStartCap(LineCapRound); chk.SetEndCap(LineCapRound); chk.SetLineJoin(LineJoinRound);
				Point pts[3] = { Point(p.x - 8, p.y), Point(p.x - 2, p.y + 6), Point(p.x + 9, p.y - 6) };
				g.DrawLines(&chk, pts, 3);
			}
		}
	}	// Graphics가 여기서 끝나야 아래 GDI 글자가 위에 올라감

	pDC->SetBkMode(TRANSPARENT);
	CFont* pOldFont = pDC->SelectObject(&m_status);

	// 원 안 숫자 + 단계 이름
	for (int i = 0; i < N; i++) {
		CPoint p = posOf(i);
		bool done = isDone(i), now = (i == cur);

		if (!done) {										// 숫자 : 글자 폭을 재서 정확히 가운데
			pDC->SelectObject(now ? &m_header : &m_status);
			pDC->SetTextColor(now ? gWarn : gOff);
			CString num; num.Format(_T("%d"), i + 1);
			CSize sz = pDC->GetTextExtent(num);
			pDC->TextOut(p.x - sz.cx / 2, p.y - sz.cy / 2, num);
		}

		pDC->SelectObject(now ? &m_header : &m_status);		// 진행 중인 단계 이름만 굵게
		pDC->SetTextColor(done ? gOk : now ? gWarn : gMuted);
		CRect label(p.x - (int)gap / 2, p.y + R + 10, p.x + (int)gap / 2, p.y + R + 36);
		pDC->DrawText(names[i], label, DT_CENTER | DT_TOP | DT_SINGLELINE);
	}

	pDC->SelectObject(pOldFont);
}

// 현재 단계 패널 글자 갱신
void CProjectMFCDlg::UpdateStepText() {
	const LPCTSTR* names = (robotKind == 2) ? kReturnSteps : (robotKind == 3) ? kErrSteps : kRentSteps;
	CString kind = (robotKind == 2) ? _T("반납") : (robotKind == 3) ? _T("고장 회수") : _T("대여");
	m_clrStepName = RGB(0xA9, 0xB4, 0xC4);					// 회색

	CString name, info;
	if (robotKind == 0 || robotStep == 0) {
		name = _T("대기 중");
		info = _T("");
	}
	else if (robotStep == 6) {
		name = _T("작업 완료");
		info.Format(_T("%s · 6 / 6 단계"), kind.GetString());
	}
	else {
		name = names[robotStep - 1];
		info.Format(_T("%s · %d / 6 단계"), kind.GetString(), robotStep);
	}

	SetDlgItemText(IDC_STEPNAME, name);
	SetDlgItemText(IDC_STEPINFO, info);
	GetDlgItem(IDC_STEPNAME)->Invalidate();
}

// 로봇 작업 상태 글자 갱신 (제목 옆)
void CProjectMFCDlg::UpdateRobotChip() {
	CString text;

	if (robotKind == 0) {
		text = _T("작업 대기");
		m_clrBotState = RGB(0xA9, 0xB4, 0xC4);				// 회색
	}
	else {
		CString kind = (robotKind == 2) ? _T("반납") : (robotKind == 3) ? _T("회수") : _T("대여");

		if (robotStep == 6) {
			text = kind + _T(" 작업");
			m_clrBotState = RGB(0xA9, 0xB4, 0xC4);			// 회색
			//m_clrBotState = RGB(0x3D, 0xDC, 0x84);			// 초록
		}
		else {
			text = kind + _T(" 작업 중");
			m_clrBotState = RGB(0x3D, 0xDC, 0x84);			// 초록
		}
	}

	SetDlgItemText(IDC_botStatus, text);
	GetDlgItem(IDC_botStatus)->Invalidate();				// 색만 바뀌어도 다시 그리기
}

// PLC에서 받은 로봇 작업 상태 반영
void CProjectMFCDlg::UpdateRobotStatus(const WORD* reg) {
	int kind = reg[ROBOT_KIND_IDX];
	int step = reg[ROBOT_STEP_IDX];

	if (kind > 3) kind = 0;						// 0 대기 / 1 대여 / 2 반납 / 3 고장 회수
	if (step > 6) step = 0;
	if (kind == 0) step = 0;

	// 완료(6단계)는 잠깐만 보여 주고 화면은 대기로 (PLC 값은 그대로 둠)
	if (step == 6) {
		if (s_doneTick == 0) s_doneTick = GetTickCount64();
		else if (GetTickCount64() - s_doneTick >= DONE_HOLD_MS) { kind = 0; step = 0; }
	}
	else {
		s_doneTick = 0;							// 다음 작업이 시작되면 다시 셀 준비
	}

	if (kind != robotKind || step != robotStep) {
		robotKind = kind;
		robotStep = step;
		InvalidateCtrl(IDC_STEPS);
		UpdateRobotChip();
		UpdateStepText();
	}
}

void CProjectMFCDlg::InvalidateCtrl(UINT id) {
	CRect rc;
	GetDlgItem(id)->GetWindowRect(&rc);
	ScreenToClient(&rc);
	InvalidateRect(rc, TRUE);
}