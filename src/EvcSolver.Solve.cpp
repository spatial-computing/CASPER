// ===============================================================================================
// Evacuation Solver: Solve function
// Description: After solve is called, the program load all inuts, calls the right optimizer, then
// writes the outputs.
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#include "stdafx.h"
#include "NameConstants.h"
#include "EvcSolver.h"
#include "FibonacciHeap.h"
#include "Flocking.h"

// includes variable for commit hash / git describe string
#include "gitdescribe.h"

STDMETHODIMP EvcSolver::Solve(INAContext* pNAContext, IGPMessages* pMessages, ITrackCancel* pTrackCancel, VARIANT_BOOL* pIsPartialSolution)
{
	//******************************************************************************************/
	// Solve is the method that is called to perform the actual network analysis. The solver component
	// should be in a valid state before this method should be called. For example, within the ArcMap
	// application, Network Analyst performs certain validation checks on the solver and its associated output
	// before enabling the Solve command (on the Network Analyst toolbar) for any given solver.
	// This validation includes the following status checks:
	// 1) check that the solver has created a valid context
	// 2) check that the solver has created a valid NALayer
	// 3) check that the solver's associated NAClass feature classes currently have valid cardinalities
	//    (this last concept is discussed in the BuildClassDefinitions method of this custom solver)

	// Once the solver component has produced a valid state for appropriate processing (such as defined above),
	// this method should be available to call
	// NOTE: for consistency within custom applications, similar validation checks should also be implemented
	// before calling the Solve method on any solver

	#ifdef TRACE
	std::ofstream f;
	f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
	time_t curr = time(NULL);
	char timeBuff[50];
	ctime_s(timeBuff, 50, &curr);
	f << "Trace start: " << timeBuff << std::endl;
	f.close();
	#endif

	#ifdef DEBUG
	void * emptyPtr1 = NULL;
	void * emptyPtr2 = nullptr;
	OutputDebugStringW(emptyPtr1 == emptyPtr2 && emptyPtr2 == emptyPtr1 ? L"c++11 pointer test pass\n" : L"c++11 pointer test fail\n");
	_ASSERT_EXPR(emptyPtr1 == emptyPtr2 && emptyPtr2 == emptyPtr1, L"c++11 pointer test fail");

	// heap validity test case
	{
		struct HeapNode
		{
			size_t value;
			double key;
			HeapNode(size_t _value = 0, double _key = 0.0) : value(_value), key(_key) { }
			operator double() const { return key; }
			bool operator==(const HeapNode & right) const { return value == right.value; }
			struct HeapNodeHasher : public std::unary_function<HeapNode, size_t> { size_t operator()(const HeapNode & e) const { return e.value; } };
		};

		MyFibonacciHeap<HeapNode, HeapNode::HeapNodeHasher> testHeap;
		srand(unsigned int(time(0)));
		HeapNode data[10000];
		HeapNode min1, min2;

		for (size_t i = 0; i < 10000; ++i) data[i] = HeapNode(i, rand());
		for (size_t i = 0; i < 5000; ++i) testHeap.Insert(data[i]);
		for (size_t i = 0; i < 5000; i += 20)
		{
			data[i].key -= 100;
			testHeap.UpdateKey(data[i]);
		}
		for (size_t i = 5000; i < 10000; ++i) testHeap.Insert(data[i]);
		min1 = testHeap.DeleteMin();
		bool minHeapTestPass = true;
		while (!testHeap.empty())
		{
			min2 = testHeap.DeleteMin();
			_ASSERT_EXPR(min1.key <= min2.key, L"Heap property violation");
			minHeapTestPass &= min1.key <= min2.key;
			min1 = min2;
		}
		OutputDebugStringW(minHeapTestPass ? L"heap property test pass\n" : L"heap property test fail\n");
	}
	#endif

	HRESULT hr = S_OK;
	double globalEvcCost = -1.0, carmaSec = 0.0;
	unsigned int EvacueesWithRestrictedSafezone = 0;

	// init memory usage function and set the base
	peakMemoryUsage = 0l;
	hProcessPeakMemoryUsage = nullptr;
	UpdatePeakMemoryUsage();
	SIZE_T baseMemoryUsage = peakMemoryUsage;
	bool exportEdgeStat = VarExportEdgeStat == VARIANT_TRUE, IsSafeZoneMissed = false;

	// Check for null parameter variables (the track cancel variable is typically considered optional)
	if (!pNAContext || !pMessages) return E_POINTER;

	// The partialSolution variable is used to indicate to the caller of this method whether or not we were only able to
	// find a partial solution to the problem. We initialize this variable to false and set it to true only in certain
	// conditional cases (e.g., some stops/points are unreachable, a Evacuee point is not located, etc.)
	*pIsPartialSolution = VARIANT_FALSE;

	// NOTE: this is an appropriate place to check if the user is licensed to run
	// your solver and fail with "E_NOT_LICENSED" or similar.

	// Clear the GP messages
	if (FAILED(hr = pMessages->Clear())) return hr;

	// Validate the context (i.e., make sure that it is bound to a network dataset)
	INetworkDatasetPtr ipNetworkDataset;
	if (FAILED(hr = pNAContext->get_NetworkDataset(&ipNetworkDataset))) return hr;

	if (!ipNetworkDataset) return ATL::AtlReportError(this->GetObjectCLSID(), _T("Context does not have a valid network dataset."), IID_INASolver);

	// NOTE: this is also a good place to perform any additional necessary validation, such as
	// synchronizing the attribute names set on your solver with those of the context's network dataset

	// Check for a Step Progress bar on the track cancel parameter variable.
	// This can be used to indicate progress and output messages to the client throughout the solve
	CancelTrackerHelper cancelTrackerHelper;
	IStepProgressorPtr ipStepProgressor;
	if (pTrackCancel)
	{
		IProgressorPtr ipProgressor;
		if (FAILED(hr = pTrackCancel->get_Progressor(&ipProgressor))) return hr;

		ipStepProgressor = ipProgressor;

		// We use the cancel tracker helper object to disassociate and re-associate the cancel tracker from the progress bar
		// during and after the Solve, respectively. This prevents calls to ITrackCancel::Continue from stepping our progress bar
		cancelTrackerHelper.ManageTrackCancel(pTrackCancel);
	}

	// Some timing functions
	FILETIME cpuTimeS, cpuTimeE, sysTimeS, sysTimeE, createTime, exitTime;
	BOOL c;
	bool flagBadDynamicChangeSnapping = false;
	double inputSecSys, calcSecSys, flockSecSys, outputSecSys, inputSecCpu, calcSecCpu, flockSecCpu, outputSecCpu;
	__int64 tenNanoSec64;

	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeS, &cpuTimeS);

	//******************************************************************************************/
	// Reset each NAClass to an appropriate state (as necessary) before proceeding
	// This is typically done in order to:
	// 1) remove any features that were previously created in our output NAClasses from previous solves
	// 2) check the input NAClasses for output fields and reset these field values as necessary
	// In our case, we do not have any input NAClasses with output fields, and we only have one output feature class,
	// so we can simply clear any features currently present in the "LineData" NAClass.
	// NOTE: if you have multiple input/output NAClasses to clean up, you can simply loop through all NAClasses,
	// get their NAClassDefinition, check whether or not each is an input/output class, and reset it accordingly
	INamedSetPtr ipNAClasses = nullptr;
	if (FAILED(hr = pNAContext->get_NAClasses(&ipNAClasses))) return hr;

	// remove any features that might have been created on previous solves
	IUnknownPtr ipUnk = nullptr;
	if (FAILED(hr = ipNAClasses->get_ItemByName(ATL::CComBSTR(CS_ROUTES_NAME), &ipUnk))) return hr;
	INAClassPtr ipRoutesNAClass(ipUnk);
	if (FAILED(hr = ipRoutesNAClass->DeleteAllRows())) return hr;

	ipUnk = nullptr;
	if (FAILED(hr = ipNAClasses->get_ItemByName(ATL::CComBSTR(CS_EDGES_NAME), &ipUnk))) return hr;
	INAClassPtr ipEdgesNAClass(ipUnk);
	if (FAILED(hr = ipEdgesNAClass->DeleteAllRows())) return hr;

	ipUnk = nullptr;
	if (FAILED(hr = ipNAClasses->get_ItemByName(ATL::CComBSTR(CS_FLOCKS_NAME), &ipUnk))) return hr;
	INAClassPtr ipFlocksNAClass(ipUnk);
	if (flockingEnabled == VARIANT_TRUE && !ipUnk) flockingEnabled = VARIANT_FALSE;
	if (ipFlocksNAClass) { if (FAILED(hr = ipFlocksNAClass->DeleteAllRows())) return hr; }

	//******************************************************************************************/
	// Setup the Network Forward Star for traversal
	// Create a Forward Star object from the INetworkQuery interface of the network dataset

	// QI the Forward Star to INetworkForwardStarEx
	// This interface can be used to setup necessary traversal constraints on the Forward Star before proceeding
	// This typically includes:
	// 1) setting the traversal direction (through INetworkForwardStarSetup::IsForwardTraversal)
	// 2) setting any restrictions on the forward star (e.g., one-way restrictions, restricted turns, etc.)
	// 3) setting the U-turn policy (through INetworkForwardStarSetup::Backtrack)
	// 4) setting the hierarchy (through INetworkForwardStarSetup::HierarchyAttribute)
	// 5) setting up traversable/non-traversable elements (in our case, we will be setting up barriers as non-traversable)
	INetworkQueryPtr ipNetworkQuery(ipNetworkDataset);
	INetworkForwardStarPtr ipNetworkForwardStar;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStar(&ipNetworkForwardStar))) return hr;
	INetworkForwardStarExPtr ipForwardStar(ipNetworkForwardStar);
	if (FAILED(hr = ipForwardStar->put_BacktrackPolicy(backtrack))) return hr;

	// this will create the backward traversal object to query for adjacencies during pre-processing
	INetworkForwardStarPtr ipNetworkBackwardStar;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStar(&ipNetworkBackwardStar))) return hr;
	INetworkForwardStarExPtr ipBackwardStar(ipNetworkBackwardStar);
	if (FAILED(hr = ipBackwardStar->put_IsForwardTraversal(VARIANT_FALSE))) return hr;
	if (FAILED(hr = ipBackwardStar->put_BacktrackPolicy(backtrack))) return hr;

	// Get the "Barriers" NAClass table (we need the NALocation objects from this NAClass to push barriers into the Forward Star)
	ITablePtr ipBarriersTable = nullptr;
	if (FAILED(hr = GetNAClassTable(pNAContext, ATL::CComBSTR(CS_OldBARRIERS_NAME), &ipBarriersTable, false))) return hr;

	if (ipBarriersTable)
	{
		// Load the barriers
		if (FAILED(hr = LoadBarriers(ipBarriersTable, ipNetworkQuery, ipForwardStar))) return hr;
		if (FAILED(hr = LoadBarriers(ipBarriersTable, ipNetworkQuery, ipBackwardStar))) return hr;
	}
	INetworkAttribute2Ptr networkAttrib = nullptr;
	VARIANT_BOOL useRestriction;

	// loading restriction attributes into the forward star. this will enforce all available restrictions.
	for (std::vector<INetworkAttribute2Ptr>::const_iterator Iter = turnAttribs.begin(); Iter != turnAttribs.end(); Iter++)
	{
		networkAttrib = *Iter;
		if (FAILED(hr = networkAttrib->get_UseByDefault(&useRestriction))) return hr;
		if (useRestriction == VARIANT_TRUE)
		{
			if (FAILED(hr = ipForwardStar->AddRestrictionAttribute(networkAttrib))) return hr;
			if (FAILED(hr = ipBackwardStar->AddRestrictionAttribute(networkAttrib))) return hr;
		}
	}

	// Get the "Evacuee Points" NAClass table (we need the NALocation objects from
	// this NAClass as the starting points for Forward Star traversals)
	ITablePtr ipEvacueePointsTable;
	if (FAILED(hr = GetNAClassTable(pNAContext, ATL::CComBSTR(CS_EVACUEES_NAME), &ipEvacueePointsTable))) return hr;
	// Same for safe zone points
	ITablePtr ipZonesTable;
	if (FAILED(hr = GetNAClassTable(pNAContext, ATL::CComBSTR(CS_ZONES_NAME), &ipZonesTable))) return hr;

	// Create variables for looping through the cursor and traversing the network
	IRowESRI * ipRow;
	ICursorPtr ipCursor;
	INALocationObjectPtr ipNALocationObject;
	INALocationPtr ipNALocation(CLSID_NALocation);
	IEnumNetworkElementPtr ipEnumNetworkElement;
	std::vector<double> GlobalEvcCostAtIteration, EffectiveIterationRatio;
	INetworkElementPtr ipElement, ipOtherElement;
	long sourceOID, sourceID;
	double posAlong, posAlongEdge, fromPosition, toPosition;
	VARIANT_BOOL keepGoing, isLocated, isRestricted;
	esriNetworkElementType elementType;
	esriNAEdgeSideType side;

	// Initialize Caches
	// This cache will maintain a list of all created vertices/edges. You can retrieve
	// them later using EID. The benefit of using this cache is that we
	// can maintain one-to-one relationship between network junctions and vertices.
	// This will particularly be helpful with the heuristic calculator part of the algorithm.
	auto ecache = std::shared_ptr<NAEdgeCache>(new DEBUG_NEW_PLACEMENT NAEdgeCache(capAttributeID, costAttributeID, SaturationPerCap, CriticalDensPerCap, twoWayShareCapacity == VARIANT_TRUE,
		                                                                           initDelayCostPerPop, trafficModel, ipForwardStar, ipBackwardStar, ipNetworkQuery, hr));
	if (FAILED(hr)) return hr;

	// since some vertices inside the cache will point to edges, it's safer to create this object last so that it gets destroyed (pop out of function stack) before ecache
	auto vcache = std::shared_ptr<NAVertexCache>(new DEBUG_NEW_PLACEMENT NAVertexCache());

	// Vertex table structures
	auto safeZoneList = std::shared_ptr<SafeZoneTable>(new DEBUG_NEW_PLACEMENT SafeZoneTable(100));
	long nameFieldIndex = 0l, popFieldIndex = 0l, capFieldIndex = 0l, objectID, curbSideIndex = -1;
	VARIANT evName, pop, cap;

	// read cost attribute unit
	INetworkAttributePtr costAttrib;
	esriNetworkAttributeUnits unit;
	esriNACurbApproachType curbApproach;
	VARIANT curbApproachVar;
	FlockProfile flockProfile(flockingEnabled ? flockingProfile : FLOCK_PROFILE_CAR);
	if (FAILED(hr = ipNetworkDataset->get_AttributeByID(costAttributeID, &costAttrib))) return hr;
	if (FAILED(hr = costAttrib->get_Units(&unit))) return hr;
	double costPerDay = GetUnitPerDay(unit, flockProfile.UsualSpeed);
	double costPerSec = costPerDay / (3600.0 * 24.0);

	if (ipStepProgressor) ipStepProgressor->put_Message(ATL::CComBSTR(L"Collecting input points")); // add more specific information here if appropriate

	//******************************************************************************************/
	// here we begin collecting safe zone points for all the evacuees

	// Get a cursor on the zones table to loop through each row
	if (FAILED(hr = ipZonesTable->FindField(ATL::CComBSTR(CS_FIELD_CAP), &capFieldIndex))) return hr;
	if (FAILED(hr = ipZonesTable->FindField(ATL::CComBSTR(CS_FIELD_CURBAPPROACH), &curbSideIndex))) return hr;
	if (FAILED(hr = ipZonesTable->Search(nullptr, VARIANT_TRUE, &ipCursor))) return hr;
	while (ipCursor->NextRow(&ipRow) == S_OK)
	{
		ipNALocationObject = ipRow;
		if (!ipNALocationObject) // we only want valid NALocationObjects
		{
			// If this Evacuee point is an invalid NALocationObject, we will only be able to find a partial solution
			*pIsPartialSolution = VARIANT_TRUE;
			continue;
		}

		if (FAILED(hr = ipNALocationObject->QueryNALocation(ipNALocation))) return hr;

		// Once we have the NALocation, we need to check if it is actually located within the network dataset
		isLocated = VARIANT_FALSE;
		if (ipNALocation)
		{
			if (FAILED(hr = ipNALocation->get_IsLocated(&isLocated))) return hr;
		}

		// We are only concerned with located safe zone point NALocations
		if (isLocated)
		{
			// Get the SourceID for the NALocation
			if (FAILED(hr = ipNALocation->get_SourceID(&sourceID))) return hr;

			// Get the SourceOID for the NALocation
			if (FAILED(hr = ipNALocation->get_SourceOID(&sourceOID))) return hr;

			// Get the PosAlong for the NALocation
			if (FAILED(hr = ipNALocation->get_SourcePosition(&posAlong))) return hr;

			// Get the side of street for the NALocation
			if (FAILED(hr = ipNALocation->get_Side(&side))) return hr;

			if (FAILED(hr = ipRow->get_Value(capFieldIndex, &cap))) return hr;

			// Get the side of curb that we can approach
			if (FAILED(hr = ipRow->get_Value(curbSideIndex, &curbApproachVar))) curbApproach = esriNACurbApproachType::esriNANoUTurn;
			else curbApproach = (esriNACurbApproachType)curbApproachVar.intVal;

			// Once we have a located NALocation, we query the network to obtain its associated network elements
			if (FAILED(hr = ipNetworkQuery->get_ElementsByOID(sourceID, sourceOID, &ipEnumNetworkElement))) return hr;

			// We must loop through the returned elements, looking for an appropriate ending point
			ipEnumNetworkElement->Reset();

			while (ipEnumNetworkElement->Next(&ipElement) == S_OK)
			{
				// We must then check the returned element type
				ipElement->get_ElementType(&elementType);

				// If the element is a junction, then it is the starting point of traversal
				// We simply add its EID to the table.
				if (elementType == esriNETJunction)
				{
					if (FAILED(hr = ipForwardStar->get_IsRestricted(ipElement, &isRestricted))) return hr;
					if (!isRestricted) safeZoneList->insert(new DEBUG_NEW_PLACEMENT SafeZone(ipElement, nullptr, 0, cap));
				}

				// If the element is an edge, then we must check the fromPosition and toPosition
				if (elementType == esriNETEdge)
				{
					INetworkEdgePtr ipEdge(ipElement);
					
					if (FAILED(hr = ipEdge->QueryPositions(&fromPosition, &toPosition))) return hr;
					posAlongEdge = (posAlong - fromPosition) / (toPosition - fromPosition);

					if (fromPosition <= posAlong && posAlong <= toPosition &&
						(side == esriNAEdgeSideType::esriNAEdgeSideRight && curbApproach != esriNACurbApproachType::esriNALeftSideOfVehicle) ||
						(side == esriNAEdgeSideType::esriNAEdgeSideLeft && curbApproach != esriNACurbApproachType::esriNARightSideOfVehicle))
					{
						// Our NALocation lies along this edge element
						// We will start our traversal from the junctions of this edge
						// and then we check the other edge in the opposite direction

						if (FAILED(hr = ipForwardStar->get_IsRestricted(ipEdge, &isRestricted))) return hr;
						if (!isRestricted)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
							INetworkJunctionPtr ipCurrentJunction(ipOtherElement);
							if (FAILED(hr = ipEdge->QueryJunctions(ipCurrentJunction, nullptr))) return hr;
							IsSafeZoneMissed |= !(safeZoneList->insert(new DEBUG_NEW_PLACEMENT SafeZone(ipCurrentJunction, ecache->New(ipEdge), posAlongEdge, cap)));
						}
					}

					if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipOtherElement))) return hr;
					INetworkEdgePtr ipOtherEdge(ipOtherElement);
					if (FAILED(hr = ipEdge->QueryEdgeInOtherDirection(ipOtherEdge))) return hr;
					if (FAILED(hr = ipOtherEdge->QueryPositions(&fromPosition, &toPosition))) return hr;
					posAlongEdge = (posAlong - fromPosition) / (toPosition - fromPosition);

					if (toPosition <= posAlong && posAlong <= fromPosition &&
						(side == esriNAEdgeSideType::esriNAEdgeSideRight && curbApproach != esriNACurbApproachType::esriNARightSideOfVehicle) ||
						(side == esriNAEdgeSideType::esriNAEdgeSideLeft && curbApproach != esriNACurbApproachType::esriNALeftSideOfVehicle))
					{
						if (FAILED(hr = ipForwardStar->get_IsRestricted(ipOtherEdge, &isRestricted))) return hr;
						if (!isRestricted)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
							INetworkJunctionPtr ipCurrentJunction(ipOtherElement);
							if (FAILED(hr = ipOtherEdge->QueryJunctions(ipCurrentJunction, nullptr))) return hr;

							IsSafeZoneMissed |= !(safeZoneList->insert(new DEBUG_NEW_PLACEMENT SafeZone(ipCurrentJunction, ecache->New(ipOtherEdge), posAlongEdge, cap)));
						}
					}
				}
			}
		}
	}

	// Get a cursor on the Evacuee points table to loop through each row
	long evacueeCount;
	if (FAILED(hr = ipEvacueePointsTable->Search(nullptr, VARIANT_TRUE, &ipCursor))) return hr;
	if (FAILED(hr = ipEvacueePointsTable->RowCount(nullptr, &evacueeCount))) return hr;
	auto Evacuees = std::shared_ptr<EvacueeList>(new DEBUG_NEW_PLACEMENT EvacueeList(evacueeGroupingOption, evacueeCount));
	Evacuee * currentEvacuee;
	NAVertexPtr myVertex;

	// pre-process evacuee NALayer primary field index
	if (FAILED(hr = ipEvacueePointsTable->FindField(ATL::CComBSTR(CS_FIELD_CURBAPPROACH), &curbSideIndex))) return hr;
	if (FAILED(hr = ipEvacueePointsTable->FindField(ATL::CComBSTR(CS_FIELD_NAME), &nameFieldIndex))) return hr;
	if (FAILED(hr = ipEvacueePointsTable->FindField(ATL::CComBSTR(CS_FIELD_EVC_POP2), &popFieldIndex))) return hr;
	if (popFieldIndex < 1) { if (FAILED(hr = ipEvacueePointsTable->FindField(ATL::CComBSTR(CS_FIELD_EVC_POP1), &popFieldIndex))) return hr; }

	while (ipCursor->NextRow(&ipRow) == S_OK)
	{
		ipNALocationObject = ipRow;
		if (!ipNALocationObject) // we only want valid NALocationObjects
		{
			// If this Evacuee point is an invalid NALocationObject, we will only be able to find a partial solution
			*pIsPartialSolution = VARIANT_TRUE;
			continue;
		}

		if (FAILED(hr = ipNALocationObject->QueryNALocation(ipNALocation))) return hr;

		// Once we have the NALocation, we need to check if it is actually located within the network dataset
		isLocated = VARIANT_FALSE;
		if (ipNALocation)
		{
			if (FAILED(hr = ipNALocation->get_IsLocated(&isLocated))) return hr;
		}

		// We are only concerned with located evacuee point NALocations
		if (!isLocated)
		{
			// If this Evacuee point is unlocated, we will only be able to find a partial solution
			*pIsPartialSolution = VARIANT_TRUE;
		}
		else
		{
			// Get the SourceID for the NALocation
			if (FAILED(hr = ipNALocation->get_SourceID(&sourceID))) return hr;

			// Get the SourceOID for the NALocation
			if (FAILED(hr = ipNALocation->get_SourceOID(&sourceOID))) return hr;

			// Get the PosAlong for the NALocation
			if (FAILED(hr = ipNALocation->get_SourcePosition(&posAlong))) return hr;

			// Get the side of street for the NALocation
			if (FAILED(hr = ipNALocation->get_Side(&side))) return hr;

			// Get the side of curb that we can approach
			if (FAILED(hr = ipRow->get_Value(curbSideIndex, &curbApproachVar))) curbApproach = esriNACurbApproachType::esriNANoUTurn;
			else curbApproach = (esriNACurbApproachType)curbApproachVar.intVal;

			// Get the OID of the evacuee NALocation
			if (FAILED(hr = ipRow->get_Value(nameFieldIndex, &evName))) return hr;
			if (FAILED(hr = ipRow->get_Value(popFieldIndex, &pop))) return hr;
			if (FAILED(hr = ipRow->get_OID(&objectID))) return hr;
			currentEvacuee = new DEBUG_NEW_PLACEMENT Evacuee(evName, pop.dblVal, objectID);

			// Once we have a located NALocation, we query the network to obtain its associated network elements
			if (FAILED(hr = ipNetworkQuery->get_ElementsByOID(sourceID, sourceOID, &ipEnumNetworkElement))) return hr;

			// We must loop through the returned elements, looking for an appropriate starting point
			ipEnumNetworkElement->Reset();

			while (ipEnumNetworkElement->Next(&ipElement) == S_OK)
			{
				// We must then check the returned element type
				ipElement->get_ElementType(&elementType);

				// If the element is a junction, then it is the starting point of traversal
				// We simply add its EID to the heap and break out of the enumerating loop
				if (elementType == esriNETJunction)
				{
					if (FAILED(hr = ipForwardStar->get_IsRestricted(ipElement, &isRestricted))) return hr;
					if (!isRestricted)
					{
						myVertex = new DEBUG_NEW_PLACEMENT NAVertex(ipElement, nullptr);
						currentEvacuee->VerticesAndRatio->push_back(myVertex);
					}
				}

				// If the element is an edge, then we must check the fromPosition and toPosition to be certain it holds an appropriate starting point
				if (elementType == esriNETEdge)
				{
					INetworkEdgePtr ipEdge(ipElement);
					if (FAILED(hr = ipEdge->QueryPositions(&fromPosition, &toPosition))) return hr;
					posAlongEdge = (toPosition - posAlong) / (toPosition - fromPosition);

					if (fromPosition <= posAlong && posAlong <= toPosition &&
						(side == esriNAEdgeSideType::esriNAEdgeSideRight && curbApproach != esriNACurbApproachType::esriNALeftSideOfVehicle) ||
						(side == esriNAEdgeSideType::esriNAEdgeSideLeft && curbApproach != esriNACurbApproachType::esriNARightSideOfVehicle))
					{
						// Our NALocation lies along this edge element
						// We will start our traversal from the junctions of this edge
						// and then we check the other edge in the opposite direction

						if (FAILED(hr = ipForwardStar->get_IsRestricted(ipEdge, &isRestricted))) return hr;
						if (!isRestricted)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
							INetworkJunctionPtr ipCurrentJunction(ipOtherElement);
							if (FAILED(hr = ipEdge->QueryJunctions(nullptr, ipCurrentJunction))) return hr;

							myVertex = new DEBUG_NEW_PLACEMENT NAVertex(ipCurrentJunction, ecache->New(ipEdge));
							myVertex->GVal = posAlongEdge /** myVertex->GetBehindEdge()->OriginalCost*/;
							currentEvacuee->VerticesAndRatio->push_back(myVertex);
						}
					}

					if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipOtherElement))) return hr;
					INetworkEdgePtr ipOtherEdge(ipOtherElement);
					if (FAILED(hr = ipEdge->QueryEdgeInOtherDirection(ipOtherEdge))) return hr;
					if (FAILED(hr = ipOtherEdge->QueryPositions(&fromPosition, &toPosition))) return hr;
					posAlongEdge = (toPosition - posAlong) / (toPosition - fromPosition);

					if (toPosition <= posAlong && posAlong <= fromPosition &&
						(side == esriNAEdgeSideType::esriNAEdgeSideRight && curbApproach != esriNACurbApproachType::esriNARightSideOfVehicle) ||
						(side == esriNAEdgeSideType::esriNAEdgeSideLeft && curbApproach != esriNACurbApproachType::esriNALeftSideOfVehicle))
					{
						if (FAILED(hr = ipForwardStar->get_IsRestricted(ipOtherEdge, &isRestricted))) return hr;
						if (!isRestricted)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
							INetworkJunctionPtr ipCurrentJunction(ipOtherElement);
							if (FAILED(hr = ipOtherEdge->QueryJunctions(nullptr, ipCurrentJunction))) return hr;

							myVertex = new DEBUG_NEW_PLACEMENT NAVertex(ipCurrentJunction, ecache->New(ipOtherEdge));
							myVertex->GVal = posAlongEdge /** myVertex->GetBehindEdge()->OriginalCost*/;
							currentEvacuee->VerticesAndRatio->push_back(myVertex);
						}
					}
				}
			}
			if (currentEvacuee->VerticesAndRatio->size() > 0) Evacuees->Insert(currentEvacuee);
			else delete currentEvacuee;
		}
	}

	// load dynamic changes table
	ipUnk = nullptr;
	ITablePtr ipDynamicTable = nullptr;
	if (FAILED(hr = ipNAClasses->get_ItemByName(ATL::CComBSTR(CS_DYNCHANGES_NAME), &ipUnk))) return hr;
	bool DynamicTableExist = ipUnk;
	if (DynamicTableExist) { if (FAILED(hr = GetNAClassTable(pNAContext, ATL::CComBSTR(CS_DYNCHANGES_NAME), &ipDynamicTable))) return hr; }
	std::shared_ptr<DynamicDisaster> disasterTable(new DEBUG_NEW_PLACEMENT DynamicDisaster(ipDynamicTable, CASPERDynamicMode, flagBadDynamicChangeSnapping, solverMethod));

	Evacuees->FinilizeGroupings(5.0 * costPerSec, disasterTable->GetDynamicMode()); // five seconds diameter for clustering

	// timing
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeE, &cpuTimeE);
	tenNanoSec64 = (*((__int64 *) &sysTimeE)) - (*((__int64 *) &sysTimeS));
	inputSecSys = tenNanoSec64 / 10000000.0;
	tenNanoSec64 = (*((__int64 *) &cpuTimeE)) - (*((__int64 *) &cpuTimeS));
	inputSecCpu = tenNanoSec64 / 10000000.0;
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeS, &cpuTimeS);

	if (ipStepProgressor) if (FAILED(hr = ipStepProgressor->Show())) return hr;
	std::vector<unsigned int> CARMAExtractCounts;

	//******************************************************************************************/
	// this will call the core part of the algorithm.
	hr = S_OK;
	UpdatePeakMemoryUsage();
	if (FAILED(hr = SolveMethod(ipNetworkQuery, pMessages, pTrackCancel, ipStepProgressor, Evacuees, vcache, ecache, safeZoneList, carmaSec, CARMAExtractCounts,
		ipNetworkDataset, EvacueesWithRestrictedSafezone, GlobalEvcCostAtIteration, EffectiveIterationRatio, disasterTable))) return hr;

	// timing
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeE, &cpuTimeE);
	tenNanoSec64 = (*((__int64 *) &sysTimeE)) - (*((__int64 *) &sysTimeS));
	calcSecSys = tenNanoSec64 / 10000000.0;
	tenNanoSec64 = (*((__int64 *) &cpuTimeE)) - (*((__int64 *) &cpuTimeS));
	calcSecCpu = tenNanoSec64 / 10000000.0;
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeS, &cpuTimeS);

	disasterTable->Flush();
	ecache->InitSourceCache();

	//******************************************************************************************/
	// Write output

	// Now that we have completed our traversal of the network from the Evacuee points, we must output the connected/disconnected edges
	// to the "LineData" NAClass

	// Setup a message on our step progress bar indicating that we are outputting feature information
	if (ipStepProgressor) ipStepProgressor->put_Message(ATL::CComBSTR(L"Writing output features"));

	// looping through processed evacuees and generate routes in output feature class
	std::list<EvcPathPtr>::const_iterator tpit;
	std::vector<EvcPathPtr>::const_iterator pit;
	bool sourceNotFoundFlag = false;
	IFeatureClassContainerPtr ipFeatureClassContainer(ipNetworkDataset);
	size_t StuckEvacuee = 0;

	// load the Mercator projection and analysis projection
	ISpatialReferencePtr ipNAContextSR;
	if (FAILED(hr = pNAContext->get_SpatialReference(&ipNAContextSR))) return hr;

	IProjectedCoordinateSystemPtr ipNAContextPC;
	ISpatialReferenceFactoryPtr pSpatRefFact = ISpatialReferenceFactoryPtr(CLSID_SpatialReferenceEnvironment);
	if (FAILED(hr = pSpatRefFact->CreateProjectedCoordinateSystem(esriSRProjCS_WGS1984WorldMercator, &ipNAContextPC))) return hr;
	ISpatialReferencePtr ipSpatialRef = ipNAContextPC;
	std::vector<EvcPathPtr> tempPathList(Evacuees->size());
	tempPathList.clear();

	for (const auto & currentEvacuee : *Evacuees)
	{
		// get all points from the stack and make one polyline from them. this will be the path.
		if (currentEvacuee->Paths->empty() || currentEvacuee->Status == EvacueeStatus::Unreachable)
		{
			if (currentEvacuee->Population > 0.0)
			{
				*pIsPartialSolution = VARIANT_TRUE;
				++StuckEvacuee;
			}
		}
		else
		{
			for (tpit = currentEvacuee->Paths->begin(); tpit != currentEvacuee->Paths->end(); tpit++) tempPathList.push_back(*tpit);
		}
	}

	// If we reach this point, we have some features to output to the Routes NAClass
	// Reset the progress bar based on the number of features that we must output
	if (ipStepProgressor)
	{
		// Step progress bar range = 0 through numberOfOutputSteps
		if (FAILED(hr = ipStepProgressor->put_MinRange(0))) return hr;
		if (exportEdgeStat)
		{
			if (FAILED(hr = ipStepProgressor->put_MaxRange((long)(ecache->Size() + tempPathList.size())))) return hr;
		}
		else
		{
			if (FAILED(hr = ipStepProgressor->put_MaxRange((long)(tempPathList.size())))) return hr;
		}
		if (FAILED(hr = ipStepProgressor->put_StepValue(1))) return hr;
		if (FAILED(hr = ipStepProgressor->put_Position(0))) return hr;
	}

	std::sort(tempPathList.begin(), tempPathList.end(), EvcPath::LessThanPathOrder2);

	// Get the "Routes" NAClass feature class
	IFeatureClassPtr ipRoutesFC(ipRoutesNAClass);
	
	// Create an insert cursor and feature buffer from the "Routes" feature class to be used to write routes
	IFeatureCursorPtr ipFeatureCursorR, ipFeatureCursorU;
	if (FAILED(hr = ipRoutesFC->Insert(VARIANT_TRUE, &ipFeatureCursorR))) return hr;

	IFeatureBufferPtr ipFeatureBufferR;
	if (FAILED(hr = ipRoutesFC->CreateFeatureBuffer(&ipFeatureBufferR))) return hr;

	// Query for the appropriate field index values in the "routes" feature class
	long evNameFieldIndex = -1, evacTimeFieldIndex  = -1, orgTimeFieldIndex = -1, RIDFieldIndex    = -1;

	if (FAILED(hr = ipRoutesFC->FindField(ATL::CComBSTR(CS_FIELD_EVC_NAME), &evNameFieldIndex))) return hr;
	if (FAILED(hr = ipRoutesFC->FindField(ATL::CComBSTR(CS_FIELD_E_TIME), &evacTimeFieldIndex))) return hr;
	if (FAILED(hr = ipRoutesFC->FindField(ATL::CComBSTR(CS_FIELD_E_ORG), &orgTimeFieldIndex))) return hr;
	if (FAILED(hr = ipRoutesFC->FindField(ATL::CComBSTR(CS_FIELD_RID), &RIDFieldIndex))) return hr;
	if (FAILED(hr = ipRoutesFC->FindField(ATL::CComBSTR(CS_FIELD_EVC_POP2), &popFieldIndex))) return hr;
	if (popFieldIndex < 0) { if (FAILED(hr = ipRoutesFC->FindField(ATL::CComBSTR(CS_FIELD_E_POP), &popFieldIndex))) return hr; }

#ifdef DEBUG
	std::wostringstream os_;
	os_.precision(3);
	os_ << "Path stat output as CSV" << std::endl;
	os_ << "PathID,PredictedCost,EvacuationCost,FinalCost" << std::endl;
	OutputDebugStringW(os_.str().c_str());
#endif
	for (const auto & p : tempPathList)
	{
		if (FAILED(hr = p->AddPathToFeatureBuffers(pTrackCancel, ipNetworkDataset, ipFeatureClassContainer, sourceNotFoundFlag, ipStepProgressor, globalEvcCost, ipFeatureBufferR,
			ipFeatureCursorR, evNameFieldIndex, evacTimeFieldIndex, orgTimeFieldIndex, popFieldIndex))) return hr;
	}

	// flush the insert buffer
	ipFeatureCursorR->Flush();

	// copy all route OIDs to RouteID field
	if (RIDFieldIndex > -1)
	{
		IFeaturePtr routeFeature = nullptr;
		long routeID = -1;
		if (FAILED(hr = ipRoutesFC->Update(nullptr, VARIANT_TRUE, &ipFeatureCursorU))) return hr;
		if (FAILED(hr = ipFeatureCursorU->NextFeature(&routeFeature))) return hr;
		while (routeFeature)
		{
			if (FAILED(hr = routeFeature->get_OID(&routeID))) return hr;     // get OID
			if (FAILED(hr = routeFeature->put_Value(RIDFieldIndex, ATL::CComVariant(routeID)))) return hr;    // put OID as routeID
			if (FAILED(hr = ipFeatureCursorU->UpdateFeature(routeFeature))) return hr;    // put update back in table
			if (FAILED(hr = ipFeatureCursorU->NextFeature(&routeFeature))) return hr;     // for loop next feature
		}
	}

	//******************************************************************************************/
	// Exporting EdgeStat data to output featureClass

	if (exportEdgeStat)
	{
		// Get the "Routes" NAClass feature class
		IFeatureClassPtr ipEdgesFC(ipEdgesNAClass);
		IFeatureCursorPtr ipFeatureCursor;
		IFeatureBufferPtr ipFeatureBuffer;
		NAEdgePtr edge;
		long sourceIDFieldIndex, sourceOIDFieldIndex, resPopFieldIndex, travCostFieldIndex, orgCostFieldIndex, dirFieldIndex, eidFieldIndex, congestionFieldIndex;

		// Create an insert cursor and feature buffer from the "EdgeStat" feature class to be used to write edges
		if (FAILED(hr = ipEdgesFC->Insert(VARIANT_TRUE, &ipFeatureCursor))) return hr;
		if (FAILED(hr = ipEdgesFC->CreateFeatureBuffer(&ipFeatureBuffer))) return hr;

		// Query for the appropriate field index values in the "EdgeStat" feature class
		if (FAILED(hr = ipEdgesFC->FindField(ATL::CComBSTR(CS_FIELD_SOURCE_ID), &sourceIDFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(ATL::CComBSTR(CS_FIELD_SOURCE_OID), &sourceOIDFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(ATL::CComBSTR(CS_FIELD_DIR), &dirFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(ATL::CComBSTR(CS_FIELD_Congestion), &congestionFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(ATL::CComBSTR(CS_FIELD_TravCost), &travCostFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(ATL::CComBSTR(CS_FIELD_OrgCost), &orgCostFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(ATL::CComBSTR(CS_FIELD_EID), &eidFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(ATL::CComBSTR(CS_FIELD_ReservPop2), &resPopFieldIndex))) return hr;
		if (resPopFieldIndex < 1) { if (FAILED(hr = ipEdgesFC->FindField(ATL::CComBSTR(CS_FIELD_ReservPop1), &resPopFieldIndex))) return hr; }

		for (NAEdgeTableItr it = ecache->AlongBegin(); it != ecache->AlongEnd(); it++)
		{
			if (ipStepProgressor) ipStepProgressor->Step();
			// Check to see if the user wishes to continue or cancel the solve (i.e., check whether or not the user has hit the ESC key to stop processing)
			if (pTrackCancel)
			{
				if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
				if (keepGoing == VARIANT_FALSE) return E_ABORT;
			}

			edge = it->second;
			if (FAILED(hr = edge->InsertEdgeToFeatureCursor(ipNetworkDataset, ipFeatureClassContainer, ipFeatureBuffer, ipFeatureCursor, eidFieldIndex, sourceIDFieldIndex, sourceOIDFieldIndex, dirFieldIndex,
				                                            resPopFieldIndex, travCostFieldIndex, orgCostFieldIndex, congestionFieldIndex, sourceNotFoundFlag))) return hr;
		}

		for (NAEdgeTableItr it = ecache->AgainstBegin(); it != ecache->AgainstEnd(); it++)
		{
			if (ipStepProgressor) ipStepProgressor->Step();
			// Check to see if the user wishes to continue or cancel the solve (i.e., check whether or not the user has hit the ESC key to stop processing)
			if (pTrackCancel)
			{
				if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
				if (keepGoing == VARIANT_FALSE) return E_ABORT;
			}

			edge = it->second;
			if (FAILED(hr = edge->InsertEdgeToFeatureCursor(ipNetworkDataset, ipFeatureClassContainer, ipFeatureBuffer, ipFeatureCursor, eidFieldIndex, sourceIDFieldIndex, sourceOIDFieldIndex, dirFieldIndex,
				                                            resPopFieldIndex, travCostFieldIndex, orgCostFieldIndex, congestionFieldIndex, sourceNotFoundFlag))) return hr;
		}

		// flush the insert buffer
		ipFeatureCursor->Flush();
	}

	if (sourceNotFoundFlag) pMessages->AddWarning(ATL::CComBSTR(_T("A network source could not be found by source ID.")));

	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeE, &cpuTimeE);
	tenNanoSec64 = (*((__int64 *) &sysTimeE)) - (*((__int64 *) &sysTimeS));
	outputSecSys = tenNanoSec64 / 10000000.0;
	tenNanoSec64 = (*((__int64 *) &cpuTimeE)) - (*((__int64 *) &cpuTimeS));
	outputSecCpu = tenNanoSec64 / 10000000.0;
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeS, &cpuTimeS);

	//******************************************************************************************/
	// Perform flocking simulation if requested

	// At this stage we create many evacuee points within a flocking simulation environment to validate the calculated results
	ATL::CString collisionMsg, simulationIncompleteEndingMsg;
	std::vector<FlockingLocationPtr> * history = nullptr;
	std::list<double> * collisionTimes = nullptr;

	if (flockingEnabled == VARIANT_TRUE)
	{
		// Get the "Flocks" NAClass feature class
		IFeatureCursorPtr ipFeatureCursor;
		IFeatureBufferPtr ipFeatureBuffer;
		PathSegment * pathSegment;
		IFeatureClassPtr ipFlocksFC(ipFlocksNAClass);
		long nameFieldIndex, timeFieldIndex, traveledFieldIndex, speedXFieldIndex, speedYFieldIndex, idFieldIndex, speedFieldIndex, costFieldIndex, statFieldIndex, ptimeFieldIndex;
		time_t baseTime = time(NULL), thisTime = 0;
		bool movingObjectLeft;
		wchar_t thisTimeBuf[25];
		tm local;
		ATL::CComVariant featureID(0);
		EvcPathPtr path;

		// project to Mercator for the simulator
		for (const auto & currentEvacuee : *Evacuees)
		{
			// get all points from the stack and make one polyline from them. this will be the path.
			if (!currentEvacuee->Paths->empty())
			{
				for (tpit = currentEvacuee->Paths->begin(); tpit != currentEvacuee->Paths->end(); tpit++)
				{
					path = *tpit;
					for (auto psit = path->cbegin(); psit != path->cend(); ++psit)
					{
						pathSegment = *psit;
						if (FAILED(hr = pathSegment->pline->Project(ipNAContextPC))) return hr;
					}
				}
			}
		}

		// init
		if (FAILED(hr = ipStepProgressor->put_Position(0))) return hr;
		if (ipStepProgressor) ipStepProgressor->put_Message(ATL::CComBSTR(L"Initializing flocking environment"));
		auto flock = std::shared_ptr<FlockingEnviroment>(new DEBUG_NEW_PLACEMENT FlockingEnviroment(flockingSnapInterval, flockingSimulationInterval, initDelayCostPerPop));

		// run simulation
		try
		{
			flock->Init(Evacuees, ipNetworkQuery, &flockProfile, twoWayShareCapacity == VARIANT_TRUE);
			if (ipStepProgressor) ipStepProgressor->put_Message(ATL::CComBSTR(L"Running flocking simulation"));
			if (FAILED(hr = flock->RunSimulation(ipStepProgressor, pTrackCancel, globalEvcCost))) return hr;
		}
		catch(const std::exception & e)
		{
			ATL::CComBSTR ccombstrErr("Critical error during flocking simulation: ");
			hr = ccombstrErr.Append(e.what());
			pMessages->AddError(-1, ccombstrErr);
		}

		// retrieve results even if it's empty or error
		flock->GetResult(&history, &collisionTimes, &movingObjectLeft);

		// project back to analysis coordinate system
		for (const auto & currentEvacuee : *Evacuees)
		{
			for (tpit = currentEvacuee->Paths->begin(); tpit != currentEvacuee->Paths->end(); tpit++)
			{
				path = *tpit;
				for (auto psit = path->cbegin(); psit != path->cend(); ++psit)
				{
					pathSegment = *psit;
					if (FAILED(hr = pathSegment->pline->Project(ipNAContextSR))) return hr;
				}
			}
		}

		// start writing into the feature class
		if (ipStepProgressor)
		{
			ipStepProgressor->put_Message(ATL::CComBSTR(L"Writing flocking results"));
			ipStepProgressor->put_MinRange(0);
			ipStepProgressor->put_MaxRange((long)(history->size()));
			ipStepProgressor->put_StepValue(1);
			ipStepProgressor->put_Position(0);
		}

		// Create an insert cursor and feature buffer from the "Flocks" feature class to be used to write edges
		if (FAILED(hr = ipFlocksFC->Insert(VARIANT_TRUE, &ipFeatureCursor))) return hr;
		if (FAILED(hr = ipFlocksFC->CreateFeatureBuffer(&ipFeatureBuffer))) return hr;

		// Query for the appropriate field index values in the "EdgeStat" feature class
		if (FAILED(hr = ipFlocksFC->FindField(ATL::CComBSTR(CS_FIELD_NAME), &nameFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(ATL::CComBSTR(CS_FIELD_ID), &idFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(ATL::CComBSTR(CS_FIELD_COST), &costFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(ATL::CComBSTR(CS_FIELD_TRAVELED), &traveledFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(ATL::CComBSTR(CS_FIELD_VelocityX), &speedXFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(ATL::CComBSTR(CS_FIELD_VelocityY), &speedYFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(ATL::CComBSTR(CS_FIELD_SPEED), &speedFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(ATL::CComBSTR(CS_FIELD_TIME), &timeFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(ATL::CComBSTR(CS_FIELD_PTIME), &ptimeFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(ATL::CComBSTR(CS_FIELD_STATUS), &statFieldIndex))) return hr;

		for(FlockingLocationItr it = history->begin(); it != history->end(); it++)
		{
			if (pTrackCancel)
			{
				if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
				if (keepGoing == VARIANT_FALSE) return E_ABORT;
			}

			// generate time as Unicode string
			thisTime = baseTime + time_t((*it)->GTime / costPerSec);
			localtime_s(&local, &thisTime);
			wcsftime(thisTimeBuf, 25, L"%Y/%m/%d %H:%M:%S", &local);

			// Store the feature values on the feature buffer
			if (FAILED(hr = (*it)->MyLocation->Project(ipNAContextSR))) return hr;
			if (FAILED(hr = ipFeatureBuffer->putref_Shape((*it)->MyLocation))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(idFieldIndex, ATL::CComVariant((*it)->ID)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(nameFieldIndex, (*it)->GroupName))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(costFieldIndex, ATL::CComVariant((*it)->MyTime)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(traveledFieldIndex, ATL::CComVariant((*it)->Traveled)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(speedXFieldIndex, ATL::CComVariant((*it)->Velocity.x)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(speedYFieldIndex, ATL::CComVariant((*it)->Velocity.y)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(speedFieldIndex, ATL::CComVariant((*it)->Velocity.length())))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(timeFieldIndex, ATL::CComVariant(thisTimeBuf)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(ptimeFieldIndex, ATL::CComVariant((*it)->GTime / (costPerSec * 60.0))))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(statFieldIndex, ATL::CComVariant(static_cast<unsigned char>((*it)->MyStatus))))) return hr;

			// Insert the feature buffer in the insert cursor
			if (FAILED(hr = ipFeatureCursor->InsertFeature(ipFeatureBuffer, &featureID))) return hr;
			if (ipStepProgressor) ipStepProgressor->Step();
		}

		// incomplete ending message
		simulationIncompleteEndingMsg.Empty();

		// message about simulation time
		if (movingObjectLeft)
		{
			simulationIncompleteEndingMsg = _T("Max simulation time reached therefore not all objects get to a safe area. Probably the predicted evacuation time was too low.");

			// generate a new row indicating an incomplete simulation
			thisTime = baseTime + time_t(0);
			localtime_s(&local, &thisTime);
			wcsftime(thisTimeBuf, 25, L"%Y/%m/%d %H:%M:%S", &local);
			FlockingLocationItr it = history->begin();

			// Store the feature values on the feature buffer
			if (FAILED(hr = ipFeatureBuffer->putref_Shape((*it)->MyLocation))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(idFieldIndex, ATL::CComVariant(0)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(nameFieldIndex, ATL::CComVariant("0")))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(costFieldIndex, ATL::CComVariant(99999)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(traveledFieldIndex, ATL::CComVariant(0)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(speedXFieldIndex, ATL::CComVariant(0)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(speedYFieldIndex, ATL::CComVariant(0)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(speedFieldIndex, ATL::CComVariant(0)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(timeFieldIndex, ATL::CComVariant(thisTimeBuf)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(ptimeFieldIndex, ATL::CComVariant(99999)))) return hr;

			// print out the status
			if (FAILED(hr = ipFeatureBuffer->put_Value(statFieldIndex, ATL::CComVariant(_T("E"))))) return hr;

			// Insert the feature buffer in the insert cursor
			if (FAILED(hr = ipFeatureCursor->InsertFeature(ipFeatureBuffer, &featureID))) return hr;
		}

		// message about collisions
		collisionMsg.Empty();
		if (collisionTimes && collisionTimes->size() > 0)
		{
			for (std::list<double>::const_iterator ct = collisionTimes->begin(); ct != collisionTimes->end(); ct++)
			{
				if (collisionMsg.IsEmpty()) collisionMsg.AppendFormat(_T("%.3f"), *ct);
				else collisionMsg.AppendFormat(_T(", %.3f"), *ct);
			}
		}

		// flush the insert buffer and release
		ipFeatureCursor->Flush();
	}

	// timing
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeE, &cpuTimeE);
	tenNanoSec64 = (*((__int64 *) &sysTimeE)) - (*((__int64 *) &sysTimeS));
	flockSecSys = tenNanoSec64 / 10000000.0;
	tenNanoSec64 = (*((__int64 *) &cpuTimeE)) - (*((__int64 *) &cpuTimeS));
	flockSecCpu = tenNanoSec64 / 10000000.0;

	//******************************************************************************************/
	// Close it and clean it
	ATL::CString performanceMsg, CARMALoopMsg, ZeroHurMsg, CARMAExtractsMsg, CacheHitMsg, initMsg, iterationMsg1, iterationMsg2;
	size_t mem = (peakMemoryUsage - baseMemoryUsage) / 1048576;

	initMsg.Format(_T("%s(%s) version %s. %d routes are generated from the evacuee points. %d evacuee(s) were unreachable."), PROJ_NAME, PROJ_ARCH, _T(GIT_DESCRIBE), tempPathList.size(), StuckEvacuee);
	CARMALoopMsg.Format(_T("The algorithm performed %d CARMA loop(s) in %.2f seconds. Peak memory usage (exclude flocking) was %d MB."), CARMAExtractCounts.size(), carmaSec, max(0, mem));
	CacheHitMsg.Format(_T("Traffic model calculation had %.2f%% cache hit."), ecache->GetCacheHitPercentage());

	performanceMsg.Format(_T("Timing: Input = %.2f (kernel), %.2f (user); Calculation = %.2f (kernel), %.2f (user); Output = %.2f (kernel), %.2f (user); Flocking = %.2f (kernel), %.2f (user); Total = %.2f"),
		inputSecSys, inputSecCpu, calcSecSys, calcSecCpu, outputSecSys, outputSecCpu, flockSecSys, flockSecCpu,
		inputSecSys + inputSecCpu + calcSecSys + calcSecCpu + flockSecSys + flockSecCpu + outputSecSys + outputSecCpu);

	std::stringstream ss;
	ss.imbue(std::locale(""));
	if (CARMAExtractCounts.size() > 0)
	{
		CARMAExtractsMsg.Format(_T("The following is the number of CARMA heap extracts in each loop: "));
		for (std::vector<unsigned int>::size_type i = 0; i < CARMAExtractCounts.size(); ++i)
		{
			if (i == 0) ss << CARMAExtractCounts[0];
			else        ss << " | " << CARMAExtractCounts[i];
		}
		CARMAExtractsMsg.Append(ATL::CString(ss.str().c_str()));
	}
	if (GlobalEvcCostAtIteration.size() == 1)
	{
		iterationMsg1.Format(_T("The program ran for 1 iteration. Evacuation cost at the end is: %.2f"), GlobalEvcCostAtIteration[0]);
	}
	else if (GlobalEvcCostAtIteration.size() > 1)
	{
		iterationMsg1.Format(_T("The program ran for %d iterations. Evacuation costs at each iteration are: %.2f"), GlobalEvcCostAtIteration.size(), GlobalEvcCostAtIteration[0]);
		for (size_t i = 1; i < GlobalEvcCostAtIteration.size(); ++i) iterationMsg1.AppendFormat(_T(", %.2f"), GlobalEvcCostAtIteration[i]);
		if (!EffectiveIterationRatio.empty())
		{
			iterationMsg2.Format(_T("The effective iteration ratio at each pass: %.3f"), EffectiveIterationRatio[0]);
			for (size_t i = 1; i < EffectiveIterationRatio.size(); ++i) iterationMsg2.AppendFormat(_T(", %.3f"), EffectiveIterationRatio[i]);
		}
	}

	pMessages->AddMessage(ATL::CComBSTR(initMsg));
	pMessages->AddMessage(ATL::CComBSTR(performanceMsg));
	pMessages->AddMessage(ATL::CComBSTR(CARMALoopMsg));
	if (!CARMAExtractsMsg.IsEmpty()) pMessages->AddMessage(ATL::CComBSTR(CARMAExtractsMsg));
	pMessages->AddMessage(ATL::CComBSTR(iterationMsg1));
	if (!iterationMsg2.IsEmpty()) pMessages->AddMessage(ATL::CComBSTR(iterationMsg2));
	if (ecache->GetCacheHitPercentage() < 80.0) pMessages->AddMessage(ATL::CComBSTR(CacheHitMsg));

	if (EvacueesWithRestrictedSafezone > 0)
	{
		ATL::CString RestrictedWarning;
		RestrictedWarning.Format(_T("For %d evacuee(s), a route could not be found because all reachable safe zones were restricted. Make sure you have some non-restricted safe zones with a non-zero capacity."), EvacueesWithRestrictedSafezone);
		pMessages->AddWarning(ATL::CComBSTR(RestrictedWarning));
	}

	if (!(simulationIncompleteEndingMsg.IsEmpty())) pMessages->AddWarning(ATL::CComBSTR(simulationIncompleteEndingMsg));
	if (IsSafeZoneMissed) pMessages->AddWarning(ATL::CComBSTR(
		L"One or more safe zones where snapped into the same network junction and hence they were merged into one safe zone. If this is not OK, use a different Network Location setting."));

	if (!(collisionMsg.IsEmpty()))
	{
		collisionMsg.Insert(0, _T("Some collisions have been reported at the following intervals: "));
		pMessages->AddWarning(ATL::CComBSTR(collisionMsg));
	}

	// check if user wants a dynamic CASPER but we don't have the dynamic change table
	if (!DynamicTableExist && CASPERDynamicMode != DynamicMode::Disabled)
	{
		CASPERDynamicMode = DynamicMode::Disabled;
		pMessages->AddWarning(ATL::CComBSTR(_T("You have enabled the dynamic CASPER mode but the network analysis layer does not have the DynamicChanges feature class.")));
	}
	if (disasterTable->GetDynamicMode() > DynamicMode::Simple && exportEdgeStat)
		pMessages->AddMessage(ATL::CComBSTR(_T("You have enabled the dynamic CASPER mode and export edge statistics. The exported statistics would be the final edge status (at infinite time) and does not reflect the intermediate edge congestions. The Routes layer however does reflect the most accurate congestions.")));

	if (disasterTable->GetDynamicMode() > DynamicMode::Simple && flockingEnabled == VARIANT_TRUE)
		pMessages->AddWarning(ATL::CComBSTR(_T("You have enabled the dynamic CASPER mode and flocking simulation. The simulation does not honor the dynamic changes and hence the results will not necessarily comply.")));
	
	if (Evacuees->IsSeperationDisabledForDynamicCASPER())
		pMessages->AddWarning(ATL::CComBSTR(_T("You have enabled the dynamic CASPER mode and evacuee seperation feature. They are not compatible so evacuee seperation has been temporarily disabled.")));
	
	if (flagBadDynamicChangeSnapping)
		pMessages->AddWarning(ATL::CComBSTR(_T("You have snapped some or all of DynamicChange polygons to vertices instead of edges and hence I cannot apply them properly. They have been ignored.")));

	// since vertices inside the cache are still pointing to some edges it's safer to clean them first
	vcache = nullptr;
	ecache = nullptr;
	
	return hr;
}

double GetUnitPerDay(esriNetworkAttributeUnits unit, double assumedSpeed)
{
	double costPerDay = 1.0;

	switch (unit)
	{
	case esriNAUSeconds:
		costPerDay = 3600.0 * 24.0;
	case esriNAUMinutes:
		costPerDay = 60.0 * 24.0;
		break;
	case esriNAUHours:
		costPerDay = 24.0;
		break;
	case esriNAUInches:
		costPerDay = 39.370079 * assumedSpeed * 3600.0 * 24.0;
		break;
	case esriNAUFeet:
		costPerDay = 3.28084 * assumedSpeed * 3600.0 * 24.0;
		break;
	case esriNAUYards:
		costPerDay = 1.093613 * assumedSpeed * 3600.0 * 24.0;
		break;
	case esriNAUMiles:
		costPerDay = 0.000621 * assumedSpeed * 3600.0 * 24.0;
		break;
	case esriNAUMillimeters:
		costPerDay = 1000.0 * assumedSpeed * 3600.0 * 24.0;
		break;
	case esriNAUCentimeters:
		costPerDay = 100.0 * assumedSpeed * 3600.0 * 24.0;
		break;
	case esriNAUMeters:
		costPerDay = assumedSpeed * 3600.0 * 24.0;
		break;
	case esriNAUKilometers:
		costPerDay = 0.001 * assumedSpeed * 3600.0 * 24.0;
		break;
	case esriNAUDecimeters:
		costPerDay = 0.1 * assumedSpeed * 3600.0 * 24.0;
	}

	return costPerDay;
}
