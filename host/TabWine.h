#if !defined(AFX_TABSDL_H__798A9124_C906_446C_822D_322B5AB6C499__INCLUDED_)
#define AFX_TABSDL_H__798A9124_C906_446C_822D_322B5AB6C499__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TabWine.h : header file
//
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CTabWine dialog

class CTabWine : public CDialog
{
// Construction
public:
	CTabWine(CWnd* pParent = NULL);   // standard constructor
	BOOL OnInitDialog();

private:
	BOOL PreTranslateMessage(MSG *);
	void ShowHideDependencies(BOOL);

	// Dialog Data
	//{{AFX_DATA(CTabWine)
	enum { IDD = IDD_TAB_WINE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTabWine)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTabWine)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif 