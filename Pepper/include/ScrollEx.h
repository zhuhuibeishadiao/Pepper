#pragma once
#include <afxwin.h>
#include "ScrollExRes.h"

namespace HEXCTRL {

	class CScrollEx : public CWnd
	{
	public:
		CScrollEx() {}
		~CScrollEx() {}
		bool Create(CWnd* pWnd, int iScrollType, ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax);
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
		bool IsVisible() { return m_fVisible; }
		BOOL OnNcActivate(BOOL bActive);
		void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
		void OnNcPaint();
		void OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		void OnMouseMove(UINT nFlags, CPoint point);
		void OnLButtonUp(UINT nFlags, CPoint point);
	protected:
		DECLARE_MESSAGE_MAP()
		void SendParentScrollMsg();
		void DrawScrollBar();
		void DrawArrows(CDC* pDC);
		void DrawThumb(CDC* pDC);
		CRect GetScrollRect(bool fWithNCArea = false);
		CRect GetScrollWorkAreaRect();
		CRect GetThumbRect();
		UINT GetThumbSizeWH();
		UINT GetThumbPos();
		long double GetThumbScrollingSize();
		void SetThumbPos(int uiPos);
		CRect GetFirstArrowRect();
		CRect GetLastArrowRect();
		CRect GetFirstChannelRect();
		CRect GetLastChannelRect();
		UINT GetScrollSizeWH();
		UINT GetScrollWorkAreaSizeWH(); //Scroll area size (WH) without arrow buttons.
		CRect GetParentRect();
		bool IsVert();
		bool IsThumbDragging();
		void ResetTimers();
		afx_msg void OnTimer(UINT_PTR nIDEvent);
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
		const unsigned m_uiThumbSizeMin = 10;

		//Timers:
		static constexpr auto IDT_FIRSTCLICK = 0x7ff0;
		static constexpr auto IDT_CLICKREPEAT = 0x7ff1;
		const int TIMER_TIME_FIRSTCLICK = 200;
		const int TIMER_TIME_REPEAT = 50;

		//Bitmap related:
		CBitmap m_bmpScroll;
		const unsigned m_uiFirstBtnOffset { 0 };
		const unsigned m_uiLastBtnOffset { 61 };
		const unsigned m_uiFirstBtnSize { 17 };
		const unsigned m_uiLastBtnSize { 17 };
		bool m_fCreated { false };
		bool m_fVisible { false };
	};
}