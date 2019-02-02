#include "stdafx.h"
#include "ScrollEx.h"
#include <cmath>

using namespace HEXCTRL;

BEGIN_MESSAGE_MAP(CScrollEx, CWnd)
	ON_WM_TIMER()
END_MESSAGE_MAP()

bool CScrollEx::Create(CWnd * pWndParent, int iScrollType,
	ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax)
{
	if (!pWndParent || (iScrollType != SB_VERT && iScrollType != SB_HORZ))
		return false;

	if (!CWnd::CreateEx(0, nullptr, nullptr, 0, 0, 0, 0, 0, nullptr, 0))
		return false;

	m_iScrollType = iScrollType;
	m_pwndParent = pWndParent;

	UINT uiBmp;
	if (IsVert())
	{
		uiBmp = IDB_SCROLL_V;
		m_uiScrollBarSizeWH = GetSystemMetrics(SM_CXVSCROLL);
	}
	else
	{
		uiBmp = IDB_SCROLL_H;
		m_uiScrollBarSizeWH = GetSystemMetrics(SM_CXHSCROLL);
	}

	if (!m_bmpScroll.LoadBitmapW(uiBmp))
		return false;

	m_fCreated = true;
	SetScrollSizes(ullScrolline, ullScrollPage, ullScrollSizeMax);

	return true;
}

void CScrollEx::SetScrollSizes(ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax)
{
	if (!m_fCreated)
		return;

	m_ullScrollLine = ullScrolline;
	m_ullScrollPage = ullScrollPage;
	m_ullScrollSizeMax = ullScrollSizeMax;

	GetParent()->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	DrawScrollBar();
}

ULONGLONG CScrollEx::SetScrollPos(ULONGLONG ullNewPos)
{
	if (!m_fCreated)
		return 0;

	m_ullScrollPosPrev = m_ullScrollPosCur;
	m_ullScrollPosCur = ullNewPos;

	CRect rc = GetParentRect();
	int iPageSize = 0;
	if (IsVert())
		iPageSize = rc.Height();
	else
		iPageSize = rc.Width();

	ULONGLONG ullMax = m_ullScrollSizeMax - iPageSize;
	if (m_ullScrollPosCur > ullMax)
		m_ullScrollPosCur = ullMax;

	SendParentScrollMsg();
	DrawScrollBar();

	return m_ullScrollPosPrev;
}

ULONGLONG CScrollEx::GetScrollPos()
{
	if (!m_fCreated)
		return 0;

	return m_ullScrollPosCur;
}

LONGLONG CScrollEx::GetScrollPosDelta(ULONGLONG & ullPosCurr, ULONGLONG & ullPosPrev)
{
	if (!m_fCreated)
		return 0;

	ullPosCurr = m_ullScrollPosCur;
	ullPosPrev = m_ullScrollPosPrev;

	return LONGLONG(m_ullScrollPosCur - m_ullScrollPosPrev);
}

void CScrollEx::ScrollLineDown()
{
	ULONGLONG ullCur = GetScrollPos();
	ULONGLONG ullNew;
	if (ULONGLONG_MAX - ullCur < m_ullScrollLine) //To avoid overflow.
		ullNew = ULONGLONG_MAX;
	else
		ullNew = ullCur + m_ullScrollLine;
	SetScrollPos(ullNew);
}

void CScrollEx::ScrollLineRight()
{
	ScrollLineDown();
}

void CScrollEx::ScrollLineUp()
{
	ULONGLONG ullCur = GetScrollPos();
	ULONGLONG ullNew;
	if (m_ullScrollLine > ullCur) //To avoid overflow.
		ullNew = 0;
	else
		ullNew = ullCur - m_ullScrollLine;
	SetScrollPos(ullNew);
}

void CScrollEx::ScrollLineLeft()
{
	ScrollLineUp();
}

void CScrollEx::ScrollPageDown()
{
	ULONGLONG ullCur = GetScrollPos();
	ULONGLONG ullNew;
	if (ULONGLONG_MAX - ullCur < m_ullScrollPage) //To avoid overflow.
		ullNew = ULONGLONG_MAX;
	else
		ullNew = ullCur + m_ullScrollPage;
	SetScrollPos(ullNew);

}

void CScrollEx::ScrollPageUp()
{
	ULONGLONG ullCur = GetScrollPos();
	ULONGLONG ullNew;
	if (m_ullScrollPage > ullCur) //To avoid overflow.
		ullNew = 0;
	else
		ullNew = ullCur - m_ullScrollPage;
	SetScrollPos(ullNew);
}

void CScrollEx::ScrollHome()
{
	SetScrollPos(0);
}

void CScrollEx::ScrollEnd()
{
	SetScrollPos(m_ullScrollSizeMax);
}

void CScrollEx::SetThumbPos(int iPos)
{
	CRect rcWorkArea = GetScrollWorkAreaRect();
	UINT uiThumbSize = GetThumbSizeWH();
	ULONGLONG ullNewScrollPos;

	if (iPos < 0)
		ullNewScrollPos = 0;
	else if (iPos == 0x7FFFFFFF)
		ullNewScrollPos = m_ullScrollSizeMax;
	else
	{
		if (IsVert())
		{
			if (iPos + (int)uiThumbSize > rcWorkArea.Height())
				iPos = rcWorkArea.Height() - uiThumbSize;
		}
		else
		{
			if (iPos + (int)uiThumbSize > rcWorkArea.Width())
				iPos = rcWorkArea.Width() - uiThumbSize;
		}
		ullNewScrollPos = (ULONGLONG)std::llroundl(iPos * GetThumbScrollingSize());
	}
	SetScrollPos(ullNewScrollPos);
}

bool CScrollEx::IsVert()
{
	return m_iScrollType == SB_VERT ? true : false;
}

bool CScrollEx::IsThumbDragging()
{
	return m_iScrollBarState == SCROLLSTATE::THUMB_CLICK ? true : false;
}

void CScrollEx::DrawScrollBar()
{
	if (!IsVisible())
		return;
	CRect rcScrollNC = GetScrollRect(true);

	CWnd* pwndParent = GetParent();
	CWindowDC parentDC(pwndParent);
	CMemDC memDC(parentDC, rcScrollNC);
	CDC* pDC = &memDC.GetDC();
	CRect rcScroll = GetScrollRect();
	pDC->FillSolidRect(&rcScrollNC, m_clrBkNC); //NC Bk.
	pDC->FillSolidRect(&rcScroll, m_clrBkScrollBar); //Bk.
	DrawArrows(pDC);
	DrawThumb(pDC);
}

void CScrollEx::DrawArrows(CDC * pDC)
{
	CRect rcScroll = GetScrollRect();
	CDC compatDC;
	compatDC.CreateCompatibleDC(pDC);
	compatDC.SelectObject(&m_bmpScroll);

	int iFirstBtnOffsetDrawX, iFirstBtnOffsetDrawY, iFirstBtnWH, iFirstBtnBmpOffsetX, iFirstBtnBmpOffsetY;
	int iLastBtnOffsetDrawX, iLastBtnOffsetDrawY, iLastBtnWH, iLastBtnBmpOffsetX, iLastBtnBmpOffsetY;
	if (IsVert())
	{
		//First arrow button: offsets in bitmap to take from
		//and screen coords to print to.
		iFirstBtnBmpOffsetX = 0;
		iFirstBtnBmpOffsetY = 0;
		iFirstBtnOffsetDrawX = rcScroll.left;
		iFirstBtnOffsetDrawY = rcScroll.top;
		iFirstBtnWH = rcScroll.Width();

		iLastBtnBmpOffsetX = 0;
		iLastBtnBmpOffsetY = m_iLastBtnOffset;
		iLastBtnOffsetDrawX = rcScroll.left;
		iLastBtnOffsetDrawY = rcScroll.bottom - rcScroll.Width();
		iLastBtnWH = rcScroll.Width();
	}
	else
	{
		iFirstBtnBmpOffsetX = 0;
		iFirstBtnBmpOffsetY = 0;
		iFirstBtnOffsetDrawX = rcScroll.left;
		iFirstBtnOffsetDrawY = rcScroll.top;
		iFirstBtnWH = rcScroll.Height();

		iLastBtnBmpOffsetX = m_iLastBtnOffset;
		iLastBtnBmpOffsetY = 0;
		iLastBtnOffsetDrawX = rcScroll.right - rcScroll.Height();
		iLastBtnOffsetDrawY = rcScroll.top;
		iLastBtnWH = rcScroll.Height();
	}
	//First arrow button.
	pDC->StretchBlt(iFirstBtnOffsetDrawX, iFirstBtnOffsetDrawY, iFirstBtnWH, iFirstBtnWH,
		&compatDC, iFirstBtnBmpOffsetX, iFirstBtnBmpOffsetY, m_iFirstBtnSize, m_iFirstBtnSize, SRCCOPY);

	//Last arrow button.
	pDC->StretchBlt(iLastBtnOffsetDrawX, iLastBtnOffsetDrawY, iLastBtnWH, iLastBtnWH,
		&compatDC, iLastBtnBmpOffsetX, iLastBtnBmpOffsetY, m_iLastBtnSize, m_iLastBtnSize, SRCCOPY);
}

void CScrollEx::DrawThumb(CDC* pDC)
{
	CRect rcThumb = GetThumbRect();
	if (!rcThumb.IsRectEmpty())
	{
		int iThumbBmpOffsetX, iThumbBmpOffsetY, iThumbBmpSizeX, iThumbBmpSizeY;

		if (IsVert())
		{
			iThumbBmpOffsetX = 0;
			iThumbBmpOffsetY = m_iThumbOffset;
			iThumbBmpSizeX = m_iFirstBtnSize; //Scroll width.
			iThumbBmpSizeY = m_iThumbSize;
		}
		else
		{
			iThumbBmpOffsetX = m_iThumbOffset;
			iThumbBmpOffsetY = 0;
			iThumbBmpSizeX = m_iThumbSize;
			iThumbBmpSizeY = m_iFirstBtnSize; //Scroll height.
		}

		switch (m_iScrollBarState)
		{
		case SCROLLSTATE::THUMB_HOVER:
			pDC->FillSolidRect(rcThumb, RGB(255, 0, 0));
			break;
		default:
			pDC->FillSolidRect(rcThumb, m_clrThumb);
		}
		//	pDC->StretchBlt(rcThumb.left, rcThumb.top, rcThumb.Width(), rcThumb.Height(),
		//		compatDC, iThumbBmpOffsetX, iThumbBmpOffsetY, iThumbBmpSizeX, iThumbBmpSizeY, SRCCOPY);
	}
}

void CScrollEx::SendParentScrollMsg()
{
	if (!m_fCreated)
		return;

	UINT uiMsg = IsVert() ? WM_VSCROLL : WM_HSCROLL;
	GetParent()->SendMessageW(uiMsg);
}

long double CScrollEx::GetThumbScrollingSize()
{
	if (!m_fCreated)
		return -1;

	long double uiWAWOThumb = GetScrollWorkAreaSizeWH() - GetThumbSizeWH(); //Work area without thumb.
	int iPage;
	if (IsVert())
		iPage = GetParentRect().Height();
	else
		iPage = GetParentRect().Width();

	return (m_ullScrollSizeMax - iPage) / uiWAWOThumb;
}

UINT CScrollEx::GetThumbSizeWH()
{
	UINT uiScrollWorkAreaSizeWH = GetScrollWorkAreaSizeWH();
	CRect rcParent = GetParentRect();
	long double dDelta;

	UINT uiThumbSize;
	if (IsVert())
		dDelta = (long double)rcParent.Height() / m_ullScrollSizeMax;
	else
		dDelta = (long double)rcParent.Width() / m_ullScrollSizeMax;

	uiThumbSize = (UINT)std::lroundl((long double)uiScrollWorkAreaSizeWH * dDelta);

	if (uiThumbSize < m_iThumbSize)
		uiThumbSize = m_iThumbSize;

	return uiThumbSize;
}

CRect CScrollEx::GetThumbRect()
{
	UINT uiThumbSize = GetThumbSizeWH();
	if (!uiThumbSize)
		return 0;

	CRect rcScrollWA = GetScrollWorkAreaRect();
	CRect rcThumb;
	if (IsVert())
	{
		rcThumb.left = rcScrollWA.left;
		rcThumb.top = rcScrollWA.top + GetThumbPos();
		rcThumb.right = rcThumb.left + m_uiScrollBarSizeWH;
		rcThumb.bottom = rcThumb.top + uiThumbSize;
	}
	else
	{
		rcThumb.left = rcScrollWA.left + GetThumbPos();
		rcThumb.top = rcScrollWA.top;
		rcThumb.right = rcThumb.left + uiThumbSize;
		rcThumb.bottom = rcThumb.top + m_uiScrollBarSizeWH;
	}

	return rcThumb;
}

CRect CScrollEx::GetFirstArrowRect()
{
	CRect rc = GetScrollRect();
	if (IsVert())
		rc.bottom = rc.top + m_iFirstBtnSize;
	else
		rc.right = rc.left + m_iFirstBtnSize;

	return rc;
}

CRect CScrollEx::GetLastArrowRect()
{
	CRect rc = GetScrollRect();
	if (IsVert())
		rc.top = rc.bottom - m_iFirstBtnSize;
	else
		rc.left = rc.right - m_iFirstBtnSize;

	return rc;
}

CRect CScrollEx::GetFirstChannelRect()
{
	CRect rcThumb = GetThumbRect();
	CRect rcArrow = GetFirstArrowRect();
	CRect rc;
	if (IsVert())
		rc.SetRect(rcArrow.left, rcArrow.bottom, rcArrow.right, rcThumb.top);
	else
		rc.SetRect(rcArrow.right, rcArrow.top, rcThumb.left, rcArrow.bottom);

	return rc;
}

CRect CScrollEx::GetLastChannelRect()
{
	CRect rcThumb = GetThumbRect();
	CRect rcArrow = GetLastArrowRect();
	CRect rc;
	if (IsVert())
		rc.SetRect(rcArrow.left, rcThumb.bottom, rcArrow.right, rcArrow.top);
	else
		rc.SetRect(rcThumb.left, rcArrow.top, rcArrow.left, rcArrow.bottom);

	return rc;
}

CRect CScrollEx::GetScrollRect(bool fWithNCArea)
{
	if (!m_fCreated)
		return 0;

	CWnd* pwndParent = GetParent();

	CRect rcClient, rcWnd, rcScroll;
	pwndParent->GetWindowRect(&rcWnd);
	pwndParent->GetClientRect(&rcClient);
	pwndParent->ClientToScreen(&rcClient);

	int iLeftDelta = rcClient.left - rcWnd.left;
	int iTopDelta = rcClient.top - rcWnd.top;

	if (IsVert())
	{
		rcScroll.left = rcClient.right + iLeftDelta;
		rcScroll.top = rcClient.top + iTopDelta;
		rcScroll.right = rcScroll.left + m_uiScrollBarSizeWH;
		if (fWithNCArea)
			rcScroll.bottom = rcWnd.bottom;
		else
			//rcScroll.bottom = rcClient.bottom + iTopDelta;
			rcScroll.bottom = rcScroll.top + rcClient.Height();
	}
	else
	{
		rcScroll.left = rcClient.left + iLeftDelta;
		rcScroll.top = rcClient.bottom + iTopDelta;
		rcScroll.bottom = rcScroll.top + m_uiScrollBarSizeWH;
		if (fWithNCArea)
			rcScroll.right = rcWnd.right;
		else
			//rcScroll.right = rcClient.right + iLeftDelta;
			rcScroll.right = rcScroll.left + rcClient.Width();
	}
	pwndParent->ScreenToClient(&rcScroll);

	return rcScroll;
}

CRect CScrollEx::GetScrollWorkAreaRect()
{
	CRect rc = GetScrollRect();
	if (IsVert())
		rc.DeflateRect(0, m_uiScrollBarSizeWH, 0, m_uiScrollBarSizeWH);
	else
		rc.DeflateRect(m_uiScrollBarSizeWH, 0, m_uiScrollBarSizeWH, 0);

	return rc;
}

CRect CScrollEx::GetParentRect()
{
	CRect rc;
	GetParent()->GetClientRect(&rc);

	return rc;
}

UINT CScrollEx::GetScrollSizeWH()
{
	UINT uiSizeInPixels;

	if (IsVert())
		uiSizeInPixels = GetScrollRect().Height();
	else
		uiSizeInPixels = GetScrollRect().Width();

	return uiSizeInPixels;
}

UINT CScrollEx::GetScrollWorkAreaSizeWH()
{
	UINT uiScrollWorkArea;
	UINT uiScrollSize = GetScrollSizeWH();

	if (uiScrollSize <= m_uiScrollBarSizeWH * 2)
		uiScrollWorkArea = 0;
	else
		uiScrollWorkArea = uiScrollSize - m_uiScrollBarSizeWH * 2;

	return uiScrollWorkArea;
}

UINT CScrollEx::GetThumbPos()
{
	ULONGLONG ullScrollPos = GetScrollPos();
	long double dThumbScrollingSize = GetThumbScrollingSize();

	UINT uiThumbPos;
	if (ullScrollPos <= dThumbScrollingSize)
		uiThumbPos = 0;
	else
		uiThumbPos = (UINT)std::lroundl(ullScrollPos / dThumbScrollingSize);

	return uiThumbPos;
}

void CScrollEx::ResetTimers()
{
	if (m_iScrollBarState > 0)
	{
		m_iScrollBarState = 0;
		KillTimer(IDT_FIRSTCLICK);
		KillTimer(IDT_CLICKREPEAT);
	}
}

void CScrollEx::OnSetCursor(CWnd * pWnd, UINT nHitTest, UINT message)
{
	if (!m_fCreated || nHitTest == HTTOPLEFT || nHitTest == HTLEFT || nHitTest == HTTOPRIGHT || nHitTest == HTSIZE
		|| nHitTest == HTBOTTOMLEFT || nHitTest == HTRIGHT || nHitTest == HTBOTTOM || nHitTest == HTBOTTOMRIGHT)
		return;

	switch (message)
	{
	case WM_LBUTTONDOWN:
	{
		POINT pt;
		GetCursorPos(&pt);
		GetParent()->ScreenToClient(&pt);

		if (IsVisible())
		{
			if (GetThumbRect().PtInRect(pt))
			{
				m_ptCursorCur = pt;
				m_iScrollBarState = SCROLLSTATE::THUMB_CLICK;
				GetParent()->SetCapture();
			}
			else if (GetFirstArrowRect().PtInRect(pt))
			{
				ScrollLineUp();
				m_iScrollBarState = SCROLLSTATE::FIRSTBUTTON_CLICK;
				GetParent()->SetCapture();
				SetTimer(IDT_FIRSTCLICK, TIMER_TIME_FIRSTCLICK, nullptr);
			}
			else if (GetLastArrowRect().PtInRect(pt))
			{
				ScrollLineDown();
				m_iScrollBarState = SCROLLSTATE::LASTBUTTON_CLICK;
				GetParent()->SetCapture();
				SetTimer(IDT_FIRSTCLICK, TIMER_TIME_FIRSTCLICK, nullptr);
			}
			else if (GetFirstChannelRect().PtInRect(pt))
			{
				ScrollPageUp();
				m_iScrollBarState = SCROLLSTATE::FIRSTCHANNEL_CLICK;
				GetParent()->SetCapture();
				SetTimer(IDT_FIRSTCLICK, TIMER_TIME_FIRSTCLICK, nullptr);
			}
			else if (GetLastChannelRect().PtInRect(pt))
			{
				ScrollPageDown();
				m_iScrollBarState = SCROLLSTATE::LASTCHANNEL_CLICK;
				GetParent()->SetCapture();
				SetTimer(IDT_FIRSTCLICK, TIMER_TIME_FIRSTCLICK, nullptr);
			}
		}
	}
	break;
	}
}

void CScrollEx::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (!m_fCreated)
		return;

	if (m_iScrollBarState > 0)
	{
		m_iScrollBarState = 0;
		KillTimer(IDT_FIRSTCLICK);
		KillTimer(IDT_CLICKREPEAT);
		ReleaseCapture();
		DrawScrollBar();
	}
}

void CScrollEx::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_fCreated)
		return;

	if (IsThumbDragging())
	{
		CRect rc = GetScrollWorkAreaRect();
		int iNewPos;

		if (IsVert())
		{
			if (point.y < rc.top)
				iNewPos = 0;
			else if (point.y > rc.bottom)
				iNewPos = 0x7FFFFFFF;
			else
				iNewPos = GetThumbPos() + (point.y - m_ptCursorCur.y);
		}
		else
		{
			if (point.x < rc.left)
				iNewPos = 0;
			else if (point.x > rc.right)
				iNewPos = 0x7FFFFFFF;
			else
				iNewPos = GetThumbPos() + (point.x - m_ptCursorCur.x);
		}

		m_ptCursorCur = point;
		SetThumbPos(iNewPos);
		SendParentScrollMsg();
	}
}

void CScrollEx::OnNcPaint()
{
	if (!m_fCreated)
		return;

	DrawScrollBar();
}

BOOL CScrollEx::OnNcActivate(BOOL bActive)
{
	if (!m_fCreated)
		return FALSE;

	GetParent()->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	return TRUE;
}

void CScrollEx::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	if (!m_fCreated)
		return;

	CRect rc = lpncsp->rgrc[0];
	ULONGLONG ullCurPos = GetScrollPos();
	if (IsVert())
	{
		if (rc.Height() < m_ullScrollSizeMax)
		{
			m_fVisible = true;
			if (ullCurPos + rc.Height() > m_ullScrollSizeMax)
				SetScrollPos(m_ullScrollSizeMax - rc.Height());
			else
				DrawScrollBar();
			lpncsp->rgrc[0].right -= m_uiScrollBarSizeWH;
		}
		else
		{
			SetScrollPos(0);
			m_fVisible = false;
		}
	}
	else
	{
		if (rc.Width() < m_ullScrollSizeMax)
		{
			m_fVisible = true;
			if (ullCurPos + rc.Width() > m_ullScrollSizeMax)
				SetScrollPos(m_ullScrollSizeMax - rc.Width());
			else
				DrawScrollBar();
			lpncsp->rgrc[0].bottom -= m_uiScrollBarSizeWH;
		}
		else
		{
			SetScrollPos(0);
			m_fVisible = false;
		}
	}
}

void CScrollEx::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case IDT_FIRSTCLICK:
		KillTimer(IDT_FIRSTCLICK);
		SetTimer(IDT_CLICKREPEAT, TIMER_TIME_REPEAT, nullptr);
		break;
	case IDT_CLICKREPEAT:
	{
		switch (m_iScrollBarState)
		{
		case SCROLLSTATE::FIRSTBUTTON_CLICK:
			ScrollLineUp();
			break;
		case SCROLLSTATE::LASTBUTTON_CLICK:
			ScrollLineDown();
			break;
		case SCROLLSTATE::FIRSTCHANNEL_CLICK:
		{
			CPoint pt;	GetCursorPos(&pt);
			CRect rc = GetThumbRect();
			GetParent()->ClientToScreen(rc);
			if (IsVert()) {
				if (pt.y < rc.top)
					ScrollPageUp();
			}
			else {
				if (pt.x < rc.left)
					ScrollPageUp();
			}
		}
		break;
		case SCROLLSTATE::LASTCHANNEL_CLICK:
			CPoint pt;	GetCursorPos(&pt);
			CRect rc = GetThumbRect();
			GetParent()->ClientToScreen(rc);
			if (IsVert()) {
				if (pt.y > rc.bottom)
					ScrollPageDown();
			}
			else {
				if (pt.x > rc.right)
					ScrollPageDown();
			}
		}
	}
	break;
	}

	CWnd::OnTimer(nIDEvent);
}