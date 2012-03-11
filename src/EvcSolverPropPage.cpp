// Copyright 2010 ESRI
// 
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
// 
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
// 
// See the use restrictions at http://help.arcgis.com/en/sdk/10.0/usageRestrictions.htm

#include "stdafx.h"
#include "EvcSolverPropPage.h"

// EvcSolverPropPage

/////////////////////////////////////////////////////////////////////////////
// IPropertyPage

STDMETHODIMP EvcSolverPropPage::Show(UINT nCmdShow)
{
	// If we are showing the property page, propulate it
	// from the EvcSolver object.
	if ((nCmdShow & (SW_SHOW|SW_SHOWDEFAULT)))
	{
		// set the loaded network restrictions
		BSTR * names;
		int i, c, selectedIndex;

		// set the solver method names
		EVC_SOLVER_METHOD method;
		m_ipEvcSolver->get_SolverMethod(&method);
		::SendMessage(m_hComboMethod, CB_RESETCONTENT, NULL, NULL);
		::SendMessage(m_hComboMethod, CB_ADDSTRING, NULL, (LPARAM)(_T("SP")));
		::SendMessage(m_hComboMethod, CB_ADDSTRING, NULL, (LPARAM)(_T("CCRP")));
		::SendMessage(m_hComboMethod, CB_ADDSTRING, NULL, (LPARAM)(_T("CASPER")));
		::SendMessage(m_hComboMethod, CB_SETCURSEL, (WPARAM)method, 0);

		// set flags
		VARIANT_BOOL val;
		m_ipEvcSolver->get_SeparableEvacuee(&val);
		if (val) ::SendMessage(m_hSeparable, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
		else  ::SendMessage(m_hSeparable, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
		m_ipEvcSolver->get_ExportEdgeStat(&val);
		if (val) ::SendMessage(m_hEdgeStat, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
		else  ::SendMessage(m_hEdgeStat, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
		m_ipEvcSolver->get_FlockingEnabled(&val);
		if (val) ::SendMessage(m_hCheckFlock, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
		else  ::SendMessage(m_hCheckFlock, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);

		// set the solver cost method names
		m_ipEvcSolver->get_CostMethod(&method);
		::SendMessage(m_hComboCostMethod, CB_RESETCONTENT, NULL, NULL);
		::SendMessage(m_hComboCostMethod, CB_ADDSTRING, NULL, (LPARAM)(_T("SP")));
		::SendMessage(m_hComboCostMethod, CB_ADDSTRING, NULL, (LPARAM)(_T("CCRP")));
		::SendMessage(m_hComboCostMethod, CB_ADDSTRING, NULL, (LPARAM)(_T("CASPER")));
		::SendMessage(m_hComboCostMethod, CB_SETCURSEL, (WPARAM)method, 0);

		// set the loaded network discriptive attribs
		m_ipEvcSolver->get_DiscriptiveAttributesCount(&c);
		m_ipEvcSolver->get_DiscriptiveAttributes(&names);
		::SendMessage(m_hCapCombo, CB_RESETCONTENT, NULL, NULL);
		for (i = 0; i < c; i++) ::SendMessage(m_hCapCombo, CB_ADDSTRING, NULL, (LPARAM)(names[i]));		
		m_ipEvcSolver->get_CapacityAttribute(&selectedIndex);
		::SendMessage(m_hCapCombo, CB_SETCURSEL, selectedIndex, 0);
		delete [] names;

		// set the loaded network cost attribs
		m_ipEvcSolver->get_CostAttributesCount(&c);
		m_ipEvcSolver->get_CostAttributes(&names);
		::SendMessage(m_hCostCombo, CB_RESETCONTENT, NULL, NULL);
		for (i = 0; i < c; i++) ::SendMessage(m_hCostCombo, CB_ADDSTRING, NULL, (LPARAM)(names[i]));		
		m_ipEvcSolver->get_CostAttribute(&selectedIndex);
		::SendMessage(m_hCostCombo, CB_SETCURSEL, selectedIndex, 0);
		delete [] names;

		// critical density
		BSTR critical;
		m_ipEvcSolver->get_CriticalDensPerCap(&critical);
		::SendMessage(m_hEditCritical, WM_SETTEXT, NULL, (LPARAM)critical);
		delete [] critical;

		// saturation constant
		BSTR sat;
		m_ipEvcSolver->get_SaturationPerCap(&sat);
		::SendMessage(m_hEditSat, WM_SETTEXT, NULL, (LPARAM)sat);
		delete [] sat;

		// cost per zone density
		BSTR density;
		m_ipEvcSolver->get_CostPerZoneDensity(&density);
		::SendMessage(m_hEditDensity, WM_SETTEXT, NULL, (LPARAM)density);
		delete [] density;

		// set flocking snap interval number at the textbox
		BSTR flock;
		m_ipEvcSolver->get_FlockingSnapInterval(&flock);
		::SendMessage(m_hEditSnapFlock, WM_SETTEXT, NULL, (LPARAM)flock);
		delete [] flock;

		// set flocking simulation interval number at the textbox
		BSTR simul;
		m_ipEvcSolver->get_FlockingSimulationInterval(&simul);
		::SendMessage(m_hEditSimulationFlock, WM_SETTEXT, NULL, (LPARAM)simul);
		delete [] simul;

		SetFlockingEnabled();
		SetDirty(FALSE);
	}

	// Let the IPropertyPageImpl deal with displaying the page
	return IPropertyPageImpl<EvcSolverPropPage>::Show(nCmdShow);
}

STDMETHODIMP EvcSolverPropPage::SetObjects(ULONG nObjects, IUnknown ** ppUnk)
{
	m_ipNALayer = 0;
	m_ipEvcSolver = 0;
	m_ipDENet = 0;

	// Loop through the objects to find one that supports
	// the INALayer interface.
	for (ULONG i = 0; i < nObjects; i++)
	{
		INALayerPtr ipNALayer(ppUnk[i]);
		if (!ipNALayer) continue;

		VARIANT_BOOL         isValid;
		INetworkDatasetPtr   ipNetworkDataset;
		IDEDatasetPtr        ipDEDataset;
		IDENetworkDatasetPtr ipDENet;
		INASolverPtr         ipSolver;
		INAContextPtr        ipNAContext;

		ipNALayer->get_Context(&ipNAContext);
		if (!ipNAContext) continue;
		ipNAContext->get_Solver(&ipSolver);
		if (!(IEvcSolverPtr)ipSolver) continue;

		((ILayerPtr)ipNALayer)->get_Valid(&isValid);
		if (isValid == VARIANT_FALSE) continue;

		ipNAContext->get_NetworkDataset(&ipNetworkDataset);
		if (!ipNetworkDataset) continue;

		((IDatasetComponentPtr)ipNetworkDataset)->get_DataElement(&ipDEDataset);
		ipDENet = ipDEDataset;

		if (!ipDENet) continue;

		m_ipNALayer = ipNALayer;
		m_ipEvcSolver = ipSolver;
		m_ipDENet = ipDENet;
	}

	// Let the IPropertyPageImpl know what objects we have
	return IPropertyPageImpl<EvcSolverPropPage>::SetObjects(nObjects, ppUnk);
}

STDMETHODIMP EvcSolverPropPage::Apply(void)
{
	// Pass the m_ipEvcSolver member variable to the QueryObject method
	HRESULT hr = QueryObject(CComVariant((IUnknown*) m_ipEvcSolver));

	// Set the page to not dirty
	SetDirty(FALSE);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IPropertyPageContext

STDMETHODIMP EvcSolverPropPage::get_Priority(LONG * pPriority)
{
	if (pPriority == NULL) return E_POINTER;

	(*pPriority) = 152;

	return S_OK;
}

STDMETHODIMP EvcSolverPropPage::Applies(VARIANT unkArray, VARIANT_BOOL* pApplies)
{
	if (pApplies == NULL) return E_INVALIDARG;

	(*pApplies) = VARIANT_FALSE;

	if (V_VT(&unkArray) != (VT_ARRAY|VT_UNKNOWN)) return E_INVALIDARG;

	// Retrieve the safe array and retrieve the data
	SAFEARRAY *saArray = unkArray.parray;
	HRESULT hr = ::SafeArrayLock(saArray);

	IUnknownPtr *pUnk;
	hr = ::SafeArrayAccessData(saArray,reinterpret_cast<void**> (&pUnk));
	if (FAILED(hr)) return hr;

	// Loop through the elements and see if an NALayer with a valid IEvcSolver is present
	long lNumElements = saArray->rgsabound->cElements;
	for (long i = 0; i < lNumElements; i++)
	{
		INALayerPtr ipNALayer(pUnk[i]);
		if (!ipNALayer) continue;

		VARIANT_BOOL         isValid;
		INetworkDatasetPtr   ipNetworkDataset;
		IDEDatasetPtr        ipDEDataset;
		IDENetworkDatasetPtr ipDENet;
		INASolverPtr         ipSolver;
		INAContextPtr        ipNAContext;

		ipNALayer->get_Context(&ipNAContext);
		if (!ipNAContext) continue;
		ipNAContext->get_Solver(&ipSolver);
		if (!(IEvcSolverPtr)ipSolver) continue;

		((ILayerPtr)ipNALayer)->get_Valid(&isValid);
		if (isValid == VARIANT_FALSE) continue;

		ipNAContext->get_NetworkDataset(&ipNetworkDataset);
		if (!ipNetworkDataset) continue;

		((IDatasetComponentPtr)ipNetworkDataset)->get_DataElement(&ipDEDataset);
		ipDENet = ipDEDataset;

		if (!ipDENet) continue;

		(*pApplies) = VARIANT_TRUE;
		break;
	}

	// Cleanup
	hr = ::SafeArrayUnaccessData(saArray);
	if (FAILED(hr)) return hr;

	hr = ::SafeArrayUnlock(saArray);
	if (FAILED(hr)) return hr;

	return S_OK;
}

STDMETHODIMP EvcSolverPropPage::CreateCompatibleObject(VARIANT kind, VARIANT* pNewObject)
{
	if (pNewObject == NULL) return E_POINTER;
	return E_NOTIMPL;
}

STDMETHODIMP EvcSolverPropPage::QueryObject(VARIANT theObject)
{
	// Check if we have a marker symbol
	// If we do, apply the setting from the page.
	CComVariant vObject(theObject);
	if (vObject.vt != VT_UNKNOWN) return E_UNEXPECTED;
	// Try and QI to IEvcSolver
	IEvcSolverPtr ipSolver(vObject.punkVal);
	int size;
	if (ipSolver != 0)
	{
		// save data from drop boxes
		int selectedIndex = ::SendMessage(m_hCapCombo, CB_GETCURSEL, 0, 0);
		if (selectedIndex > -1) ipSolver->put_CapacityAttribute(selectedIndex);
		selectedIndex = ::SendMessage(m_hCostCombo, CB_GETCURSEL, 0, 0);
		if (selectedIndex > -1) ipSolver->put_CostAttribute(selectedIndex);
		selectedIndex = ::SendMessage(m_hComboMethod, CB_GETCURSEL, 0, 0);
		if (selectedIndex > -1) ipSolver->put_SolverMethod((EVC_SOLVER_METHOD)selectedIndex);
		selectedIndex = ::SendMessage(m_hComboCostMethod, CB_GETCURSEL, 0, 0);
		if (selectedIndex > -1) ipSolver->put_CostMethod((EVC_SOLVER_METHOD)selectedIndex);
		selectedIndex = ::SendMessage(m_hSeparable, BM_GETCHECK, 0, 0);
		ipSolver->put_SeparableEvacuee(selectedIndex == BST_CHECKED);
		selectedIndex = ::SendMessage(m_hEdgeStat, BM_GETCHECK, 0, 0);
		ipSolver->put_ExportEdgeStat(selectedIndex == BST_CHECKED);
		selectedIndex = ::SendMessage(m_hCheckFlock, BM_GETCHECK, 0, 0);
		ipSolver->put_FlockingEnabled(selectedIndex == BST_CHECKED);
		
		// critical density per capacity
		BSTR critical;
		size = ::SendMessage(m_hEditCritical, WM_GETTEXTLENGTH, 0, 0);
		critical = new WCHAR[size + 1];
		::SendMessage(m_hEditCritical, WM_GETTEXT, size + 1, (LPARAM)critical);
		ipSolver->put_CriticalDensPerCap(critical);
		delete [] critical;

		// saturation density per capacity
		BSTR sat;
		size = ::SendMessage(m_hEditSat, WM_GETTEXTLENGTH, 0, 0);
		sat = new WCHAR[size + 1];
		::SendMessage(m_hEditSat, WM_GETTEXT, size + 1, (LPARAM)sat);
		ipSolver->put_SaturationPerCap(sat);
		delete [] sat;
		
		// cost per zone density
		BSTR density;
		size = ::SendMessage(m_hEditDensity, WM_GETTEXTLENGTH, 0, 0);
		density = new WCHAR[size + 1];
		::SendMessage(m_hEditDensity, WM_GETTEXT, size + 1, (LPARAM)density);
		ipSolver->put_CostPerZoneDensity(density);
		delete [] density;
		
		// flock snap interval
		BSTR flock;
		size = ::SendMessage(m_hEditSnapFlock, WM_GETTEXTLENGTH, 0, 0);
		flock = new WCHAR[size + 1];
		::SendMessage(m_hEditSnapFlock, WM_GETTEXT, size + 1, (LPARAM)flock);
		ipSolver->put_FlockingSnapInterval(flock);
		delete [] flock;
		
		// flock simulation interval
		BSTR simul;
		size = ::SendMessage(m_hEditSimulationFlock, WM_GETTEXTLENGTH, 0, 0);
		simul = new WCHAR[size + 1];
		::SendMessage(m_hEditSimulationFlock, WM_GETTEXT, size + 1, (LPARAM)simul);
		ipSolver->put_FlockingSimulationInterval(simul);
		delete [] simul;
	}
	return S_OK;
}

STDMETHODIMP EvcSolverPropPage::GetHelpFile(LONG controlID, BSTR* pHelpFile)
{
	if (pHelpFile == NULL) return E_POINTER;
	return E_NOTIMPL;
}

STDMETHODIMP EvcSolverPropPage::GetHelpId(LONG controlID, LONG* pHelpID)
{
	if (pHelpID == NULL) return E_POINTER;
	return E_NOTIMPL;
}

STDMETHODIMP EvcSolverPropPage::Cancel()
{
	// In this case do nothing
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Dialog

LRESULT EvcSolverPropPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_hCapCombo = GetDlgItem(IDC_COMBO_CAPACITY);
	m_hCostCombo = GetDlgItem(IDC_COMBO_COST);
	m_hComboMethod = GetDlgItem(IDC_COMBO_METHOD);
	m_hComboCostMethod = GetDlgItem(IDC_COMBO_CostMethod);
	m_hEditCritical = GetDlgItem(IDC_EDIT_Critical);
	m_hEditSat = GetDlgItem(IDC_EDIT_SAT);
	m_hSeparable = GetDlgItem(IDC_CHECK_SEPARABLE);
	m_hEdgeStat = GetDlgItem(IDC_CHECK_EDGESTAT);
	m_hEditDensity = GetDlgItem(IDC_EDIT_ZoneDensity);
	m_hEditSnapFlock = GetDlgItem(IDC_EDIT_FlockSnapInterval);
	m_hEditSimulationFlock = GetDlgItem(IDC_EDIT_FlockSimulationInterval);
	m_hCheckFlock = GetDlgItem(IDC_CHECK_Flock);

	HWND m_hGroupFlock = GetDlgItem(IDC_FlockOptions);
	HWND m_hlblSimulationFlock = GetDlgItem(IDC_STATIC_FlockSimulationInterval);
	HWND m_hlblSnapFlock = GetDlgItem(IDC_STATIC_FlockSnapInterval);

#ifdef _FLOCK
	::ShowWindow(m_hGroupFlock, SW_SHOW);
	::ShowWindow(m_hEditSnapFlock, SW_SHOW);
	::ShowWindow(m_hEditSimulationFlock, SW_SHOW);
	::ShowWindow(m_hCheckFlock, SW_SHOW);
	::ShowWindow(m_hlblSimulationFlock, SW_SHOW);
	::ShowWindow(m_hlblSnapFlock, SW_SHOW);
#else
	::ShowWindow(m_hGroupFlock, SW_HIDE);
	::ShowWindow(m_hEditSnapFlock, SW_HIDE);
	::ShowWindow(m_hEditSimulationFlock, SW_HIDE);
	::ShowWindow(m_hCheckFlock, SW_HIDE);
	::ShowWindow(m_hlblSimulationFlock, SW_HIDE);
	::ShowWindow(m_hlblSnapFlock, SW_HIDE);
#endif

	return 0;
}

LRESULT EvcSolverPropPage::OnEnChangeEditSat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnEnChangeEditCritical(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeComboMethod(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeComboCostmethod(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeComboCapacity(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeCostCapacity(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnBnClickedCheckSeparable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnBnClickedCheckEdgestat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnEnChangeEditZonedensity(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

void EvcSolverPropPage::SetFlockingEnabled()
{	
	int flag = ::SendMessage(m_hCheckFlock, BM_GETCHECK, 0, 0);
	if (flag == BST_CHECKED) flag = 1; else flag = 0;
	
	::SendMessage(m_hEditSnapFlock, WM_ENABLE, flag, 0);
	::SendMessage(m_hEditSimulationFlock, WM_ENABLE, flag, 0);	
}

LRESULT EvcSolverPropPage::OnBnClickedCheckFlock(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	SetFlockingEnabled();
	return 0;
}

LRESULT EvcSolverPropPage::OnEnChangeEditFlocksnapinterval(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnEnChangeEditFlocksimulationinterval(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}
