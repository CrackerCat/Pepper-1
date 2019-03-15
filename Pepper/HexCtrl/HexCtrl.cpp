/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/Pepper/blob/master/LICENSE                                 *
* This is a Hex control for MFC apps, implemented as CWnd derived class.			    *
* The usage is quite simple:														    *
* 1. Construct CHexCtrl object — HEXCTRL::CHexCtrl myHex;								*
* 2. Call myHex.Create member function to create an instance.   					    *
* 3. Call myHex.SetData method to set the data and its size to display as hex.	        *
****************************************************************************************/
#include "stdafx.h"
#include "HexCtrl.h"
#include "ScrollEx.h"
#include "HexCtrlDlgAbout.h"
#include "HexCtrlDlgSearch.h"
#include "strsafe.h"
#pragma comment(lib, "Dwmapi.lib")

using namespace HEXCTRL;

namespace HEXCTRL {
	/********************************************
	* Internal enums and structs.				*
	********************************************/
	namespace {
		enum HEXCTRL_SHOWAS { ASBYTE = 1, ASWORD = 2, ASDWORD = 4, ASQWORD = 8 };
		enum HEXCTRL_CLIPBOARD { COPY_HEX, COPY_HEXFORMATTED, COPY_ASCII };
		enum HEXCTRL_MENU {
			IDM_MAIN_SEARCH, IDM_MAIN_COPYASHEX, IDM_MAIN_COPYASHEXFORMATTED, IDM_MAIN_COPYASASCII, IDM_MAIN_ABOUT,
			IDM_SUB_SHOWASBYTE, IDM_SUB_SHOWASWORD, IDM_SUB_SHOWASDWORD, IDM_SUB_SHOWASQWORD
		};
	}
}

/************************************************************************
* CHexCtrl implementation.												*
************************************************************************/
BEGIN_MESSAGE_MAP(CHexCtrl, CWnd)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_WM_MBUTTONDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_WM_NCACTIVATE()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_CHAR()
END_MESSAGE_MAP()

CHexCtrl::CHexCtrl()
{
	for (unsigned i = 0; i < m_dwCapacityMax; i++)
	{
		WCHAR wstr[3];
		swprintf_s(wstr, 3, L"%X", i);
		m_umapCapacity[i] = wstr;
	}

	//Submenu for data showing options:
	m_menuSubShowAs.CreatePopupMenu();
	m_menuSubShowAs.AppendMenuW(MF_STRING | MF_CHECKED, IDM_SUB_SHOWASBYTE, L"BYTE");
	m_menuSubShowAs.AppendMenuW(MF_STRING, IDM_SUB_SHOWASWORD, L"WORD");
	m_menuSubShowAs.AppendMenuW(MF_STRING, IDM_SUB_SHOWASDWORD, L"DWORD");
	m_menuSubShowAs.AppendMenuW(MF_STRING, IDM_SUB_SHOWASQWORD, L"QWORD");

	//Main menu:
	m_menuMain.CreatePopupMenu();
	m_menuMain.AppendMenuW(MF_STRING, IDM_MAIN_SEARCH, L"Search...	Ctrl+F");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_POPUP, (DWORD_PTR)m_menuSubShowAs.m_hMenu, L"Show data as...");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_STRING, IDM_MAIN_COPYASHEX, L"Copy as Hex...	Ctrl+C");
	m_menuMain.AppendMenuW(MF_STRING, IDM_MAIN_COPYASHEXFORMATTED, L"Copy as Formatted Hex...");
	m_menuMain.AppendMenuW(MF_STRING, IDM_MAIN_COPYASASCII, L"Copy as Ascii...");
	m_menuMain.AppendMenuW(MF_SEPARATOR);
	m_menuMain.AppendMenuW(MF_STRING, IDM_MAIN_ABOUT, L"About");

	m_pDlgSearch->Create(IDD_HEXCTRL_SEARCH, this);

	m_dwShowAs = HEXCTRL_SHOWAS::ASBYTE;
}

CHexCtrl::~CHexCtrl()
{
}

bool CHexCtrl::Create(const HEXCREATESTRUCT& hcs)
{
	if (IsCreated()) //Already created.
		return false;

	m_dwCtrlId = hcs.uId;
	m_fFloat = hcs.fFloat;
	m_pwndParentOwner = hcs.pwndParent;
	if (hcs.pwndMsg)
		m_pwndMsg = hcs.pwndMsg;
	else
		m_pwndMsg = hcs.pwndParent;
	if (hcs.pstColor)
		m_stColor = *hcs.pstColor;

	m_stBrushBkSelected.CreateSolidBrush(m_stColor.clrBkSelected);

	DWORD dwStyle;
	if (m_fFloat)
		dwStyle = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
	else
		dwStyle = WS_VISIBLE | WS_CHILD;

	//Font related.//////////////////////////////////////////////
	LOGFONTW lf { };
	if (hcs.pLogFont)
		lf = *hcs.pLogFont;
	else {
		StringCchCopyW(lf.lfFaceName, 9, L"Consolas");
		lf.lfHeight = 18;
	}

	if (!m_fontHexView.CreateFontIndirectW(&lf))
	{
		NONCLIENTMETRICSW ncm { sizeof(NONCLIENTMETRICSW) };
		SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		lf = ncm.lfMessageFont;
		lf.lfHeight = 18; //For some reason above func returns lfHeight value as MAX_LONG.

		m_fontHexView.CreateFontIndirectW(&lf);
	}
	lf.lfHeight = 16;
	m_fontBottomRect.CreateFontIndirectW(&lf);
	//End of font related.///////////////////////////////////////

	CRect rc = hcs.rect;
	if (rc.IsRectNull() && m_fFloat)
	{	//If initial rect is null, and it's a float window HexCtrl, then place it at screen center.

		int iPosX = GetSystemMetrics(SM_CXSCREEN) / 4;
		int iPosY = GetSystemMetrics(SM_CYSCREEN) / 4;
		int iPosCX = iPosX * 3;
		int iPosCY = iPosY * 3;
		rc.SetRect(iPosX, iPosY, iPosCX, iPosCY);
	}

	HCURSOR hCur;
	if (!(hCur = (HCURSOR)LoadImageW(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED)))
		return false;
	if (!CWnd::CreateEx(hcs.dwExStyles, AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW, hCur),
		L"HexControl", dwStyle, rc, m_pwndParentOwner, m_fFloat ? 0 : m_dwCtrlId))
		return false;

	//Removing window's border frame.
	MARGINS marg { 0, 0, 0, 1 };
	DwmExtendFrameIntoClientArea(m_hWnd, &marg);
	SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	m_pstScrollV->Create(this, SB_VERT, 0, 0, 0); //Actual sizes are set in RecalcAll().
	m_pstScrollH->Create(this, SB_HORZ, 0, 0, 0);
	m_pstScrollV->AddSibling(m_pstScrollH.get());
	m_pstScrollH->AddSibling(m_pstScrollV.get());

	RecalcAll();
	m_fCreated = true;

	return true;
}

bool CHexCtrl::IsCreated()
{
	return m_fCreated;
}

void CHexCtrl::SetData(const HEXDATASTRUCT& hds)
{
	if (hds.pwndMsg)
		m_pwndMsg = hds.pwndMsg;

	//Virtual mode is possible only when there is a msg window
	//to which data requests will be sent.
	if (hds.fVirtual && !m_pwndMsg)
		return;

	m_pData = hds.pData;
	m_ullDataSize = hds.ullDataSize;
	m_fVirtual = hds.fVirtual;
	m_fMutable = hds.fMutable;
	m_dwOffsetDigits = hds.ullDataSize <= 0xfffffffful ? 8 : (hds.ullDataSize < 0xfffffffffful ? 10 : (hds.ullDataSize < 0xfffffffffffful ? 12 :
		(hds.ullDataSize < 0xfffffffffffffful ? 14 : 16)));
	RecalcAll();

	if (hds.ullSelectionSize)
		ShowOffset(hds.ullSelectionStart, hds.ullSelectionSize);
	else
	{
		m_ullSelectionClick = m_ullSelectionStart = m_ullSelectionEnd = m_ullBytesSelected = 0;
		m_pstScrollV->SetScrollPos(0);
		m_pstScrollH->SetScrollPos(0);
		UpdateInfoText();
	}
}

void CHexCtrl::ClearData()
{
	m_ullDataSize = 0;
	m_pData = nullptr;
	m_ullSelectionClick = m_ullSelectionStart = m_ullSelectionEnd = m_ullBytesSelected = 0;

	m_pstScrollV->SetScrollPos(0);
	m_pstScrollH->SetScrollPos(0);
	m_pstScrollV->SetScrollSizes(0, 0, 0);
	UpdateInfoText();
}

void CHexCtrl::ShowOffset(ULONGLONG ullOffset, ULONGLONG ullSize)
{
	SetSelection(ullOffset, ullOffset, ullSize, true);
}

void CHexCtrl::SetFont(const LOGFONT* pLogFontNew)
{
	if (!pLogFontNew)
		return;

	m_fontHexView.DeleteObject();
	m_fontHexView.CreateFontIndirectW(pLogFontNew);

	RecalcAll();
}

void CHexCtrl::SetFontSize(UINT uiSize)
{
	//Prevent font size from being too small or too big.
	if (uiSize < 9 || uiSize > 75)
		return;

	LOGFONT lf;
	m_fontHexView.GetLogFont(&lf);
	lf.lfHeight = uiSize;
	m_fontHexView.DeleteObject();
	m_fontHexView.CreateFontIndirectW(&lf);

	RecalcAll();
}

UINT CHexCtrl::GetFontSize()
{
	LOGFONT lf;
	m_fontHexView.GetLogFont(&lf);

	return lf.lfHeight;
}

void CHexCtrl::SetColor(const HEXCOLORSTRUCT& clr)
{
	m_stColor = clr;
	RedrawWindow();
}

void CHexCtrl::SetCapacity(DWORD dwCapacity)
{
	if (dwCapacity < 1 || dwCapacity > m_dwCapacityMax)
		return;

	if (dwCapacity < m_dwCapacity)
		dwCapacity -= dwCapacity % m_dwShowAs;
	else
		dwCapacity += m_dwShowAs - (dwCapacity % m_dwShowAs);

	if (dwCapacity < (DWORD)m_dwShowAs)
		dwCapacity = m_dwShowAs;
	else if (dwCapacity > m_dwCapacityMax)
		dwCapacity = m_dwCapacityMax;

	m_dwCapacity = dwCapacity;
	m_dwCapacityBlockSize = m_dwCapacity / 2;
	RecalcAll();
}

int CHexCtrl::GetDlgCtrlID()const
{
	return m_dwCtrlId;
}

CWnd* CHexCtrl::GetParent()const
{
	return m_pwndParentOwner;
}

void CHexCtrl::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	SetFocus();

	CWnd::OnActivate(nState, pWndOther, bMinimized);
}

void CHexCtrl::OnDestroy()
{
	NMHDR nmh { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_DESTROY };
	if (m_pwndMsg)
		m_pwndMsg->SendMessageW(WM_NOTIFY, nmh.idFrom, (LPARAM)&nmh);
	if (m_pwndParentOwner)
	{
		m_pwndParentOwner->SendMessageW(WM_NOTIFY, nmh.idFrom, (LPARAM)&nmh);
		m_pwndParentOwner->SetForegroundWindow();
	}
	m_fCreated = false;
	ClearData();

	CWnd::OnDestroy();
}

void CHexCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_fLMousePressed)
	{
		//If LMouse is pressed but cursor is outside client area.
		//SetCapture() behaviour.

		CRect rcClient;
		GetClientRect(&rcClient);
		//Checking for scrollbars existence first.
		if (m_pstScrollH->IsVisible())
		{
			if (point.x < rcClient.left) {
				m_pstScrollH->ScrollLineLeft();
				point.x = m_iIndentFirstHexChunk;
			}
			else if (point.x >= rcClient.right) {
				m_pstScrollH->ScrollLineRight();
				point.x = m_iFourthVertLine - 1;
			}
		}
		if (m_pstScrollV->IsVisible())
		{
			if (point.y < m_iHeightTopRect) {
				m_pstScrollV->ScrollLineUp();
				point.y = m_iHeightTopRect;
			}
			else if (point.y >= m_iHeightWorkArea) {
				m_pstScrollV->ScrollLineDown();
				point.y = m_iHeightWorkArea - 1;
			}
		}
		const ULONGLONG ullHit = HitTest(&point);
		if (ullHit != -1)
		{
			if (ullHit <= m_ullSelectionClick) {
				m_ullSelectionStart = ullHit;
				m_ullSelectionEnd = m_ullSelectionClick + 1;
			}
			else {
				m_ullSelectionStart = m_ullSelectionClick;
				m_ullSelectionEnd = ullHit + 1;
			}

			m_ullBytesSelected = m_ullSelectionEnd - m_ullSelectionStart;

			UpdateInfoText();
		}

		RedrawWindow();
	}
	else
	{
		m_pstScrollV->OnMouseMove(nFlags, point);
		m_pstScrollH->OnMouseMove(nFlags, point);
	}
}

BOOL CHexCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (nFlags == MK_CONTROL)
	{
		SetFontSize(GetFontSize() + zDelta / WHEEL_DELTA * 2);
		return TRUE;
	}
	else if (nFlags & (MK_CONTROL | MK_SHIFT))
	{
		SetCapacity(m_dwCapacity + zDelta / WHEEL_DELTA);
		return TRUE;
	}

	if (zDelta > 0) //Scrolling Up.
		m_pstScrollV->ScrollPageUp();
	else
		m_pstScrollV->ScrollPageDown();

	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CHexCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	const ULONGLONG ullHit = HitTest(&point);
	if (ullHit == -1)
		return;

	SetCapture();
	if (m_ullBytesSelected && (nFlags & MK_SHIFT))
	{
		if (ullHit <= m_ullSelectionClick)
		{
			m_ullSelectionStart = ullHit;
			m_ullSelectionEnd = m_ullSelectionClick + 1;
		}
		else
		{
			m_ullSelectionStart = m_ullSelectionClick;
			m_ullSelectionEnd = ullHit + 1;
		}

		m_ullBytesSelected = m_ullSelectionEnd - m_ullSelectionStart;
	}
	else
	{
		m_ullSelectionClick = m_ullSelectionStart = ullHit;
		m_ullSelectionEnd = m_ullSelectionStart + 1;
		m_ullBytesSelected = 1;
	}

	m_ullCursorPos = m_ullSelectionStart;
	m_fCursorHigh = true;
	m_fLMousePressed = true;
	UpdateInfoText();
}

void CHexCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_fLMousePressed = false;
	ReleaseCapture();

	m_pstScrollV->OnLButtonUp(nFlags, point);
	m_pstScrollH->OnLButtonUp(nFlags, point);

	CWnd::OnLButtonUp(nFlags, point);
}

void CHexCtrl::OnMButtonDown(UINT nFlags, CPoint point)
{
}

BOOL CHexCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT uiId = LOWORD(wParam);

	switch (uiId)
	{
	case IDM_MAIN_SEARCH:
		if (m_fVirtual)
			MessageBoxW(m_wstrErrVirtual.data(), L"Error", MB_ICONEXCLAMATION);
		else
			m_pDlgSearch->ShowWindow(SW_SHOW);
		break;
	case IDM_MAIN_COPYASHEX:
		if (m_fVirtual)
			MessageBoxW(m_wstrErrVirtual.data(), L"Error", MB_ICONEXCLAMATION);
		else
			CopyToClipboard(HEXCTRL_CLIPBOARD::COPY_HEX);
		break;
	case IDM_MAIN_COPYASHEXFORMATTED:
		if (m_fVirtual)
			MessageBoxW(m_wstrErrVirtual.data(), L"Error", MB_ICONEXCLAMATION);
		else
			CopyToClipboard(HEXCTRL_CLIPBOARD::COPY_HEXFORMATTED);
		break;
	case IDM_MAIN_COPYASASCII:
		if (m_fVirtual)
			MessageBoxW(m_wstrErrVirtual.data(), L"Error", MB_ICONEXCLAMATION);
		else
			CopyToClipboard(HEXCTRL_CLIPBOARD::COPY_ASCII);
		break;
	case IDM_MAIN_ABOUT:
	{
		CHexDlgAbout m_dlgAbout;
		m_dlgAbout.DoModal();
	}
	break;
	case IDM_SUB_SHOWASBYTE:
		SetShowAs(HEXCTRL_SHOWAS::ASBYTE);
		break;
	case IDM_SUB_SHOWASWORD:
		SetShowAs(HEXCTRL_SHOWAS::ASWORD);
		break;
	case IDM_SUB_SHOWASDWORD:
		SetShowAs(HEXCTRL_SHOWAS::ASDWORD);
		break;
	case IDM_SUB_SHOWASQWORD:
		SetShowAs(HEXCTRL_SHOWAS::ASQWORD);
		break;
	}

	return CWnd::OnCommand(wParam, lParam);
}

void CHexCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	m_menuMain.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, point.x, point.y, this);
}

void CHexCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (GetKeyState(VK_CONTROL) < 0) //With Ctrl pressed.
	{
		switch (nChar)
		{
		case 'F':
			m_pDlgSearch->ShowWindow(SW_SHOW);
			break;
		case 'C':
			CopyToClipboard(HEXCTRL_CLIPBOARD::COPY_HEX);
			break;
		case 'A':
			SelectAll();;
			break;
		}
	}
	else if (GetAsyncKeyState(VK_SHIFT) < 0) //With Shift pressed.
	{
		switch (nChar)
		{
		case VK_RIGHT:
			if (m_fMutable)
			{
				if (m_ullCursorPos == m_ullSelectionClick || m_ullCursorPos == m_ullSelectionStart || m_ullCursorPos == m_ullSelectionEnd)
				{
					if (m_ullSelectionStart == m_ullSelectionClick)
						SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullBytesSelected + 1);
					else
						SetSelection(m_ullSelectionClick, m_ullSelectionStart + 1, m_ullBytesSelected - 1);
				}
				else
				{
					m_ullSelectionClick = m_ullSelectionStart = m_ullCursorPos;
					SetSelection(m_ullSelectionClick, m_ullSelectionClick, 1);
				}
			}
			else if (m_ullBytesSelected)
			{
				if (m_ullSelectionStart == m_ullSelectionClick)
					SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullBytesSelected + 1);
				else
					SetSelection(m_ullSelectionClick, m_ullSelectionStart + 1, m_ullBytesSelected - 1);
			}
			break;
		case VK_LEFT:
			if (m_fMutable)
			{
				if (m_ullCursorPos == m_ullSelectionClick || m_ullCursorPos == m_ullSelectionStart || m_ullCursorPos == m_ullSelectionEnd)
				{
					if (m_ullSelectionStart == m_ullSelectionClick && m_ullBytesSelected > 1)
						SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullBytesSelected - 1);
					else
						SetSelection(m_ullSelectionClick, m_ullSelectionStart - 1, m_ullBytesSelected + 1);
				}
				else
				{
					m_ullSelectionClick = m_ullSelectionStart = m_ullCursorPos;
					SetSelection(m_ullSelectionClick, m_ullSelectionClick, 1);
				}
			}
			else if (m_ullBytesSelected)
			{
				if (m_ullSelectionStart == m_ullSelectionClick && m_ullBytesSelected > 1)
					SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullBytesSelected - 1);
				else
					SetSelection(m_ullSelectionClick, m_ullSelectionStart - 1, m_ullBytesSelected + 1);
			}
			break;
		case VK_DOWN:
			if (m_fMutable)
			{
				if (m_ullCursorPos == m_ullSelectionClick || m_ullCursorPos == m_ullSelectionStart || m_ullCursorPos == m_ullSelectionEnd)
				{
					if (m_ullSelectionStart == m_ullSelectionClick)
						SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullBytesSelected + m_dwCapacity);
					else if (m_ullSelectionStart < m_ullSelectionClick)
					{
						ULONGLONG dwStartAt = m_ullBytesSelected > m_dwCapacity ? m_ullSelectionStart + m_dwCapacity : m_ullSelectionClick;
						ULONGLONG dwBytes = m_ullBytesSelected >= m_dwCapacity ? m_ullBytesSelected - m_dwCapacity : m_dwCapacity;
						SetSelection(m_ullSelectionClick, dwStartAt, dwBytes ? dwBytes : 1);
					}
				}
				else {
					m_ullSelectionClick = m_ullSelectionStart = m_ullCursorPos;
					SetSelection(m_ullSelectionClick, m_ullSelectionClick, 1);
				}
			}
			else if (m_ullBytesSelected)
			{
				if (m_ullSelectionStart == m_ullSelectionClick)
					SetSelection(m_ullSelectionClick, m_ullSelectionClick, m_ullBytesSelected + m_dwCapacity);
				else if (m_ullSelectionStart < m_ullSelectionClick)
				{
					ULONGLONG dwStartAt = m_ullBytesSelected > m_dwCapacity ? m_ullSelectionStart + m_dwCapacity : m_ullSelectionClick;
					ULONGLONG dwBytes = m_ullBytesSelected >= m_dwCapacity ? m_ullBytesSelected - m_dwCapacity : m_dwCapacity;
					SetSelection(m_ullSelectionClick, dwStartAt, dwBytes ? dwBytes : 1);
				}
			}
			break;
		case VK_UP:
			if (m_fMutable)
			{
				if (m_ullCursorPos == m_ullSelectionClick || m_ullCursorPos == m_ullSelectionStart || m_ullCursorPos == m_ullSelectionEnd)
				{
					if (m_ullSelectionStart == 0)
						return;

					if (m_ullSelectionStart < m_ullSelectionClick)
					{
						ULONGLONG dwStartAt;
						ULONGLONG dwBytes;
						if (m_ullSelectionStart < m_dwCapacity)
						{
							dwStartAt = 0;
							dwBytes = m_ullBytesSelected + m_ullSelectionStart;
						}
						else
						{
							dwStartAt = m_ullSelectionStart - m_dwCapacity;
							dwBytes = m_ullBytesSelected + m_dwCapacity;
						}
						SetSelection(m_ullSelectionClick, dwStartAt, dwBytes);
					}
					else
					{
						ULONGLONG dwStartAt = m_ullBytesSelected >= m_dwCapacity ? m_ullSelectionClick : m_ullSelectionClick - m_dwCapacity + 1;
						ULONGLONG dwBytes = m_ullBytesSelected >= m_dwCapacity ? m_ullBytesSelected - m_dwCapacity : m_dwCapacity;
						SetSelection(m_ullSelectionClick, dwStartAt, dwBytes ? dwBytes : 1);
					}
				}
				else {
					m_ullSelectionClick = m_ullSelectionStart = m_ullCursorPos;
					SetSelection(m_ullSelectionClick, m_ullSelectionClick, 1);
				}
			}
			else if (m_ullBytesSelected)
			{
				if (m_ullSelectionStart == 0)
					return;

				if (m_ullSelectionStart < m_ullSelectionClick)
				{
					ULONGLONG dwStartAt;
					ULONGLONG dwBytes;
					if (m_ullSelectionStart < m_dwCapacity)
					{
						dwStartAt = 0;
						dwBytes = m_ullBytesSelected + m_ullSelectionStart;
					}
					else
					{
						dwStartAt = m_ullSelectionStart - m_dwCapacity;
						dwBytes = m_ullBytesSelected + m_dwCapacity;
					}
					SetSelection(m_ullSelectionClick, dwStartAt, dwBytes);
				}
				else
				{
					ULONGLONG dwStartAt = m_ullBytesSelected >= m_dwCapacity ? m_ullSelectionClick : m_ullSelectionClick - m_dwCapacity + 1;
					ULONGLONG dwBytes = m_ullBytesSelected >= m_dwCapacity ? m_ullBytesSelected - m_dwCapacity : m_dwCapacity;
					SetSelection(m_ullSelectionClick, dwStartAt, dwBytes ? dwBytes : 1);
				}
			}
			break;
		}
	}
	else
	{
		switch (nChar)
		{
		case VK_RIGHT:
			if (m_fMutable)
				CursorMoveRight();
			else
				SetSelection(m_ullSelectionClick + 1, m_ullSelectionClick + 1, 1);
			break;
		case VK_LEFT:
			if (m_fMutable)
				CursorMoveLeft();
			else
				SetSelection(m_ullSelectionClick - 1, m_ullSelectionClick - 1, 1);
			break;
		case VK_DOWN:
			if (m_fMutable)
				CursorMoveDown();
			else
				SetSelection(m_ullSelectionClick + m_dwCapacity, m_ullSelectionClick + m_dwCapacity, 1);
			break;
		case VK_UP:
			if (m_fMutable)
				CursorMoveUp();
			else
				SetSelection(m_ullSelectionClick - m_dwCapacity, m_ullSelectionClick - m_dwCapacity, 1);
			break;
		case VK_PRIOR: //Page-Up
			m_pstScrollV->ScrollPageUp();
			break;
		case VK_NEXT:  //Page-Down
			m_pstScrollV->ScrollPageDown();
			break;
		case VK_HOME:
			m_pstScrollV->ScrollHome();
			break;
		case VK_END:
			m_pstScrollV->ScrollEnd();
			break;
		}
	}
}

void CHexCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (m_fCursorAscii)
		SetSingleByteData(m_ullCursorPos, nChar);
	else
	{
		unsigned char chByte;
		if (nChar >= 0x30 && nChar <= 0x39)		 //Digits.
			chByte = nChar - 0x30;
		else if (nChar >= 0x41 && nChar <= 0x46) //Hex letters uppercase.
			chByte = nChar - 0x37;
		else if (nChar >= 0x61 && nChar <= 0x66) //Hex letters lowercase.
			chByte = nChar - 0x57;
		else
			return;

		SetSingleByteData(m_ullCursorPos, chByte, false, m_fCursorHigh);
	}
}

void CHexCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (m_pstScrollV->GetScrollPosDelta() != 0)
		RedrawWindow();
}

void CHexCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (m_pstScrollH->GetScrollPosDelta() != 0)
		RedrawWindow();
}

void CHexCtrl::OnPaint()
{
	CPaintDC dc(this);

	int iScrollH = (int)m_pstScrollH->GetScrollPos();
	CRect rcClient;
	GetClientRect(rcClient);
	//Drawing through CMemDC to avoid flickering.
	CMemDC memDC(dc, rcClient);
	CDC& rDC = memDC.GetDC();

	RECT rc; //Used for all local rect related drawing.
	rDC.GetClipBox(&rc);
	rDC.FillSolidRect(&rc, m_stColor.clrBk);
	rDC.SelectObject(&m_penLines);
	rDC.SelectObject(&m_fontHexView);

	//To prevent drawing in too small window (can cause hangs).
	if (rcClient.Height() < m_iHeightTopRect + m_iHeightBottomOffArea)
		return;

	//Find the ullLineStart and ullLineEnd position, draw the visible portion.
	const ULONGLONG ullLineStart = GetCurrentLineV();
	ULONGLONG ullLineEndtmp = 0;
	if (m_ullDataSize) {
		ullLineEndtmp = ullLineStart + (rcClient.Height() - m_iHeightTopRect - m_iHeightBottomOffArea) / m_sizeLetter.cy;
		//If m_dwDataCount is really small we adjust dwLineEnd to be not bigger than maximum allowed.
		if (ullLineEndtmp > (m_ullDataSize / m_dwCapacity))
			ullLineEndtmp = (m_ullDataSize % m_dwCapacity) ? m_ullDataSize / m_dwCapacity + 1 : m_ullDataSize / m_dwCapacity;
	}
	const ULONGLONG ullLineEnd = ullLineEndtmp;

	const auto iFirstHorizLine = 0;
	const auto iSecondHorizLine = iFirstHorizLine + m_iHeightTopRect - 1;
	const auto iThirdHorizLine = iFirstHorizLine + rcClient.Height() - m_iHeightBottomOffArea;
	const auto iFourthHorizLine = iThirdHorizLine + m_iHeightBottomRect;

	//First horizontal line.
	rDC.MoveTo(0, iFirstHorizLine);
	rDC.LineTo(m_iFourthVertLine, iFirstHorizLine);

	//Second horizontal line.
	rDC.MoveTo(0, iSecondHorizLine);
	rDC.LineTo(m_iFourthVertLine, iSecondHorizLine);

	//Third horizontal line.
	rDC.MoveTo(0, iThirdHorizLine);
	rDC.LineTo(m_iFourthVertLine, iThirdHorizLine);

	//Fourth horizontal line.
	rDC.MoveTo(0, iFourthHorizLine);
	rDC.LineTo(m_iFourthVertLine, iFourthHorizLine);

	//«Offset» text.
	rc.left = m_iFirstVertLine - iScrollH; rc.top = iFirstHorizLine;
	rc.right = m_iSecondVertLine - iScrollH; rc.bottom = iSecondHorizLine;
	rDC.SetTextColor(m_stColor.clrTextCaption);
	rDC.DrawTextW(L"Offset", 6, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//«Bytes total:» text.
	rc.left = m_iFirstVertLine + 5;	rc.top = iThirdHorizLine + 1;
	rc.right = rcClient.right > m_iFourthVertLine ? rcClient.right : m_iFourthVertLine;
	rc.bottom = iFourthHorizLine;
	rDC.FillSolidRect(&rc, m_stColor.clrBkInfoRect);
	rDC.SetTextColor(m_stColor.clrTextInfoRect);
	rDC.SelectObject(&m_fontBottomRect);
	rDC.DrawTextW(m_wstrBottomText.data(), (int)m_wstrBottomText.size(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	rDC.SelectObject(&m_fontHexView);
	rDC.SetTextColor(m_stColor.clrTextCaption);
	rDC.SetBkColor(m_stColor.clrBk);

	int iCapacityShowAs = 1;
	int iIndentCapacityX = 0;
	//Loop for printing top Capacity numbers.
	for (unsigned iterCapacity = 0; iterCapacity < m_dwCapacity; iterCapacity++)
	{
		int x = m_iIndentFirstHexChunk + m_sizeLetter.cx + iIndentCapacityX; //Top capacity numbers (0 1 2 3 4 5 6 7...)
		//Top capacity numbers, second block (8 9 A B C D E F...).
		if (iterCapacity >= m_dwCapacityBlockSize && m_dwShowAs == HEXCTRL_SHOWAS::ASBYTE)
			x = m_iIndentFirstHexChunk + m_sizeLetter.cx + iIndentCapacityX + m_iSpaceBetweenBlocks;

		//If iterCapacity >= 16 (0xA), then two chars needed (10, 11,... 1F) to be printed.
		int c = 1;
		if (iterCapacity >= 16) {
			c = 2;
			x -= m_sizeLetter.cx;
		}
		ExtTextOutW(rDC.m_hDC, x - iScrollH, iFirstHorizLine + m_iIndentTextCapacityY, NULL, nullptr,
			m_umapCapacity.at(iterCapacity).data(), c, nullptr);

		iIndentCapacityX += m_iSizeHexByte;
		if (iCapacityShowAs == m_dwShowAs) {
			iIndentCapacityX += m_iSpaceBetweenHexChunks;
			iCapacityShowAs = 1;
		}
		else
			iCapacityShowAs++;
	}
	//"Ascii" text.
	rc.left = m_iThirdVertLine - iScrollH; rc.top = iFirstHorizLine;
	rc.right = m_iFourthVertLine - iScrollH; rc.bottom = iSecondHorizLine;
	rDC.DrawTextW(L"Ascii", 5, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//First Vertical line.
	rDC.MoveTo(m_iFirstVertLine - iScrollH, 0);
	rDC.LineTo(m_iFirstVertLine - iScrollH, iFourthHorizLine);

	//Second Vertical line.
	rDC.MoveTo(m_iSecondVertLine - iScrollH, 0);
	rDC.LineTo(m_iSecondVertLine - iScrollH, iThirdHorizLine);

	//Third Vertical line.
	rDC.MoveTo(m_iThirdVertLine - iScrollH, 0);
	rDC.LineTo(m_iThirdVertLine - iScrollH, iThirdHorizLine);

	//Fourth Vertical line.
	rDC.MoveTo(m_iFourthVertLine - iScrollH, 0);
	rDC.LineTo(m_iFourthVertLine - iScrollH, iFourthHorizLine);

	//Current line to print.
	int iLine = 0;
	//Loop for printing hex and Ascii line by line.
	for (ULONGLONG iterLines = ullLineStart; iterLines < ullLineEnd; iterLines++)
	{
		//Drawing offset with bk color depending on selection range.
		if (m_ullBytesSelected && (iterLines * m_dwCapacity + m_dwCapacity) > m_ullSelectionStart &&
			(iterLines * m_dwCapacity) < m_ullSelectionEnd)
			rDC.SetBkColor(m_stColor.clrBkSelected);
		else
			rDC.SetBkColor(m_stColor.clrBk);

		//Left column offset printing (00000001...0000FFFF...).
		wchar_t pwszOffset[16];
		ToWchars(iterLines * m_dwCapacity, pwszOffset, m_dwOffsetDigits / 2);
		rDC.SetTextColor(m_stColor.clrTextCaption);
		ExtTextOutW(rDC.m_hDC, m_sizeLetter.cx - iScrollH, m_iHeightTopRect + (m_sizeLetter.cy * iLine),
			NULL, nullptr, pwszOffset, m_dwOffsetDigits, nullptr);

		int iIndentHexX = 0;
		int iIndentAsciiX = 0;
		int iShowAs = 1;
		//Main loop for printing Hex chunks and Ascii chars.
		for (unsigned iterChunks = 0; iterChunks < m_dwCapacity; iterChunks++)
		{
			//Additional space between capacity halves. Only with BYTEs representation.
			int iIndentBetweenBlocks = 0;
			if (iterChunks >= m_dwCapacityBlockSize && m_dwShowAs == HEXCTRL_SHOWAS::ASBYTE)
				iIndentBetweenBlocks = m_iSpaceBetweenBlocks;

			const UINT iHexPosToPrintX = m_iIndentFirstHexChunk + iIndentHexX + iIndentBetweenBlocks - iScrollH;
			const UINT iHexPosToPrintY = m_iHeightTopRect + m_sizeLetter.cy * iLine;
			const UINT iAsciiPosToPrintX = m_iIndentAscii + iIndentAsciiX - iScrollH;
			const UINT iAsciiPosToPrintY = m_iHeightTopRect + m_sizeLetter.cy * iLine;

			//Index of the next char (in m_pData) to draw.
			const ULONGLONG ullIndexByteToPrint = iterLines * m_dwCapacity + iterChunks;

			if (ullIndexByteToPrint < m_ullDataSize) //Draw until reaching the end of m_dwDataCount.
			{
				//Hex chunk to print.
				unsigned char chByteToPrint = GetByte(ullIndexByteToPrint);
				wchar_t pwszHexToPrint[2];
				pwszHexToPrint[0] = m_pwszHexMap[(chByteToPrint & 0xF0) >> 4];
				pwszHexToPrint[1] = m_pwszHexMap[(chByteToPrint & 0x0F)];

				//Selection draw with different BK color.
				COLORREF clrBk, clrBkAscii, clrTextHex, clrTextAscii;
				if (m_ullBytesSelected && ullIndexByteToPrint >= m_ullSelectionStart && ullIndexByteToPrint < m_ullSelectionEnd)
				{
					clrBk = clrBkAscii = m_stColor.clrBkSelected;
					clrTextHex = clrTextAscii = m_stColor.clrTextSelected;
					//Space between hex chunks (excluding last hex in a row) filling with bk_selected color.
					if (ullIndexByteToPrint < (m_ullSelectionEnd - 1) && (ullIndexByteToPrint + 1) % m_dwCapacity &&
						((ullIndexByteToPrint + 1) % m_dwShowAs) == 0)
					{	//Rect of the space between Hex chunks. Needed for proper selection drawing.
						rc.left = iHexPosToPrintX + m_iSizeHexByte;
						rc.right = rc.left + m_iSpaceBetweenHexChunks;
						rc.top = iHexPosToPrintY;
						rc.bottom = iHexPosToPrintY + m_sizeLetter.cy;
						if (iterChunks == m_dwCapacityBlockSize - 1) //Space between capacity halves.
							rc.right += m_iSpaceBetweenBlocks;

						rDC.FillRect(&rc, &m_stBrushBkSelected);
					}
				}
				else
				{
					clrBk = clrBkAscii = m_stColor.clrBk;
					clrTextHex = m_stColor.clrTextHex;
					clrTextAscii = m_stColor.clrTextAscii;
				}
				//Hex chunk printing.
				if (m_fMutable && ullIndexByteToPrint == m_ullCursorPos)
				{
					rDC.SetBkColor(m_fCursorHigh ? m_stColor.clrBkCursor : clrBk);
					rDC.SetTextColor(m_fCursorHigh ? m_stColor.clrTextCursor : clrTextHex);
					ExtTextOutW(rDC.m_hDC, iHexPosToPrintX, iHexPosToPrintY, 0, nullptr, &pwszHexToPrint[0], 1, nullptr);
					rDC.SetBkColor(!m_fCursorHigh ? m_stColor.clrBkCursor : clrBk);
					rDC.SetTextColor(!m_fCursorHigh ? m_stColor.clrTextCursor : clrTextHex);
					ExtTextOutW(rDC.m_hDC, iHexPosToPrintX + m_sizeLetter.cx, iHexPosToPrintY, 0, nullptr, &pwszHexToPrint[1], 1, nullptr);
				}
				else
				{
					rDC.SetBkColor(clrBk);
					rDC.SetTextColor(clrTextHex);
					ExtTextOutW(rDC.m_hDC, iHexPosToPrintX, iHexPosToPrintY, 0, nullptr, pwszHexToPrint, 2, nullptr);
				}

				//Ascii to print.
				wchar_t wchAscii = chByteToPrint;
				if (wchAscii < 32 || wchAscii >= 0x7f) //For non printable Ascii just print a dot.
					wchAscii = '.';

				//Ascii printing.
				if (m_fMutable && ullIndexByteToPrint == m_ullCursorPos) {
					clrBkAscii = m_stColor.clrBkCursor;
					clrTextAscii = m_stColor.clrTextCursor;
				}
				rDC.SetBkColor(clrBkAscii);
				rDC.SetTextColor(clrTextAscii);
				ExtTextOutW(rDC.m_hDC, iAsciiPosToPrintX, iAsciiPosToPrintY, 0, nullptr, &wchAscii, 1, nullptr);
			}
			//Increasing indents for next print, for both - Hex and Ascii.
			iIndentHexX += m_iSizeHexByte;
			if (iShowAs == m_dwShowAs) {
				iIndentHexX += m_iSpaceBetweenHexChunks;
				iShowAs = 1;
			}
			else
				iShowAs++;
			iIndentAsciiX += m_iSpaceBetweenAscii;
		}
		iLine++;
	}
}

void CHexCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	RecalcWorkAreaHeight(cy);
	RecalcScrollPageSize();
}

BOOL CHexCtrl::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

BOOL CHexCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	m_pstScrollV->OnSetCursor(pWnd, nHitTest, message);
	m_pstScrollH->OnSetCursor(pWnd, nHitTest, message);

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CHexCtrl::OnNcActivate(BOOL bActive)
{
	m_pstScrollV->OnNcActivate(bActive);
	m_pstScrollH->OnNcActivate(bActive);

	return CWnd::OnNcActivate(bActive);
}

void CHexCtrl::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	CWnd::OnNcCalcSize(bCalcValidRects, lpncsp);

	//Sequence is important — H->V.
	m_pstScrollH->OnNcCalcSize(bCalcValidRects, lpncsp);
	m_pstScrollV->OnNcCalcSize(bCalcValidRects, lpncsp);
}

void CHexCtrl::OnNcPaint()
{
	Default();

	m_pstScrollV->OnNcPaint();
	m_pstScrollH->OnNcPaint();
}

void CHexCtrl::RecalcAll()
{
	ULONGLONG ullCurLineV = GetCurrentLineV();

	//Current font size related.
	HDC hDC = ::GetDC(m_hWnd);
	SelectObject(hDC, m_fontHexView.m_hObject);
	TEXTMETRICW tm { };
	GetTextMetricsW(hDC, &tm);
	m_sizeLetter.cx = tm.tmAveCharWidth;
	m_sizeLetter.cy = tm.tmHeight + tm.tmExternalLeading;
	::ReleaseDC(m_hWnd, hDC);

	m_iFirstVertLine = 0;
	m_iSecondVertLine = m_dwOffsetDigits * m_sizeLetter.cx + m_sizeLetter.cx * 2;
	m_iSizeHexByte = m_sizeLetter.cx * 2;
	m_iSpaceBetweenBlocks = m_sizeLetter.cx * 2;
	m_iSpaceBetweenHexChunks = m_sizeLetter.cx;
	m_iDistanceBetweenHexChunks = m_iSizeHexByte * m_dwShowAs + m_iSpaceBetweenHexChunks;
	m_iThirdVertLine = m_iSecondVertLine + m_iDistanceBetweenHexChunks * (m_dwCapacity / m_dwShowAs)
		+ m_iSpaceBetweenBlocks + m_sizeLetter.cx;
	m_iIndentAscii = m_iThirdVertLine + m_sizeLetter.cx;
	m_iSpaceBetweenAscii = m_sizeLetter.cx + 1;
	m_iFourthVertLine = m_iIndentAscii + (m_iSpaceBetweenAscii * m_dwCapacity) + m_sizeLetter.cx;
	m_iIndentFirstHexChunk = m_iSecondVertLine + m_sizeLetter.cx;
	m_iSizeFirstHalf = m_iIndentFirstHexChunk + m_dwCapacityBlockSize * (m_sizeLetter.cx * 2) +
		(m_dwCapacityBlockSize / m_dwShowAs - 1) * m_iSpaceBetweenHexChunks;
	m_iHeightTopRect = int(m_sizeLetter.cy * 1.5);
	m_iIndentTextCapacityY = m_iHeightTopRect / 2 - (m_sizeLetter.cy / 2);

	RecalcScrollSizes();
	m_pstScrollV->SetScrollPos(ullCurLineV * m_sizeLetter.cy);

	RedrawWindow();
}

void CHexCtrl::RecalcWorkAreaHeight(int iClientHeight)
{
	m_iHeightWorkArea = iClientHeight - m_iHeightBottomOffArea -
		((iClientHeight - m_iHeightTopRect - m_iHeightBottomOffArea) % m_sizeLetter.cy);
}

void CHexCtrl::RecalcScrollSizes(int iClientHeight, int iClientWidth)
{
	if (iClientHeight == 0 && iClientWidth == 0)
	{
		CRect rc;
		GetClientRect(&rc);
		iClientHeight = rc.Height();
		iClientWidth = rc.Width();
	}

	RecalcWorkAreaHeight(iClientHeight);
	//Scroll sizes according to current font size.
	m_pstScrollV->SetScrollSizes(m_sizeLetter.cy, m_iHeightWorkArea - m_iHeightTopRect,
		m_iHeightTopRect + m_iHeightBottomOffArea + m_sizeLetter.cy * (m_ullDataSize / m_dwCapacity + 2));
	m_pstScrollH->SetScrollSizes(m_sizeLetter.cx, iClientWidth, m_iFourthVertLine + 1);
}

void CHexCtrl::RecalcScrollPageSize()
{
	UINT uiPage = m_iHeightWorkArea - m_iHeightTopRect;
	m_pstScrollV->SetScrollPageSize(uiPage);
}

ULONGLONG CHexCtrl::GetCurrentLineV()
{
	return m_pstScrollV->GetScrollPos() / m_sizeLetter.cy;
}

ULONGLONG CHexCtrl::HitTest(LPPOINT pPoint)
{
	int iY = pPoint->y;
	int iX = pPoint->x + (int)m_pstScrollH->GetScrollPos(); //To compensate horizontal scroll.
	ULONGLONG ullCurLine = GetCurrentLineV();
	ULONGLONG ullHexChunk;
	m_fCursorAscii = false;

	//Checking if cursor is within hex chunks area.
	if ((iX >= m_iIndentFirstHexChunk) && (iX < m_iThirdVertLine) && (iY >= m_iHeightTopRect) && (iY <= m_iHeightWorkArea))
	{
		//Additional space between halves. Only in BYTE's view mode.
		int iBetweenBlocks;
		if (m_dwShowAs == HEXCTRL_SHOWAS::ASBYTE && iX > m_iSizeFirstHalf)
			iBetweenBlocks = m_iSpaceBetweenBlocks;
		else
			iBetweenBlocks = 0;

		//Calculate hex chunk.
		DWORD dwX = iX - m_iIndentFirstHexChunk - iBetweenBlocks;
		DWORD dwChunkX { };
		int iSpaceBetweenHexChunks = 0;
		for (unsigned i = 1; i <= m_dwCapacity; i++)
		{
			if ((i % m_dwShowAs) == 0)
				iSpaceBetweenHexChunks += m_iSpaceBetweenHexChunks;
			if ((m_iSizeHexByte * i + iSpaceBetweenHexChunks) > dwX)
			{
				dwChunkX = i - 1;
				break;
			}
		}
		ullHexChunk = dwChunkX + ((iY - m_iHeightTopRect) / m_sizeLetter.cy) * m_dwCapacity + (ullCurLine  * m_dwCapacity);
	}
	else if ((iX >= m_iIndentAscii) && (iX < (m_iIndentAscii + m_iSpaceBetweenAscii * (int)m_dwCapacity))
		&& (iY >= m_iHeightTopRect) && iY <= m_iHeightWorkArea)
	{
		//Calculate ullHit Ascii symbol.
		ullHexChunk = ((iX - m_iIndentAscii) / m_iSpaceBetweenAscii) +
			((iY - m_iHeightTopRect) / m_sizeLetter.cy) * m_dwCapacity + (ullCurLine * m_dwCapacity);
		m_fCursorAscii = true;
	}
	else
		ullHexChunk = -1;

	//If cursor is out of end-bound of hex chunks or Ascii chars.
	if (ullHexChunk >= m_ullDataSize)
		ullHexChunk = -1;

	return ullHexChunk;
}

void CHexCtrl::HexPoint(ULONGLONG ullChunk, ULONGLONG& ullCx, ULONGLONG& ullCy)
{
	int iBetweenBlocks;
	if (m_dwShowAs == HEXCTRL_SHOWAS::ASBYTE && (ullChunk % m_dwCapacity) > m_dwCapacityBlockSize)
		iBetweenBlocks = m_iSpaceBetweenBlocks;
	else
		iBetweenBlocks = 0;

	ullCx = m_iIndentFirstHexChunk + iBetweenBlocks + (ullChunk % m_dwCapacity) * m_iSizeHexByte;
	ullCx += ((ullChunk % m_dwCapacity) / m_dwShowAs) * m_iSpaceBetweenHexChunks;

	if (ullChunk % m_dwCapacity)
		ullCy = (ullChunk / m_dwCapacity) * m_sizeLetter.cy + m_sizeLetter.cy;
	else
		ullCy = (ullChunk / m_dwCapacity) * m_sizeLetter.cy;
}

void CHexCtrl::CopyToClipboard(UINT nType)
{
	if (!m_ullBytesSelected)
		return;

	const char* const pszHexMap { "0123456789ABCDEF" };
	std::string strToClipboard;
	switch (nType)
	{
	case COPY_HEX:
	{
		for (unsigned i = 0; i < m_ullBytesSelected; i++) {
			strToClipboard += pszHexMap[((unsigned char)m_pData[m_ullSelectionStart + i] & 0xF0) >> 4];
			strToClipboard += pszHexMap[((unsigned char)m_pData[m_ullSelectionStart + i] & 0x0F)];
		}
		break;
	}
	case COPY_HEXFORMATTED:
	{
		//How many spaces are needed to be inserted at the beginnig.
		DWORD dwModStart = m_ullSelectionStart % m_dwCapacity;

		//When to insert first "\r\n".
		DWORD dwTail = m_dwCapacity - dwModStart;
		DWORD dwNextBlock = (m_dwCapacity % 2) ? m_dwCapacityBlockSize + 2 : m_dwCapacityBlockSize + 1;

		//If at least two rows are selected.
		if (dwModStart + m_ullBytesSelected > m_dwCapacity) {
			size_t sCount = (dwModStart * 2) + (dwModStart / m_dwShowAs);
			//Additional spaces between halves. Only in BYTE's view mode.
			sCount += m_dwShowAs == HEXCTRL_SHOWAS::ASBYTE ? (dwTail <= m_dwCapacityBlockSize ? 2 : 0) : 0;
			strToClipboard.insert(0, sCount, ' ');
		}

		for (unsigned i = 0; i < m_ullBytesSelected; i++)
		{
			strToClipboard += pszHexMap[((unsigned char)m_pData[m_ullSelectionStart + i] & 0xF0) >> 4];
			strToClipboard += pszHexMap[((unsigned char)m_pData[m_ullSelectionStart + i] & 0x0F)];

			if (i < (m_ullBytesSelected - 1) && (dwTail - 1) != 0)
				if (m_dwShowAs == HEXCTRL_SHOWAS::ASBYTE && dwTail == dwNextBlock)
					strToClipboard += "   "; //Additional spaces between halves. Only in BYTE's view mode.
				else if (((m_dwCapacity - dwTail + 1) % m_dwShowAs) == 0) //Add space after hex full chunk, ShowAs_size depending.
					strToClipboard += " ";
			if (--dwTail == 0 && i < (m_ullBytesSelected - 1)) //Next row.
			{
				strToClipboard += "\r\n";
				dwTail = m_dwCapacity;
			}
		}
		break;
	}
	case COPY_ASCII:
	{
		for (unsigned i = 0; i < m_ullBytesSelected; i++)
		{
			char ch = m_pData[m_ullSelectionStart + i];
			//If next byte is zero —> substitute it with space.
			if (ch == 0)
				ch = ' ';
			strToClipboard += ch;
		}
		break;
	}
	}

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strToClipboard.length() + 1);
	if (!hMem) {
		MessageBox(L"GlobalAlloc error.", L"Error", MB_ICONERROR);
		return;
	}
	LPVOID hMemLock = GlobalLock(hMem);
	if (!hMemLock) {
		MessageBox(L"GlobalLock error.", L"Error", MB_ICONERROR);
		return;
	}
	memcpy(hMemLock, strToClipboard.data(), strToClipboard.length() + 1);
	GlobalUnlock(hMem);
	OpenClipboard();
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

void CHexCtrl::UpdateInfoText()
{
	if (!m_ullDataSize)
		m_wstrBottomText.clear();
	else
	{
		m_wstrBottomText.resize(128);
		if (m_ullBytesSelected)
			m_wstrBottomText.resize(swprintf_s(&m_wstrBottomText[0], 128, L"Selected: 0x%llX(%llu); Block: 0x%llX(%llu) - 0x%llX(%llu)",
				m_ullBytesSelected, m_ullBytesSelected, m_ullSelectionStart, m_ullSelectionStart, m_ullSelectionEnd - 1, m_ullSelectionEnd - 1));
		else
			m_wstrBottomText.resize(swprintf_s(&m_wstrBottomText[0], 128, L"Bytes total: 0x%llX(%llu)", m_ullDataSize, m_ullDataSize));
	}
	RedrawWindow();
}

void CHexCtrl::ToWchars(ULONGLONG ull, wchar_t* pwsz, DWORD dwSize)
{
	//Converts dwSize bytes of ull to wchar_t*.
	for (unsigned i = 0; i < dwSize; i++)
	{
		pwsz[i * 2] = m_pwszHexMap[((ull >> ((dwSize - 1 - i) << 3)) & 0xF0) >> 4];
		pwsz[i * 2 + 1] = m_pwszHexMap[(ull >> ((dwSize - 1 - i) << 3)) & 0x0F];
	}
}

UCHAR CHexCtrl::GetByte(ULONGLONG ullIndex)
{
	//If it's virtual data control we aquire next byte_to_print from m_pwndMsg window.
	if (m_fVirtual && m_pwndMsg)
	{
		HEXNOTIFYSTRUCT hexntfy { { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_GETDATA },
			ullIndex, 0 };
		m_pwndMsg->SendMessageW(WM_NOTIFY, hexntfy.hdr.idFrom, (LPARAM)&hexntfy);
		return hexntfy.chByte;
	}
	else
		return m_pData[ullIndex];
}

void CHexCtrl::SetShowAs(DWORD dwShowAs)
{
	//Unchecking all menus and checking only the current needed.
	m_dwShowAs = dwShowAs;
	for (int i = 0; i < m_menuSubShowAs.GetMenuItemCount(); i++)
		m_menuSubShowAs.CheckMenuItem(i, MF_UNCHECKED | MF_BYPOSITION);

	int id { };
	switch (dwShowAs)
	{
	case HEXCTRL_SHOWAS::ASBYTE:
		id = 0;
		break;
	case HEXCTRL_SHOWAS::ASWORD:
		id = 1;
		break;
	case HEXCTRL_SHOWAS::ASDWORD:
		id = 2;
		break;
	case HEXCTRL_SHOWAS::ASQWORD:
		id = 3;
		break;
	}
	m_menuSubShowAs.CheckMenuItem(id, MF_CHECKED | MF_BYPOSITION);
	SetCapacity(m_dwCapacity);
	RecalcAll();
}

void CHexCtrl::SetSingleByteData(ULONGLONG ullByte, BYTE chData, bool fWhole, bool fHighPart, bool fMoveNext)
{
	//Changes single byte in memory, High or Low part of it, depending on fHighPart.
	if (!m_fMutable || ullByte >= m_ullDataSize)
		return;

	unsigned char chByteNew;
	if (fWhole) {
		if (fMoveNext)
			SetCursorPos(m_ullCursorPos + 1, true);
		chByteNew = chData;
	}
	else
	{	//If just one part (High/Low) of byte must be changed.
		unsigned char chByte = GetByte(ullByte);

		if (fHighPart)
			chByteNew = (chData << 4) | (chByte & 0x0F);
		else
			chByteNew = (chData & 0x0F) | (chByte & 0xF0);

		if (fMoveNext)
		{
			if (!fHighPart)
				SetCursorPos(m_ullCursorPos + 1, true);
			else
				SetCursorPos(m_ullCursorPos, false);
		}
	}

	if (!m_fVirtual)
		m_pData[ullByte] = chByteNew;

	if (m_pwndMsg)
	{
		HEXNOTIFYSTRUCT hexntfy { { m_hWnd, (UINT)GetDlgCtrlID(), HEXCTRL_MSG_SETDATA },
			ullByte, chByteNew };
		m_pwndMsg->SendMessageW(WM_NOTIFY, hexntfy.hdr.idFrom, (LPARAM)&hexntfy);
	}

	RedrawWindow();
}

void CHexCtrl::SetCursorPos(ULONGLONG ullPos, bool fHighPart)
{
	m_ullCursorPos = ullPos;
	if (m_ullCursorPos >= m_ullDataSize)
	{
		m_ullCursorPos = m_ullDataSize - 1;
		m_fCursorHigh = false;
	}
	else
		m_fCursorHigh = fHighPart;

	CursorScroll();
	RedrawWindow();
}

void CHexCtrl::CursorMoveRight()
{
	if (m_fCursorAscii)
		SetCursorPos(m_ullCursorPos + 1, m_fCursorHigh);
	else if (m_fCursorHigh)
		SetCursorPos(m_ullCursorPos, false);
	else
		SetCursorPos(m_ullCursorPos + 1, true);
}

void CHexCtrl::CursorMoveLeft()
{
	if (m_fCursorAscii)
	{
		ULONGLONG ull = m_ullCursorPos;
		if (ull > 0) //To avoid overflow.
			ull--;
		else
			return; //Zero index byte.

		SetCursorPos(ull, m_fCursorHigh);
	}
	else if (m_fCursorHigh)
	{
		ULONGLONG ull = m_ullCursorPos;
		if (ull > 0) //To avoid overflow.
			ull--;
		else
			return; //Zero index byte.

		SetCursorPos(ull, false);
	}
	else
		SetCursorPos(m_ullCursorPos, true);
}

void CHexCtrl::CursorMoveUp()
{
	ULONGLONG ull = m_ullCursorPos;
	if (ull > m_dwCapacity)
		ull -= m_dwCapacity;
	SetCursorPos(ull, m_fCursorHigh);
}

void CHexCtrl::CursorMoveDown()
{
	SetCursorPos(m_ullCursorPos + m_dwCapacity, m_fCursorHigh);
}

void CHexCtrl::CursorScroll()
{
	ULONGLONG ullCurrScrollV = m_pstScrollV->GetScrollPos();
	ULONGLONG ullCurrScrollH = m_pstScrollH->GetScrollPos();
	ULONGLONG ullCx, ullCy;
	HexPoint(m_ullCursorPos, ullCx, ullCy);
	CRect rcClient;
	GetClientRect(&rcClient);

	//New scroll depending on selection direction: top <-> bottom.
	ULONGLONG ullEnd = m_ullCursorPos; //ullEnd is inclusive here.
	ULONGLONG ullMaxV = ullCurrScrollV + rcClient.Height() - m_iHeightBottomOffArea - m_iHeightTopRect -
		((rcClient.Height() - m_iHeightTopRect - m_iHeightBottomOffArea) % m_sizeLetter.cy);
	ULONGLONG ullNewStartV = m_ullCursorPos / m_dwCapacity * m_sizeLetter.cy;
	ULONGLONG ullNewEndV = ullEnd / m_dwCapacity * m_sizeLetter.cy;

	ULONGLONG ullNewScrollV { }, ullNewScrollH { };
	if (ullNewEndV >= ullMaxV)
		ullNewScrollV = ullCurrScrollV + m_sizeLetter.cy;
	else
	{
		if (ullNewEndV >= ullCurrScrollV)
			ullNewScrollV = ullCurrScrollV;
		else if (ullNewStartV <= ullCurrScrollV)
			ullNewScrollV = ullCurrScrollV - m_sizeLetter.cy;
	}

	ULONGLONG ullMaxClient = ullCurrScrollH + rcClient.Width() - m_iSizeHexByte;
	if (ullCx >= ullMaxClient)
		ullNewScrollH = ullCurrScrollH + (ullCx - ullMaxClient);
	else if (ullCx < ullCurrScrollH)
		ullNewScrollH = ullCx;
	else
		ullNewScrollH = ullCurrScrollH;

	ullNewScrollV -= ullNewScrollV % m_sizeLetter.cy;

	m_pstScrollV->SetScrollPos(ullNewScrollV);
	if (m_pstScrollH->IsVisible())
		m_pstScrollH->SetScrollPos(ullNewScrollH);
}

void CHexCtrl::Search(HEXSEARCH& rSearch)
{
	rSearch.fFound = false;
	ULONGLONG ullStartAt = rSearch.ullStartAt;
	ULONGLONG ullSizeBytes;
	ULONGLONG ullUntil;
	std::string strSearch { };

	if (rSearch.wstrSearch.empty() || m_ullDataSize == 0 || rSearch.ullStartAt > (m_ullDataSize - 1))
		return m_pDlgSearch->SearchCallback();

	int iSizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &rSearch.wstrSearch[0], (int)rSearch.wstrSearch.size(), nullptr, 0, nullptr, nullptr);
	std::string strSearchAscii(iSizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, &rSearch.wstrSearch[0], (int)rSearch.wstrSearch.size(), &strSearchAscii[0], iSizeNeeded, nullptr, nullptr);

	switch (rSearch.dwSearchType)
	{
	case SEARCH_HEX:
	{
		DWORD dwIterations = DWORD(strSearchAscii.size() / 2 + strSearchAscii.size() % 2);
		std::string strToUL;
		char* pEndPtr { };

		for (size_t i = 0; i < dwIterations; i++)
		{
			if (i + 2 <= strSearchAscii.size())
				strToUL = strSearchAscii.substr(i * 2, 2);
			else
				strToUL = strSearchAscii.substr(i * 2, 1);

			unsigned long ulNumber = strtoul(strToUL.data(), &pEndPtr, 16);
			if (ulNumber == 0 && (pEndPtr == strToUL.data() || *pEndPtr != '\0'))
			{
				rSearch.fFound = false;
				m_pDlgSearch->SearchCallback();
				return;
			}

			strSearch += (unsigned char)ulNumber;
		}

		ullSizeBytes = strSearch.size();
		if (ullSizeBytes > m_ullDataSize)
			goto End;

		break;
	}
	case SEARCH_ASCII:
	{
		ullSizeBytes = strSearchAscii.size();
		if (ullSizeBytes > m_ullDataSize)
			goto End;

		strSearch = std::move(strSearchAscii);
		break;
	}
	case SEARCH_UNICODE:
	{
		ullSizeBytes = rSearch.wstrSearch.length() * sizeof(wchar_t);
		if (ullSizeBytes > m_ullDataSize)
			goto End;

		break;
	}
	}

	///////////////Actual Search:////////////////////////////////////////////
	switch (rSearch.dwSearchType) {
	case SEARCH_HEX:
	case SEARCH_ASCII:
	{
		if (rSearch.iDirection == SEARCH_FORWARD)
		{
			ullUntil = m_ullDataSize - strSearch.size();
			ullStartAt = rSearch.fSecondMatch ? rSearch.ullStartAt + 1 : 0;

			for (ULONGLONG i = ullStartAt; i <= ullUntil; i++)
			{
				if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
				{
					rSearch.fFound = true;
					rSearch.ullStartAt = i;
					rSearch.fWrap = false;
					goto End;
				}
			}

			ullStartAt = 0;
			for (ULONGLONG i = ullStartAt; i <= ullUntil; i++)
			{
				if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
				{
					rSearch.fFound = true;
					rSearch.ullStartAt = i;
					rSearch.fWrap = true;
					rSearch.iWrap = SEARCH_END;
					rSearch.fCount = true;
					goto End;
				}
			}
		}
		if (rSearch.iDirection == SEARCH_BACKWARD)
		{
			if (rSearch.fSecondMatch && ullStartAt > 0)
			{
				ullStartAt--;
				for (int i = (int)ullStartAt; i >= 0; i--)
				{
					if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
					{
						rSearch.fFound = true;
						rSearch.ullStartAt = i;
						rSearch.fWrap = false;
						goto End;
					}
				}
			}

			ullStartAt = m_ullDataSize - strSearch.size();
			for (int i = (int)ullStartAt; i >= 0; i--)
			{
				if (memcmp(m_pData + i, strSearch.data(), strSearch.size()) == 0)
				{
					rSearch.fFound = true;
					rSearch.ullStartAt = i;
					rSearch.fWrap = true;
					rSearch.iWrap = SEARCH_BEGINNING;
					rSearch.fCount = false;
					goto End;
				}
			}
		}
		break;
	}
	case SEARCH_UNICODE:
	{
		if (rSearch.iDirection == SEARCH_FORWARD)
		{
			ullUntil = m_ullDataSize - ullSizeBytes;
			ullStartAt = rSearch.fSecondMatch ? rSearch.ullStartAt + 1 : 0;

			for (ULONGLONG i = ullStartAt; i <= ullUntil; i++)
			{
				if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.length()) == 0)
				{
					rSearch.fFound = true;
					rSearch.ullStartAt = i;
					rSearch.fWrap = false;
					goto End;
				}
			}

			ullStartAt = 0;
			for (ULONGLONG i = ullStartAt; i <= ullUntil; i++)
			{
				if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.length()) == 0)
				{
					rSearch.fFound = true;
					rSearch.ullStartAt = i;
					rSearch.iWrap = SEARCH_END;
					rSearch.fWrap = true;
					rSearch.fCount = true;
					goto End;
				}
			}
		}
		else if (rSearch.iDirection == SEARCH_BACKWARD)
		{
			if (rSearch.fSecondMatch && ullStartAt > 0)
			{
				ullStartAt--;
				for (int i = (int)ullStartAt; i >= 0; i--)
				{
					if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.length()) == 0)
					{
						rSearch.fFound = true;
						rSearch.ullStartAt = i;
						rSearch.fWrap = false;
						goto End;
					}
				}
			}

			ullStartAt = m_ullDataSize - ullSizeBytes;
			for (int i = (int)ullStartAt; i >= 0; i--)
			{
				if (wmemcmp((const wchar_t*)(m_pData + i), rSearch.wstrSearch.data(), rSearch.wstrSearch.length()) == 0)
				{
					rSearch.fFound = true;
					rSearch.ullStartAt = i;
					rSearch.fWrap = true;
					rSearch.iWrap = SEARCH_BEGINNING;
					rSearch.fCount = false;
					goto End;
				}
			}
		}
		break;
	}
	}

End:
	{
		m_pDlgSearch->SearchCallback();
		if (rSearch.fFound)
			SetSelection(rSearch.ullStartAt, rSearch.ullStartAt, ullSizeBytes, true);
	}
}

void CHexCtrl::SetSelection(ULONGLONG ullClick, ULONGLONG ullStart, ULONGLONG ullSize, bool fHighlight)
{
	if (ullClick >= m_ullDataSize || ullStart >= m_ullDataSize || !ullSize)
		return;
	if ((ullStart + ullSize) > m_ullDataSize)
		ullSize = m_ullDataSize - ullStart;

	ULONGLONG ullCurrScrollV = m_pstScrollV->GetScrollPos();
	ULONGLONG ullCurrScrollH = m_pstScrollH->GetScrollPos();
	ULONGLONG ullCx, ullCy;
	HexPoint(ullStart, ullCx, ullCy);
	CRect rcClient;
	GetClientRect(&rcClient);

	//New scroll depending on selection direction: top <-> bottom.
	//fHighlight means centralize scroll position on the screen (used in Search()).
	ULONGLONG ullEnd = ullStart + ullSize; //ullEnd is exclusive.
	ULONGLONG ullMaxV = ullCurrScrollV + rcClient.Height() - m_iHeightBottomOffArea - m_iHeightTopRect -
		((rcClient.Height() - m_iHeightTopRect - m_iHeightBottomOffArea) % m_sizeLetter.cy);
	ULONGLONG ullNewStartV = ullStart / m_dwCapacity * m_sizeLetter.cy;
	ULONGLONG ullNewEndV = (ullEnd - 1) / m_dwCapacity * m_sizeLetter.cy;

	ULONGLONG ullNewScrollV { }, ullNewScrollH { };
	if (fHighlight)
	{
		//To prevent negative numbers.
		if (ullNewStartV > m_iHeightWorkArea / 2)
			ullNewScrollV = ullNewStartV - m_iHeightWorkArea / 2;
		else
			ullNewScrollV = 0;

		ullNewScrollH = (ullStart % m_dwCapacity) * m_iSizeHexByte;
		ullNewScrollH += (ullNewScrollH / m_iDistanceBetweenHexChunks) * m_iSpaceBetweenHexChunks;
	}
	else
	{
		if (ullStart == ullClick)
		{
			if (ullNewEndV >= ullMaxV)
				ullNewScrollV = ullCurrScrollV + m_sizeLetter.cy;
			else
			{
				if (ullNewEndV >= ullCurrScrollV)
					ullNewScrollV = ullCurrScrollV;
				else if (ullNewStartV <= ullCurrScrollV)
					ullNewScrollV = ullCurrScrollV - m_sizeLetter.cy;
			}
		}
		else
		{
			if (ullNewStartV < ullCurrScrollV)
				ullNewScrollV = ullCurrScrollV - m_sizeLetter.cy;
			else
			{
				if (ullNewStartV < ullMaxV)
					ullNewScrollV = ullCurrScrollV;
				else
					ullNewScrollV = ullCurrScrollV + m_sizeLetter.cy;
			}
		}

		ULONGLONG ullMaxClient = ullCurrScrollH + rcClient.Width() - m_iSizeHexByte;
		if (ullCx >= ullMaxClient)
			ullNewScrollH = ullCurrScrollH + (ullCx - ullMaxClient);
		else if (ullCx < ullCurrScrollH)
			ullNewScrollH = ullCx;
		else
			ullNewScrollH = ullCurrScrollH;
	}
	ullNewScrollV -= ullNewScrollV % m_sizeLetter.cy;

	m_ullSelectionClick = ullClick;
	m_ullSelectionStart = m_ullCursorPos = ullStart;
	m_ullSelectionEnd = ullEnd;
	m_ullBytesSelected = ullEnd - ullStart;

	m_pstScrollV->SetScrollPos(ullNewScrollV);
	if (m_pstScrollH->IsVisible())
		m_pstScrollH->SetScrollPos(ullNewScrollH);

	UpdateInfoText();
}

void CHexCtrl::SelectAll()
{
	if (!m_ullDataSize)
		return;

	m_ullSelectionClick = m_ullSelectionStart = 0;
	m_ullSelectionEnd = m_ullBytesSelected = m_ullDataSize;
	UpdateInfoText();
}
