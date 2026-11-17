#include "pch.h"
#include "DarkHeader.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(DarkHeader, CHeaderCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_MESSAGE(HDM_LAYOUT, &DarkHeader::OnLayout)
END_MESSAGE_MAP()

void DarkHeader::OnPaint()
{
	CPaintDC dc(this);
	CRect rc; GetClientRect(&rc);
	dc.FillSolidRect(rc, RGB(0x22, 0x30, 0x49));            // 머리글 배경

	CFont* pOld = dc.SelectObject(GetFont());
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(RGB(0xA9, 0xB4, 0xC4));                 // 머리글 글자색

	for (int i = 0; i < GetItemCount(); i++)
	{
		CRect r; GetItemRect(i, &r);
		dc.FillSolidRect(r.right - 1, r.top + 6, 1, r.Height() - 12, RGB(0x3A, 0x4A, 0x63));   // 칸 구분선

		TCHAR buf[128] = {};
		HDITEM hi = {};
		hi.mask = HDI_TEXT | HDI_FORMAT;
		hi.pszText = buf; hi.cchTextMax = 127;
		GetItem(i, &hi);

		UINT fmt = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_CENTER;
		switch (hi.fmt & HDF_JUSTIFYMASK) {
		case HDF_CENTER: fmt |= DT_CENTER; break;
		case HDF_RIGHT:  fmt |= DT_RIGHT;  break;
		default:         fmt |= DT_LEFT;   break;
		}
		r.DeflateRect(8, 0, 4, 0);
		dc.DrawText(buf, r, fmt);
	}
	dc.SelectObject(pOld);
}

// 열 머리글 두께 정의
LRESULT DarkHeader::OnLayout(WPARAM wParam, LPARAM lParam)
{
	LRESULT res = DarkHeader::DefWindowProc(HDM_LAYOUT, wParam, lParam);
	HDLAYOUT* p = reinterpret_cast<HDLAYOUT*>(lParam);
	const int kHeight = 28;            // 원하는 머리글 높이(px)
	p->pwpos->cy = kHeight;
	p->prc->top = kHeight;            // 리스트 본문이 헤더 아래에서 시작하게 함
	return res;
}