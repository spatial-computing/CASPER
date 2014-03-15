// Copyright 2011 ESRI
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
#include "NameConstants.h"
#include "EvcSolver.h"

#pragma warning(push)
#pragma warning(disable : 4100) /* Ignore warnings for unreferenced function parameters */

// EvcSolver

/////////////////////////////////////////////////////////////////////
// INASolverOutputGeneralization
STDMETHODIMP EvcSolver::put_OutputGeometryPrecision(VARIANT value)
{
	return S_OK;
}

STDMETHODIMP EvcSolver::get_OutputGeometryPrecision(VARIANT * value)
{
	return S_OK;
}

STDMETHODIMP EvcSolver::put_OutputGeometryPrecisionUnits(esriUnits value)
{
	return S_OK;
}

STDMETHODIMP EvcSolver::get_OutputGeometryPrecisionUnits(esriUnits * value)
{
	return S_OK;
}

/////////////////////////////////////////////////////////////////////
// INARouteSolver2

STDMETHODIMP EvcSolver::get_StartTime(DATE * Value)
{
	if (!Value) return E_POINTER;
	*Value = 0.0;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_StartTime(DATE Value)
{
	return S_OK;
}

STDMETHODIMP EvcSolver::get_UseStartTime(VARIANT_BOOL * Value)
{
	if (!Value) return E_POINTER;
	*Value = VARIANT_FALSE;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_UseStartTime(VARIANT_BOOL Value)
{
	return S_OK;
}

STDMETHODIMP EvcSolver::get_OutputLines(esriNAOutputLineType * Value)
{
	if (!Value) return E_POINTER;
	*Value = m_outputLineType;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_OutputLines(esriNAOutputLineType Value)
{
	m_outputLineType = Value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_CreateTraversalResult(VARIANT_BOOL * Value)
{
	if (!Value) return E_POINTER;
	*Value = m_CreateTraversalResult;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_CreateTraversalResult(VARIANT_BOOL Value)
{
	m_CreateTraversalResult = Value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_UseTimeWindows(VARIANT_BOOL * Value)
{
	if (!Value) return E_POINTER;
	*Value = m_UseTimeWindows;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_UseTimeWindows(VARIANT_BOOL Value)
{
	m_UseTimeWindows = Value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_PreserveLastStop(VARIANT_BOOL * Value)
{
	if (!Value) return E_POINTER;
	*Value = m_PreserveLastStop;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_PreserveLastStop(VARIANT_BOOL Value)
{
	m_PreserveLastStop = Value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_PreserveFirstStop(VARIANT_BOOL * Value)
{
	if (!Value) return E_POINTER;
	*Value = m_PreserveFirstStop;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_PreserveFirstStop(VARIANT_BOOL Value)
{
	m_PreserveFirstStop = Value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_FindBestSequence(VARIANT_BOOL * Value)
{
	if (!Value) return E_POINTER;
	*Value = m_FindBestSequence;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_FindBestSequence(VARIANT_BOOL Value)
{
	m_FindBestSequence = Value;
	m_bPersistDirty = true;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////
// INASolver

STDMETHODIMP EvcSolver::Bind(INAContext* pContext, IDENetworkDataset* pNetwork, IGPMessages* pMessages)
{
	// Bind() is a method used to re-associate the solver with a given network dataset and its schema. Calling Bind()
	// on the solver re-attaches the solver to the NAContext based on the current network dataset settings.
	// This is typically used to update the solver and its context based on changes in the network dataset's available
	// restrictions, hierarchy attributes, cost attributes, etc.	

	// load network attributes for later configuration and usage
	// this will be used to load restriction and also to load proper impedance (cost) value
	
	INetworkAttribute2Ptr networkAttrib = 0;
	long count, i;
	HRESULT hr;
	esriNetworkAttributeUsageType utype;
	esriNetworkAttributeDataType dtype;	
	IUnknownPtr unk;

	if (pNetwork)
	{
		if (FAILED(hr = pNetwork->get_Attributes(&allAttribs))) return hr;
		if (FAILED(hr = allAttribs->get_Count(&count))) return hr;

		turnAttribs.clear();
		costAttribs.clear();
		discriptiveAttribs.clear();

		for (i = 0; i < count; i++)
		{
			if (FAILED(hr = allAttribs->get_Element(i, &unk))) return hr;
			networkAttrib = unk;
			if (FAILED(hr = networkAttrib->get_UsageType(&utype))) return hr;
			if (FAILED(hr = networkAttrib->get_DataType(&dtype))) return hr;
			if (utype == esriNAUTRestriction) turnAttribs.insert(turnAttribs.end(), networkAttrib);			
			else if (utype == esriNAUTCost) costAttribs.insert(costAttribs.end(), networkAttrib);
			else if (utype == esriNAUTDescriptor && (dtype == esriNADTDouble || dtype == esriNADTInteger))
				discriptiveAttribs.insert(discriptiveAttribs.end(), networkAttrib);
		}

		if (costAttributeID == -1) costAttribs[0]->get_ID(&costAttributeID);
		if (capAttributeID  == -1) discriptiveAttribs[0]->get_ID(&capAttributeID);		

		// Agents setup
		// NOTE: this is an appropriate place to find and attach any agents used by this solver.
		// For example, the route solver would attach the directions agent.
		INamedSetPtr agents;
		IUnknownPtr pStreet;
		INAContextHelperPtr ipContextHelper(pContext);
		if (FAILED(hr = pContext->get_Agents(&agents))) return hr;	
		if (FAILED(hr = agents->get_ItemByName(L"StreetDirectionsAgent", &pStreet))) return hr;
		if (!pStreet)
		{
			pStreetAgent = INAStreetDirectionsAgentPtr(CLSID_NAStreetDirectionsAgent);
			((INAAgentPtr)pStreetAgent)->Initialize(pNetwork, ipContextHelper);	
			if (FAILED(hr = agents->Add(L"StreetDirectionsAgent", pStreetAgent))) return hr;
		}
		else pStreetAgent = pStreet;
	}
	return S_OK;  
}

STDMETHODIMP EvcSolver::get_Name(BSTR * pName)
{
	// This name is locale/language independent and should not be translated. 
	if (!pName) return E_POINTER;
	*pName = ::SysAllocString(CS_NAME);

	return S_OK;
}

STDMETHODIMP EvcSolver::get_DisplayName(BSTR * pName)
{
	// This name should be translated and would typically come from
	// a string resource. 
	if (!pName) return E_POINTER;
	*pName = ::SysAllocString(CS_DISPLAY_NAME);

	return S_OK;
}

STDMETHODIMP EvcSolver::get_ClassDefinitions(INamedSet ** ppDefinitions)
{
	if (!ppDefinitions) return E_POINTER;

	*ppDefinitions = 0;

	// get_ClassDefinitions() will return the default NAClasses that this solver uses.
	// We will define a NAClass for "Evacuee Points", "Barriers", and "Lines".
	// Barriers are just like the other NA Solvers' Barriers.
	// Lines are the sets of connected/disconnected lines that this solver discovers.
	// Lines are akin to the Routes output from a Route Solver.
	// Evacuee Points are the origins of traversal for this solver.

	ISpatialReferencePtr ipUnkSR(CLSID_UnknownCoordinateSystem);

	HRESULT hr;
	if (FAILED(hr = BuildClassDefinitions(ipUnkSR, ppDefinitions, 0)))
		return AtlReportError(GetObjectCLSID(), _T("Failed to create class definitions."), IID_INASolver, hr);

	return S_OK;
}

STDMETHODIMP EvcSolver::get_CanUseHierarchy(VARIANT_BOOL * pCanUseHierarchy)
{
	// This solver does not make use of hierarchies
	if (!pCanUseHierarchy) return E_POINTER;
	*pCanUseHierarchy = VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP EvcSolver::get_CanAccumulateAttributes(VARIANT_BOOL * pCanAccumulateAttrs)
{
	// This solver does not support attribute accumulation
	if (!pCanAccumulateAttrs) return E_POINTER;
	*pCanAccumulateAttrs = VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP EvcSolver::get_Properties(IPropertySet ** ppPropSet)
{
	if (!ppPropSet) return E_POINTER;

	// The use of the property set has been deprecated at 9.2.
	// Clients should use the accessors and mutators on the solver interfaces.

	*ppPropSet = 0;

	return E_NOTIMPL;
}

STDMETHODIMP EvcSolver::CreateLayer(INAContext * pContext, INALayer ** ppLayer)
{
	if (!ppLayer) return E_POINTER;

	*ppLayer = 0;

	// This is an appropriate place to check if the user is licensed to run
	// your solver and fail with "E_NOT_LICENSED" or similar.

	// Create our custom symbolizer and use it to create the NALayer.
	// NOTE: we are assuming here that there is only one symbolizer to
	// consider.  The Network Analyst framework and ESRI solvers support the notion
	// that there can be many symbolizers in CATID_NetworkAnalystSymbolizer.
	// One can iterate the objects in this category and call the Applies()
	// method to see if the symbolizer should be used for a particular solver/context.
	// There is also a get_Priority() method that is used to determine which
	// to use if many Apply.

	INASymbolizerPtr ipNASymbolizer(CLSID_EvcSolverSymbolizer);

	return ipNASymbolizer->CreateLayer(pContext, ppLayer);
}

STDMETHODIMP EvcSolver::UpdateLayer(INALayer* pLayer, VARIANT_BOOL* pLayerUpdated)
{
	if (!pLayer || !pLayerUpdated) return E_POINTER;

	// This method is called after Solve() and gives us a chance to react to the results of
	// the Solve and change the layer. For example, the Service Area solver updates
	// its layer renderers after Solve has been called to adjust for unique values.

	// We will not need to update our layer, since our renderers will remain the same throughout

	*pLayerUpdated = VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP EvcSolver::CreateContext(IDENetworkDataset* pNetwork, BSTR contextName, INAContext** ppNAContext)
{
	if (!pNetwork || !ppNAContext) return E_POINTER;

	if (!contextName || !::wcslen(contextName)) return E_INVALIDARG;

	HRESULT hr;
	*ppNAContext = 0;

	// CreateContext() is called by the command that creates the solver and
	// initializes it.  Below we'll:
	//
	// - create the class definitions
	// - set up defaults
	//
	// After this method is called, this class should be prepared to have
	// its solve method called.

	// This is an appropriate place to check if the user is licensed to run
	// your solver and fail with "E_NOT_LICENSED" or similar.

	// Get the NDS SpatialRef, that will be the same for the context as well as all spatial NAClasses
	IDEGeoDatasetPtr ipDEGeoDataset(pNetwork);
	if (!ipDEGeoDataset) return E_INVALIDARG;
	ISpatialReferencePtr ipNAContextSR;
	if (FAILED(hr = ipDEGeoDataset->get_SpatialReference(&ipNAContextSR))) return hr;

	IUnknownPtr           ipUnknown;
	INamedSetPtr          ipNAClassDefinitions;
	INAClassDefinitionPtr ipEvacueePointsClassDef, ipBarriersClassDef, ipRoutesClassDef, ipZonesClassDef, ipEdgeStatClassDef, ipFlocksClassDef;

	// Build the class definitions
	if (FAILED(hr = BuildClassDefinitions(ipNAContextSR, &ipNAClassDefinitions, pNetwork))) return hr;

	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_EVACUEES_NAME), &ipUnknown);
	ipEvacueePointsClassDef = ipUnknown;

	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_ZONES_NAME), &ipUnknown);
	ipZonesClassDef = ipUnknown;

	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_BARRIERS_NAME), &ipUnknown);
	ipBarriersClassDef = ipUnknown;

	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_ROUTES_NAME), &ipUnknown);
	ipRoutesClassDef = ipUnknown;

	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_EDGES_NAME), &ipUnknown);
	ipEdgeStatClassDef = ipUnknown;
#if defined(_FLOCK)
	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_FLOCKS_NAME), &ipUnknown);
	ipFlocksClassDef = ipUnknown;
#endif
	// Create a context and initialize it
	INAContextPtr     ipNAContext(CLSID_NAContext);
	INAContextEditPtr ipNAContextEdit(ipNAContext);
	if (FAILED(hr = ipNAContextEdit->Initialize(contextName, pNetwork))) return hr;
	if (FAILED(hr = ipNAContextEdit->putref_Solver(static_cast<INASolver*>(this)))) return hr;

	// Create NAClasses for each of our class definitions
	INamedSetPtr ipNAClasses;
	INAClassPtr  ipNAClass;

	ipNAContext->get_NAClasses(&ipNAClasses);
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipZonesClassDef, &ipNAClass))) return hr;
	ipNAClasses->Add(CComBSTR(CS_ZONES_NAME), (IUnknownPtr)ipNAClass);
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipEvacueePointsClassDef, &ipNAClass))) return hr;
	ipNAClasses->Add(CComBSTR(CS_EVACUEES_NAME), (IUnknownPtr)ipNAClass);
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipBarriersClassDef, &ipNAClass))) return hr;
	ipNAClasses->Add(CComBSTR(CS_BARRIERS_NAME), (IUnknownPtr)ipNAClass);
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipRoutesClassDef, &ipNAClass))) return hr;
	ipNAClasses->Add(CComBSTR(CS_ROUTES_NAME), (IUnknownPtr)ipNAClass);
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipEdgeStatClassDef, &ipNAClass))) return hr;
	ipNAClasses->Add(CComBSTR(CS_EDGES_NAME), (IUnknownPtr)ipNAClass);
#if defined(_FLOCK)
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipFlocksClassDef, &ipNAClass))) return hr;
	ipNAClasses->Add(CComBSTR(CS_FLOCKS_NAME), (IUnknownPtr)ipNAClass);
#endif

	// NOTE: this is an appropriate place to set up any hierarchy defaults if your
	// solver supports using a hierarchy attribute (this solver does not). This is
	// also an appropriate place to set the default impedance attribute if you
	// had one.

	// Initialize the default field mappings
	// NOTE: this is an appropriate place to set up any default field mappings to be used

	// Return the context once it has been fully created and initialized
	*ppNAContext = ipNAContext;
	ipNAContext->AddRef();

	// Set up our solver defaults
	m_outputLineType = esriNAOutputLineTrueShapeWithMeasure;
	costAttributeID = -1;
	capAttributeID = -1;
	SaturationPerCap = 500.0;
	CriticalDensPerCap = 20.0;
	solverMethod = EVC_SOLVER_METHOD_CASPER;
	trafficModel = EVC_TRAFFIC_MODEL_CASPER;
	flockingProfile = FLOCK_PROFILE_CAR;
	
	m_CreateTraversalResult = VARIANT_TRUE;
	m_FindBestSequence = VARIANT_FALSE;
	m_PreserveFirstStop = VARIANT_FALSE;
	m_PreserveLastStop = VARIANT_FALSE;
	m_UseTimeWindows = VARIANT_FALSE;
	separable = VARIANT_FALSE;
	exportEdgeStat = VARIANT_TRUE;
	costPerDensity = 0.0f;
	flockingEnabled = VARIANT_FALSE;
	twoWayShareCapacity = VARIANT_TRUE;
	flockingSnapInterval = 0.1f;
	flockingSimulationInterval = 0.01f;
	initDelayCostPerPop = 0.0f;
	CARMAPerformanceRatio = 0.1f;

	backtrack = esriNFSBAtDeadEndsOnly;

	return S_OK;
}

STDMETHODIMP EvcSolver::UpdateContext(INAContext* pNAContext, IDENetworkDataset* pNetwork, IGPMessages* pMessages)
{
	// UpdateContext() is a method used to update the context based on any changes that may have been made to the
	// solver settings. This typically includes changes to the set of accumulation attribute names, etc., which can
	// be set as fields in the context's NAClass schemas
	return S_OK;
}

/////////////////////////////////////////////////////////////////////
// IEvcSolver

STDMETHODIMP EvcSolver::get_FlockingEnabled(VARIANT_BOOL * value)
{
	*value = flockingEnabled;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_FlockingEnabled(VARIANT_BOOL value)
{
	flockingEnabled = value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_TwoWayShareCapacity(VARIANT_BOOL * value)
{
	*value = twoWayShareCapacity;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_TwoWayShareCapacity(VARIANT_BOOL value)
{
	twoWayShareCapacity = value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_ExportEdgeStat(VARIANT_BOOL * value)
{
	*value = exportEdgeStat;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_ExportEdgeStat(VARIANT_BOOL value)
{
	exportEdgeStat = value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_SeparableEvacuee(VARIANT_BOOL * value)
{
	*value = separable;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_SeparableEvacuee(VARIANT_BOOL value)
{
	separable = value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_TrafficModel(EVC_TRAFFIC_MODEL * value)
{
	*value = trafficModel;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_TrafficModel(EVC_TRAFFIC_MODEL value)
{
	trafficModel = value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_FlockingProfile(FLOCK_PROFILE * value)
{
	*value = flockingProfile;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_FlockingProfile(FLOCK_PROFILE value)
{
	flockingProfile = value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_CARMAPerformanceRatio(BSTR * value)
{
	if (value)
	{
		*value = new DEBUG_NEW_PLACEMENT WCHAR[100];
		swprintf_s(*value, 100, L"%.3f", CARMAPerformanceRatio);
	}
	return S_OK;
}

STDMETHODIMP EvcSolver::put_CARMAPerformanceRatio(BSTR value)
{
	swscanf_s(value, L"%f", &CARMAPerformanceRatio);
	CARMAPerformanceRatio = min(max(CARMAPerformanceRatio, 0.0f), 1.0f);
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_SolverMethod(EVC_SOLVER_METHOD * value)
{
	*value = solverMethod;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_SolverMethod(EVC_SOLVER_METHOD value)
{
	solverMethod = value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_SaturationPerCap(BSTR * value)
{
	if (value)
	{
		*value = new DEBUG_NEW_PLACEMENT WCHAR[100];
		swprintf_s(*value, 100, L"%.2f", SaturationPerCap);
	}
	return S_OK;
}

STDMETHODIMP EvcSolver::get_FlockingSnapInterval(BSTR * value)
{	
	if (value)
	{
		*value = new DEBUG_NEW_PLACEMENT WCHAR[100];
		swprintf_s(*value, 100, L"%.4f", flockingSnapInterval);
	}
	return S_OK;
}

STDMETHODIMP EvcSolver::get_FlockingSimulationInterval(BSTR * value)
{	
	if (value)
	{
		*value = new DEBUG_NEW_PLACEMENT WCHAR[100];
		swprintf_s(*value, 100, L"%.4f", flockingSimulationInterval);
	}
	return S_OK;
}

STDMETHODIMP EvcSolver::get_CriticalDensPerCap(BSTR * value)
{	
	if (value)
	{
		*value = new DEBUG_NEW_PLACEMENT WCHAR[100];
		swprintf_s(*value, 100, L"%.2f", CriticalDensPerCap);
	}
	return S_OK;
}

STDMETHODIMP EvcSolver::get_CostPerZoneDensity(BSTR * value)
{	
	if (value)
	{
		*value = new DEBUG_NEW_PLACEMENT WCHAR[100];
		swprintf_s(*value, 100, L"%.2f", costPerDensity);
	}
	return S_OK;
}

STDMETHODIMP EvcSolver::get_InitDelayCostPerPop(BSTR * value)
{	
	if (value)
	{
		*value = new DEBUG_NEW_PLACEMENT WCHAR[100];
		swprintf_s(*value, 100, L"%.4f", initDelayCostPerPop);
	}
	return S_OK;
}

STDMETHODIMP EvcSolver::put_FlockingSnapInterval(BSTR value)
{	
	swscanf_s(value, L"%f", &flockingSnapInterval);
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_InitDelayCostPerPop(BSTR value)
{	
	swscanf_s(value, L"%f", &initDelayCostPerPop);
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_FlockingSimulationInterval(BSTR value)
{	
	swscanf_s(value, L"%f", &flockingSimulationInterval);
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_SaturationPerCap(BSTR value)
{	
	swscanf_s(value, L"%f", &SaturationPerCap);
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_CriticalDensPerCap(BSTR value)
{	
	swscanf_s(value, L"%f", &CriticalDensPerCap);
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_CostPerZoneDensity(BSTR value)
{	
	swscanf_s(value, L"%f", &costPerDensity);
	m_bPersistDirty = true;
	return S_OK;
}

// returns the name of the heuristic attributes loaded from the network dataset.
// this will be called from the property page so the user can select.
STDMETHODIMP EvcSolver::get_CostAttributes(BSTR ** Names)
{
	size_t count = costAttribs.size(), i;
	HRESULT hr;
	BSTR * names = new DEBUG_NEW_PLACEMENT BSTR[count];

	for (i = 0; i < count; i++) if (FAILED(hr = costAttribs[i]->get_Name(&(names[i])))) return hr;	

	*Names = names;

	return S_OK;
}

// returns the name of the heuristic attributes loaded from the network dataset.
// this will be called from the property page so the user can select.
STDMETHODIMP EvcSolver::get_DiscriptiveAttributes(BSTR ** Names)
{
	size_t count = discriptiveAttribs.size(), i;
	HRESULT hr;
	BSTR * names = new DEBUG_NEW_PLACEMENT BSTR[count];

	for (i = 0; i < count; i++) if (FAILED(hr = discriptiveAttribs[i]->get_Name(&(names[i])))) return hr;	

	*Names = names;

	return S_OK;
}

// returns the count of available heuristic attributes from the network dataset to the property page
STDMETHODIMP EvcSolver::get_CostAttributesCount(size_t * Count)
{
	size_t count = costAttribs.size();
	*Count = count;
	return S_OK;
}

// returns the count of available heuristic attributes from the network dataset to the property page
STDMETHODIMP EvcSolver::get_DiscriptiveAttributesCount(size_t * Count)
{
	size_t count = discriptiveAttribs.size();
	*Count = count;
	return S_OK;
}

// Gets the selected cost attribute back to the property page
STDMETHODIMP EvcSolver::get_CapacityAttribute(size_t * index)
{	
	size_t count = discriptiveAttribs.size(), i;
	HRESULT hr;
	long ID;

	for (i = 0; i < count; i++)
	{
		if (FAILED(hr = discriptiveAttribs[i]->get_ID(&ID))) return hr;
		if (capAttributeID == ID)
		{
			if (index) *index = i;
			break;
		}
	}
	return S_OK;
}

// Gets the selected cost attribute back to the property page
STDMETHODIMP EvcSolver::get_CostAttribute(size_t * index)
{	
	size_t count = costAttribs.size(), i;
	HRESULT hr;
	long ID;

	for (i = 0; i < count; i++)
	{
		if (FAILED(hr = costAttribs[i]->get_ID(&ID))) return hr;
		if (costAttributeID == ID)
		{
			if (index) *index = i;
			break;
		}
	}
	return S_OK;
}

// Sets the selected cost attribute based on what user selected in property page
STDMETHODIMP EvcSolver::put_CapacityAttribute(size_t index)
{		
	HRESULT hr;
	if (FAILED(hr = discriptiveAttribs[index]->get_ID(&capAttributeID))) return hr;
	m_bPersistDirty = true;
	return S_OK;
}

// Sets the selected cost attribute based on what user selected in property page
STDMETHODIMP EvcSolver::put_CostAttribute(size_t index)
{		
	HRESULT hr;
	if (FAILED(hr = costAttribs[index]->get_ID(&costAttributeID))) return hr;
	m_bPersistDirty = true;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////
// INASolverSettings2

STDMETHODIMP EvcSolver::put_AttributeParameterValue(BSTR AttributeName, BSTR paramName, VARIANT value)
{	
	INetworkAttribute2Ptr networkAttrib = 0;
	INetworkAttributeParameterPtr param = 0;
	IArray * params = 0;
	long c1, c2, i, j;
	HRESULT hr;
	IUnknownPtr unk;
	BSTR name1, name2;

	if (allAttribs)
	{
		if (FAILED(hr = allAttribs->get_Count(&c1))) return hr;
		
		for (i = 0; i < c1; i++)
		{
			if (FAILED(hr = allAttribs->get_Element(i, &unk))) return hr;
			networkAttrib = unk;
			if (FAILED(hr = networkAttrib->get_Name(&name1))) return hr;
			if (wcscmp(name1, AttributeName) == 0)
			{
				if (FAILED(hr = networkAttrib->get_Parameters(&params))) return hr;
				if (FAILED(hr = params->get_Count(&c2))) return hr;
				for (j = 0; j < c2; j++)
				{
					if (FAILED(hr = params->get_Element(j, &unk))) return hr;
					param = unk;
					if (FAILED(hr = param->get_Name(&name2))) return hr;
					if (wcscmp(name2, paramName) == 0)
					{
						if (FAILED(hr = param->put_Value(value))) return hr;
						break;
					}
				}
				break;
			}
		}
	}
	return S_OK;
}

STDMETHODIMP EvcSolver::get_AttributeParameterValue(BSTR AttributeName, BSTR paramName, VARIANT * value)
{
	INetworkAttribute2Ptr networkAttrib = 0;
	INetworkAttributeParameterPtr param = 0;
	IArray * params = 0;
	long c1, c2, i, j;
	HRESULT hr;
	IUnknownPtr unk;
	BSTR name1, name2;

	if (allAttribs)
	{
		if (FAILED(hr = allAttribs->get_Count(&c1))) return hr;
		
		for (i = 0; i < c1; i++)
		{
			if (FAILED(hr = allAttribs->get_Element(i, &unk))) return hr;
			networkAttrib = unk;
			if (FAILED(hr = networkAttrib->get_Name(&name1))) return hr;
			if (wcscmp(name1, AttributeName) == 0)
			{
				if (FAILED(hr = networkAttrib->get_Parameters(&params))) return hr;
				if (FAILED(hr = params->get_Count(&c2))) return hr;
				for (j = 0; j < c2; j++)
				{
					if (FAILED(hr = params->get_Element(j, &unk))) return hr;
					param = unk;
					if (FAILED(hr = param->get_Name(&name2))) return hr;
					if (wcscmp(name2, paramName) == 0)
					{
						if (FAILED(hr = param->get_Value(value))) return hr;
						break;
					}
				}
				break;
			}
		}
	}
	return S_OK;
}

STDMETHODIMP EvcSolver::put_ResetHierarchyRangesOnBind(VARIANT_BOOL value)
{
	return E_NOTIMPL; // This solver does not support hierarchy
}

STDMETHODIMP EvcSolver::get_ResetHierarchyRangesOnBind(VARIANT_BOOL * value)
{
	return E_NOTIMPL; // This solver does not support hierarchy
}

STDMETHODIMP EvcSolver::put_ImpedanceAttributeName(BSTR attributeName)
{
	size_t count = costAttribs.size(), i;
	HRESULT hr;
	BSTR name;

	for (i = 0; i < count; i++)
	{
		if (FAILED(hr = costAttribs[i]->get_Name(&name))) return hr;
		if (wcscmp(name, attributeName) == 0)
		{
			if (FAILED(hr = costAttribs[i]->get_ID(&costAttributeID))) return hr;
			m_bPersistDirty = true;
			break;
		}
	}
	return S_OK;
}

STDMETHODIMP EvcSolver::get_ImpedanceAttributeName(BSTR * pAttributeName)
{
	size_t count = costAttribs.size(), i;
	HRESULT hr;
	long ID = -1;

	if (!pAttributeName) return E_POINTER;

	for (i = 0; i < count; i++)
	{
		if (FAILED(hr = costAttribs[i]->get_ID(&ID))) return hr;
		if (ID == costAttributeID)
		{
			if (FAILED(hr = costAttribs[i]->get_Name(pAttributeName))) return hr;
			break;
		}
	}	
	return S_OK;
}

// returns the name of the turn attributes loaded from the network dataset.
// this will be called from the property page so the user can select.
STDMETHODIMP EvcSolver::get_RestrictionAttributeNames(IStringArray ** ppAttributeName)
{
	size_t count = turnAttribs.size(), i;
	HRESULT hr;
	BSTR name;
	IStringArrayPtr names(CLSID_StrArray);

	for (i = 0; i < count; i++)
	{
		if (FAILED(hr = turnAttribs[i]->get_Name(&name))) return hr;
		names->Add(name);
	}	
	
	names->AddRef();
	*ppAttributeName = names;

	return S_OK;
}

STDMETHODIMP EvcSolver::putref_RestrictionAttributeNames(IStringArray * pAttributeName)
{
	turnAttributeNames = pAttributeName;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_RestrictUTurns(esriNetworkForwardStarBacktrack Backtrack)
{
	backtrack = Backtrack;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_RestrictUTurns(esriNetworkForwardStarBacktrack * pBacktrack)
{
	if (pBacktrack) *pBacktrack = backtrack;	
	return S_OK;
}

STDMETHODIMP EvcSolver::get_AccumulateAttributeNames(IStringArray ** ppAttributeNames)
{
	return E_NOTIMPL; // This solver does not support attribute accumulation
}

STDMETHODIMP EvcSolver::putref_AccumulateAttributeNames(IStringArray * pAttributeNames)
{
	return E_NOTIMPL; // This solver does not support attribute accumulation
}

STDMETHODIMP EvcSolver::put_IgnoreInvalidLocations(VARIANT_BOOL ignoreInvalidLocations)
{
	return S_OK; // This solver does not support ignoring invalid locations
}

STDMETHODIMP EvcSolver::get_IgnoreInvalidLocations(VARIANT_BOOL * pIgnoreInvalidLocations)
{
	*pIgnoreInvalidLocations = VARIANT_TRUE;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_UseHierarchy(VARIANT_BOOL useHierarchy)
{
	return S_OK; // This solver does not support hierarchy attributes
}

STDMETHODIMP EvcSolver::get_UseHierarchy(VARIANT_BOOL* pUseHierarchy)
{
	if (!pUseHierarchy) return E_POINTER;

	// This solver does not support hierarchy attributes

	*pUseHierarchy = VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP EvcSolver::put_HierarchyAttributeName(BSTR attributeName)
{
	return E_NOTIMPL; // This solver does not support hierarchy attributes
}

STDMETHODIMP EvcSolver::get_HierarchyAttributeName(BSTR* pattributeName)
{
	return E_NOTIMPL; // This solver does not support hierarchy attributes
}

STDMETHODIMP EvcSolver::put_HierarchyLevelCount(long Count)
{
	return S_OK; // This solver does not support hierarchy attributes
}

STDMETHODIMP EvcSolver::get_HierarchyLevelCount(long * pCount)
{
	*pCount = 0;
	return S_OK; // This solver does not support hierarchy attributes
}

STDMETHODIMP EvcSolver::put_MaxValueForHierarchy(long level, long value)
{
	return E_NOTIMPL; // This solver does not support hierarchy attributes
}

STDMETHODIMP EvcSolver::get_MaxValueForHierarchy(long level, long * pValue)
{
	return E_NOTIMPL; // This solver does not support hierarchy attributes
}

STDMETHODIMP EvcSolver::put_NumTransitionToHierarchy(long toLevel, long value)
{
	return E_NOTIMPL; // This solver does not support hierarchy attributes
}

STDMETHODIMP EvcSolver::get_NumTransitionToHierarchy(long toLevel, long * pValue)
{
	return E_NOTIMPL; // This solver does not support hierarchy attributes
}

#pragma warning(pop)

/////////////////////////////////////////////////////////////////////
// IPersistStream

STDMETHODIMP EvcSolver::IsDirty()
{
	return (m_bPersistDirty ? S_OK : S_FALSE);
}

STDMETHODIMP EvcSolver::Load(IStream* pStm)
{
	if (!pStm) return E_POINTER;

	ULONG     numBytes;
	HRESULT   hr;

	// We need to check the saved version number
	long savedVersion;
	if (FAILED(hr = pStm->Read(&savedVersion, sizeof(savedVersion), &numBytes))) return hr;

	// We only support versions less than or equal to the current c_version
	if (savedVersion > c_version || savedVersion <= 0) return E_FAIL;

	// We need to read our persisted solver settings
	if (FAILED(hr = pStm->Read(&m_outputLineType, sizeof(m_outputLineType), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&costAttributeID, sizeof(costAttributeID), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&capAttributeID, sizeof(capAttributeID), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&trafficModel, sizeof(trafficModel), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&solverMethod, sizeof(solverMethod), &numBytes))) return hr;	
	if (FAILED(hr = pStm->Read(&SaturationPerCap, sizeof(SaturationPerCap), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&CriticalDensPerCap, sizeof(CriticalDensPerCap), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&m_CreateTraversalResult, sizeof(m_CreateTraversalResult), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&m_FindBestSequence, sizeof(m_FindBestSequence), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&m_PreserveFirstStop, sizeof(m_PreserveFirstStop), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&m_PreserveLastStop, sizeof(m_PreserveLastStop), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&m_UseTimeWindows, sizeof(m_UseTimeWindows), &numBytes))) return hr;	
	if (FAILED(hr = pStm->Read(&separable, sizeof(separable), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&exportEdgeStat, sizeof(exportEdgeStat), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&backtrack, sizeof(backtrack), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&costPerDensity, sizeof(costPerDensity), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&flockingEnabled, sizeof(flockingEnabled), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&flockingSnapInterval, sizeof(flockingSnapInterval), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&flockingSimulationInterval, sizeof(flockingSimulationInterval), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&twoWayShareCapacity, sizeof(twoWayShareCapacity), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&initDelayCostPerPop, sizeof(initDelayCostPerPop), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&flockingProfile, sizeof(flockingProfile), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&CARMAPerformanceRatio, sizeof(CARMAPerformanceRatio), &numBytes))) return hr;
	
	CARMAPerformanceRatio = min(max(CARMAPerformanceRatio, 0.0f), 1.0f);
	m_bPersistDirty = false;

	return S_OK;
}

STDMETHODIMP EvcSolver::Save(IStream* pStm, BOOL fClearDirty)
{
	if (fClearDirty) m_bPersistDirty = false;

	ULONG     numBytes;
	HRESULT   hr;

	// We need to persist the c_version number
	if (FAILED(hr = pStm->Write(&c_version, sizeof(c_version), &numBytes))) return hr;

	// We need to persist our solver settings
	if (FAILED(hr = pStm->Write(&m_outputLineType, sizeof(m_outputLineType), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&costAttributeID, sizeof(costAttributeID), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&capAttributeID, sizeof(capAttributeID), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&trafficModel, sizeof(trafficModel), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&solverMethod, sizeof(solverMethod), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&SaturationPerCap, sizeof(SaturationPerCap), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&CriticalDensPerCap, sizeof(CriticalDensPerCap), &numBytes))) return hr;	
	if (FAILED(hr = pStm->Write(&m_CreateTraversalResult, sizeof(m_CreateTraversalResult), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&m_FindBestSequence, sizeof(m_FindBestSequence), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&m_PreserveFirstStop, sizeof(m_PreserveFirstStop), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&m_PreserveLastStop, sizeof(m_PreserveLastStop), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&m_UseTimeWindows, sizeof(m_UseTimeWindows), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&separable, sizeof(separable), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&exportEdgeStat, sizeof(exportEdgeStat), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&backtrack, sizeof(backtrack), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&costPerDensity, sizeof(costPerDensity), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&flockingEnabled, sizeof(flockingEnabled), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&flockingSnapInterval, sizeof(flockingSnapInterval), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&flockingSimulationInterval, sizeof(flockingSimulationInterval), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&twoWayShareCapacity, sizeof(twoWayShareCapacity), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&initDelayCostPerPop, sizeof(initDelayCostPerPop), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&flockingProfile, sizeof(flockingProfile), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&CARMAPerformanceRatio, sizeof(CARMAPerformanceRatio), &numBytes))) return hr;
	
	return S_OK;
}

STDMETHODIMP EvcSolver::GetSizeMax(_ULARGE_INTEGER* pCbSize)
{
	if (!pCbSize) return E_POINTER;

	pCbSize->HighPart = 0;
	pCbSize->LowPart = sizeof(short);

	return S_OK;
}

STDMETHODIMP EvcSolver::GetClassID(CLSID *pClassID)
{
	if (!pClassID) return E_POINTER;
	*pClassID = __uuidof(EvcSolver); 

	return S_OK;
}

//////////////////////////////////////////
// private methods

HRESULT EvcSolver::BuildClassDefinitions(ISpatialReference* pSpatialRef, INamedSet** ppDefinitions, IDENetworkDataset* pDENDS)
{
	if (!pSpatialRef || !ppDefinitions) return E_POINTER;

	// This function creates the class definitions for the EvcSolver.
	// Recall, class definitions are in-memory feature classes that store the
	// inputs and outputs for a solver.  This solver's class definitions are:

	// Zones (input)                  Safe zones determine the end locations
	// - OID                          of the search
	// - Shape
	// - Name 
	// - (NALocation fields)

	// Evacuee Points (input)         Evacuee points determine the start locations
	// - OID                          of the search
	// - Shape
	// - Name 
	// - Population					  number of unseperatable people/cars at this location
	// - (NALocation fields)

	// Barriers (input)               Barriers restrict network edges from being
	// - OID                          traversable
	// - Shape
	// - Name
	// - (NALocation fields)

	// Routes (output)                Evacuation routes
	// - OID                          
	// - Shape                        evacuation route (polyline)
	// - EvcOID						  Evacuee user ID
	// - EvcTime					  Evacuation cost on this route
	// - OrgTime					  Evacuation cost assuming unlimited capacity
	// - RoutedPop					  The population who would use this route

	// Edges (output)                 Edge/street lines with their population reservations
	// - OID                          
	// - Shape                        edge/street shape (polyline)
	// - EdgeID						  Edge ID from NetworkDataset. there could be at most two edges (different directions) with one edgeID.
	// - Direction					  Direction of travel on this edge
	// - SourceID					  Referes to source street file ID
	// - SourceOID					  Referes to oid of the shape in the street file
	// - ReservPop					  The total population who whould use this edge at some time
	// - TravCost					  Traversal cost based on the selected evacuation method on this edge
	// - OrgCost					  Original traversal cost of the edge without any population

	HRESULT hr;

	// Create the class definitions named set and the variables needed to properly instantiate them
	INamedSetPtr              ipClassDefinitions(CLSID_NamedSet);
	INAClassDefinitionPtr     ipClassDef;
	INAClassDefinitionEditPtr ipClassDefEdit;
	IUIDPtr                   ipIUID;
	IFieldsPtr                ipFields;
	IFieldsEditPtr            ipFieldsEdit;
	IFieldPtr                 ipField;
	IFieldEditPtr             ipFieldEdit;

	//////////////////////////////////////////////////////////
	// Zones class definition

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriNetworkAnalyst.NALocationFeature")))) return hr;
	ipClassDefEdit->putref_ClassCLSID(ipIUID);

	// Create the fields for the class definition
	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	// Create and add an OID field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	// Create and add a Shape field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPoint);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	// Create and add a Name field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_NAME));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	// Create and add a capacity field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_CAP));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldEdit->put_Length(128);
	ipFieldsEdit->AddField(ipFieldEdit);

	// Add the NALocation fields
	AddLocationFields(ipFieldsEdit, pDENDS);

	// Add the new fields to the class definition (these must be set before setting field types)
	ipClassDefEdit->putref_Fields(ipFields);

	// Setup the field types (i.e., input/output fields, editable/non-editable fields, visible/non-visible fields, or a combination of these)
	AddLocationFieldTypes(ipClassDefEdit);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeInput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeInput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_NAME), esriNAFieldTypeInput);

	// Setup whether the NAClass is considered as input/output
	ipClassDefEdit->put_IsInput(VARIANT_TRUE);
	ipClassDefEdit->put_IsOutput(VARIANT_FALSE);

	// Setup necessary cardinality for allowing a Solve to be run 
	// NOTE: the LowerBound property is used to define the minimum number of required NALocationObjects that are required by the solver
	// to perform analysis. The UpperBound property is used to define the maximum number of NALocationObjects that are allowed by the solver
	// to perform analysis.
	// In our case, we must have at least one zone point stored in the class before allowing Solve to enabled in ArcMap, and we have no UpperBound
	ipClassDefEdit->put_LowerBound(1);

	// Give the class definition a name...
	ipClassDefEdit->put_Name(CComBSTR(CS_ZONES_NAME));

	// ...and add it to the named set
	ipClassDefinitions->Add(CComBSTR(CS_ZONES_NAME), (IUnknownPtr)ipClassDef);

	//////////////////////////////////////////////////////////
	// Evacuee Points class definition

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriNetworkAnalyst.NALocationFeature")))) return hr;
	ipClassDefEdit->putref_ClassCLSID(ipIUID);

	// Create the fields for the class definition
	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	// Create and add an OID field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	// Create and add an population field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_EVC_POP));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldEdit->put_DefaultValue(CComVariant(1.0));
	ipFieldEdit->put_IsNullable(VARIANT_FALSE);
	ipFieldsEdit->AddField(ipFieldEdit);

	// Create and add a Shape field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPoint);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	// Create and add a Name field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_NAME));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldEdit->put_Length(128);
	ipFieldsEdit->AddField(ipFieldEdit);

	// Add the NALocation fields
	AddLocationFields(ipFieldsEdit, pDENDS);

	// Add the new fields to the class definition (these must be set before setting field types)
	ipClassDefEdit->putref_Fields(ipFields);

	// Setup the field types (i.e., input/output fields, editable/non-editable fields, visible/non-visible fields, or a combination of these)
	AddLocationFieldTypes(ipClassDefEdit);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeInput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeInput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_NAME), esriNAFieldTypeInput);

	// Setup whether the NAClass is considered as input/output
	ipClassDefEdit->put_IsInput(VARIANT_TRUE);
	ipClassDefEdit->put_IsOutput(VARIANT_FALSE);

	// Setup necessary cardinality for allowing a Solve to be run 
	// NOTE: the LowerBound property is used to define the minimum number of required NALocationObjects that are required by the solver
	// to perform analysis. The UpperBound property is used to define the maximum number of NALocationObjects that are allowed by the solver
	// to perform analysis.
	// In our case, we must have at least one Evacuee point stored in the class before allowing Solve to enabled in ArcMap, and we have no UpperBound
	ipClassDefEdit->put_LowerBound(1);

	// Give the class definition a name...
	ipClassDefEdit->put_Name(CComBSTR(CS_EVACUEES_NAME));

	// ...and add it to the named set
	ipClassDefinitions->Add(CComBSTR(CS_EVACUEES_NAME), (IUnknownPtr)ipClassDef);

	//////////////////////////////////////////////////////////
	// Barriers class definition

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriNetworkAnalyst.NALocationFeature")))) return hr;
	ipClassDefEdit->putref_ClassCLSID(ipIUID);

	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPoint);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_NAME));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldEdit->put_Length(128);
	ipFieldsEdit->AddField(ipFieldEdit);

	AddLocationFields(ipFieldsEdit, pDENDS);

	ipClassDefEdit->putref_Fields(ipFields);  

	AddLocationFieldTypes(ipClassDefEdit);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeInput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeInput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_NAME), esriNAFieldTypeInput);

	ipClassDefEdit->put_IsInput(VARIANT_TRUE);
	ipClassDefEdit->put_IsOutput(VARIANT_FALSE);

	ipClassDefEdit->put_Name(CComBSTR(CS_BARRIERS_NAME));
	ipClassDefinitions->Add(CComBSTR(CS_BARRIERS_NAME), (IUnknownPtr)ipClassDef);

	//////////////////////////////////////////////////////////
	// Flocks class definition 

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriGeoDatabase.Feature")))) return hr;
	ipClassDefEdit->putref_ClassCLSID(ipIUID);

	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPoint);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_NAME));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_ID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_COST));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_VelocityX));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_VelocityY));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SPEED));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_TRAVELED));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_TIME));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_PTIME));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_STATUS));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldEdit->put_Length(1);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipClassDefEdit->putref_Fields(ipFields);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_ID), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_NAME), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_COST), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_VelocityX), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_VelocityY), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SPEED), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_TRAVELED), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_TIME), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_PTIME), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_STATUS), esriNAFieldTypeOutput);

	ipClassDefEdit->put_IsInput(VARIANT_FALSE);
	ipClassDefEdit->put_IsOutput(VARIANT_TRUE);

	ipClassDefEdit->put_Name(CComBSTR(CS_FLOCKS_NAME));
#if defined(_FLOCK)
	ipClassDefinitions->Add(CComBSTR(CS_FLOCKS_NAME), (IUnknownPtr)ipClassDef);
#endif

	//////////////////////////////////////////////////////////
	// Routes class definition 

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriGeoDatabase.Feature")))) return hr;
	ipClassDefEdit->putref_ClassCLSID(ipIUID);

	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPolyline);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_EVC_NAME));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_E_TIME));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_E_ORG));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_E_POP));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipClassDefEdit->putref_Fields(ipFields);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_EVC_NAME), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_E_TIME), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_E_POP), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_E_ORG), esriNAFieldTypeOutput);

	ipClassDefEdit->put_IsInput(VARIANT_FALSE);
	ipClassDefEdit->put_IsOutput(VARIANT_TRUE);

	ipClassDefEdit->put_Name(CComBSTR(CS_ROUTES_NAME));
	ipClassDefinitions->Add(CComBSTR(CS_ROUTES_NAME), (IUnknownPtr)ipClassDef);

	//////////////////////////////////////////////////////////
	// EdgeStat class definition 

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriGeoDatabase.Feature")))) return hr;
	ipClassDefEdit->putref_ClassCLSID(ipIUID);

	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPolyline);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_EID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_DIR));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SOURCE_ID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SOURCE_OID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_ReservPop));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_TravCost));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OrgCost));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_Congestion));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipClassDefEdit->putref_Fields(ipFields);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SOURCE_ID), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SOURCE_OID), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_ReservPop), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_TravCost), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OrgCost), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_Congestion), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_DIR), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_EID), esriNAFieldTypeOutput);

	ipClassDefEdit->put_IsInput(VARIANT_FALSE);
	ipClassDefEdit->put_IsOutput(VARIANT_TRUE);

	ipClassDefEdit->put_Name(CComBSTR(CS_EDGES_NAME));
	ipClassDefinitions->Add(CComBSTR(CS_EDGES_NAME), (IUnknownPtr)ipClassDef);

	// Return the class definitions once we have finished
	ipClassDefinitions->AddRef();
	*ppDefinitions = ipClassDefinitions;

	return S_OK;
}

HRESULT EvcSolver::GetNAClassTable(INAContext* pContext, BSTR className, ITable** ppTable)
{
	if (!pContext || !ppTable) return E_POINTER;

	HRESULT hr;
	INamedSetPtr ipNamedSet;

	if (FAILED(hr = pContext->get_NAClasses(&ipNamedSet))) return hr;

	IUnknownPtr ipUnk;
	if (FAILED(hr = ipNamedSet->get_ItemByName(className, &ipUnk))) return hr;

	ITablePtr ipTable(ipUnk);

	if (!ipTable) return AtlReportError(GetObjectCLSID(), _T("Context has an invalid NAClass."), IID_INASolver);

	ipTable->AddRef();
	*ppTable = ipTable;

	return S_OK;
}

HRESULT EvcSolver::LoadBarriers(ITable* pTable, INetworkQuery* pNetworkQuery, INetworkForwardStarEx* pNetworkForwardStarEx)
{
	if (!pTable || !pNetworkForwardStarEx) return E_POINTER;

	HRESULT hr;

	// Initialize all network elements as traversable
	if (FAILED(hr = pNetworkForwardStarEx->RemoveElementRestrictions())) return hr;

	// Get a cursor on the table to loop through each row
	ICursorPtr ipCursor;
	if (FAILED(hr = pTable->Search(0, VARIANT_TRUE, &ipCursor))) return hr;

	// Create variables for looping through the cursor and setting up barriers
	IRowESRI * ipRow;
	INALocationObjectPtr ipNALocationObject;
	INALocationPtr ipNALocation(CLSID_NALocation);
	IEnumNetworkElementPtr ipEnumNetworkElement;
	long sourceID = -1, sourceOID = -1;
	double posAlong, fromPosition, toPosition;

	INetworkEdgePtr ipReverseEdge;  // we need one edge element for querying the opposing direction
	INetworkElementPtr ipElement;
	pNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement);
	ipReverseEdge = ipElement;
	esriNetworkElementType elementType;

	// Loop through the cursor getting the NALocation of each NALocationObject,  
	// then query the network elements associated with each NALocation,
	// and set their traversability to false
	while (ipCursor->NextRow(&ipRow) == S_OK)
	{
		ipNALocationObject = ipRow;
		if (!ipNALocationObject) continue;// we only want valid NALocationObjects			
		if (FAILED(hr = ipNALocationObject->QueryNALocation(ipNALocation))) return hr;

		// Once we have the NALocation, we need to check if it is actually located within the network dataset
		VARIANT_BOOL isLocated = VARIANT_FALSE;
		if (ipNALocation)
		{
			if (FAILED(hr = ipNALocation->get_IsLocated(&isLocated))) return hr;
		}

		// We are only concerned with located NALocations
		if (isLocated)
		{
			// Get the SourceID for the NALocation
			if (FAILED(hr = ipNALocation->get_SourceID(&sourceID))) return hr;

			// Get the SourceOID for the NALocation
			if (FAILED(hr = ipNALocation->get_SourceOID(&sourceOID))) return hr;

			// Get the PosAlong for the NALocation
			if (FAILED(hr = ipNALocation->get_SourcePosition(&posAlong))) return hr;

			// Once we have a located NALocation, we query the network to obtain its associated network elements
			if (FAILED(hr = pNetworkQuery->get_ElementsByOID(sourceID, sourceOID, &ipEnumNetworkElement))) return hr;

			// Set the traversability for all appropriate network elements to false
			ipEnumNetworkElement->Reset();
			while (ipEnumNetworkElement->Next(&ipElement) == S_OK)
			{
				if (FAILED(hr = ipElement->get_ElementType(&elementType))) return hr;

				switch (elementType)
				{
				case esriNETJunction:
					{
						INetworkJunctionPtr ipJunction(ipElement);
						// If it is a junction, we can just set the element to non-traversable
						if (FAILED(hr = pNetworkForwardStarEx->AddJunctionRestriction(ipJunction))) return hr;

						break;
					}
				case esriNETEdge:
					{
						// If it is an edge, we must make sure it is an appropriate edge to set as non-traversable (by checking its fromPosition and toPosition)
						INetworkEdgePtr ipEdge(ipElement);
						if (FAILED(hr = ipEdge->QueryPositions(&fromPosition, &toPosition))) return hr;

						if (fromPosition <= posAlong && posAlong <= toPosition)
						{
							// Our NALocation lies along this edge element
							// Set it as non-traversable in both directions  

							// First, set this edge element as non-traversable in the AlongDigitized direction (this is the default direction of edge elements returned from the get_ElementsByOID method above)
							INetworkEdgePtr ipEdge(ipElement);
							if (FAILED(hr = pNetworkForwardStarEx->AddEdgeRestriction(ipEdge, 0.0, 1.0))) return hr;

							// Then, query the edge element to get its opposite direction
							if (FAILED(hr = ipEdge->QueryEdgeInOtherDirection(ipReverseEdge))) return hr;

							// Finally, set this edge as non-traversable in the AgainstDigitized direction
							if (FAILED(hr = pNetworkForwardStarEx->AddEdgeRestriction(ipReverseEdge, 0.0, 1.0))) return hr;

							break;
						}
					}
				}
			}
		}
	}

	return S_OK;
}

HRESULT EvcSolver::AddLocationFields(IFieldsEdit* pFieldsEdit, IDENetworkDataset* pDENDS)
{
	if (!pFieldsEdit) return E_POINTER;

	HRESULT                   hr;
	IFieldPtr                 ipField;
	IFieldEditPtr             ipFieldEdit;
	IRangeDomainPtr           ipRangeDomain;
	CComVariant                longDefaultValue(static_cast<long>(-1));
	CComVariant                doubleDefaultValue(static_cast<double>(-1));

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SOURCE_ID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldEdit->put_DefaultValue(longDefaultValue);
	ipFieldEdit->put_Precision(0);
	ipFieldEdit->put_Required(VARIANT_TRUE);
	ipFieldEdit->put_Length(4);

	// if we can get source names, then add them to a coded value domain
	if (pDENDS)
	{
		IArrayPtr               ipSourceArray;
		if (FAILED(hr = pDENDS->get_Sources(&ipSourceArray))) return hr;

		ICodedValueDomainPtr    ipCodedValueDomain(CLSID_CodedValueDomain);
		IDomainPtr(ipCodedValueDomain)->put_Name(CS_FIELD_SOURCE_ID);
		IDomainPtr(ipCodedValueDomain)->put_FieldType(esriFieldTypeInteger);

		IUnknownPtr             ipUnk;
		INetworkSourcePtr       ipSource;
		long                    sourceCount;
		ipSourceArray->get_Count(&sourceCount);
		for (long i = 0; i < sourceCount; i++)
		{
			ipSourceArray->get_Element(i, &ipUnk);
			ipSource = ipUnk;

			long        sourceID;
			ipSource->get_ID(&sourceID);
			CComBSTR     name;
			ipSource->get_Name(&name);
			CComVariant     value(sourceID);

			ipCodedValueDomain->AddCode(value, name);
		}
		ipFieldEdit->putref_Domain((IDomainPtr)ipCodedValueDomain);
	}

	pFieldsEdit->AddField(ipFieldEdit);

	// Add the source OID field
	ipField.CreateInstance(CLSID_Field);
	ipRangeDomain.CreateInstance(CLSID_RangeDomain);
	IDomainPtr(ipRangeDomain)->put_Name(CS_FIELD_SOURCE_OID);
	IDomainPtr(ipRangeDomain)->put_FieldType(esriFieldTypeInteger);
	ipRangeDomain->put_MinValue(CComVariant((long)-1));
	ipRangeDomain->put_MaxValue(CComVariant((long)LONG_MAX));

	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SOURCE_OID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldEdit->put_DefaultValue(longDefaultValue);
	ipFieldEdit->put_Precision(0);
	ipFieldEdit->put_Length(4);
	ipFieldEdit->put_Required(VARIANT_TRUE);
	ipFieldEdit->putref_Domain((IDomainPtr)ipRangeDomain);
	pFieldsEdit->AddField(ipFieldEdit);

	// Add the position field
	ipField.CreateInstance(CLSID_Field);
	ipRangeDomain.CreateInstance(CLSID_RangeDomain);
	IDomainPtr(ipRangeDomain)->put_Name(CS_FIELD_POSITION);
	IDomainPtr(ipRangeDomain)->put_FieldType(esriFieldTypeDouble);
	ipRangeDomain->put_MinValue(CComVariant((double)0.0));
	ipRangeDomain->put_MaxValue(CComVariant((double)1.0));
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_POSITION));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldEdit->put_DefaultValue(doubleDefaultValue);
	ipFieldEdit->put_Precision(0);
	ipFieldEdit->put_Length(8);
	ipFieldEdit->put_Required(VARIANT_TRUE);
	ipFieldEdit->putref_Domain((IDomainPtr)ipRangeDomain);
	pFieldsEdit->AddField(ipFieldEdit);

	// Add the side of Edge domain
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SIDE_OF_EDGE));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldEdit->put_DefaultValue(longDefaultValue);
	ipFieldEdit->put_Precision(0);
	ipFieldEdit->put_Length(4);
	ipFieldEdit->put_Required(VARIANT_TRUE);

	IDomainPtr    ipSideDomain;
	CreateSideOfEdgeDomain(&ipSideDomain);
	ipFieldEdit->putref_Domain(ipSideDomain);
	pFieldsEdit->AddField(ipFieldEdit);

	// Add the curb approach domain
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_CURBAPPROACH));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldEdit->put_DefaultValue(CComVariant((long)esriNAEitherSideOfVehicle));
	ipFieldEdit->put_Precision(0);
	ipFieldEdit->put_Length(4);
	ipFieldEdit->put_Required(VARIANT_TRUE);

	IDomainPtr    ipCurbDomain;
	CreateCurbApproachDomain(&ipCurbDomain);
	ipFieldEdit->putref_Domain(ipCurbDomain);
	pFieldsEdit->AddField(ipFieldEdit);

	// Add the status field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_STATUS));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldEdit->put_DefaultValue(CComVariant((long)0));
	ipFieldEdit->put_Precision(0);
	ipFieldEdit->put_Length(4);
	ipFieldEdit->put_Required(VARIANT_TRUE);

	// set up coded value domains for the the status values
	ICodedValueDomainPtr    ipCodedValueDomain(CLSID_CodedValueDomain);
	CreateStatusCodedValueDomain(ipCodedValueDomain);
	ipFieldEdit->putref_Domain((IDomainPtr)ipCodedValueDomain);
	pFieldsEdit->AddField(ipFieldEdit);

	return S_OK;
}

HRESULT EvcSolver::CreateSideOfEdgeDomain(IDomain** ppDomain)
{
	if (!ppDomain) return E_POINTER;

	HRESULT hr;
	ICodedValueDomainPtr ipCodedValueDomain(CLSID_CodedValueDomain);

	// Domains need names in order to serialize them
	IDomainPtr(ipCodedValueDomain)->put_Name(CComBSTR(CS_SIDEOFEDGE_DOMAINNAME));
	IDomainPtr(ipCodedValueDomain)->put_FieldType(esriFieldTypeInteger);

	CComVariant value(static_cast<long>(esriNAEdgeSideLeft));
	if (FAILED(hr = ipCodedValueDomain->AddCode(value, CComBSTR(L"Left Side")))) return hr;

	value.lVal = esriNAEdgeSideRight;
	if (FAILED(hr = ipCodedValueDomain->AddCode(value, CComBSTR(L"Right Side")))) return hr;

	*ppDomain = (IDomainPtr)ipCodedValueDomain;
	ipCodedValueDomain->AddRef();
	return S_OK;
}

HRESULT EvcSolver::CreateCurbApproachDomain(IDomain** ppDomain)
{
	if (!ppDomain) return E_POINTER;

	HRESULT       hr;
	ICodedValueDomainPtr    ipCodedValueDomain(CLSID_CodedValueDomain);

	IDomainPtr(ipCodedValueDomain)->put_Name(CComBSTR(CS_CURBAPPROACH_DOMAINNAME));
	IDomainPtr(ipCodedValueDomain)->put_FieldType(esriFieldTypeInteger);

	CComVariant value(static_cast<long>(esriNAEitherSideOfVehicle));
	if (FAILED(hr = ipCodedValueDomain->AddCode(value, CComBSTR(L"Either side of vehicle")))) return hr;

	value.lVal = esriNARightSideOfVehicle;
	if (FAILED(hr = ipCodedValueDomain->AddCode(value, CComBSTR(L"Right side of vehicle")))) return hr;

	value.lVal = esriNALeftSideOfVehicle;
	if (FAILED(hr = ipCodedValueDomain->AddCode(value, CComBSTR(L"Left side of vehicle")))) return hr;

	*ppDomain = (IDomainPtr)ipCodedValueDomain;
	ipCodedValueDomain->AddRef();
	return S_OK;
}

HRESULT EvcSolver::CreateStatusCodedValueDomain(ICodedValueDomain* pCodedValueDomain)
{
	if (!pCodedValueDomain) return E_POINTER;

	IDomainPtr(pCodedValueDomain)->put_Name(CComBSTR(CS_LOCATIONSTATUS_DOMAINNAME));
	IDomainPtr(pCodedValueDomain)->put_FieldType(esriFieldTypeInteger);

	CComVariant value(static_cast<long>(esriNAObjectStatusOK)); // this code is exclusive with the other codes
	pCodedValueDomain->AddCode(value, CComBSTR(L"OK"));

	value.lVal = esriNAObjectStatusNotLocated;
	pCodedValueDomain->AddCode(value, CComBSTR(L"Not located"));

	value.lVal = esriNAObjectStatusElementNotLocated;
	pCodedValueDomain->AddCode(value, CComBSTR(L"Network element not located"));

	value.lVal = esriNAObjectStatusElementNotTraversable;
	pCodedValueDomain->AddCode(value, CComBSTR(L"Element not traversible"));

	value.lVal = esriNAObjectStatusInvalidFieldValues;
	pCodedValueDomain->AddCode(value, CComBSTR(L"Invalid field values"));

	value.lVal = esriNAObjectStatusNotReached;
	pCodedValueDomain->AddCode(value, CComBSTR(L"Not reached"));

	value.lVal = esriNAObjectStatusTimeWindowViolation;
	pCodedValueDomain->AddCode(value, CComBSTR(L"Time window violation"));

	return S_OK;
}

HRESULT EvcSolver::AddLocationFieldTypes(INAClassDefinitionEdit* pClassDef)
{
	if (!pClassDef) return E_INVALIDARG;

	pClassDef->put_FieldType(CComBSTR(CS_FIELD_SOURCE_ID), esriNAFieldTypeInput);
	pClassDef->put_FieldType(CComBSTR(CS_FIELD_SOURCE_OID), esriNAFieldTypeInput);
	pClassDef->put_FieldType(CComBSTR(CS_FIELD_POSITION), esriNAFieldTypeInput);
	pClassDef->put_FieldType(CComBSTR(CS_FIELD_SIDE_OF_EDGE), esriNAFieldTypeInput);
	pClassDef->put_FieldType(CComBSTR(CS_FIELD_CURBAPPROACH), esriNAFieldTypeInput);
	pClassDef->put_FieldType(CComBSTR(CS_FIELD_STATUS), esriNAFieldTypeInput | esriNAFieldTypeNotEditable);

	return S_OK;
}
