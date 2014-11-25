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
#include "NameConstants.h"

// includes variable for commit hash / git describe string
#include "gitdescribe.h"

// EvcSolverPropPage

/////////////////////////////////////////////////////////////////////////////
// IPropertyPage

STDMETHODIMP EvcSolverPropPage::Show(UINT nCmdShow)
{
	// If we are showing the property page, populate it
	// from the EvcSolver object.
	if ((nCmdShow & (SW_SHOW|SW_SHOWDEFAULT)))
	{
		// set the loaded network restrictions
		BSTR * names;
		size_t i, c, selectedIndex;

		// set the solver method names
		EvcSolverMethod method;
		m_ipEvcSolver->get_SolverMethod(&method);
		::SendMessage(m_hComboMethod, CB_RESETCONTENT, NULL, NULL);
		::SendMessage(m_hComboMethod, CB_ADDSTRING, NULL, (LPARAM)(OPTIMIZATION_SP));
		::SendMessage(m_hComboMethod, CB_ADDSTRING, NULL, (LPARAM)(OPTIMIZATION_CCRP));
		::SendMessage(m_hComboMethod, CB_ADDSTRING, NULL, (LPARAM)(OPTIMIZATION_CASPER));
		::SendMessage(m_hComboMethod, CB_SETCURSEL, (WPARAM)method, NULL);

		// set the flocking profile names
		FLOCK_PROFILE profile;
		m_ipEvcSolver->get_FlockingProfile(&profile);
		::SendMessage(m_hCmbFlockProfile, CB_RESETCONTENT, NULL, NULL);
		::SendMessage(m_hCmbFlockProfile, CB_ADDSTRING, NULL, (LPARAM)(_T("Car")));
		::SendMessage(m_hCmbFlockProfile, CB_ADDSTRING, NULL, (LPARAM)(_T("Person")));
		::SendMessage(m_hCmbFlockProfile, CB_ADDSTRING, NULL, (LPARAM)(_T("Bike")));
		::SendMessage(m_hCmbFlockProfile, CB_SETCURSEL, (WPARAM)profile, NULL);

		// set the flocking profile names
		CARMASort sort;
		m_ipEvcSolver->get_CARMASortSetting(&sort);
		::SendMessage(m_hCmbCarmaSort, CB_RESETCONTENT, NULL, NULL);
		::SendMessage(m_hCmbCarmaSort, CB_ADDSTRING, NULL, (LPARAM)(_T("None")));
		::SendMessage(m_hCmbCarmaSort, CB_ADDSTRING, NULL, (LPARAM)(_T("FW Once")));
		::SendMessage(m_hCmbCarmaSort, CB_ADDSTRING, NULL, (LPARAM)(_T("FW Continuous")));
		::SendMessage(m_hCmbCarmaSort, CB_ADDSTRING, NULL, (LPARAM)(_T("BW Once")));
		::SendMessage(m_hCmbCarmaSort, CB_ADDSTRING, NULL, (LPARAM)(_T("BW Continuous")));
		::SendMessage(m_hCmbCarmaSort, CB_SETCURSEL, (WPARAM)sort, NULL);

		// set the uturn combo box
		esriNetworkForwardStarBacktrack uturn;
		if ((INASolverSettingsPtr)m_ipEvcSolver)
		{
			((INASolverSettingsPtr)(m_ipEvcSolver))->get_RestrictUTurns(&uturn);
			::SendMessage(m_hUTurnCombo, CB_RESETCONTENT, NULL, NULL);
			::SendMessage(m_hUTurnCombo, CB_ADDSTRING, NULL, (LPARAM)(_T("Not Allowed")));
			::SendMessage(m_hUTurnCombo, CB_ADDSTRING, NULL, (LPARAM)(_T("Always Allowed")));
			::SendMessage(m_hUTurnCombo, CB_ADDSTRING, NULL, (LPARAM)(_T("Only at dead ends")));
			::SendMessage(m_hUTurnCombo, CB_ADDSTRING, NULL, (LPARAM)(_T("At dead ends and intersections")));
			::SendMessage(m_hUTurnCombo, CB_SETCURSEL, (WPARAM)uturn, NULL);
		}

		// set the flocking profile names
		EvacueeGrouping evcOption;
		m_ipEvcSolver->get_EvacueeGroupingOption(&evcOption);
		::SendMessage(m_hcmbEvcOptions, CB_RESETCONTENT, NULL, NULL);
		::SendMessage(m_hcmbEvcOptions, CB_ADDSTRING, NULL, (LPARAM)(_T("None")));
		::SendMessage(m_hcmbEvcOptions, CB_ADDSTRING, NULL, (LPARAM)(_T("Merge")));
		::SendMessage(m_hcmbEvcOptions, CB_ADDSTRING, NULL, (LPARAM)(_T("Separate")));
		::SendMessage(m_hcmbEvcOptions, CB_ADDSTRING, NULL, (LPARAM)(_T("Merge and Seperate")));
		::SendMessage(m_hcmbEvcOptions, CB_SETCURSEL, (WPARAM)evcOption, NULL);

		// set flags
		VARIANT_BOOL val;
		m_ipEvcSolver->get_ExportEdgeStat(&val);
		if (val == VARIANT_TRUE) ::SendMessage(m_hEdgeStat, BM_SETCHECK, (WPARAM)BST_CHECKED, NULL);
		else  ::SendMessage(m_hEdgeStat, BM_SETCHECK, (WPARAM)BST_UNCHECKED, NULL);
		m_ipEvcSolver->get_FlockingEnabled(&val);
		if (val == VARIANT_TRUE) ::SendMessage(m_hCheckFlock, BM_SETCHECK, (WPARAM)BST_CHECKED, NULL);
		else  ::SendMessage(m_hCheckFlock, BM_SETCHECK, (WPARAM)BST_UNCHECKED, NULL);
		m_ipEvcSolver->get_TwoWayShareCapacity(&val);
		if (val == VARIANT_TRUE) ::SendMessage(m_hCheckShareCap, BM_SETCHECK, (WPARAM)BST_CHECKED, NULL);
		else  ::SendMessage(m_hCheckShareCap, BM_SETCHECK, (WPARAM)BST_UNCHECKED, NULL);
		m_ipEvcSolver->get_ThreeGenCARMA(&val);
		if (val == VARIANT_TRUE) ::SendMessage(m_hThreeGenCARMA, BM_SETCHECK, (WPARAM)BST_CHECKED, NULL);
		else  ::SendMessage(m_hThreeGenCARMA, BM_SETCHECK, (WPARAM)BST_UNCHECKED, NULL);

		// set the solver traffic model names
		EvcTrafficModel model;
		m_ipEvcSolver->get_TrafficModel(&model);
		::SendMessage(m_hComboTrafficModel, CB_RESETCONTENT, NULL, NULL);
		::SendMessage(m_hComboTrafficModel, CB_ADDSTRING, NULL, (LPARAM)(TRAFFIC_MODEL_FLAT));
		::SendMessage(m_hComboTrafficModel, CB_ADDSTRING, NULL, (LPARAM)(TRAFFIC_MODEL_STEP));
		::SendMessage(m_hComboTrafficModel, CB_ADDSTRING, NULL, (LPARAM)(TRAFFIC_MODEL_LINEAR));
		::SendMessage(m_hComboTrafficModel, CB_ADDSTRING, NULL, (LPARAM)(TRAFFIC_MODEL_POWER));
		::SendMessage(m_hComboTrafficModel, CB_ADDSTRING, NULL, (LPARAM)(TRAFFIC_MODEL_EXP));
		::SendMessage(m_hComboTrafficModel, CB_SETCURSEL, (WPARAM)model, NULL);

		// set the loaded network descriptive attribs
		m_ipEvcSolver->get_DiscriptiveAttributes(c, &names);
		::SendMessage(m_hCapCombo, CB_RESETCONTENT, NULL, NULL);
		for (i = 0; i < c; i++) ::SendMessage(m_hCapCombo, CB_ADDSTRING, NULL, (LPARAM)(names[i]));
		m_ipEvcSolver->get_CapacityAttribute(&selectedIndex);
		::SendMessage(m_hCapCombo, CB_SETCURSEL, selectedIndex, NULL);
		delete [] names;

		// set the loaded network cost attribs
		m_ipEvcSolver->get_CostAttributes(c, &names);
		::SendMessage(m_hCostCombo, CB_RESETCONTENT, NULL, NULL);
		for (i = 0; i < c; i++) ::SendMessage(m_hCostCombo, CB_ADDSTRING, NULL, (LPARAM)(names[i]));
		m_ipEvcSolver->get_CostAttribute(&selectedIndex);
		::SendMessage(m_hCostCombo, CB_SETCURSEL, selectedIndex, NULL);
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
		delete[] carma;

		// set iterative ratio
		BSTR iterative;
		m_ipEvcSolver->get_IterativeRatio(&iterative);
		::SendMessage(m_heditIterative, WM_SETTEXT, NULL, (LPARAM)iterative);
		delete[] iterative;

		// set selfish ratio
		BSTR selfish;
		m_ipEvcSolver->get_SelfishRatio(&selfish);
		::SendMessage(m_heditSelfish, WM_SETTEXT, NULL, (LPARAM)selfish);
		delete [] selfish;

		SetFlockingEnabled();
		SetDirty(FALSE);
	}

	// Let the IPropertyPageImpl deal with displaying the page
	return IPropertyPageImpl<EvcSolverPropPage>::Show(nCmdShow);
}

STDMETHODIMP EvcSolverPropPage::SetObjects(ULONG nObjects, IUnknown ** ppUnk)
{
	m_ipNALayer = nullptr;
	m_ipEvcSolver = nullptr;
	m_ipDENet = nullptr;

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
	HRESULT hr = QueryObject(ATL::CComVariant((IUnknown*)m_ipEvcSolver));

	// Set the page to not dirty
	SetDirty(FALSE);

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IPropertyPageContext

STDMETHODIMP EvcSolverPropPage::get_Priority(LONG * pPriority)
{
	if (pPriority == nullptr) return E_POINTER;

	(*pPriority) = 152;

	return S_OK;
}

STDMETHODIMP EvcSolverPropPage::Applies(VARIANT unkArray, VARIANT_BOOL* pApplies)
{
	if (pApplies == nullptr) return E_INVALIDARG;

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
	ATL::CComVariant vObject(theObject);
	if (vObject.vt != VT_UNKNOWN) return E_UNEXPECTED;
	// Try and QI to IEvcSolver
	IEvcSolverPtr ipSolver(vObject.punkVal);
	LRESULT size;
	if (ipSolver != nullptr)
	{
		// save data from drop boxes
		LRESULT selectedIndex = ::SendMessage(m_hCapCombo, CB_GETCURSEL, NULL, NULL);
		if (selectedIndex > -1) ipSolver->put_CapacityAttribute((size_t)selectedIndex);
		selectedIndex = ::SendMessage(m_hCostCombo, CB_GETCURSEL, NULL, NULL);
		if (selectedIndex > -1) ipSolver->put_CostAttribute((size_t)selectedIndex);
		selectedIndex = ::SendMessage(m_hComboMethod, CB_GETCURSEL, NULL, NULL);
		if (selectedIndex > -1) ipSolver->put_SolverMethod((EvcSolverMethod)selectedIndex);
		selectedIndex = ::SendMessage(m_hComboTrafficModel, CB_GETCURSEL, NULL, NULL);
		if (selectedIndex > -1) ipSolver->put_TrafficModel((EvcTrafficModel)selectedIndex);
		selectedIndex = ::SendMessage(m_hCmbFlockProfile, CB_GETCURSEL, NULL, NULL);
		if (selectedIndex > -1) ipSolver->put_FlockingProfile((FLOCK_PROFILE)selectedIndex);
		selectedIndex = ::SendMessage(m_hCmbCarmaSort, CB_GETCURSEL, NULL, NULL);
		if (selectedIndex > -1) ipSolver->put_CARMASortSetting((CARMASort)selectedIndex);
		selectedIndex = ::SendMessage(m_hcmbEvcOptions, CB_GETCURSEL, NULL, NULL);
		if (selectedIndex > -1) ipSolver->put_EvacueeGroupingOption((EvacueeGrouping)selectedIndex);

		if ((INASolverSettingsPtr)m_ipEvcSolver)
		{
			selectedIndex = ::SendMessage(m_hUTurnCombo, CB_GETCURSEL, NULL, NULL);
			if (selectedIndex > -1) ((INASolverSettingsPtr)(m_ipEvcSolver))->put_RestrictUTurns((esriNetworkForwardStarBacktrack)selectedIndex);
		}

		// flags
		selectedIndex = ::SendMessage(m_hThreeGenCARMA, BM_GETCHECK, NULL, NULL);
		if (selectedIndex == BST_CHECKED) ipSolver->put_ThreeGenCARMA(VARIANT_TRUE);
		else ipSolver->put_ThreeGenCARMA(VARIANT_FALSE);

		selectedIndex = ::SendMessage(m_hEdgeStat, BM_GETCHECK, NULL, NULL);
		if (selectedIndex == BST_CHECKED) ipSolver->put_ExportEdgeStat(VARIANT_TRUE);
		else ipSolver->put_ExportEdgeStat(VARIANT_FALSE);

		selectedIndex = ::SendMessage(m_hCheckFlock, BM_GETCHECK, NULL, NULL);
		if (selectedIndex == BST_CHECKED) ipSolver->put_FlockingEnabled(VARIANT_TRUE);
		else ipSolver->put_FlockingEnabled(VARIANT_FALSE);

		selectedIndex = ::SendMessage(m_hCheckShareCap, BM_GETCHECK, NULL, NULL);
		if (selectedIndex == BST_CHECKED) ipSolver->put_TwoWayShareCapacity(VARIANT_TRUE);
		else ipSolver->put_TwoWayShareCapacity(VARIANT_FALSE);

		// critical density per capacity
		BSTR critical;
		size = ::SendMessage(m_hEditCritical, WM_GETTEXTLENGTH, NULL, NULL);
		critical = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_hEditCritical, WM_GETTEXT, size + 1, (LPARAM)critical);
		ipSolver->put_CriticalDensPerCap(critical);
		delete [] critical;

		// init delay cost per population
		BSTR delay;
		size = ::SendMessage(m_hEditInitCost, WM_GETTEXTLENGTH, NULL, NULL);
		delay = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_hEditInitCost, WM_GETTEXT, size + 1, (LPARAM)delay);
		ipSolver->put_InitDelayCostPerPop(delay);
		delete [] delay;

		// CARMA ratio
		BSTR carma;
		size = ::SendMessage(m_heditCARMA, WM_GETTEXTLENGTH, NULL, NULL);
		carma = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_heditCARMA, WM_GETTEXT, size + 1, (LPARAM)carma);
		ipSolver->put_CARMAPerformanceRatio(carma);
		delete [] carma;

		// iterative ratio
		BSTR iterative;
		size = ::SendMessage(m_heditIterative, WM_GETTEXTLENGTH, NULL, NULL);
		iterative = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_heditIterative, WM_GETTEXT, size + 1, (LPARAM)iterative);
		ipSolver->put_IterativeRatio(iterative);
		delete[] iterative;

		// selfish ratio
		BSTR selfish;
		size = ::SendMessage(m_heditSelfish, WM_GETTEXTLENGTH, NULL, NULL);
		selfish = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_heditSelfish, WM_GETTEXT, size + 1, (LPARAM)selfish);
		ipSolver->put_SelfishRatio(selfish);
		delete[] selfish;

		// saturation density per capacity
		BSTR sat;
		size = ::SendMessage(m_hEditSat, WM_GETTEXTLENGTH, NULL, NULL);
		sat = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_hEditSat, WM_GETTEXT, size + 1, (LPARAM)sat);
		ipSolver->put_SaturationPerCap(sat);
		delete [] sat;

		// cost per zone density
		BSTR density;
		size = ::SendMessage(m_hEditDensity, WM_GETTEXTLENGTH, NULL, NULL);
		density = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_hEditDensity, WM_GETTEXT, size + 1, (LPARAM)density);
		ipSolver->put_CostPerZoneDensity(density);
		delete [] density;

		// flock snap interval
		BSTR flock;
		size = ::SendMessage(m_hEditSnapFlock, WM_GETTEXTLENGTH, NULL, NULL);
		flock = new DEBUG_NEW_PLACEMENT WCHAR[size + 1];
		::SendMessage(m_hEditSnapFlock, WM_GETTEXT, size + 1, (LPARAM)flock);
		ipSolver->put_FlockingSnapInterval(flock);
		delete [] flock;

		// flock simulation interval
		BSTR simul;
		size = ::SendMessage(m_hEditSimulationFlock, WM_GETTEXTLENGTH, NULL, NULL);
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
	if (pNewObject == nullptr) return E_POINTER;
	return E_NOTIMPL;
}

STDMETHODIMP EvcSolverPropPage::GetHelpFile(LONG controlID, BSTR* pHelpFile)
{
	if (pHelpFile == nullptr) return E_POINTER;
	return E_NOTIMPL;
}

STDMETHODIMP EvcSolverPropPage::GetHelpId(LONG controlID, LONG* pHelpID)
{
	if (pHelpID == nullptr) return E_POINTER;
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// Dialog

// helper function
// ref: http://stackoverflow.com/questions/371018/create-modified-hfont-from-hfont
static HFONT CreateBoldWindowFont(HWND window, bool makeLarger = false)
{
	const HFONT font = GetWindowFont(window);
	LOGFONT fontAttributes = { 0 };
	::GetObject(font, sizeof(fontAttributes), &fontAttributes);
	fontAttributes.lfWeight = FW_BOLD;
	if (makeLarger) fontAttributes.lfHeight = -13;
	return ::CreateFontIndirect(&fontAttributes);
}

LRESULT EvcSolverPropPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
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
	m_hThreeGenCARMA = GetDlgItem(IDL_CHECK_CARMAGEN);
	m_heditSelfish = GetDlgItem(IDC_EDIT_SELFISH);
	m_heditIterative = GetDlgItem(IDC_EDIT_Iterative);
	m_hCmbCarmaSort = GetDlgItem(IDC_COMBO_CarmaSort);
	m_hcmbEvcOptions = GetDlgItem(IDC_CMB_GroupOption);
	m_hUTurnCombo = GetDlgItem(IDC_COMBO_UTurn);

	// release date label
	HWND m_hlblRelease = GetDlgItem(IDC_RELEASE);
	wchar_t compileDateBuff[500];
	swprintf_s(compileDateBuff, 500, L"Release: %s  |  <a href=\"http://facebook.com/arccasper\">Like</a>  |  <a href=\"http://esri.com/arccasper\">Info</a>  |  <a href=\"https://www.dropbox.com/sh/b01zkyb6ka56xiv/oOjJBINPIr\">Download</a>", _T(__DATE__));
	::SendMessage(m_hlblRelease, WM_SETTEXT, NULL, (LPARAM)(compileDateBuff));
	swprintf_s(compileDateBuff, 500, L"%s %s", PROJ_NAME, _T(GIT_DESCRIBE));
	::SendMessage(GetDlgItem(IDC_STATIC_Title), WM_SETTEXT, NULL, (LPARAM)(compileDateBuff));

	// using bold font for title and gropu boxes
	HWND groupBoxes[] = { GetDlgItem(IDC_SearchGroup), GetDlgItem(IDC_GeneralOptions), GetDlgItem(IDC_CapacityOptions), GetDlgItem(IDC_FlockOptions), GetDlgItem(IDC_RoutingOptions) };
	HFONT boldFont = CreateBoldWindowFont(groupBoxes[0]);
	HFONT bigFont = CreateBoldWindowFont(groupBoxes[0], true);
	for (const auto & h : groupBoxes) SetWindowFont(h, boldFont, TRUE);
	SetWindowFont(GetDlgItem(IDC_STATIC_Title), bigFont, true);

	return S_OK;
}

#pragma warning(pop)

void EvcSolverPropPage::SetFlockingEnabled()
{
	LRESULT flag = ::SendMessage(m_hCheckFlock, BM_GETCHECK, NULL, NULL);
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
	return S_OK;
}

LRESULT EvcSolverPropPage::OnEnChangeEditCritical(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeComboMethod(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeComboCostmethod(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeComboCapacity(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeCostCapacity(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnBnClickedCheckSeparable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnBnClickedCheckEdgestat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnEnChangeEditZonedensity(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnBnClickedCheckFlock(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	SetFlockingEnabled();
	return S_OK;
}

LRESULT EvcSolverPropPage::OnEnChangeEditFlocksnapinterval(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnEnChangeEditFlocksimulationinterval(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnBnClickedCheckSharecap(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnEnChangeEditInitDelay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeComboProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnEnChangeEditCARMA(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnNMClickRelease(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	NMLINK* pNMLink = (NMLINK*)pNMHDR;
	LITEM   item    = pNMLink->item;
	ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnBnClickedCheckCarmagen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnEnChangeEditSelfish(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnEnChangeEditIterative(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeComboCARMASort(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeComboUTurn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}

LRESULT EvcSolverPropPage::OnCbnSelchangeComboEvcOption(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDirty(TRUE);
	//refresh property sheet
	//m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	return S_OK;
}
