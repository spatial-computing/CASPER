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

// ===============================================================================================
// Evacuation Solver: Main class definition
// Description: The definition of IEvcSolver is also here
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#pragma once

#include "CatIDs\ArcCATIDs.h"     // component category IDs
#include "Evacuee.h"
#include "NAEdge.h"
#include "NAVertex.h"
#include "Flocking.h"
#include "FibonacciHeap.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

// IEvcSolver
[
	object,
	uuid("59da3a5d-109d-41f3-afdc-711b5262c769"),
	oleautomation, helpstring("IEvcSolver Interface"),
	pointer_default(unique)
]
__interface IEvcSolver : IUnknown
{
	[propput, helpstring("Sets the saturation constant per capacity")]
		HRESULT SaturationPerCap([in] BSTR value);
	[propget, helpstring("Gets the selected saturation constant per capacity")]
		HRESULT SaturationPerCap([out, retval] BSTR * value);
	[propput, helpstring("Sets the critical density per capacity")]
		HRESULT CriticalDensPerCap([in] BSTR value);
	[propget, helpstring("Gets the selected critical density per capacity")]
		HRESULT CriticalDensPerCap([out, retval] BSTR * value);
	[propget, helpstring("Lists descriptive attributes from the network dataset")]
		HRESULT DiscriptiveAttributes([out] unsigned __int3264 & count, [out, retval] BSTR ** names);
	[propput, helpstring("Sets the selected capacity attribute index")]
		HRESULT CapacityAttribute([in] unsigned __int3264 index);
	[propget, helpstring("Gets the selected capacity attribute index")]
		HRESULT CapacityAttribute([out, retval] unsigned __int3264 * index);
	[propput, helpstring("Sets the Evacuee Grouping Option flag")]
		HRESULT EvacueeGroupingOption([in] EvacueeGrouping value);
	[propget, helpstring("Gets the Evacuee Grouping Option flag")]
		HRESULT EvacueeGroupingOption([out, retval] EvacueeGrouping * value);
	[propput, helpstring("Sets the 3rd generation carma flag")]
		HRESULT ThreeGenCARMA([in] VARIANT_BOOL value);
	[propget, helpstring("Gets the 3rd generation carma flag")]
		HRESULT ThreeGenCARMA([out, retval] VARIANT_BOOL * value);
	[propput, helpstring("Sets the export edge stat flag")]
		HRESULT ExportEdgeStat([in] VARIANT_BOOL value);
	[propget, helpstring("Gets the export edge stat flag")]
		HRESULT ExportEdgeStat([out, retval] VARIANT_BOOL * value);
	[propput, helpstring("Sets the solver method")]
		HRESULT SolverMethod([in] EvcSolverMethod value);
	[propget, helpstring("Gets the selected solver method")]
		HRESULT SolverMethod([out, retval] EvcSolverMethod * value);
	[propput, helpstring("Sets the carma sort setting")]
		HRESULT CARMASortSetting([in] CARMASort value);
	[propget, helpstring("Gets the carma sort setting")]
		HRESULT CARMASortSetting([out, retval] CARMASort * value);
	[propput, helpstring("Sets the cost method")]
		HRESULT TrafficModel([in] EvcTrafficModel value);
	[propget, helpstring("Gets the selected cost method")]
		HRESULT TrafficModel([out, retval] EvcTrafficModel * value);
	[propput, helpstring("Sets the cost per safe zone density")]
		HRESULT CostPerZoneDensity([in] BSTR value);
	[propget, helpstring("Gets the cost per safe zone density")]
		HRESULT CostPerZoneDensity([out, retval] BSTR * value);
	[propput, helpstring("Sets the flocking simulation interval in minutes")]
		HRESULT FlockingSimulationInterval([in] BSTR value);
	[propget, helpstring("Gets the flocking simulation interval in minutes")]
		HRESULT FlockingSimulationInterval([out, retval] BSTR * value);
	[propput, helpstring("Sets the flocking snapshot interval in minutes")]
		HRESULT FlockingSnapInterval([in] BSTR value);
	[propget, helpstring("Gets the flocking snapshot interval in minutes")]
		HRESULT FlockingSnapInterval([out, retval] BSTR * value);
	[propput, helpstring("Sets the flocking mode to on/off")]
		HRESULT FlockingEnabled([in] VARIANT_BOOL value);
	[propget, helpstring("Gets the flocking mode")]
		HRESULT FlockingEnabled([out, retval] VARIANT_BOOL * value);
	[propput, helpstring("Sets the two-way road capacity sharing")]
		HRESULT TwoWayShareCapacity([in] VARIANT_BOOL value);
	[propget, helpstring("Gets the two way road capacity sharing")]
		HRESULT TwoWayShareCapacity([out, retval] VARIANT_BOOL * value);
	[propput, helpstring("Sets the initial delay cost per population")]
		HRESULT InitDelayCostPerPop([in] BSTR value);
	[propget, helpstring("Gets the initial delay cost per population")]
		HRESULT InitDelayCostPerPop([out, retval] BSTR * value);
	[propput, helpstring("Sets the flocking profile")]
		HRESULT FlockingProfile([in] FLOCK_PROFILE value);
	[propget, helpstring("Gets the selected flocking profile")]
		HRESULT FlockingProfile([out, retval] FLOCK_PROFILE * value);
	[propput, helpstring("Sets the ratio of CARMA algorithm")]
		HRESULT CARMAPerformanceRatio([in] BSTR value);
	[propget, helpstring("Gets the ratio of CARMA algorithm")]
		HRESULT CARMAPerformanceRatio([out, retval] BSTR * value);
	[propput, helpstring("Sets the ratio of selfish routing")]
		HRESULT SelfishRatio([in] BSTR value);
	[propget, helpstring("Gets the ratio of selfish routing")]
		HRESULT SelfishRatio([out, retval] BSTR * value);
	[propput, helpstring("Sets the ratio of iterative solver")]
		HRESULT IterativeRatio([in] BSTR value);
	[propget, helpstring("Gets the ratio of iterative solver")]
		HRESULT IterativeRatio([out, retval] BSTR * value);

	/// replacement for ISolverSetting2 functionality until I found that bug
	[propput, helpstring("Sets the selected cost attribute index")]
		HRESULT CostAttribute([in] unsigned __int3264 index);
	[propget, helpstring("Gets the selected cost attribute index")]
		HRESULT CostAttribute([out, retval] unsigned __int3264 * index);
	[propget, helpstring("Lists impedance attributes from the network dataset")]
		HRESULT CostAttributes([out] unsigned __int3264 & count, [out, retval] BSTR ** names);
};

// EvcSolver
[
	coclass,
	default(IEvcSolver),
	threading(apartment),
	vi_progid("CustomSolver.EvcSolver"),
	progid("CustomSolver.EvcSolver.1"),
	version(1.0),
	uuid("7b081f99-b691-46b1-b756-aa72868c8683"),
	helpstring("EvcSolver Class")
]
class ATL_NO_VTABLE EvcSolver :
	public IEvcSolver,
	public INASolver,
	public INASolverSettings,
	public IPersistStream,
	public INASolverOutputGeneralization
{
public:
	EvcSolver() :
		  m_outputLineType(esriNAOutputLineTrueShape),
		  m_bPersistDirty(false),
		  c_version(7),
		  c_featureRetrievalInterval(500)
	  {
	  }

	  DECLARE_PROTECT_FINAL_CONSTRUCT()

	  // Register the solver in the Network Analyst solvers component category so that it can be dynamically discovered as an available solver.
	  // For example, this will allow the creation of a new Evacuation Solver analysis layer from the menu dropdown on the Network Analyst toolbar.
	  BEGIN_CATEGORY_MAP(EvcSolver)
		  IMPLEMENTED_CATEGORY(__uuidof(CATID_NetworkAnalystSolver))
	  END_CATEGORY_MAP()

	  HRESULT FinalConstruct()
	  {
		  return S_OK;
	  }

	  void FinalRelease()
	  {
	  }

	// INASolverOutputGeneralization
	STDMETHOD(put_OutputGeometryPrecision)(VARIANT value);
	STDMETHOD(get_OutputGeometryPrecision)(VARIANT * value);
	STDMETHOD(put_OutputGeometryPrecisionUnits)(esriUnits value);
	STDMETHOD(get_OutputGeometryPrecisionUnits)(esriUnits * value);

	// IEvcSolver
	STDMETHOD(put_ThreeGenCARMA)(VARIANT_BOOL threeGenCARMA);
	STDMETHOD(get_ThreeGenCARMA)(VARIANT_BOOL* threeGenCARMA);
	STDMETHOD(put_ExportEdgeStat)(VARIANT_BOOL   value);
	STDMETHOD(get_ExportEdgeStat)(VARIANT_BOOL * value);
	STDMETHOD(put_EvacueeGroupingOption)(EvacueeGrouping   value);
	STDMETHOD(get_EvacueeGroupingOption)(EvacueeGrouping * value);
	STDMETHOD(put_SolverMethod)(EvcSolverMethod   value);
	STDMETHOD(get_SolverMethod)(EvcSolverMethod * value);
	STDMETHOD(put_CARMASortSetting)(CARMASort   value);
	STDMETHOD(get_CARMASortSetting)(CARMASort * value);
	STDMETHOD(put_TrafficModel)(EvcTrafficModel   value);
	STDMETHOD(get_TrafficModel)(EvcTrafficModel * value);
	STDMETHOD(put_SaturationPerCap)(BSTR   value);
	STDMETHOD(get_SaturationPerCap)(BSTR * value);
	STDMETHOD(put_CriticalDensPerCap)(BSTR   value);
	STDMETHOD(get_CriticalDensPerCap)(BSTR * value);
	STDMETHOD(get_DiscriptiveAttributes)(unsigned __int3264 & count, BSTR ** names);
	STDMETHOD(put_CapacityAttribute)(unsigned __int3264   index);
	STDMETHOD(get_CapacityAttribute)(unsigned __int3264 * index);
	STDMETHOD(get_CostPerZoneDensity)(BSTR * value);
	STDMETHOD(put_CostPerZoneDensity)(BSTR   value);
	STDMETHOD(get_FlockingEnabled)(VARIANT_BOOL * value);
	STDMETHOD(put_FlockingEnabled)(VARIANT_BOOL   value);
	STDMETHOD(get_TwoWayShareCapacity)(VARIANT_BOOL * value);
	STDMETHOD(put_TwoWayShareCapacity)(VARIANT_BOOL   value);
	STDMETHOD(get_FlockingSnapInterval)(BSTR * value);
	STDMETHOD(put_FlockingSnapInterval)(BSTR   value);
	STDMETHOD(get_FlockingSimulationInterval)(BSTR * value);
	STDMETHOD(put_FlockingSimulationInterval)(BSTR   value);
	STDMETHOD(get_InitDelayCostPerPop)(BSTR * value);
	STDMETHOD(put_InitDelayCostPerPop)(BSTR   value);
	STDMETHOD(put_FlockingProfile)(FLOCK_PROFILE   value);
	STDMETHOD(get_FlockingProfile)(FLOCK_PROFILE * value);
	STDMETHOD(put_CARMAPerformanceRatio)(BSTR   value);
	STDMETHOD(get_CARMAPerformanceRatio)(BSTR * value);
	STDMETHOD(put_SelfishRatio)(BSTR   value);
	STDMETHOD(get_SelfishRatio)(BSTR * value); 
	STDMETHOD(put_IterativeRatio)(BSTR   value);
	STDMETHOD(get_IterativeRatio)(BSTR * value);

	/// replacement for ISolverSetting2 functionality until I found that bug
	STDMETHOD(put_CostAttribute)(unsigned __int3264 index);
	STDMETHOD(get_CostAttribute)(unsigned __int3264 * index);
	STDMETHOD(get_CostAttributes)(unsigned __int3264 & count, BSTR ** names);

	// INARouteSolver2
	STDMETHOD(get_OutputLines)(esriNAOutputLineType* pVal);
	STDMETHOD(put_OutputLines)(esriNAOutputLineType newVal);
	STDMETHOD(get_CreateTraversalResult)(VARIANT_BOOL * Value);
	STDMETHOD(put_CreateTraversalResult)(VARIANT_BOOL Value);
	STDMETHOD(get_FindBestSequence)(VARIANT_BOOL * Value);
	STDMETHOD(put_FindBestSequence)(VARIANT_BOOL Value);
	STDMETHOD(get_PreserveFirstStop)(VARIANT_BOOL * Value);
	STDMETHOD(put_PreserveFirstStop)(VARIANT_BOOL Value);
	STDMETHOD(get_PreserveLastStop)(VARIANT_BOOL * Value);
	STDMETHOD(put_PreserveLastStop)(VARIANT_BOOL Value);
	STDMETHOD(get_UseTimeWindows)(VARIANT_BOOL * Value);
	STDMETHOD(put_UseTimeWindows)(VARIANT_BOOL Value);
	STDMETHOD(get_StartTime)(DATE * Value);
	STDMETHOD(put_StartTime)(DATE Value);
	STDMETHOD(get_UseStartTime)(VARIANT_BOOL * Value);
	STDMETHOD(put_UseStartTime)(VARIANT_BOOL Value);

	// INASolver
	STDMETHOD(get_Name)(BSTR* pName);
	STDMETHOD(get_DisplayName)(BSTR* pName);
	STDMETHOD(get_ClassDefinitions)(INamedSet** ppDefinitions);
	STDMETHOD(get_CanUseHierarchy)(VARIANT_BOOL* pCanUseHierarchy);
	STDMETHOD(get_CanAccumulateAttributes)(VARIANT_BOOL* pCanAccumulateAttrs);
	STDMETHOD(get_Properties)(IPropertySet** ppPropSet);
	STDMETHOD(CreateLayer)(INAContext* pContext, INALayer** pplayer);
	STDMETHOD(UpdateLayer)(INALayer* player, VARIANT_BOOL* pLayerUpdated);
	STDMETHOD(Solve)(INAContext* pNAContext, IGPMessages* pMessages, ITrackCancel* pTrackCancel, VARIANT_BOOL* pIsPartialSolution);
	STDMETHOD(CreateContext)(IDENetworkDataset* pNetwork, BSTR contextName, INAContext** ppNAContext);
	STDMETHOD(UpdateContext)(INAContext* pNAContext, IDENetworkDataset* pNetwork, IGPMessages* pMessages);
	STDMETHOD(Bind)(INAContext* pContext, IDENetworkDataset* pNetwork, IGPMessages* pMessages);

	// INASolverSettings2
	STDMETHOD(get_AccumulateAttributeNames)(IStringArray** ppAttributeNames);
	STDMETHOD(putref_AccumulateAttributeNames)(IStringArray* pAttributeNames);
	STDMETHOD(put_ImpedanceAttributeName)(BSTR attributeName);
	STDMETHOD(get_ImpedanceAttributeName)(BSTR* pAttributeName);
	STDMETHOD(put_IgnoreInvalidLocations)(VARIANT_BOOL ignoreInvalidLocations);
	STDMETHOD(get_IgnoreInvalidLocations)(VARIANT_BOOL* pIgnoreInvalidLocations);
	STDMETHOD(get_RestrictionAttributeNames)(IStringArray** ppAttributeName);
	STDMETHOD(putref_RestrictionAttributeNames)(IStringArray* pAttributeName);
	STDMETHOD(put_RestrictUTurns)(esriNetworkForwardStarBacktrack backtrack);
	STDMETHOD(get_RestrictUTurns)(esriNetworkForwardStarBacktrack * pBacktrack);
	STDMETHOD(put_UseHierarchy)(VARIANT_BOOL useHierarchy);
	STDMETHOD(get_UseHierarchy)(VARIANT_BOOL* pUseHierarchy);
	STDMETHOD(put_HierarchyAttributeName)(BSTR attributeName);
	STDMETHOD(get_HierarchyAttributeName)(BSTR* pAttributeName);
	STDMETHOD(put_HierarchyLevelCount)(long Count);
	STDMETHOD(get_HierarchyLevelCount)(long* pCount);
	STDMETHOD(put_MaxValueForHierarchy)(long level, long value);
	STDMETHOD(get_MaxValueForHierarchy)(long level, long* pValue);
	STDMETHOD(put_NumTransitionToHierarchy)(long toLevel, long value);
	STDMETHOD(get_NumTransitionToHierarchy)(long toLevel, long* pValue);
	STDMETHOD(put_AttributeParameterValue)(BSTR AttributeName, BSTR paramName, VARIANT value);
	STDMETHOD(get_AttributeParameterValue)(BSTR AttributeName, BSTR paramName, VARIANT * value);
	STDMETHOD(put_ResetHierarchyRangesOnBind)(VARIANT_BOOL value);
	STDMETHOD(get_ResetHierarchyRangesOnBind)(VARIANT_BOOL * value);

	// IPersistStream
	STDMETHOD(IsDirty)();
	STDMETHOD(Load)(IStream* pStm);
	STDMETHOD(Save)(IStream* pstm, BOOL fClearDirty);
	STDMETHOD(GetSizeMax)(_ULARGE_INTEGER* pCbSize);
	STDMETHOD(GetClassID)(CLSID *pClassID);

private:

	HRESULT SolveMethod(INetworkQueryPtr, IGPMessages *, ITrackCancel *, IStepProgressorPtr, std::shared_ptr<EvacueeList>, std::shared_ptr<NAVertexCache>, std::shared_ptr<NAEdgeCache>,
		    std::shared_ptr<SafeZoneTable>, double &, std::vector<unsigned int> &, INetworkDatasetPtr, unsigned int &, std::vector<double> &, std::vector<double> &);
	HRESULT CARMALoop(INetworkQueryPtr, IStepProgressorPtr, IGPMessages*, ITrackCancel*, std::shared_ptr<EvacueeList>, std::shared_ptr<std::vector<EvacueePtr>>, std::shared_ptr<NAVertexCache>,
		    std::shared_ptr<NAEdgeCache>, std::shared_ptr<SafeZoneTable>, size_t &, std::shared_ptr<NAEdgeMapTwoGen>, std::shared_ptr<NAEdgeContainer>, std::vector<unsigned int> &, double, double &, bool, CARMASort);
	HRESULT BuildClassDefinitions(ISpatialReference* pSpatialRef, INamedSet** ppDefinitions, IDENetworkDataset* pDENDS);
	HRESULT CreateSideOfEdgeDomain(IDomain** ppDomain);
	HRESULT CreateCurbApproachDomain(IDomain** ppDomain);
	HRESULT CreateStatusCodedValueDomain(ICodedValueDomain* pCodedValueDomain);
	HRESULT CreateFlockingCodedValueDomain(ICodedValueDomain* pCodedValueDomain);
	HRESULT AddLocationFields(IFieldsEdit* pFieldsEdit, IDENetworkDataset* pDENDS);
	HRESULT AddLocationFieldTypes(INAClassDefinitionEdit* pClassDef);
	HRESULT GetNAClassTable(INAContext* pContext, BSTR className, ITable** ppTable);
	HRESULT LoadBarriers(ITable* pTable, INetworkQuery* pNetworkQuery, INetworkForwardStarEx* pNetworkForwardStarEx);
	HRESULT PrepareUnvisitedVertexForHeap(INetworkJunctionPtr junction, NAEdgePtr edge, NAEdgePtr prevEdge, double edgeCost, NAVertexPtr myVertex, std::shared_ptr<NAEdgeCache> ecache,
		    std::shared_ptr<NAEdgeMapTwoGen> closedList, std::shared_ptr<NAVertexCache> vcache, INetworkQueryPtr ipNetworkQuery, bool checkOldClosedlist = true) const;
	HRESULT DeterminMinimumPop2Route(std::shared_ptr<EvacueeList>, INetworkDatasetPtr, double &, bool &) const;
	size_t  FindPathsThatNeedToBeProcessedInIteration(std::shared_ptr<EvacueeList>, std::shared_ptr<std::vector<EvcPathPtr>>, std::vector<double> &);
	void    MarkDirtyEdgesAsUnVisited(NAEdgeMap *, std::shared_ptr<NAEdgeContainer>, std::shared_ptr<NAEdgeContainer>, double, EvcSolverMethod) const;
	void    NonRecursiveMarkAndRemove(NAEdgePtr, NAEdgeMap *, std::shared_ptr<NAEdgeContainer>) const;
	bool    GeneratePath(SafeZonePtr, NAVertexPtr, double &, int &, EvacueePtr, double, bool) const;
	void    UpdatePeakMemoryUsage();

	esriNAOutputLineType	m_outputLineType;
	bool					m_bPersistDirty;
	bool					ShouldCARMACheckForDecreasedCost;
	long					costAttributeID;
	long					capAttributeID;
	INAStreetDirectionsAgentPtr pStreetAgent;
	float					SaturationPerCap;
	float					CriticalDensPerCap;
	EvcSolverMethod		    solverMethod;
	EvcTrafficModel		    trafficModel;
	float					costPerDensity;
	VARIANT_BOOL			flockingEnabled;
	float					flockingSnapInterval;
	float					flockingSimulationInterval;
	float					initDelayCostPerPop;
	FLOCK_PROFILE			flockingProfile;
	float                   CARMAPerformanceRatio;
	float                   selfishRatio;
	float                   iterateRatio;
	SIZE_T					peakMemoryUsage;
	HANDLE					hProcessPeakMemoryUsage;
	CARMASort               CarmaSortCriteria;
	EvacueeGrouping         evacueeGroupingOption;

	VARIANT_BOOL twoWayShareCapacity;
	VARIANT_BOOL ThreeGenCARMA;
	VARIANT_BOOL VarExportEdgeStat;
	VARIANT_BOOL m_CreateTraversalResult;
	VARIANT_BOOL m_FindBestSequence;
	VARIANT_BOOL m_PreserveFirstStop;
	VARIANT_BOOL m_PreserveLastStop;
	VARIANT_BOOL m_UseTimeWindows;

	esriNetworkForwardStarBacktrack backtrack;
	IStringArrayPtr turnAttributeNames;

	std::vector<INetworkAttribute2Ptr>	turnAttribs;
	std::vector<INetworkAttribute2Ptr>	costAttribs;
	std::vector<INetworkAttribute2Ptr>	discriptiveAttribs;
	IArray								* allAttribs;
	const long			c_version;
	long                savedVersion;
	const long			c_featureRetrievalInterval;
};

// Smart Pointer for IEvcSolver (for use within this project)
_COM_SMARTPTR_TYPEDEF(IEvcSolver, __uuidof(IEvcSolver));

// Simple helper class for managing the cancel tracker object during Solve
class CancelTrackerHelper
{
public:
	virtual ~CancelTrackerHelper()
	{
		if (m_ipTrackCancel && m_ipProgressor)
		{
			m_ipTrackCancel->put_Progressor(m_ipProgressor);
			m_ipProgressor->Hide();
		}
	}

	void ManageTrackCancel(ITrackCancel* pTrackCancel)
	{
		m_ipTrackCancel = pTrackCancel;
		if (m_ipTrackCancel)
		{
			m_ipTrackCancel->get_Progressor(&m_ipProgressor);
			m_ipTrackCancel->put_Progressor(0);
		}
	}

private:
	ITrackCancelPtr m_ipTrackCancel;
	IProgressorPtr  m_ipProgressor;
};

// Utility functions
double GetUnitPerDay(esriNetworkAttributeUnits unit, double assumedSpeed);

HRESULT PrepareVerticesForHeap(NAVertexPtr point, std::shared_ptr<NAVertexCache> vcache, std::shared_ptr<NAEdgeCache> ecache, NAEdgeMap * closedList, std::vector<NAEdgePtr> & readyEdges, double pop,
							   EvcSolverMethod solverMethod, double selfishRatio, double MaxEvacueeCostSoFar, QueryDirection dir);
HRESULT InsertLeafEdgesToHeap(INetworkQueryPtr ipNetworkQuery, std::shared_ptr<NAVertexCache> vcache, std::shared_ptr<NAEdgeCache> ecache, MyFibonacciHeap<NAEdgePtr, NAEdgePtrHasher, NAEdgePtrEqual> & heap, std::shared_ptr<NAEdgeContainer> leafs
								#ifdef DEBUG
								, double minPop2Route, EvcSolverMethod solverMethod
								#endif
								);
