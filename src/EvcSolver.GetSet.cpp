#include "stdafx.h"
#include "NameConstants.h"
#include "EvcSolver.h"

/////////////////////////////////////////////////////////////////////
// INARouteSolver2

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
		return ATL::AtlReportError(GetObjectCLSID(), _T("Failed to create class definitions."), IID_INASolver, hr);

	return S_OK;
}

STDMETHODIMP EvcSolver::get_StartTime(DATE * Value)
{
	if (!Value) return E_POINTER;
	*Value = 0.0;
	return S_OK;
}

#pragma warning(push)
#pragma warning(disable : 4100) /* Ignore warnings for unreferenced function parameters */

STDMETHODIMP EvcSolver::put_StartTime(DATE Value)
{
	return S_OK;
}

STDMETHODIMP EvcSolver::put_UseStartTime(VARIANT_BOOL Value)
{
	return S_OK;
}

#pragma warning(pop)

STDMETHODIMP EvcSolver::get_UseStartTime(VARIANT_BOOL * Value)
{
	if (!Value) return E_POINTER;
	*Value = VARIANT_FALSE;
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

STDMETHODIMP EvcSolver::put_ThreeGenCARMA(VARIANT_BOOL value)
{
	ThreeGenCARMA = value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_ThreeGenCARMA(VARIANT_BOOL * value)
{
	*value = ThreeGenCARMA;
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
	*value = VarExportEdgeStat;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_ExportEdgeStat(VARIANT_BOOL value)
{
	VarExportEdgeStat = value;
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

STDMETHODIMP EvcSolver::get_CARMASortSetting(CARMASort * value)
{
	*value = CarmaSortCriteria;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_CARMASortSetting(CARMASort value)
{
	CarmaSortCriteria = value;
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_TrafficModel(EvcTrafficModel * value)
{
	*value = trafficModel;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_TrafficModel(EvcTrafficModel value)
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

STDMETHODIMP EvcSolver::get_IterativeRatio(BSTR * value)
{
	if (value)
	{
		*value = new DEBUG_NEW_PLACEMENT WCHAR[100];
		swprintf_s(*value, 100, L"%.3f", iterativeRatio);
	}
	return S_OK;
}

STDMETHODIMP EvcSolver::put_IterativeRatio(BSTR value)
{
	swscanf_s(value, L"%f", &iterativeRatio);
	iterativeRatio = min(max(iterativeRatio, 0.0f), 1.0f);
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_SelfishRatio(BSTR * value)
{
	if (value)
	{
		*value = new DEBUG_NEW_PLACEMENT WCHAR[100];
		swprintf_s(*value, 100, L"%.3f", selfishRatio);
	}
	return S_OK;
}

STDMETHODIMP EvcSolver::put_SelfishRatio(BSTR value)
{
	swscanf_s(value, L"%f", &selfishRatio);
	selfishRatio = min(max(selfishRatio, 0.0f), 1.0f);
	m_bPersistDirty = true;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_SolverMethod(EvcSolverMethod * value)
{
	*value = solverMethod;
	return S_OK;
}

STDMETHODIMP EvcSolver::put_SolverMethod(EvcSolverMethod value)
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
STDMETHODIMP EvcSolver::get_CostAttributes(unsigned __int3264 & count, BSTR ** Names)
{
	count = costAttribs.size();
	HRESULT hr;
	BSTR * names = new DEBUG_NEW_PLACEMENT BSTR[count];
	*Names = names;

	for (size_t i = 0; i < count; i++) if (FAILED(hr = costAttribs[i]->get_Name(&(names[i])))) return hr;

	return S_OK;
}

// returns the name of the heuristic attributes loaded from the network dataset.
// this will be called from the property page so the user can select.
STDMETHODIMP EvcSolver::get_DiscriptiveAttributes(unsigned __int3264 & count, BSTR ** Names)
{
	count = discriptiveAttribs.size();
	HRESULT hr;
	BSTR * names = new DEBUG_NEW_PLACEMENT BSTR[count];
	*Names = names;

	for (size_t i = 0; i < count; i++) if (FAILED(hr = discriptiveAttribs[i]->get_Name(&(names[i])))) return hr;

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

STDMETHODIMP EvcSolver::get_UseHierarchy(VARIANT_BOOL* pUseHierarchy)
{
	if (!pUseHierarchy) return E_POINTER;

	// This solver does not support hierarchy attributes

	*pUseHierarchy = VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP EvcSolver::get_IgnoreInvalidLocations(VARIANT_BOOL * pIgnoreInvalidLocations)
{
	*pIgnoreInvalidLocations = VARIANT_TRUE;
	return S_OK;
}

STDMETHODIMP EvcSolver::get_HierarchyLevelCount(long * pCount)
{
	*pCount = 0;
	return S_OK; // This solver does not support hierarchy attributes
}

#pragma warning(push)
#pragma warning(disable : 4100) /* Ignore warnings for unreferenced function parameters */

STDMETHODIMP EvcSolver::put_ResetHierarchyRangesOnBind(VARIANT_BOOL value)
{
	return E_NOTIMPL; // This solver does not support hierarchy
}

STDMETHODIMP EvcSolver::get_ResetHierarchyRangesOnBind(VARIANT_BOOL * value)
{
	return E_NOTIMPL; // This solver does not support hierarchy
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

STDMETHODIMP EvcSolver::put_UseHierarchy(VARIANT_BOOL useHierarchy)
{
	return S_OK; // This solver does not support hierarchy attributes
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

#pragma warning(pop)