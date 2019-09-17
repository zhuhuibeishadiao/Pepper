/****************************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/										*
* This software is available under the "MIT License".                                               *
* https://github.com/jovibor/Pepper/blob/master/LICENSE												*
* Pepper - PE (x86) and PE+ (x64) files viewer, based on libpe: https://github.com/jovibor/Pepper	*
* libpe - Windows library for reading PE (x86) and PE+ (x64) files inner structure information.		*
* https://github.com/jovibor/libpe																	*
****************************************************************************************************/
#include "stdafx.h"
#include "ViewRightBR.h"
#include "constants.h"

BEGIN_MESSAGE_MAP(CWndDlgSample, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CWndDlgSample::OnPaint()
{
	CPaintDC dc(this);
	if (m_pImgRes)
		m_pImgRes->Draw(&dc, 0, POINT { 0, 0 }, ILD_NORMAL);
}

IMPLEMENT_DYNCREATE(CViewRightBR, CScrollView)

BEGIN_MESSAGE_MAP(CViewRightBR, CScrollView)
	ON_WM_SIZE()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

void CViewRightBR::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	SetScrollSizes(MM_TEXT, CSize(0, 0));

	m_pChildFrame = (CChildFrame*)GetParentFrame();
	m_pMainDoc = (CPepperDoc*)GetDocument();
	m_pLibpe = m_pMainDoc->m_pLibpe;

	m_EditBRB.Create(WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL
		| ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL, CRect(0, 0, 0, 0), this, 0x01);

	LOGFONTW lf { };
	StringCchCopyW(lf.lfFaceName, 9, L"Consolas");
	lf.lfHeight = 18;
	if (!m_fontEditRes.CreateFontIndirectW(&lf))
	{
		NONCLIENTMETRICSW ncm;
		ncm.cbSize = sizeof(NONCLIENTMETRICS);
		SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		m_fontEditRes.CreateFontIndirectW(&ncm.lfMessageFont);
	}
	m_EditBRB.SetFont(&m_fontEditRes);

	m_stlcs.pwndParent = this;
	m_stlcs.stColor.clrTooltipText = RGB(255, 255, 255);
	m_stlcs.stColor.clrTooltipBk = RGB(0, 132, 132);
	m_stlcs.stColor.clrHeaderText = RGB(255, 255, 255);
	m_stlcs.stColor.clrHeaderBk = RGB(0, 132, 132);
	m_stlcs.dwHeaderHeight = 35;

	m_lf.lfHeight = 16;
	StringCchCopyW(m_lf.lfFaceName, 9, L"Consolas");
	m_stlcs.pListLogFont = &m_lf;
	m_hdrlf.lfHeight = 17;
	m_hdrlf.lfWeight = FW_BOLD;
	StringCchCopyW(m_hdrlf.lfFaceName, 16, L"Times New Roman");
	m_stlcs.pHeaderLogFont = &m_hdrlf;

	CreateListTLSCallbacks();
}

void CViewRightBR::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* pHint)
{
	//If it's UpdateAllViews call for top right Hex (IDC_HEX_RIGHT_TR), (from top left Resource Tree) we do nothing.
	if (!m_pChildFrame || LOWORD(lHint) == IDC_HEX_RIGHT_TR || LOWORD(lHint) == ID_DOC_EDITMODE)
		return;

	//If any but Resources Update we destroy m_wndDlgSample, if it's currently created.
	if (LOWORD(lHint) != IDC_SHOW_RESOURCE_RBR)
	{
		if (m_wndDlgSample.m_hWnd)
			m_wndDlgSample.DestroyWindow();
	}

	if (m_hwndActive)
		::ShowWindow(m_hwndActive, SW_HIDE);

	CRect rcParent, rcClient;
	GetParent()->GetWindowRect(&rcParent);
	GetClientRect(&rcClient);
	m_fDrawRes = false;

	switch (LOWORD(lHint))
	{
	case IDC_LIST_TLS:
		m_stListTLSCallbacks->SetWindowPos(this, 0, 0, rcClient.Width(), rcClient.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_hwndActive = m_stListTLSCallbacks->m_hWnd;
		m_pChildFrame->m_stSplitterRightBottom.ShowCol(1);
		m_pChildFrame->m_stSplitterRightBottom.SetColumnInfo(0, rcParent.Width() / 2, 0);
		break;
	case IDC_TREE_RESOURCE:
		m_pChildFrame->m_stSplitterRightBottom.ShowCol(1);
		m_pChildFrame->m_stSplitterRightBottom.SetColumnInfo(0, rcParent.Width() / 3, 0);
		break;
	case IDC_SHOW_RESOURCE_RBR:
		ShowResource(reinterpret_cast<RESHELPER*>(pHint));
		m_pChildFrame->m_stSplitterRightBottom.ShowCol(1);
		m_pChildFrame->m_stSplitterRightBottom.SetColumnInfo(0, rcParent.Width() / 3, 0);
		break;
	case IDC_LIST_DEBUG_ENTRY:
		CreateDebugEntry(HIWORD(lHint));
		m_pChildFrame->m_stSplitterRightBottom.ShowCol(1);
		m_pChildFrame->m_stSplitterRightBottom.SetColumnInfo(0, rcParent.Width() / 2, 0);
		break;
	default:
		m_pChildFrame->m_stSplitterRightBottom.HideCol(1);
	}

	m_pChildFrame->m_stSplitterRightBottom.RecalcLayout();
}

void CViewRightBR::ShowResource(const RESHELPER* pResHelper)
{
	m_stImgRes.DeleteImageList();
	m_iResTypeToDraw = -1;
	m_iImgResWidth = 0;
	m_iImgResHeight = 0;
	m_vecImgRes.clear();
	m_wstrEditBRB.clear();

	if (pResHelper)
	{
		//Destroy Dialog Sample window if it's any other resource type now.
		if (pResHelper->IdResType != 5 && m_wndDlgSample.m_hWnd)
			m_wndDlgSample.DestroyWindow();

		if (pResHelper->pData->empty())
			return ResLoadError();

		switch (pResHelper->IdResType)
		{
		case 1: //RT_CURSOR
		case 3: //RT_ICON
			CreateIconCursor(pResHelper);
			break;
		case 2: //RT_BITMAP
			CreateBitmap(pResHelper);
			break;
			//case 4: //RT_MENU
			//break;
		case 5: //RT_DIALOG
			CreateDlg(pResHelper);
			break;
		case 6: //RT_STRING
			CreateStrings(pResHelper);
			break;
		case 12: //RT_GROUP_CURSOR
		case 14: //RT_GROUP_ICON
			CreateGroupIconCursor(pResHelper);
			break;
		case 16: //RT_VERSION
			CreateVersion(pResHelper);
			break;
		case 24: //RT_MANIFEST
			CreateManifest(pResHelper);
			break;
		case 241: //RT_TOOLBAR
			CreateToolbar(pResHelper);
			break;
		default:
			m_iResTypeToDraw = pResHelper->IdResType;
			m_fDrawRes = true;
		}
	}
	else
	{
		//Destroy Dialog Sample window if it's just Resource window Update.
		if (m_wndDlgSample.m_hWnd)
			m_wndDlgSample.DestroyWindow();
	}

	RedrawWindow();
}

void CViewRightBR::CreateIconCursor(const RESHELPER* pResHelper)
{
	HICON hIcon;
	ICONINFO iconInfo;

	hIcon = CreateIconFromResourceEx((PBYTE)pResHelper->pData->data(), (DWORD)pResHelper->pData->size(),
		(pResHelper->IdResType == 3) ? TRUE : FALSE, 0x00030000, 0, 0, LR_DEFAULTCOLOR);
	if (!hIcon)
		return ResLoadError();
	if (!GetIconInfo(hIcon, &iconInfo))
		return ResLoadError();
	if (!GetObjectW(iconInfo.hbmMask, sizeof(BITMAP), &m_stBmp))
		return ResLoadError();
	DeleteObject(iconInfo.hbmColor);
	DeleteObject(iconInfo.hbmMask);

	m_stImgRes.Create(m_stBmp.bmWidth,
		(pResHelper->IdResType == 3) ? m_stBmp.bmHeight : m_stBmp.bmWidth, ILC_COLORDDB, 0, 1);
	m_stImgRes.SetBkColor(m_clrBkImgList);
	if (m_stImgRes.Add(hIcon) == -1)
		return ResLoadError();

	DestroyIcon(hIcon);
	SetScrollSizes(MM_TEXT, CSize(m_stBmp.bmWidth,
		(pResHelper->IdResType == 3) ? m_stBmp.bmHeight : m_stBmp.bmWidth));

	m_iResTypeToDraw = pResHelper->IdResType;
	m_fDrawRes = true;
}

void CViewRightBR::CreateBitmap(const RESHELPER * pResHelper)
{
	BITMAPINFO* pDIBInfo = (BITMAPINFO*)pResHelper->pData->data();
	int iColors = pDIBInfo->bmiHeader.biClrUsed ? pDIBInfo->bmiHeader.biClrUsed : 1 << pDIBInfo->bmiHeader.biBitCount;
	LPVOID pDIBBits;

	if (pDIBInfo->bmiHeader.biBitCount > 8)
		pDIBBits = (LPVOID)((PDWORD)(pDIBInfo->bmiColors + pDIBInfo->bmiHeader.biClrUsed) +
		((pDIBInfo->bmiHeader.biCompression == BI_BITFIELDS) ? 3 : 0));
	else
		pDIBBits = (LPVOID)(pDIBInfo->bmiColors + iColors);

	HDC hDC = ::GetDC(m_hWnd);
	HBITMAP hBitmap = CreateDIBitmap(hDC, &pDIBInfo->bmiHeader, CBM_INIT, pDIBBits, pDIBInfo, DIB_RGB_COLORS);
	::ReleaseDC(m_hWnd, hDC);
	if (!hBitmap)
		return ResLoadError();

	if (!GetObjectW(hBitmap, sizeof(BITMAP), &m_stBmp))
		return ResLoadError();

	CBitmap bmp;
	if (!bmp.Attach(hBitmap))
		return ResLoadError();

	m_stImgRes.Create(m_stBmp.bmWidth, m_stBmp.bmHeight, ILC_COLORDDB, 0, 1);
	if (m_stImgRes.Add(&bmp, nullptr) == -1)
		return ResLoadError();

	m_iResTypeToDraw = 2;
	m_fDrawRes = true;
	SetScrollSizes(MM_TEXT, CSize(m_stBmp.bmWidth, m_stBmp.bmHeight));
	bmp.DeleteObject();
}

void CViewRightBR::CreateDlg(const RESHELPER * pResHelper)
{
#pragma pack(push, 4)
	struct DLGTEMPLATEEX //Helper struct. Not completed.
	{
		WORD      dlgVer;
		WORD      signature;
		DWORD     helpID;
		DWORD     exStyle;
		DWORD     style;
		WORD      cDlgItems;
		short     x;
		short     y;
		short     cx;
		short     cy;
		WORD      menu;
	};
#pragma pack(pop)

	ParceDlgTemplate((PBYTE)pResHelper->pData->data(), pResHelper->pData->size());

	HWND hwndResDlg = CreateDialogIndirectParamW(nullptr,
		(LPCDLGTEMPLATEW)pResHelper->pData->data(), m_hWnd, nullptr, 0);

	if (!hwndResDlg && GetLastError() == ERROR_INVALID_WINDOW_HANDLE)
	{
		//Trying to set dialog's Items count to 0, to avoid failing on CreateWindowEx 
		//within CreateDialogIndirectParamW, for Dialog items (controls) with CustomClassName.
		//It can be overcome though, by creating a WindowClass with that custom name, 
		//but it's a lot of overhead for every such control: Fetch CustomClassName within ParceDlgTemplate
		//then register it with AfxRegisterClass.
		//Instead, just showing empty dialog without controls.
		PWORD pWordDlgItems;
		WORD wOld;
		if (*(((PWORD)pResHelper->pData->data()) + 1) == 0xFFFF) //DLGTEMPLATEEX
		{
			pWordDlgItems = &((DLGTEMPLATEEX*)pResHelper->pData->data())->cDlgItems;
			wOld = *pWordDlgItems;
		}
		else //DLGTEMPLATE
		{
			pWordDlgItems = &((DLGTEMPLATE*)pResHelper->pData->data())->cdit;
			wOld = *pWordDlgItems;
		}
		*pWordDlgItems = 0;
		hwndResDlg = CreateDialogIndirectParamW(nullptr,
			(LPCDLGTEMPLATEW)pResHelper->pData->data(), m_hWnd, nullptr, 0);
		*pWordDlgItems = wOld;
	}

	if (hwndResDlg)
	{
		CRect rcDlg;
		::GetWindowRect(hwndResDlg, &rcDlg);
		int iPosX = 0, iPosY = 0;
		UINT uFlags = SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER;
		if (!m_wndDlgSample.m_hWnd)
		{
			if (!m_wndDlgSample.CreateEx(m_dwExStyles, AfxRegisterWndClass(0),
				L"Sample Dialog...", m_dwStyles, 0, 0, 0, 0, m_hWnd, 0))
			{
				MessageBoxW(L"Sample Dialog window Create failed.", L"Error");
				return;
			}
			iPosX = (GetSystemMetrics(SM_CXSCREEN) / 2) - rcDlg.Width() / 2;
			iPosY = (GetSystemMetrics(SM_CYSCREEN) / 2) - rcDlg.Height() / 2;
			uFlags &= ~SWP_NOMOVE;
		}

		HDC hDC = ::GetDC(m_wndDlgSample.m_hWnd);
		HDC hDCMemory = CreateCompatibleDC(hDC);
		HBITMAP hBitmap = CreateCompatibleBitmap(hDC, rcDlg.Width(), rcDlg.Height());
		::SelectObject(hDCMemory, hBitmap);

		//To avoid window pop-up removing Windows animation temporarily, then restore back.
		ANIMATIONINFO aninfo;
		aninfo.cbSize = sizeof(ANIMATIONINFO);
		SystemParametersInfoW(SPI_GETANIMATION, aninfo.cbSize, &aninfo, 0);
		int iMinAnimate = aninfo.iMinAnimate;
		if (iMinAnimate) {
			aninfo.iMinAnimate = 0;
			SystemParametersInfoW(SPI_SETANIMATION, aninfo.cbSize, &aninfo, SPIF_SENDCHANGE);
		}
		LONG_PTR lLong = GetWindowLongPtrW(hwndResDlg, GWL_EXSTYLE);
		SetWindowLongPtrW(hwndResDlg, GWL_EXSTYLE, lLong | WS_EX_LAYERED);
		::SetLayeredWindowAttributes(hwndResDlg, 0, 1, LWA_ALPHA);

		::ShowWindow(hwndResDlg, SW_SHOWNOACTIVATE);
		::PrintWindow(hwndResDlg, hDCMemory, 0);
		::DestroyWindow(hwndResDlg);

		if (iMinAnimate) {
			aninfo.iMinAnimate = iMinAnimate;
			SystemParametersInfoW(SPI_SETANIMATION, aninfo.cbSize, &aninfo, SPIF_SENDCHANGE);
		}

		DeleteDC(hDCMemory);
		::ReleaseDC(m_wndDlgSample.m_hWnd, hDC);

		CBitmap bmp;
		if (!bmp.Attach(hBitmap))
			return ResLoadError();
		m_stImgRes.Create(rcDlg.Width(), rcDlg.Height(), ILC_COLORDDB, 0, 1);
		if (m_stImgRes.Add(&bmp, nullptr) == -1)
			return ResLoadError();
		bmp.DeleteObject();

		AdjustWindowRectEx(rcDlg, m_dwStyles, 0, m_dwExStyles); //Get window size with desirable client rect.
		m_wndDlgSample.SetWindowPos(this, iPosX, iPosY, rcDlg.Width(), rcDlg.Height(), uFlags);
		m_wndDlgSample.RedrawWindow(); //Draw dialog bitmap.

		m_EditBRB.SetWindowTextW(m_wstrEditBRB.data()); //Set Dialog resources info to Editbox.
		CRect rcClient;
		GetClientRect(&rcClient);
		m_EditBRB.SetWindowPos(this, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);

		m_hwndActive = m_EditBRB.m_hWnd;
	}
	else
		return ResLoadError();
}

void CViewRightBR::ParceDlgTemplate(PBYTE pDataDlgRes, size_t nSize)
{
#pragma pack(push, 4)
	struct DLGTEMPLATEEX //Helper struct. Not completed.
	{
		WORD      dlgVer;
		WORD      signature;
		DWORD     helpID;
		DWORD     exStyle;
		DWORD     style;
		WORD      cDlgItems;
		short     x;
		short     y;
		short     cx;
		short     cy;
		WORD      menu;
	};
	struct DLGITEMTEMPLATEEX //Helper struct. Not completed.
	{
		DWORD     helpID;
		DWORD     exStyle;
		DWORD     style;
		short     x;
		short     y;
		short     cx;
		short     cy;
		DWORD     id;
	};
#pragma pack(pop)

	//All Window and Dialog styles except default (0).
	static const std::map<DWORD, std::wstring> mapDlgStyles {
		TO_WSTR_MAP(DS_3DLOOK),
		TO_WSTR_MAP(DS_ABSALIGN),
		TO_WSTR_MAP(DS_CENTER),
		TO_WSTR_MAP(DS_CENTERMOUSE),
		TO_WSTR_MAP(DS_CONTEXTHELP),
		TO_WSTR_MAP(DS_CONTROL),
		TO_WSTR_MAP(DS_FIXEDSYS),
		TO_WSTR_MAP(DS_LOCALEDIT),
		TO_WSTR_MAP(DS_MODALFRAME),
		TO_WSTR_MAP(DS_NOFAILCREATE),
		TO_WSTR_MAP(DS_NOIDLEMSG),
		TO_WSTR_MAP(DS_SETFONT),
		TO_WSTR_MAP(DS_SETFOREGROUND),
		TO_WSTR_MAP(DS_SHELLFONT),
		TO_WSTR_MAP(DS_SYSMODAL),
		TO_WSTR_MAP(WS_BORDER),
		TO_WSTR_MAP(WS_CAPTION),
		TO_WSTR_MAP(WS_CHILD),
		TO_WSTR_MAP(WS_CHILDWINDOW),
		TO_WSTR_MAP(WS_CLIPCHILDREN),
		TO_WSTR_MAP(WS_CLIPSIBLINGS),
		TO_WSTR_MAP(WS_DISABLED),
		TO_WSTR_MAP(WS_DLGFRAME),
		TO_WSTR_MAP(WS_HSCROLL),
		TO_WSTR_MAP(WS_MAXIMIZE),
		TO_WSTR_MAP(WS_MAXIMIZEBOX),
		TO_WSTR_MAP(WS_MINIMIZE),
		TO_WSTR_MAP(WS_MINIMIZEBOX),
		TO_WSTR_MAP(WS_POPUP),
		TO_WSTR_MAP(WS_SYSMENU),
		TO_WSTR_MAP(WS_THICKFRAME),
		TO_WSTR_MAP(WS_VISIBLE),
		TO_WSTR_MAP(WS_VSCROLL)
	};

	//All Extended Window styles except default (0).
	static const std::map<DWORD, std::wstring> mapDlgExStyles
	{
		TO_WSTR_MAP(WS_EX_ACCEPTFILES),
		TO_WSTR_MAP(WS_EX_APPWINDOW),
		TO_WSTR_MAP(WS_EX_CLIENTEDGE),
		TO_WSTR_MAP(WS_EX_COMPOSITED),
		TO_WSTR_MAP(WS_EX_CONTEXTHELP),
		TO_WSTR_MAP(WS_EX_CONTROLPARENT),
		TO_WSTR_MAP(WS_EX_DLGMODALFRAME),
		TO_WSTR_MAP(WS_EX_LAYERED),
		TO_WSTR_MAP(WS_EX_LAYOUTRTL),
		TO_WSTR_MAP(WS_EX_LEFTSCROLLBAR),
		TO_WSTR_MAP(WS_EX_MDICHILD),
		TO_WSTR_MAP(WS_EX_NOACTIVATE),
		TO_WSTR_MAP(WS_EX_NOINHERITLAYOUT),
		TO_WSTR_MAP(WS_EX_NOPARENTNOTIFY),
		TO_WSTR_MAP(WS_EX_RIGHT),
		TO_WSTR_MAP(WS_EX_RTLREADING),
		TO_WSTR_MAP(WS_EX_STATICEDGE),
		TO_WSTR_MAP(WS_EX_TOOLWINDOW),
		TO_WSTR_MAP(WS_EX_TOPMOST),
		TO_WSTR_MAP(WS_EX_TRANSPARENT),
		TO_WSTR_MAP(WS_EX_WINDOWEDGE),
	};

	PBYTE pDataHdr = pDataDlgRes;
	bool fDlgEx { false };
	if (*(((PWORD)pDataHdr) + 1) == 0xFFFF) //DLGTEMPLATEEX
		fDlgEx = true;

	DWORD dwStyles, dwExStyles;
	WORD wDlgItems;
	short x, y, cx, cy;
	WCHAR* pwstrMenuRes { }; size_t lengthMenuRes { }; WORD wMenuResOrdinal { };
	WCHAR* pwstrTypeFace { }; size_t lengthTypeFace { };

	if (fDlgEx) //DLGTEMPLATEEX
	{
		dwStyles = ((DLGTEMPLATEEX*)pDataHdr)->style;
		dwExStyles = ((DLGTEMPLATEEX*)pDataHdr)->exStyle;
		wDlgItems = ((DLGTEMPLATEEX*)pDataHdr)->cDlgItems;
		x = ((DLGTEMPLATEEX*)pDataHdr)->x;
		y = ((DLGTEMPLATEEX*)pDataHdr)->y;
		cx = ((DLGTEMPLATEEX*)pDataHdr)->cx;
		cy = ((DLGTEMPLATEEX*)pDataHdr)->cy;

		m_wstrEditBRB = L"DIALOGEX ";

		//Menu.
		if (((DLGTEMPLATEEX*)pDataHdr)->menu == 0) //No menu.
			pDataHdr += sizeof(DLGTEMPLATEEX);
		else if (((DLGTEMPLATEEX*)pDataHdr)->menu == 0xFFFF) //Menu ordinal.
		{
			wMenuResOrdinal = *(PWORD)(pDataHdr + sizeof(WORD));
			pDataHdr += sizeof(WORD) * 2; //Ordinal's WORD follows ((DLGTEMPLATEEX*)pDataHdr)->menu.
		}
		else //Menu wstring.
		{
			pDataHdr += sizeof(DLGTEMPLATEEX);
			pwstrMenuRes = (WCHAR*)pDataHdr;
			if (StringCbLengthW(pwstrMenuRes, nSize - ((DWORD_PTR)pDataHdr - (DWORD_PTR)pDataDlgRes), &lengthMenuRes) != S_OK)
				return ResLoadError();
			pDataHdr += lengthMenuRes + sizeof(WCHAR); //Plus null terminating.
		}
	}
	else //DLGTEMPLATE
	{
		dwStyles = ((DLGTEMPLATE*)pDataHdr)->style;
		dwExStyles = ((DLGTEMPLATE*)pDataHdr)->dwExtendedStyle;
		wDlgItems = ((DLGTEMPLATE*)pDataHdr)->cdit;
		x = ((DLGTEMPLATE*)pDataHdr)->x;
		y = ((DLGTEMPLATE*)pDataHdr)->y;
		cx = ((DLGTEMPLATE*)pDataHdr)->cx;
		cy = ((DLGTEMPLATE*)pDataHdr)->cy;
		pDataHdr += sizeof(DLGTEMPLATE);

		m_wstrEditBRB = L"DIALOG ";

		//Menu.
		if (*(PWORD)pDataHdr == 0) //No menu.
			pDataHdr += sizeof(WORD);
		else if (*(PWORD)pDataHdr == 0xFFFF) //Menu ordinal.
		{
			wMenuResOrdinal = *(PWORD)(pDataHdr + sizeof(WORD));
			pDataHdr += sizeof(WORD) * 2; //Ordinal's WORD follows menu WORD.
		}
		else //Menu wstring.
		{
			pwstrMenuRes = (WCHAR*)& pDataHdr;
			if (StringCbLengthW(pwstrMenuRes, nSize - ((DWORD_PTR)pDataHdr - (DWORD_PTR)pDataDlgRes), &lengthMenuRes) != S_OK)
				return ResLoadError();
			pDataHdr += lengthMenuRes + sizeof(WCHAR); //Plus null terminating.
		}
	}

	WCHAR wpsz[128];
	swprintf_s(wpsz, L"%i, %i, %i, %i\r\n", x, y, cx, cy);
	m_wstrEditBRB += wpsz;

	//Dialog styles stringanize.
	std::wstring wstrStyles;
	for (auto& i : mapDlgStyles)
	{
		if (i.first & dwStyles)
			if (!wstrStyles.empty())
				wstrStyles += L" | " + i.second;
			else
				wstrStyles += i.second;
	}

	m_wstrEditBRB += L"DIALOG STYLES: " + wstrStyles + L"\r\n";
	if (dwExStyles) //ExStyle sringanize.
	{
		wstrStyles.clear();
		for (auto& i : mapDlgExStyles)
		{
			if (i.first & dwExStyles)
				if (!wstrStyles.empty())
					wstrStyles += L" | " + i.second;
				else
					wstrStyles += i.second;
		}
		m_wstrEditBRB += L"DIALOG EXTENDED STYLES: " + wstrStyles + L"\r\n";
	}

	//////////////////////////////////////////////////////////////////
	//Next goes Class Name and Title that common for both templates.
	//////////////////////////////////////////////////////////////////

	//Class name.
	if (*(PWORD)pDataHdr == 0xFFFF) //Class name is ordinal.
	{
		WORD wClassOrdinal = *(PWORD)(pDataHdr + sizeof(WORD));
		pDataHdr += sizeof(WORD) * 2; //Class name WORD plus Ordinal WORD.

		m_wstrEditBRB += L"DIALOG CLASS ORDINAL: ";
		swprintf_s(wpsz, L"0x%X\r\n", wClassOrdinal);
		m_wstrEditBRB += wpsz;
	}
	else //Class name is WString.
	{
		if (*(PWORD)pDataHdr != 0x0000) //If not NULL then there is a need to align.
			pDataHdr += (sizeof(WORD) - (((DWORD_PTR)pDataHdr - (DWORD_PTR)pDataDlgRes) & 1)) & 1; //WORD Aligning.
		size_t lengthClassName;
		WCHAR* pwstrClassName = (WCHAR*)pDataHdr;
		if (StringCbLengthW(pwstrClassName, nSize - ((DWORD_PTR)pDataHdr - (DWORD_PTR)pDataDlgRes), &lengthClassName) != S_OK)
			return ResLoadError();
		pDataHdr += lengthClassName + sizeof(WCHAR); //Plus null terminating.

		m_wstrEditBRB += L"DIALOG CLASS NAME: \"";
		m_wstrEditBRB += pwstrClassName;
		m_wstrEditBRB += L"\"\r\n";
	}

	//Title (Caption)
	if (*(PWORD)pDataHdr != 0x0000) //If not NULL then there is a need to align.
		pDataHdr += (sizeof(WORD) - (((DWORD_PTR)pDataHdr - (DWORD_PTR)pDataDlgRes) & 1)) & 1; //WORD Aligning.
	size_t lengthTitle;
	WCHAR* pwstrTitle = (WCHAR*)pDataHdr;
	if (StringCbLengthW(pwstrTitle, nSize - ((DWORD_PTR)pDataHdr - (DWORD_PTR)pDataDlgRes), &lengthTitle) != S_OK)
		return ResLoadError();
	pDataHdr += lengthTitle + sizeof(WCHAR); //Plus null terminating.

	m_wstrEditBRB += L"DIALOG CAPTION: \"";
	m_wstrEditBRB += pwstrTitle;
	m_wstrEditBRB += L"\"\r\n";

	//Menu.
	if (pwstrMenuRes && lengthMenuRes)
	{
		m_wstrEditBRB += L"DIALOG MENU RESOURCE NAME: \"";
		m_wstrEditBRB += pwstrMenuRes;
		m_wstrEditBRB += L"\"\r\n";
	}
	else if (wMenuResOrdinal)
	{
		m_wstrEditBRB += L"DIALOG MENU RESOURCE ORDINAL: ";
		swprintf_s(wpsz, L"0x%X\r\n", wMenuResOrdinal);
		m_wstrEditBRB += wpsz;
	}

	//Font related stuff has little differences between templates.
	m_wstrEditBRB += L"DIALOG FONT: ";
	WORD wFontPointSize { };
	if (fDlgEx && ((dwStyles & DS_SETFONT) || (dwStyles & DS_SHELLFONT))) //DLGTEMPLATEEX
	{
		//Font related. Only if DS_SETFONT or DS_SHELLFONT styles present.

		wFontPointSize = *(PWORD)pDataHdr;
		pDataHdr += sizeof(wFontPointSize);

		WORD wFontWeight = *(PWORD)pDataHdr;
		pDataHdr += sizeof(wFontWeight);

		BYTE bItalic = *pDataHdr;
		pDataHdr += sizeof(bItalic);

		BYTE bCharset = *pDataHdr;
		pDataHdr += sizeof(bCharset);

		if (*(PWORD)pDataHdr != 0x0000) //If not NULL nedd to align.
			pDataHdr += (sizeof(WORD) - (((DWORD_PTR)pDataHdr - (DWORD_PTR)pDataDlgRes) & 1)) & 1; //WORD Aligning.
		pwstrTypeFace = (WCHAR*)pDataHdr;
		if (StringCbLengthW(pwstrTypeFace, nSize - ((DWORD_PTR)pDataHdr - (DWORD_PTR)pDataDlgRes), &lengthTypeFace) != S_OK)
			return ResLoadError();
		pDataHdr += lengthTypeFace + sizeof(WCHAR); //Plus null terminating.

		m_wstrEditBRB += L"NAME: \"";
		m_wstrEditBRB += pwstrTypeFace;
		m_wstrEditBRB += L"\"";
		//These Font params are only in DLGTEMPLATEEX.
		swprintf_s(wpsz, L", SIZE: %hu, WEIGHT: %hu, IS ITALIC: %hhu, CHARSET: %hhu", wFontPointSize, wFontWeight, bItalic, bCharset);
		m_wstrEditBRB += wpsz;
		m_wstrEditBRB += L"\r\n";
	}
	else if (!fDlgEx && (dwStyles & DS_SETFONT)) //DLGTEMPLATE
	{
		//Font related. Only if DS_SETFONT style present.

		wFontPointSize = *(PWORD)pDataHdr;
		pDataHdr += sizeof(wFontPointSize);

		if (*(PWORD)pDataHdr != 0x0000)
			pDataHdr += (sizeof(WORD) - (((DWORD_PTR)pDataHdr - (DWORD_PTR)pDataDlgRes) & 1)) & 1; //WORD Aligning.
		pwstrTypeFace = (WCHAR*)pDataHdr;
		if (StringCbLengthW(pwstrTypeFace, nSize - ((DWORD_PTR)pDataHdr - (DWORD_PTR)pDataDlgRes), &lengthTypeFace) != S_OK)
			return ResLoadError();
		pDataHdr += lengthTypeFace + sizeof(WCHAR); //Plus null terminating.

		m_wstrEditBRB += L"NAME \"";
		m_wstrEditBRB += pwstrTypeFace;
		m_wstrEditBRB += L"\"";
		swprintf_s(wpsz, L", SIZE: %hu", wFontPointSize); //Only SIZE in DLGTEMPLATE.
		m_wstrEditBRB += wpsz;
		m_wstrEditBRB += L"\r\n";
	}

	m_wstrEditBRB += L"DIALOG ITEMS: ";
	swprintf_s(wpsz, L"%i\r\n", wDlgItems);
	m_wstrEditBRB += wpsz;
	//DLGTEMPLATE(EX) end. ///////////////////////////////////////////

	//////////////////////////////////////////////////////////////////
	//Now go DLGITEMTEMPLATE(EX) data structures.
	//////////////////////////////////////////////////////////////////
	PBYTE pDataItems = pDataHdr; //Just to differentiate.

	m_wstrEditBRB += L"{\r\n"; //Open bracer for stringanize.
	for (WORD items = 0; items < wDlgItems; items++)
	{
		pDataItems += (sizeof(DWORD) - (((DWORD_PTR)pDataItems - (DWORD_PTR)pDataDlgRes) & 3)) & 3; //DWORD Aligning.
		//Out of bounds checking.
		if ((DWORD_PTR)pDataItems >= (DWORD_PTR)((PBYTE)pDataDlgRes) + nSize)
			break;

		DWORD dwItemStyles, dwItemExStyles;
		short xItem, yItem, cxItem, cyItem;
		WCHAR wpszItemStyles[128]; //Styles, ExStyles, ItemID and Coords.
		if (fDlgEx) //DLGITEMTEMPLATEEX
		{
			dwItemStyles = ((DLGITEMTEMPLATEEX*)pDataItems)->style;
			dwItemExStyles = ((DLGITEMTEMPLATEEX*)pDataItems)->exStyle;
			xItem = ((DLGITEMTEMPLATEEX*)pDataItems)->x;
			yItem = ((DLGITEMTEMPLATEEX*)pDataItems)->y;
			cxItem = ((DLGITEMTEMPLATEEX*)pDataItems)->cx;
			cyItem = ((DLGITEMTEMPLATEEX*)pDataItems)->cy;
			DWORD dwIDItem = ((DLGITEMTEMPLATEEX*)pDataItems)->id;

			pDataItems += sizeof(DLGITEMTEMPLATEEX);

			//Styles, ExStyles, ItemID and Coords. ItemID is DWORD;
			swprintf_s(wpszItemStyles, L"Styles: 0x%08lX, ExStyles: 0x%08lX, ItemID: 0x%08lX, Coords: x=%i, y=%i, cx=%i, cy=%i",
				dwItemStyles, dwItemExStyles, dwIDItem, xItem, yItem, cxItem, cyItem);
		}
		else //DLGITEMTEMPLATE
		{
			dwItemStyles = ((DLGITEMTEMPLATE*)pDataItems)->style;
			dwItemExStyles = ((DLGITEMTEMPLATE*)pDataItems)->dwExtendedStyle;
			xItem = ((DLGITEMTEMPLATE*)pDataItems)->x;
			yItem = ((DLGITEMTEMPLATE*)pDataItems)->y;
			cxItem = ((DLGITEMTEMPLATE*)pDataItems)->cx;
			cyItem = ((DLGITEMTEMPLATE*)pDataItems)->cy;
			WORD wIDItem = ((DLGITEMTEMPLATE*)pDataItems)->id;

			pDataItems += sizeof(DLGITEMTEMPLATE);

			//Styles, ExStyles, ItemID and Coords. ItemID is WORD;
			swprintf_s(wpszItemStyles, L"Styles: 0x%08lX, ExStyles: 0x%08lX, ItemID: 0x%04hX, Coords: x=%i, y=%i, cx=%i, cy=%i",
				dwItemStyles, dwItemExStyles, wIDItem, xItem, yItem, cxItem, cyItem);
		}

		static const std::map<WORD, std::wstring> mapItemClassOrd {
			{ 0x0080, L"Button" },
		{ 0x0081, L"Edit" },
		{ 0x0082, L"Static" },
		{ 0x0083, L"List box" },
		{ 0x0084, L"Scroll bar" },
		{ 0x0085, L"Combo box" },
		};
		std::wstring wstrItem = L"    ";

		//Item Class name.
		if (*(PWORD)pDataItems == 0xFFFF) //Class is ordinal
		{
			WORD wClassOrdinalItem = *(PWORD)(pDataItems + sizeof(WORD));
			pDataItems += sizeof(WORD) * 2;

			swprintf_s(wpsz, L"Class ordinal: 0x%04X", wClassOrdinalItem);
			wstrItem += wpsz;
			auto iter = mapItemClassOrd.find(wClassOrdinalItem);
			if (iter != mapItemClassOrd.end())
				wstrItem += L" (\"" + iter->second + L"\")";
			wstrItem += L", ";
		}
		else //Class name is Wstring.
		{
			pDataItems += (sizeof(WORD) - (((DWORD_PTR)pDataItems - (DWORD_PTR)pDataDlgRes) & 1)) & 1; //WORD Aligning.
			size_t lengthClassNameItem;
			WCHAR* pwstrClassNameItem = (WCHAR*)pDataItems;
			if (StringCbLengthW(pwstrClassNameItem, nSize - ((DWORD_PTR)pDataItems - (DWORD_PTR)pDataDlgRes), &lengthClassNameItem) != S_OK)
				return ResLoadError();
			pDataItems += lengthClassNameItem + sizeof(WCHAR); //Plus null terminating.

			wstrItem += L"Class name: \"";
			wstrItem += pwstrClassNameItem;
			wstrItem += L"\", ";
		}

		//Item Title (Caption).
		if (*(PWORD)pDataItems == 0xFFFF) //Item Title is ordinal
		{
			WORD wTitleOrdinalItem = *(PWORD)(pDataItems + sizeof(WORD));
			pDataItems += sizeof(WORD) * 2;

			swprintf_s(wpsz, L"Caption ordinal: 0x%04X, ", wTitleOrdinalItem);
			wstrItem += wpsz;
		}
		else //Title is wstring.
		{
			pDataItems += (sizeof(WORD) - (((DWORD_PTR)pDataItems - (DWORD_PTR)pDataDlgRes) & 1)) & 1; //WORD Aligning.
			size_t lengthTitleItem;
			WCHAR* pwstrTitleItem = (WCHAR*)pDataItems;
			if (StringCbLengthW(pwstrTitleItem, nSize - ((DWORD_PTR)pDataItems - (DWORD_PTR)pDataDlgRes), &lengthTitleItem) != S_OK)
				return ResLoadError();
			pDataItems += lengthTitleItem + sizeof(WCHAR); //Plus null terminating.

			wstrItem += L"Caption: \"";
			wstrItem += pwstrTitleItem;
			wstrItem += L"\", ";
		}

		//Extra count Item.
		WORD wExtraCountItem = *(PWORD)pDataItems;
		pDataItems += sizeof(WORD);
		if (wExtraCountItem)
		{
			pDataItems += (sizeof(WORD) - (((DWORD_PTR)pDataItems - (DWORD_PTR)pDataDlgRes) & 1)) & 1; //WORD Aligning.
			pDataItems += wExtraCountItem;
		}

		wstrItem += wpszItemStyles;
		m_wstrEditBRB += wstrItem + L"\r\n";
	}
	m_wstrEditBRB += L"}";
}

void CViewRightBR::CreateStrings(const RESHELPER * pResHelper)
{
	LPCWSTR pwszResString = reinterpret_cast<LPCWSTR>(pResHelper->pData->data());
	std::wstring wstrTmp;
	for (int i = 0; i < 16; i++)
	{
		m_wstrEditBRB += wstrTmp.assign(pwszResString + 1, (UINT)* pwszResString);
		if (i != 15)
			m_wstrEditBRB += L"\r\n";
		pwszResString += 1 + (UINT)* pwszResString;
	}

	m_EditBRB.SetWindowTextW(m_wstrEditBRB.data());
	CRect rcClient;
	GetClientRect(&rcClient);
	m_EditBRB.SetWindowPos(this, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
	m_hwndActive = m_EditBRB.m_hWnd;
}

void CViewRightBR::CreateGroupIconCursor(const RESHELPER * pResHelper)
{
	PCLIBPE_RESOURCE_ROOT pstResRoot;
	if (m_pLibpe->GetResources(pstResRoot) != S_OK)
		return;

#pragma pack(push, 2)
	struct GRPICONDIRENTRY
	{
		BYTE   bWidth;               // Width, in pixels, of the image
		BYTE   bHeight;              // Height, in pixels, of the image
		BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
		BYTE   bReserved;            // Reserved
		WORD   wPlanes;              // Color Planes
		WORD   wBitCount;            // Bits per pixel
		DWORD  dwBytesInRes;         // how many bytes in this resource?
		WORD   nID;                  // the ID
	};
	struct GRPICONDIR
	{
		WORD			  idReserved;   // Reserved (must be 0)
		WORD			  idType;	    // Resource type (1 for icons)
		WORD			  idCount;	    // How many images?
		GRPICONDIRENTRY   idEntries[1]; // The entries for each image
	};
	using LPGRPICONDIR = const GRPICONDIR*;
#pragma pack(pop)

	LPGRPICONDIR pGRPIDir = (LPGRPICONDIR)pResHelper->pData->data();
	HICON hIcon;
	ICONINFO iconInfo;

	for (int i = 0; i < pGRPIDir->idCount; i++)
	{
		auto& rootvec = pstResRoot->vecResRoot;
		for (auto& iterRoot : rootvec)
		{
			if (iterRoot.stResDirEntryRoot.Id == 3 || //RT_ICON
				iterRoot.stResDirEntryRoot.Id == 1)   //RT_CURSOR
			{
				auto& lvl2tup = iterRoot.stResLvL2;
				auto& lvl2vec = lvl2tup.vecResLvL2;

				for (auto& iterlvl2 : lvl2vec)
				{
					if (iterlvl2.stResDirEntryLvL2.Id == pGRPIDir->idEntries[i].nID)
					{
						auto& lvl3tup = iterlvl2.stResLvL3;
						auto& lvl3vec = lvl3tup.vecResLvL3;

						if (!lvl3vec.empty())
						{
							auto& data = lvl3vec.at(0).vecResRawDataLvL3;
							if (!data.empty())
							{
								hIcon = CreateIconFromResourceEx((PBYTE)data.data(), (DWORD)data.size(),
									(iterRoot.stResDirEntryRoot.Id == 3) ? TRUE : FALSE, 0x00030000, 0, 0, LR_DEFAULTCOLOR);
								if (!hIcon)
									return ResLoadError();
								if (!GetIconInfo(hIcon, &iconInfo))
									return ResLoadError();
								if (!GetObjectW(iconInfo.hbmMask, sizeof(BITMAP), &m_stBmp))
									return ResLoadError();

								DeleteObject(iconInfo.hbmColor);
								DeleteObject(iconInfo.hbmMask);

								m_vecImgRes.emplace_back(std::make_unique<CImageList>());
								auto& vecBack = m_vecImgRes.back();
								vecBack->Create(m_stBmp.bmWidth,
									(iterRoot.stResDirEntryRoot.Id == 3) ? m_stBmp.bmHeight : m_stBmp.bmWidth, ILC_COLORDDB, 0, 1);
								vecBack->SetBkColor(m_clrBkImgList);
								m_iImgResWidth += m_stBmp.bmWidth;
								m_iImgResHeight = max(m_stBmp.bmHeight, m_iImgResHeight);

								if (vecBack->Add(hIcon) == -1)
									return ResLoadError();

								DestroyIcon(hIcon);
								break;
							}
						}
					}
				}
			}
		}
	}
	SetScrollSizes(MM_TEXT, CSize(m_iImgResWidth, m_iImgResHeight));

	m_iResTypeToDraw = pResHelper->IdResType;
	m_fDrawRes = true;
}

void CViewRightBR::CreateVersion(const RESHELPER * pResHelper)
{
#pragma pack(push, 4)
	struct LANGANDCODEPAGE
	{
		WORD wLanguage;
		WORD wCodePage;
	};
#pragma pack(pop)

	static const std::map<int, std::wstring> mapVerInfoStrings {
		{ 0, L"FileDescription" },
	{ 1, L"FileVersion" },
	{ 2, L"InternalName" },
	{ 3, L"CompanyName" },
	{ 4, L"LegalCopyright" },
	{ 5, L"OriginalFilename" },
	{ 6, L"ProductName" },
	{ 7, L"ProductVersion" }
	};

	LANGANDCODEPAGE* pLangAndCP;
	UINT dwBytesOut;

	//Read the list of languages and code pages.
	VerQueryValueW(pResHelper->pData->data(), L"\\VarFileInfo\\Translation", (LPVOID*)& pLangAndCP, &dwBytesOut);

	WCHAR wstrSubBlock[50];
	DWORD dwLangCount = dwBytesOut / sizeof(LANGANDCODEPAGE);
	//Read the file description for each language and code page.
	for (unsigned iterCodePage = 0; iterCodePage < dwLangCount; iterCodePage++)
	{
		for (unsigned i = 0; i < mapVerInfoStrings.size(); i++) //sizeof pstrVerInfoStrings [];
		{
			swprintf_s(wstrSubBlock, 50, L"\\StringFileInfo\\%04x%04x\\%s",
				pLangAndCP[iterCodePage].wLanguage, pLangAndCP[iterCodePage].wCodePage, mapVerInfoStrings.at(i).data());

			m_wstrEditBRB += mapVerInfoStrings.at(i).data();
			m_wstrEditBRB += L" - ";

			WCHAR* pszBufferOut;
			if (VerQueryValueW(pResHelper->pData->data(), wstrSubBlock, (LPVOID*)& pszBufferOut, &dwBytesOut))
				if (dwBytesOut)
					m_wstrEditBRB += pszBufferOut;
			m_wstrEditBRB += L"\r\n";
		}
	}
	m_EditBRB.SetWindowTextW(m_wstrEditBRB.data());
	CRect rcClient;
	GetClientRect(&rcClient);
	m_EditBRB.SetWindowPos(this, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);

	m_hwndActive = m_EditBRB.m_hWnd;
}

void CViewRightBR::CreateManifest(const RESHELPER * pResHelper)
{
	m_wstrEditBRB.resize(pResHelper->pData->size());
	MultiByteToWideChar(CP_UTF8, 0, (LPCCH)pResHelper->pData->data(), (int)pResHelper->pData->size(), &m_wstrEditBRB[0], (int)pResHelper->pData->size());

	m_EditBRB.SetWindowTextW(m_wstrEditBRB.data());
	CRect rcClient;
	GetClientRect(&rcClient);
	m_EditBRB.SetWindowPos(this, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);

	m_hwndActive = m_EditBRB.m_hWnd;
}

void CViewRightBR::CreateToolbar(const RESHELPER * pResHelper)
{
	PCLIBPE_RESOURCE_ROOT pstResRoot;
	if (m_pLibpe->GetResources(pstResRoot) != S_OK)
		return;

	auto& rootvec = pstResRoot->vecResRoot;
	for (auto& iterRoot : rootvec)
	{
		if (iterRoot.stResDirEntryRoot.Id == 2) //RT_BITMAP
		{
			auto& lvl2tup = iterRoot.stResLvL2;
			auto& lvl2vec = lvl2tup.vecResLvL2;

			for (auto& iterlvl2 : lvl2vec)
			{
				if (iterlvl2.stResDirEntryLvL2.Id == pResHelper->IdResName)
				{
					auto& lvl3tup = iterlvl2.stResLvL3;
					auto& lvl3vec = lvl3tup.vecResLvL3;

					if (!lvl3vec.empty())
					{
						auto& data = lvl3vec.at(0).vecResRawDataLvL3;
						if (!data.empty())
						{
							RESHELPER rh(2, pResHelper->IdResName, (std::vector<std::byte>*) & data);
							ShowResource(&rh);
						}
					}
				}
			}
		}
	}
}

void CViewRightBR::OnDraw(CDC* pDC)
{
	if (!m_fDrawRes)
		return;

	CRect rcClient;
	GetClientRect(&rcClient);

	SCROLLINFO stScroll { sizeof(SCROLLINFO), SIF_ALL };
	GetScrollInfo(SB_HORZ, &stScroll, SIF_ALL);
	int nScrollHorz = stScroll.nPos;
	GetScrollInfo(SB_VERT, &stScroll, SIF_ALL);
	int nScrollVert = stScroll.nPos;

	rcClient.top += nScrollVert;
	rcClient.bottom += nScrollVert;
	rcClient.left += nScrollHorz;
	rcClient.right += nScrollHorz;
	pDC->FillSolidRect(rcClient, RGB(255, 255, 255));

	CSize sizeScroll = GetTotalSize();
	CPoint ptDrawAt;
	int x, y;

	switch (m_iResTypeToDraw)
	{
	case 1: //RT_CURSOR
	case 2: //RT_BITMAP
	case 3: //RT_ICON
	{
		//Draw at center independing of scrolls.
		pDC->FillSolidRect(rcClient, m_clrBkIcons);

		if (sizeScroll.cx > rcClient.Width())
			x = sizeScroll.cx / 2 - (m_stBmp.bmWidth / 2);
		else
			x = rcClient.Width() / 2 - (m_stBmp.bmWidth / 2);
		if (sizeScroll.cy > rcClient.Height())
			y = sizeScroll.cy / 2 - (m_stBmp.bmHeight / 2);
		else
			y = rcClient.Height() / 2 - (m_stBmp.bmHeight / 2);

		ptDrawAt.SetPoint(x, y);
		m_stImgRes.Draw(pDC, 0, ptDrawAt, ILD_NORMAL);
		break;
	}
	case 5: //RT_DIALOG
		break;
	case 12: //RT_GROUP_CURSOR
	case 14: //RT_GROUP_ICON
	{
		pDC->FillSolidRect(rcClient, m_clrBkIcons);

		if (sizeScroll.cx > rcClient.Width())
			x = sizeScroll.cx / 2 - (m_iImgResWidth / 2);
		else
			x = rcClient.Width() / 2 - (m_iImgResWidth / 2);

		for (int i = 0; i < (int)m_vecImgRes.size(); i++)
		{
			IMAGEINFO imginfo;
			m_vecImgRes.at(i)->GetImageInfo(0, &imginfo);
			int iImgHeight = imginfo.rcImage.bottom - imginfo.rcImage.top;
			if (sizeScroll.cy > rcClient.Height())
				y = sizeScroll.cy / 2 - (iImgHeight / 2);
			else
				y = rcClient.Height() / 2 - (iImgHeight / 2);

			ptDrawAt.SetPoint(x, y);
			m_vecImgRes.at(i)->Draw(pDC, 0, ptDrawAt, ILD_NORMAL);
			x += imginfo.rcImage.right - imginfo.rcImage.left;
		}
		break;
	}
	case 0xFF:
		pDC->FillSolidRect(rcClient, RGB(255, 255, 255));
		pDC->SetTextColor(RGB(255, 0, 0));
		pDC->TextOutW(0, 0, L"Unable to load resource! It's either damaged, packed or zero-length.");
		break;
	default:
		pDC->TextOutW(0, 0, L"This Resource type is not supported.");
	}
}

void CViewRightBR::OnSize(UINT nType, int cx, int cy)
{
	CScrollView::OnSize(nType, cx, cy);

	if (m_hwndActive)
		::SetWindowPos(m_hwndActive, m_hWnd, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER);
}

int CViewRightBR::CreateListTLSCallbacks()
{
	PCLIBPE_TLS pTLS;
	if (m_pLibpe->GetTLS(pTLS) != S_OK)
		return -1;

	m_stlcs.dwStyle = 0;
	m_stlcs.uID = IDC_LIST_TLS_CALLBACKS;
	m_stListTLSCallbacks->Create(m_stlcs);
	m_stListTLSCallbacks->InsertColumn(0, L"TLS Callbacks", LVCFMT_CENTER | LVCFMT_FIXED_WIDTH, 300);

	int listindex { };
	WCHAR wstr[9];

	for (auto& iterCallbacks : pTLS->vecTLSCallbacks)
	{
		swprintf_s(wstr, 9, L"%08X", iterCallbacks);
		m_stListTLSCallbacks->InsertItem(listindex, wstr);
		listindex++;
	}

	return 0;
}

void CViewRightBR::ResLoadError()
{
	m_iResTypeToDraw = 0xFF;
	m_fDrawRes = true;
	RedrawWindow();
}

void CViewRightBR::CreateDebugEntry(DWORD dwEntry)
{
	PCLIBPE_DEBUG_VEC pDebug;
	if (m_pLibpe->GetDebug(pDebug) != S_OK)
		return;

	//At the moment only IMAGE_DEBUG_TYPE_CODEVIEW info is supported.
	if (pDebug->at(dwEntry).stDebugDir.Type != IMAGE_DEBUG_TYPE_CODEVIEW)
		return;

	m_wstrEditBRB.clear();
	auto& refDebug = pDebug->at(dwEntry);
	WCHAR warr[9] { };

	if (refDebug.stDebugHdrInfo.dwHdr[0] == 0x53445352) //"RSDS"
	{
		m_wstrEditBRB = L"Signature: RSDS\r\n";
		m_wstrEditBRB += L"GUID: ";
		LPWSTR lpwstr;
		GUID guid = *((GUID*)& refDebug.stDebugHdrInfo.dwHdr[1]);
		StringFromIID(guid, &lpwstr);
		m_wstrEditBRB += lpwstr;
		m_wstrEditBRB += L"\r\n";
		m_wstrEditBRB += L"Counter/Age: ";
		DwordToWchars(refDebug.stDebugHdrInfo.dwHdr[5], warr);
		m_wstrEditBRB += warr;
		m_wstrEditBRB += L"\r\n";
	}
	else if (refDebug.stDebugHdrInfo.dwHdr[0] == 0x3031424E) //"NB10"
	{
		m_wstrEditBRB = L"Signature: NB10\r\n";
		m_wstrEditBRB += L"Offset: ";
		DwordToWchars(refDebug.stDebugHdrInfo.dwHdr[1], warr);
		m_wstrEditBRB += warr;
		m_wstrEditBRB += L"\r\n";
		m_wstrEditBRB += L"Time/Signature: ";
		DwordToWchars(refDebug.stDebugHdrInfo.dwHdr[2], warr);
		m_wstrEditBRB += warr;
		m_wstrEditBRB += L"\r\n";
		m_wstrEditBRB += L"Counter/Age: ";
		DwordToWchars(refDebug.stDebugHdrInfo.dwHdr[3], warr);
		m_wstrEditBRB += warr;
		m_wstrEditBRB += L"\r\n";
	}

	if (!refDebug.stDebugHdrInfo.strPDBName.empty())
	{
		std::wstring wstr;
		wstr.resize(refDebug.stDebugHdrInfo.strPDBName.size());
		MultiByteToWideChar(CP_UTF8, 0, (LPCCH)refDebug.stDebugHdrInfo.strPDBName.data(),
			(int)refDebug.stDebugHdrInfo.strPDBName.size(), &wstr[0], (int)refDebug.stDebugHdrInfo.strPDBName.size());
		m_wstrEditBRB += L"PDB File: ";
		m_wstrEditBRB += wstr;
		m_EditBRB.SetWindowTextW(m_wstrEditBRB.data());
	}

	CRect rc;
	GetClientRect(&rc);
	m_EditBRB.SetWindowPos(this, rc.left, rc.top, rc.right, rc.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);

	m_hwndActive = m_EditBRB.m_hWnd;
}