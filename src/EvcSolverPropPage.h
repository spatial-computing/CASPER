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

#pragma once

#include "resource.h"                                           // main symbols
#include "CatIDs\ArcCATIDs.h"     // component category IDs
#include "EvcSolver.h"

// EvcSolverPropPage
[
  coclass,
  default(IUnknown),
  threading(apartment),
  vi_progid("CustomSolver.EvcSolverPropPage"),
  progid("CustomSolver.EvcSolverPropPage.1"),
  version(1.0),
  uuid("cd267b89-0144-4e7c-929e-bcd5d82f4c4d"),
  helpstring("EvcSolverPropPage Class")
]
class ATL_NO_VTABLE EvcSolverPropPage :
  public IPropertyPageImpl<EvcSolverPropPage>,
  public CDialogImpl<EvcSolverPropPage>,
  public IPropertyPageContext
{
public:
  EvcSolverPropPage()
  {
    m_dwTitleID = IDS_TITLEEvcSolverPROPPAGE;
    m_dwHelpFileID = IDS_HELPFILEEvcSolverPROPPAGE;
    m_dwDocStringID = IDS_DOCSTRINGEvcSolverPROPPAGE;
  }

  DECLARE_PROTECT_FINAL_CONSTRUCT()

  // Register the property page in the Property Page and Layer Property Page component categories so that it can be dynamically discovered
  // as an available property page for the EvcSolver.
  BEGIN_CATEGORY_MAP(EvcSolverSymbolizer)
    IMPLEMENTED_CATEGORY(__uuidof(CATID_PropertyPages))
    IMPLEMENTED_CATEGORY(__uuidof(CATID_LayerPropertyPages))
  END_CATEGORY_MAP()

  HRESULT FinalConstruct()
  {
    return S_OK;
  }

  void FinalRelease()
  {
  }

  enum {IDD = IDD_EvcSolverPROPPAGE};

  BEGIN_MSG_MAP(EvcSolverPropPage)
	COMMAND_HANDLER(IDC_EDIT_ZoneDensity, EN_CHANGE, OnEnChangeEditZonedensity)
	COMMAND_HANDLER(IDC_CHECK_Flock, BN_CLICKED, OnBnClickedCheckFlock)
	COMMAND_HANDLER(IDC_EDIT_FlockSnapInterval, EN_CHANGE, OnEnChangeEditFlocksnapinterval)
	COMMAND_HANDLER(IDC_EDIT_FlockSimulationInterval, EN_CHANGE, OnEnChangeEditFlocksimulationinterval)
	COMMAND_HANDLER(IDC_EDIT_INITDELAY, EN_CHANGE, OnEnChangeEditInitDelay)
	COMMAND_HANDLER(IDC_CHECK_SHARECAP, BN_CLICKED, OnBnClickedCheckSharecap)
	COMMAND_HANDLER(IDC_COMBO_PROFILE, CBN_SELCHANGE, OnCbnSelchangeComboProfile)
	COMMAND_HANDLER(IDC_EDIT_CARMA, EN_CHANGE, OnEnChangeEditCARMA)
	CHAIN_MSG_MAP(IPropertyPageImpl<EvcSolverPropPage>)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_HANDLER(IDC_EDIT_SAT, EN_CHANGE, OnEnChangeEditSat)
	COMMAND_HANDLER(IDC_EDIT_Critical, EN_CHANGE, OnEnChangeEditCritical)
	COMMAND_HANDLER(IDC_COMBO_METHOD, CBN_SELCHANGE, OnCbnSelchangeComboMethod)
	COMMAND_HANDLER(IDC_COMBO_UTurn, CBN_SELCHANGE, OnCbnSelchangeComboUTurn)
  	COMMAND_HANDLER(IDC_COMBO_TRAFFICMODEL, CBN_SELCHANGE, OnCbnSelchangeComboCostmethod)
  	COMMAND_HANDLER(IDC_COMBO_CAPACITY, CBN_SELCHANGE, OnCbnSelchangeComboCapacity)
	COMMAND_HANDLER(IDC_COMBO_COST, CBN_SELCHANGE, OnCbnSelchangeCostCapacity)
	COMMAND_HANDLER(IDC_COMBO_CarmaSort, CBN_SELCHANGE, OnCbnSelchangeComboCARMASort)
	COMMAND_HANDLER(IDC_CHECK_SEPARABLE, BN_CLICKED, OnBnClickedCheckSeparable)
	COMMAND_HANDLER(IDC_CHECK_EDGESTAT, BN_CLICKED, OnBnClickedCheckEdgestat)
	NOTIFY_HANDLER(IDC_RELEASE, NM_CLICK, OnNMClickRelease)
	COMMAND_HANDLER(IDL_CHECK_CARMAGEN, BN_CLICKED, OnBnClickedCheckCarmagen)
	COMMAND_HANDLER(IDC_EDIT_SELFISH, EN_CHANGE, OnEnChangeEditSelfish)
  END_MSG_MAP()

  // Handler prototypes:
  //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

  // IPropertyPage

  STDMETHOD(Apply)(void);
  STDMETHOD(Show)(UINT nCmdShow);
  STDMETHOD(SetObjects)(ULONG nObjects, IUnknown** ppUnk);

  // IPropertyPageContext

  STDMETHOD(get_Priority)(LONG* pPriority);
  STDMETHOD(Applies)(VARIANT unkArray, VARIANT_BOOL* pApplies);
  STDMETHOD(CreateCompatibleObject)(VARIANT kind, VARIANT* pNewObject);
  STDMETHOD(QueryObject)(VARIANT theObject);
  STDMETHOD(GetHelpFile)(LONG controlID, BSTR* pHelpFile);
  STDMETHOD(GetHelpId)(LONG controlID, LONG* pHelpID);
  STDMETHOD(Cancel)();

  // Dialog

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnBnClickedRadioConnected(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnBnClickedRadioDisconnected(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnCbnSelchangeComboCost(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
  INALayerPtr             m_ipNALayer;
  IEvcSolverPtr			  m_ipEvcSolver;
  IDENetworkDatasetPtr    m_ipDENet;

  HWND                    m_hCapCombo;
  HWND                    m_hUTurnCombo;
  HWND                    m_hCostCombo;
  HWND                    m_hComboMethod;
  HWND                    m_hComboTrafficModel;
  HWND                    m_hEditCritical;
  HWND                    m_hEditSat;
  HWND                    m_hSeparable;
  HWND                    m_hEdgeStat;
  HWND                    m_hEditDensity;
  HWND                    m_hEditSnapFlock;
  HWND                    m_hEditSimulationFlock;
  HWND                    m_hCheckFlock;
  HWND                    m_hCheckShareCap;
  HWND                    m_hEditInitCost;
  HWND					  m_hCmbFlockProfile;
  HWND					  m_hCmbCarmaSort;
  HWND					  m_heditCARMA;
  HWND					  m_hThreeGenCARMA;
  HWND					  m_heditSelfish;

  void SetFlockingEnabled();

public:
	LRESULT OnLbnDblclkRestrictionlist(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnChangeEditMaxradius(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCbnSelchangeComboHeuristic(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnChangeEditSpeed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnChangeEditSat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnChangeEditCritical(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCbnSelchangeComboMethod(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCbnSelchangeComboCostmethod(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCbnSelchangeComboCapacity(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCbnSelchangeCostCapacity(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedCheckSeparable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedCheckEdgestat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnChangeEditZonedensity(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedCheckFlock(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnChangeEditFlockinterval(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnChangeEditFlocksnapinterval(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnChangeEditFlocksimulationinterval(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnChangeEditInitDelay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedCheckSharecap(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCbnSelchangeComboCARMASort(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCbnSelchangeComboUTurn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCbnSelchangeComboProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnChangeEditCARMA(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNMClickRelease(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
	LRESULT OnBnClickedCheckCarmagen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnStnClickedLablecarma2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnChangeEditSelfish(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
