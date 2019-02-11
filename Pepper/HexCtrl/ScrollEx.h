/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This code is lisenced under the "MIT License modified with�The�Commons Clause"		*
* Scroll bar control class for MFC apps.												*
* The main creation purpose of this control is the innate 32-bit range limitation		*
* of the standard Windows's scrollbar control.											*
* This control works with unsigned long long data representation and thus can operate	*
* with numbers in full 64-bit range.													*
****************************************************************************************/
#pragma once
#include <afxwin.h>
#include "HexCtrlRes.h"

namespace HEXCTRL {

	class CScrollEx : public CWnd
	{
	public:
		CScrollEx() {}
		~CScrollEx() {}
		bool Create(CWnd* pWnd, int iScrollType, ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax);
		bool IsVisible() { return m_fVisible; }
		CWnd* GetParent() { return m_pwndParent; }
		void SetScrollSizes(ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax);
		ULONGLONG SetScrollPos(ULONGLONG);
		void ScrollLineUp();
		void ScrollLineDown();
		void ScrollLineLeft();
		void ScrollLineRight();
		void ScrollPageUp();
		void ScrollPageDown();
		void ScrollPageLeft();
		void ScrollPageRight();
		void ScrollHome();
		void ScrollEnd();
		ULONGLONG GetScrollPos();
		LONGLONG GetScrollPosDelta(ULONGLONG& ullCurrPos, ULONGLONG& ullPrevPos);
		ULONGLONG GetScrollLineSize();
		ULONGLONG GetScrollPageSize();
		BOOL OnNcActivate(BOOL bActive);
		void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
		void OnNcPaint();
		void OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		void OnMouseMove(UINT nFlags, CPoint point);
		void OnLButtonUp(UINT nFlags, CPoint point);
	protected:
		DECLARE_MESSAGE_MAP()
		void DrawScrollBar();
		void DrawArrows(CDC* pDC);
		void DrawThumb(CDC* pDC);
		CRect GetScrollRect(bool fWithNCArea = false);
		CRect GetScrollWorkAreaRect(bool fClientCoord = false);
		UINT GetScrollSizeWH();
		UINT GetScrollWorkAreaSizeWH(); //Scroll area size (WH) without arrow buttons.
		CRect GetThumbRect(bool fClientCoord = false);
		UINT GetThumbSizeWH();
		UINT GetThumbPos();
		long double GetThumbScrollingSize();
		void SetThumbPos(int uiPos);
		CRect GetFirstArrowRect(bool fClientCoord = false);
		CRect GetLastArrowRect(bool fClientCoord = false);
		CRect GetFirstChannelRect(bool fClientCoord = false);
		CRect GetLastChannelRect(bool fClientCoord = false);
		CRect GetParentRect(bool fClient = true);
		bool IsVert();
		bool IsThumbDragging();
		void ResetTimers();
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		void SendParentScrollMsg();
		enum SCROLLSTATE
		{
			FIRSTBUTTON_HOVER = 1,
			FIRSTBUTTON_CLICK = 2,
			FIRSTCHANNEL_CLICK = 3,
			THUMB_HOVER = 4,
			THUMB_CLICK = 5,
			LASTCHANNEL_CLICK = 6,
			LASTBUTTON_CLICK = 7,
			LASTBUTTON_HOVER = 8
		};
	protected:
		CWnd* m_pwndParent { };
		UINT m_uiScrollBarSizeWH { };
		int m_iScrollType { };
		int m_iScrollBarState { };
		COLORREF m_clrBkNC { GetSysColor(COLOR_3DFACE) };
		COLORREF m_clrBkScrollBar { RGB(241, 241, 241) };
		COLORREF m_clrThumb { RGB(192, 192, 192) };
		CPoint m_ptCursorCur { };
		ULONGLONG m_ullScrollPosCur { 0 };
		ULONGLONG m_ullScrollPosPrev { };
		ULONGLONG m_ullScrollLine { };
		ULONGLONG m_ullScrollPage { };
		ULONGLONG m_ullScrollSizeMax { };
		const unsigned m_uiThumbSizeMin = 15;

		//Difference between parent window's Window and Client area.
		//Needed and very important in hit testing.
		int m_iTopDelta { };
		int m_iLeftDelta { };

		//Timers:
		static constexpr auto IDT_FIRSTCLICK = 0x7ff0;
		static constexpr auto IDT_CLICKREPEAT = 0x7ff1;
		const int TIMER_TIME_FIRSTCLICK = 200;
		const int TIMER_TIME_REPEAT = 50;

		//Bitmap related:
		CBitmap m_bmpArrows;
		const unsigned m_uiFirstArrowOffset { 0 };
		const unsigned m_uiLastArrowOffset { 18 };
		const unsigned m_uiArrowSize { 17 };
		bool m_fCreated { false };
		bool m_fVisible { false };
	};
}