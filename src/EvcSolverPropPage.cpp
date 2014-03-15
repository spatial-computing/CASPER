// Copyright 2010 ESRI
// 
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
// 
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
// 
// See the use restrictions at http://help.arcgis.com/en/sdk/10.1/usageRestrictions.htm

#include "stdafx.h"
#include <Windowsx.h>
#include "EvcSolverPropPage.h"
#include "NameConstants.h"

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
		size_t i, c, selectedIndex;

		// set the solver method names
		EVC_SOLVER_METHOD method;
		m_ipEvcSolver->get_SolverMethod(&method);
		::SendMessage(m_hComboMethod, CB_RESETCONTENT, NULL, NULL);
		::SendMessage(m_hComboMethod, CB_ADDSTRING, NULL, (LPARAM)(OPTIMIZATION_SP));
		::SendMessage(m_hComboMethod, CB_ADDSTRING, NULL, (LPARAM)(OPTIMIZATION_CCRP));
		::SendMessage(m_hComboMethod, CB_ADDSTRING, NULL, (LPARAM)(OPTIMIZATION_CASPER));
		::SendMessage(m_hComboMethod, CB_SETCURSEL, (WPARAM)method, 0);

		// set the flocking profile names
		FLOCK_PROFILE profile;
		m_ipEvcSolver->get_FlockingProfile(&profile);
		::SendMessage(m_hCmbFlockProfile, CB_RESETCONTENT, NULL, NULL);
		::SendMessage(m_hCmbFlockProfile, CB_ADDSTRING, NULL, (LPARAM)(_T("Car")));
		::SendMessage(m_hCmbFlockProfile, CB_ADDSTRING, NULL, (LPARAM)(_T("Person")));
		::SendMessage(m_hCmbFlockProfile, CB_ADDSTRING, NULL, (LPARAM)(_T("Bike")));
		::SendMessage(m_hCmbFlockProfile, CB_SETCURSEL, (WPARAM)profile, 0);

		// set flags
		VARIANT_BOOL val;
		m_ipEvcSolver->get_SeparableEvacuee(&val);
		if (val == VARIANT_TRUE) ::SendMessage(m_hSeparable, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
		else  ::SendMessage(m_hSeparable, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
		m_ipEvcSolver->get_ExportEdgeStat(&val);
		if (val == VARIANT_TRUE) ::SendMessage(m_hEdgeStat, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
		else  ::SendMessage(m_hEdgeStat, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
		m_ipEvcSolver->get_FlockingEnabled(&val);
		if (val == VARIANT_TRUE) ::SendMessage(m_hCheckFlock, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
		else  ::SendMessage(m_hCheckFlock, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
		m_ipEvcSolver->get_TwoWayShareCapacity(&val);
		if (val == VARIANT_TRUE) ::SendMessage(m_hCheckShareCap, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
		else  ::SendMessage(m_hCheckShareCap, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);

		// set the solver traffic model names
		EVC_TRAFFIC_MODEL model;
		m_ipEvcSolver->get_TrafficModel(&model);
		::SendMessage(m_hComboTrafficModel, CB_RESETCONTENT, NULL, NULL);
		::SendMessage(m_hComboTrafficModel, CB_ADDSTRING, NULL, (LPARAM)(TRAFFIC_MODEL_FLAT));
		::SendMessage(m_hComboTrafficModel, CB_ADDSTRING, NULL, (LPARAM)(TRAFFIC_MODEL_STEP));
		::SendMessage(m_hComboTrafficModel, CB_ADDSTRING, NULL, (LPARAM)(TRAFFIC_MODEL_LINEAR));
		::SendMessage(m_hComboTrafficModel, CB_ADDSTRING, NULL, (LPARAM)(TRAFFIC_MODEL_EXP));
		::SendMessage(m_hComboTrafficModel, CB_SETCURSEL, (WPARAM)model, 0);

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

		// set init delay cost per population
		BSTR delay;
		m_ipEvcSolver->get_InitDelayCostPerPop(&delay);
		::SendMessage(m_hEditInitCost, WM_SETTEXT, NULL, (LPARAM)delay);
		delete [] delay;

		// set CARMA ratio
		BSTR carma;
		m_ipEvcSolver->get_CARMAPerformanceRatio(&carma);
		::SendMessage(m_heditCARMA, WM_SETTEXT, NULL, (LPARAM)carma);
		delete [] carma;

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

	return hr;
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

STDMETHODIMP EvcSolverPropPage::Cancel()
{
	// In this case do nothing
	return S_OK;
}

STDMETHODIMP EvcSolverPropPage::QueryObject(VARIANT theObject)
{
	// Check if we have a marker symbol
	// If we do, apply the setting from the page.
	CComVariant vObject(theObject);
	if (vObject.vt != VT_UNKNOWN) return E_UNEXPECTED;
	// Try and QI to IEvcSolver
	IEvcSolverPtr ipSolver(vObject.punkVal);
	LRESULT size;
	if (ipSolver != 0)
	{
		// save data from drop boxes
		LRESULT selectedIndex = ::SendMessage(m_hCapCombo, CB_GETCURSEL, 0, 0);
		if (selectedIndex > -1) ipSolver->put_CapacityAttribute((size_t)selectedIndex);
		selectedIndex = ::SendMessage(m_hCostCombo, CB_GETCURSEL, 0, 0);
		if (selectedIndex > -1) ipSolver->put_CostAttribute((size_t)selectedIndex);
		selectedIndex = ::SendMessage(m_hComboMethod, CB_GETCURSEL, 0, 0);
		if (selectedIndex > -1) ipSolver->put_SolverMethod((EVC_SOLVER_METHOD)selectedIndex);
		selectedIndex = ::SendMessage(m_hComboTrafficModel, CB_GETCURSEL, 0, 0);
		if (selectedIndex > -1) ipSolver->put_TrafficModel((EVC_TRAFFIC_MODEL)selectedIndex);
		selectedIndex = ::SendMessage(m_hCmbFlockProfile, CB_GETCURSEL, 0, 0);
		if (selectedIndex > -1) ipSolver->put_FlockingProfile((FLOCK_PROFILE)selectedIndex);

		// flags
		selectedIndex = ::SendMessage(m_hSeparable, BM_GETCHECK, 0, 0);
		if (selectedIndex == BST_CHECKED) ipSolver->put_SeparableEvacuee(VARIANT_TRUE);
		else ipSolver->put_SeparableEvacuee(VARIANT_FALSE);

		selectedIndex = ::SendMessage(m_hEdgeStat, BM_GETCHECK, 0, 0);
		if (selectedIndex == BST_CHECKED) ipSolver->put_ExportEdgeStat(VARIANT_TRUE);
		else ipSolver->put_ExportEdgeStat(VARIANT_FALSE);

		selectedIndex = ::SendMessage(m_hCheckFlock, BM_GETCHECK, 0, 0);
		if (selectedIndex == BST_CHECKED) ipSolver->put_FlockingEnabled(VARIANT_TRUE);
		else ipSolver->put_FlockingEnabled(VARIANT_FALSE);

		selectedIndex = ::SendMessage(m_hCheckShareCap, BM_GETCHECK, 0, 0);
		if (selectedIndex == BST_CHECKED) ipSolver->put_TwoWayShareCapacity(VARIANT_TRUE);
		else ipSolver->put_TwoWayShareCapacity(VARIANT_FALSE);
		
		// critical density per capacity
		BSTR critical;
		size = ::SendMessage(m_hEditCritical, WM_GETTEXTLENGTH, 0, 0);
		critical = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_hEditCritical, WM_GETTEXT, size + 1, (LPARAM)critical);
		ipSolver->put_CriticalDensPerCap(critical);
		delete [] critical;
		
		// init delay cost per population
		BSTR delay;
		size = ::SendMessage(m_hEditInitCost, WM_GETTEXTLENGTH, 0, 0);
		delay = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_hEditInitCost, WM_GETTEXT, size + 1, (LPARAM)delay);
		ipSolver->put_InitDelayCostPerPop(delay);
		delete [] delay;
		
		// CARMA ratio
		BSTR carma;
		size = ::SendMessage(m_heditCARMA, WM_GETTEXTLENGTH, 0, 0);
		carma = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_heditCARMA, WM_GETTEXT, size + 1, (LPARAM)carma);
		ipSolver->put_CARMAPerformanceRatio(carma);
		delete [] carma;

		// saturation density per capacity
		BSTR sat;
		size = ::SendMessage(m_hEditSat, WM_GETTEXTLENGTH, 0, 0);
		sat = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_hEditSat, WM_GETTEXT, size + 1, (LPARAM)sat);
		ipSolver->put_SaturationPerCap(sat);
		delete [] sat;
		
		// cost per zone density
		BSTR density;
		size = ::SendMessage(m_hEditDensity, WM_GETTEXTLENGTH, 0, 0);
		density = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_hEditDensity, WM_GETTEXT, size + 1, (LPARAM)density);
		ipSolver->put_CostPerZoneDensity(density);
		delete [] density;
		
		// flock snap interval
		BSTR flock;
		size = ::SendMessage(m_hEditSnapFlock, WM_GETTEXTLENGTH, 0, 0);
		flock = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_hEditSnapFlock, WM_GETTEXT, size + 1, (LPARAM)flock);
		ipSolver->put_FlockingSnapInterval(flock);
		delete [] flock;
		
		// flock simulation interval
		BSTR simul;
		size = ::SendMessage(m_hEditSimulationFlock, WM_GETTEXTLENGTH, 0, 0);
		simul = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_hEditSimulationFlock, WM_GETTEXT, size + 1, (LPARAM)simul);
		ipSolver->put_FlockingSimulationInterval(simul);
		delete [] simul;
	}
	return S_OK;
}

#pragma warning(push)
#pragma warning(disable : 4100) /* Ignore warnings for unreferenced function parameters */

STDMETHODIMP EvcSolverPropPage::CreateCompatibleObject(VARIANT kind, VARIANT* pNewObject)
{
	if (pNewObject == NULL) return E_POINTER;
	return E_NOTIMPL;
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

/////////////////////////////////////////////////////////////////////////////
// Dialog

LRESULT EvcSolverPropPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	int cmdShow = 0;
#ifdef _FLOCK
	cmdShow = SW_SHOW;
#else
	cmdShow = SW_HIDE;	
#endif
	m_hCapCombo = GetDlgItem(IDC_COMBO_CAPACITY);
	m_hCostCombo = GetDlgItem(IDC_COMBO_COST);
	m_hComboMethod = GetDlgItem(IDC_COMBO_METHOD);
	m_hComboTrafficModel = GetDlgItem(IDC_COMBO_TRAFFICMODEL);
	m_hEditCritical = GetDlgItem(IDC_EDIT_Critical);
	m_hEditSat = GetDlgItem(IDC_EDIT_SAT);
	m_hSeparable = GetDlgItem(IDC_CHECK_SEPARABLE);
	m_hEdgeStat = GetDlgItem(IDC_CHECK_EDGESTAT);
	m_hEditDensity = GetDlgItem(IDC_EDIT_ZoneDensity);
	m_hEditSnapFlock = GetDlgItem(IDC_EDIT_FlockSnapInterval);
	m_hEditSimulationFlock = GetDlgItem(IDC_EDIT_FlockSimulationInterval);
	m_hCheckFlock = GetDlgItem(IDC_CHECK_Flock);
	m_hCheckShareCap = GetDlgItem(IDC_CHECK_SHARECAP);
	m_hEditInitCost = GetDlgItem(IDC_EDIT_INITDELAY);
	m_hCmbFlockProfile = GetDlgItem(IDC_COMBO_PROFILE);
	m_heditCARMA = GetDlgItem(IDC_EDIT_CARMA);

	HWND m_hGroupFlock = GetDlgItem(IDC_FlockOptions);
	HWND m_hlblSimulationFlock = GetDlgItem(IDC_STATIC_FlockSimulationInterval);
	HWND m_hlblSnapFlock = GetDlgItem(IDC_STATIC_FlockSnapInterval);
	HWND m_hlblFlockProfile = GetDlgItem(IDC_STATIC_FlockProfile);

	::ShowWindow(m_hGroupFlock, cmdShow);
	::ShowWindow(m_hEditSnapFlock, cmdShow);
	::ShowWindow(m_hEditSimulationFlock, cmdShow);
	::ShowWindow(m_hCheckFlock, cmdShow);
	::ShowWindow(m_hlblSimulationFlock, cmdShow);
	::ShowWindow(m_hlblSnapFlock, cmdShow);
	::ShowWindow(m_hlblFlockProfile, cmdShow);
	::ShowWindow(m_hCmbFlockProfile, cmdShow);

	// release date lable
	HWND m_hlblRelease = GetDlgItem(IDC_RELEASE);
	wchar_t compileDateBuff[100];
	swprintf_s(compileDateBuff, 100, L"<a href=\"http://esri.com/arccasper\">Release: %s</a>", _T(__DATE__));
	::SendMessage(m_hlblRelease, WM_SETTEXT, NULL, (LPARAM)(compileDateBuff));

	return 0;
}

#pragma warning(pop)

void EvcSolverPropPage::SetFlockingEnabled()
{	
	LRESULT flag = ::SendMessage(m_hCheckFlock, BM_GETCHECK, 0, 0);
	BOOL bFlag;
	if (flag == BST_CHECKED) bFlag = TRUE; else bFlag = FALSE;
	
	CWindow cwEditSnapFlock, cwEditSimulationFlock, cwCmbFlockProfile;
	cwEditSnapFlock.Attach(m_hEditSnapFlock);
	cwEditSimulationFlock.Attach(m_hEditSimulationFlock);
	cwCmbFlockProfile.Attach(m_hCmbFlockProfile);

	cwEditSnapFlock.EnableWindow(bFlag);
	cwEditSimulationFlock.EnableWindow(bFlag);
	cwCmbFlockProfile.EnableWindow(bFlag);
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

LRESULT EvcSolverPropPage::OnBnClickedCheckSharecap(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnEnChangeEditInitDelay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeComboProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnEnChangeEditCARMA(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return 0;
}

LRESULT EvcSolverPropPage::OnNMClickRelease(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	NMLINK* pNMLink = (NMLINK*)pNMHDR;
	LITEM   item    = pNMLink->item;
	ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
	return 0;
}
