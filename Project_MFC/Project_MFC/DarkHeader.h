// 리스트 컨트롤 열 머리글 테마

#pragma once
class DarkHeader : public CHeaderCtrl
{
protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
	afx_msg LRESULT OnLayout(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};