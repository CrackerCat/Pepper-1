#pragma once
#include "ChildFrm.h"
#include "HexEdit.h"

class CViewRightBottom : public CScrollView
{
public:
	DECLARE_DYNCREATE(CViewRightBottom)
protected:
	DECLARE_MESSAGE_MAP()
	CViewRightBottom() {};
	virtual ~CViewRightBottom() {};
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
private:
	PCLIBPE_SECURITY_VEC m_vecSec { };
	CHexEdit m_HexEdit;
	Ilibpe * m_pLibpe { };
	CChildFrame* m_ChildFrame { };
	CPepperDoc* m_pMainDoc { };
	CPepperList m_listImportFuncs;
	CPepperList m_listDelayImportFuncs;
	CPepperList m_listExportFuncs;
	CPepperList m_listRelocationsDescription;
	CPepperTreeCtrl m_treeResourceDirBottom;
	DWORD m_dwFileSummary { };
	LONG m_ScrollWidth { };
	LONG m_ScrollHeight { };
	CImageList m_imglTreeResource;
	CWnd* m_pActiveList { };

	int listCreateImportFuncs(UINT dllId);
	int listCreateDelayImportFuncs(UINT dllId);
	int listCreateExportFuncs();
	int listCreateRelocations(UINT blockID);
	int treeCreateResourceDir();
	int HexCtrlRightBottom(unsigned nSertId);
};


