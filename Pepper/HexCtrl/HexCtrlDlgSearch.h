/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* This software is available under the "MIT License modified with�The�Commons Clause".  *
* https://github.com/jovibor/Pepper/blob/master/LICENSE                                 *
* This is a Hex control for MFC apps, implemented as CWnd derived class.			    *
* The usage is quite simple:														    *
* 1. Construct CHexCtrl object � HEXCTRL::CHexCtrl myHex;								*
* 2. Call myHex.Create member function to create an instance.   					    *
* 3. Call myHex.SetData method to set the data and its size to display as hex.	        *
****************************************************************************************/
#pragma once
#include <afxcontrolbars.h>  //Standard MFC's controls header.
#include "res/HexCtrlRes.h"

namespace HEXCTRL {
	namespace {
		enum HEXCTRL_SEARCH {
			SEARCH_HEX, SEARCH_ASCII, SEARCH_UNICODE,
			SEARCH_FORWARD, SEARCH_BACKWARD,
			SEARCH_NOTFOUND, SEARCH_FOUND,
			SEARCH_BEGINNING, SEARCH_END
		};
	}
	struct HEXSEARCH
	{
		std::wstring	wstrSearch { };			//String search for.
		DWORD			dwSearchType { };		//Hex, Ascii, Unicode, etc...
		ULONGLONG		ullStartAt { };			//An offset, search should start at.
		int				iDirection { };			//Search direction: Forward <-> Backward.
		bool			fWrap { false };		//Was search wrapped?
		int				iWrap { };				//Wrap direction.
		bool			fSecondMatch { false }; //First or subsequent match. 
		bool			fFound { false };
		bool			fCount { true };		//Do we count matches or just print "Found".
	};

	/********************************************
	* CHexDlgSearch class definition.			*
	********************************************/
	class CHexDlgSearch : public CDialogEx
	{
	public:
		friend class CHexCtrl;
		explicit CHexDlgSearch(CWnd* m_pParent = nullptr) {}
		virtual ~CHexDlgSearch() {}
		BOOL Create(UINT nIDTemplate, CHexCtrl* pParentWnd);
		CHexCtrl* GetParent() const;
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual BOOL OnInitDialog();
		afx_msg void OnButtonSearchF();
		afx_msg void OnButtonSearchB();
		afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
		afx_msg void OnClose();
		virtual BOOL PreTranslateMessage(MSG* pMsg);
		HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
		void OnRadioBnRange(UINT nID);
		void SearchCallback();
		void ClearAll();
		DECLARE_MESSAGE_MAP()
	private:
		CHexCtrl* m_pParent { };
		HEXSEARCH m_stSearch { };
		DWORD m_dwOccurrences { };
		int m_iRadioCurrent { };
		COLORREF m_clrSearchFailed { RGB(200, 0, 0) };
		COLORREF m_clrSearchFound { RGB(0, 200, 0) };
		CBrush m_stBrushDefault;
		COLORREF m_clrMenu { GetSysColor(COLOR_MENU) };
	};
}