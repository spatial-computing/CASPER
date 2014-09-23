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
#include "NameConstants.h"
#include "EvcSolver.h"
#include "FibonacciHeap.h"
#include "Flocking.h"

STDMETHODIMP EvcSolver::Solve(INAContext* pNAContext, IGPMessages* pMessages, ITrackCancel* pTrackCancel, VARIANT_BOOL* pIsPartialSolution)
{
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
	time_t curr = time(0);
	char timeBuff[50];
	ctime_s(timeBuff, 50, &curr);
	f << "Trace start: " << timeBuff << std::endl;
	f.close();
	#endif

	HRESULT hr = S_OK;
	double globalEvcCost = -1.0, carmaSec = 0.0;
	unsigned int EvacueesWithRestrictedSafezone = 0;

	// init memory usage function and set the base
	peakMemoryUsage = 0l;
	hProcessPeakMemoryUsage = NULL;
	UpdatePeakMemoryUsage();
	SIZE_T baseMemoryUsage = peakMemoryUsage;

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

	if (!ipNetworkDataset) return AtlReportError(GetObjectCLSID(), _T("Context does not have a valid network dataset."), IID_INASolver);

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
	double inputSecSys, calcSecSys, flockSecSys, outputSecSys, inputSecCpu, calcSecCpu, flockSecCpu, outputSecCpu;
	__int64 tenNanoSec64;

	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeS, &cpuTimeS);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Reset each NAClass to an appropriate state (as necessary) before proceeding
	// This is typically done in order to:
	// 1) remove any features that were previously created in our output NAClasses from previous solves
	// 2) check the input NAClasses for output fields and reset these field values as necessary
	// In our case, we do not have any input NAClasses with output fields, and we only have one output feature class, 
	// so we can simply clear any features currently present in the "LineData" NAClass. 
	// NOTE: if you have multiple input/output NAClasses to clean up, you can simply loop through all NAClasses,
	// get their NAClassDefinition, check whether or not each is an input/output class, and reset it accordingly
	INamedSetPtr ipNAClasses;
	if (FAILED(hr = pNAContext->get_NAClasses(&ipNAClasses))) return hr;

	// remove any features that might have been created on previous solves
	IUnknownPtr ipUnk;
	if (FAILED(hr = ipNAClasses->get_ItemByName(CComBSTR(CS_ROUTES_NAME), &ipUnk))) return hr;
	INAClassPtr ipRoutesNAClass(ipUnk);
	if (FAILED(hr = ipRoutesNAClass->DeleteAllRows())) return hr;

	if (FAILED(hr = ipNAClasses->get_ItemByName(CComBSTR(CS_EDGES_NAME), &ipUnk))) return hr;
	INAClassPtr ipEdgesNAClass(ipUnk);
	if (FAILED(hr = ipEdgesNAClass->DeleteAllRows())) return hr;

	ipUnk = NULL;
	if (FAILED(hr = ipNAClasses->get_ItemByName(CComBSTR(CS_ROUTEEDGES_NAME), &ipUnk))) return hr;
	bool DoNotExportRouteEdges = ipUnk == NULL;
	INAClassPtr ipRouteEdgesNAClass(ipUnk);
	if (!DoNotExportRouteEdges) if (FAILED(hr = ipRouteEdgesNAClass->DeleteAllRows())) return hr;

	ipUnk = NULL;
	if (FAILED(hr = ipNAClasses->get_ItemByName(CComBSTR(CS_FLOCKS_NAME), &ipUnk))) return hr;
	INAClassPtr ipFlocksNAClass(ipUnk);
	if (flockingEnabled == VARIANT_TRUE && ipUnk == NULL) flockingEnabled = VARIANT_FALSE;
	if (ipFlocksNAClass != NULL) { if (FAILED(hr = ipFlocksNAClass->DeleteAllRows())) return hr; }

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
	ITablePtr ipBarriersTable;
	if (FAILED(hr = GetNAClassTable(pNAContext, CComBSTR(CS_BARRIERS_NAME), &ipBarriersTable))) return hr;

	// Load the barriers
	if (FAILED(hr = LoadBarriers(ipBarriersTable, ipNetworkQuery, ipForwardStar))) return hr;
	if (FAILED(hr = LoadBarriers(ipBarriersTable, ipNetworkQuery, ipBackwardStar))) return hr;

	INetworkAttribute2Ptr networkAttrib = 0;
	VARIANT_BOOL useRestriction;

	// loading restriction attributes into the forward star. this will enforce all available restrictions.
	for (std::vector<INetworkAttribute2Ptr>::iterator Iter = turnAttribs.begin(); Iter != turnAttribs.end(); Iter++)
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
	if (FAILED(hr = GetNAClassTable(pNAContext, CComBSTR(CS_EVACUEES_NAME), &ipEvacueePointsTable))) return hr;
	// Same for safe zone points
	ITablePtr ipZonesTable;
	if (FAILED(hr = GetNAClassTable(pNAContext, CComBSTR(CS_ZONES_NAME), &ipZonesTable))) return hr;

	// Create variables for looping through the cursor and traversing the network
	IRowESRI * ipRow;
	ICursorPtr ipCursor;
	INALocationObjectPtr ipNALocationObject;
	INALocationPtr ipNALocation(CLSID_NALocation);
	IEnumNetworkElementPtr ipEnumNetworkElement;

	INetworkEdgePtr ipCurrentEdge;
	INetworkJunctionPtr ipCurrentJunction;

	INetworkElementPtr ipElement, ipOtherElement;
	ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement);
	ipCurrentEdge = ipElement;
	ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipElement);
	ipCurrentJunction = ipElement;

	long sourceOID, sourceID;
	double posAlong, fromPosition, toPosition;
	VARIANT_BOOL keepGoing, isLocated, isRestricted;
	esriNetworkElementType elementType;
	SafeZonePtr safePoint;

	// Initialize Caches
	// This cache will maintain a list of all created vertices/edges. You can retrieve
	// them later using EID. The benefit of using this cache is that we
	// can maintain one-to-one relationship between network junctions and vertices.
	// This will particularly be helpful with the heuristic calculator part of the algorithm.
	// if (FAILED(hr = heuristicAttribs[heuristicAttribIndex]->get_ID(&capAttributeID))) return hr;  	
	NAVertexCache * vcache = new DEBUG_NEW_PLACEMENT NAVertexCache();
	NAEdgeCache   * ecache = new DEBUG_NEW_PLACEMENT NAEdgeCache(capAttributeID, costAttributeID, SaturationPerCap, CriticalDensPerCap, twoWayShareCapacity == VARIANT_TRUE,
		initDelayCostPerPop, trafficModel, ipForwardStar, ipBackwardStar, ipNetworkQuery, hr);
	if (FAILED(hr)) return hr;

	// Vertex table structures
	SafeZoneTable * safeZoneList = new DEBUG_NEW_PLACEMENT SafeZoneTable();
	SafeZoneTableInsertReturn insertRet;
	INetworkEdgePtr ipEdge(ipElement);
	INetworkEdgePtr ipOtherEdge(ipOtherElement);
	long nameFieldIndex = 0l, popFieldIndex = 0l, capFieldIndex = 0l, objectID;
	VARIANT evName, pop, cap;

	if (ipStepProgressor) ipStepProgressor->put_Message(CComBSTR(L"Collecting input point(s)")); // add more specific information here if appropriate

	///////////////////////////
	// here we begin collecting safe zone points for all the evacuees

	// Get a cursor on the zones table to loop through each row
	if (FAILED(hr = ipZonesTable->FindField(CComBSTR(CS_FIELD_CAP), &capFieldIndex))) return hr;
	if (FAILED(hr = ipZonesTable->Search(0, VARIANT_TRUE, &ipCursor))) return hr;
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

			// Once we have a located NALocation, we query the network to obtain its associated network elements
			if (FAILED(hr = ipNetworkQuery->get_ElementsByOID(sourceID, sourceOID, &ipEnumNetworkElement))) return hr;
			
			if (FAILED(hr = ipRow->get_Value(capFieldIndex, &cap))) return hr;

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
					if (!isRestricted)
					{
						safePoint = new DEBUG_NEW_PLACEMENT SafeZone(ipElement, 0, 0, cap);
						insertRet = safeZoneList->insert(SafeZoneTablePair(safePoint));
						if (!insertRet.second) delete safePoint;
					}
				}

				// If the element is an edge, then we must check the fromPosition and toPosition
				if (elementType == esriNETEdge)
				{
					if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipOtherElement))) return hr;
					ipOtherEdge = ipOtherElement;
					ipEdge = ipElement;
					if (FAILED(hr = ipEdge->QueryPositions(&fromPosition, &toPosition))) return hr;

					if (fromPosition <= posAlong && posAlong <= toPosition)
					{
						// Our NALocation lies along this edge element
						// We will start our traversal from the junctions of this edge
						// and then we check the other edge in the opposite direction

						if (FAILED(hr = ipForwardStar->get_IsRestricted(ipEdge, &isRestricted))) return hr;
						if (!isRestricted)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
							ipCurrentJunction = ipOtherElement;
							if (FAILED(hr = ipEdge->QueryJunctions(ipCurrentJunction, 0))) return hr;

							safePoint = new DEBUG_NEW_PLACEMENT SafeZone(ipCurrentJunction, ecache->New(ipEdge, true), posAlong, cap);
							insertRet = safeZoneList->insert(SafeZoneTablePair(safePoint));
							if (!insertRet.second) delete safePoint;
						}
					}

					if (FAILED(hr = ipEdge->QueryEdgeInOtherDirection(ipOtherEdge))) return hr;		  
					if (FAILED(hr = ipOtherEdge->QueryPositions(&toPosition, &fromPosition))) return hr;

					if (fromPosition <= posAlong && posAlong <= toPosition)
					{
						if (FAILED(hr = ipForwardStar->get_IsRestricted(ipOtherEdge, &isRestricted))) return hr;
						if (!isRestricted)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
							ipCurrentJunction = ipOtherElement;
							if (FAILED(hr = ipOtherEdge->QueryJunctions(ipCurrentJunction, 0))) return hr;						

							safePoint = new DEBUG_NEW_PLACEMENT SafeZone(ipCurrentJunction, ecache->New(ipOtherEdge, true), toPosition - posAlong, cap);
							insertRet = safeZoneList->insert(SafeZoneTablePair(safePoint));
							if (!insertRet.second) delete safePoint;
						}
					}
				}
			}
		}
	}
	
	// Get a cursor on the Evacuee points table to loop through each row
	if (FAILED(hr = ipEvacueePointsTable->Search(0, VARIANT_TRUE, &ipCursor))) return hr;
	EvacueeList * Evacuees = new DEBUG_NEW_PLACEMENT EvacueeList();
	Evacuee * currentEvacuee;
	NAVertexPtr myVertex;

	// pre-process evacuee NALayer primary field index
	if (FAILED(hr = ipEvacueePointsTable->FindField(CComBSTR(CS_FIELD_NAME), &nameFieldIndex))) return hr;
	if (FAILED(hr = ipEvacueePointsTable->FindField(CComBSTR(CS_FIELD_EVC_POP), &popFieldIndex))) return hr;


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

			// Once we have a located NALocation, we query the network to obtain its associated network elements
			if (FAILED(hr = ipNetworkQuery->get_ElementsByOID(sourceID, sourceOID, &ipEnumNetworkElement))) return hr;

			// We must loop through the returned elements, looking for an appropriate starting point
			ipEnumNetworkElement->Reset();

			// Get the OID of the evacuee NALocation
			if (FAILED(hr = ipRow->get_Value(nameFieldIndex, &evName))) return hr;
			if (FAILED(hr = ipRow->get_Value(popFieldIndex, &pop))) return hr;
			if (FAILED(hr = ipRow->get_OID(&objectID))) return hr;
			currentEvacuee = new DEBUG_NEW_PLACEMENT Evacuee(evName, pop.dblVal, objectID);

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
						myVertex = new DEBUG_NEW_PLACEMENT NAVertex(ipElement, 0);
						currentEvacuee->vertices->insert(currentEvacuee->vertices->end(), myVertex);
					}
				}

				// If the element is an edge, then we must check the fromPosition and toPosition to be certain it holds an appropriate starting point 
				if (elementType == esriNETEdge)
				{
					if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipOtherElement))) return hr;
					ipOtherEdge = ipOtherElement;
					ipEdge = ipElement;
					if (FAILED(hr = ipEdge->QueryPositions(&fromPosition, &toPosition))) return hr;

					if (fromPosition <= posAlong && posAlong <= toPosition)
					{
						// Our NALocation lies along this edge element
						// We will start our traversal from the junctions of this edge
						// and then we check the other edge in the opposite direction

						if (FAILED(hr = ipForwardStar->get_IsRestricted(ipEdge, &isRestricted))) return hr;
						if (!isRestricted)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
							ipCurrentJunction = ipOtherElement;
							if (FAILED(hr = ipEdge->QueryJunctions(0, ipCurrentJunction))) return hr;

							myVertex = new DEBUG_NEW_PLACEMENT NAVertex(ipCurrentJunction, ecache->New(ipEdge, true));
							myVertex->GVal = (toPosition - posAlong) * myVertex->GetBehindEdge()->OriginalCost;
							currentEvacuee->vertices->insert(currentEvacuee->vertices->end(), myVertex);
						}
					}

					if (FAILED(hr = ipEdge->QueryEdgeInOtherDirection(ipOtherEdge))) return hr;		  
					if (FAILED(hr = ipOtherEdge->QueryPositions(&toPosition, &fromPosition))) return hr;

					if (fromPosition <= posAlong && posAlong <= toPosition)
					{
						if (FAILED(hr = ipForwardStar->get_IsRestricted(ipOtherEdge, &isRestricted))) return hr;
						if (!isRestricted)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
							ipCurrentJunction = ipOtherElement;
							if (FAILED(hr = ipOtherEdge->QueryJunctions(0, ipCurrentJunction))) return hr;

							myVertex = new DEBUG_NEW_PLACEMENT NAVertex(ipCurrentJunction, ecache->New(ipOtherEdge, true));
							myVertex->GVal = posAlong * myVertex->GetBehindEdge()->OriginalCost;
							currentEvacuee->vertices->insert(currentEvacuee->vertices->end(), myVertex);							
						}
					}
				}
			}
			if (currentEvacuee->vertices->size() > 0) Evacuees->insert(Evacuees->end(), currentEvacuee);
			else delete currentEvacuee;
		}
	}

	// timing
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeE, &cpuTimeE);
	tenNanoSec64 = (*((__int64 *) &sysTimeE)) - (*((__int64 *) &sysTimeS));
	inputSecSys = tenNanoSec64 / 10000000.0;
	tenNanoSec64 = (*((__int64 *) &cpuTimeE)) - (*((__int64 *) &cpuTimeS));
	inputSecCpu = tenNanoSec64 / 10000000.0;
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeS, &cpuTimeS);

	if (ipStepProgressor) if (FAILED(hr = ipStepProgressor->Show())) return hr;
	std::vector<unsigned int> CARMAExtractCounts;

	///////////////////////////////////////
	// this will call the core part of the algorithm.
	try
	{
		hr = S_OK;
		UpdatePeakMemoryUsage();
		hr = SolveMethod(ipNetworkQuery, pMessages, pTrackCancel, ipStepProgressor, Evacuees, vcache, ecache, safeZoneList, pIsPartialSolution, carmaSec, CARMAExtractCounts, ipNetworkDataset, EvacueesWithRestrictedSafezone);
	}
	catch (std::exception & ex)
	{
		hr = -1L * abs(ERROR_UNHANDLED_EXCEPTION);
		(ex);
		#ifdef TRACE
		std::ofstream f;
		f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
		f << "Search throws: " << ex.what() << std::endl;
		f.close();
		#endif
		#ifdef DEBUG
		std::wostringstream os_;
		os_ << "Search throws: " << ex.what() << std::endl;
		OutputDebugStringW( os_.str().c_str() );
		_ASSERT(0);
		#endif
	}

	if (FAILED(hr))
	{
		// it either failed or canceled. clean up and then return.
		for(EvacueeListItr evcItr = Evacuees->begin(); evcItr != Evacuees->end(); evcItr++) delete (*evcItr);
		Evacuees->clear();
		delete Evacuees;

		// releasing all internal objects
		delete vcache;
		delete ecache;
		for (SafeZoneTableItr it = safeZoneList->begin(); it != safeZoneList->end(); it++) delete it->second;
		delete safeZoneList;

		#ifdef TRACE
		std::ofstream f;
		f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
		f << "Search exit: " << hr << std::endl;
		f.close();
		#endif

		return hr;
	}

	// timing
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeE, &cpuTimeE);
	tenNanoSec64 = (*((__int64 *) &sysTimeE)) - (*((__int64 *) &sysTimeS));
	calcSecSys = tenNanoSec64 / 10000000.0;
	tenNanoSec64 = (*((__int64 *) &cpuTimeE)) - (*((__int64 *) &cpuTimeS));
	calcSecCpu = tenNanoSec64 / 10000000.0;
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeS, &cpuTimeS);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Write output

	// Now that we have completed our traversal of the network from the Evacuee points, we must output the connected/disconnected edges
	// to the "LineData" NAClass

	// Setup a message on our step progress bar indicating that we are outputting feature information
	if (ipStepProgressor) ipStepProgressor->put_Message(CComBSTR(L"Writing output features")); 

	// looping through processed evacuees and generate routes in output feature class
	CComVariant featureID(0);
	EvcPathPtr path;
	std::list<EvcPathPtr>::iterator tpit;
	std::vector<EvcPathPtr>::iterator pit;
	double predictedCost = 0.0;
	bool sourceNotFoundFlag = false;
	IFeatureClassContainerPtr ipFeatureClassContainer(ipNetworkDataset);	
	std::vector<EvacueePtr>::iterator eit;
	
	// load the Mercator projection and analysis projection
	ISpatialReferencePtr ipNAContextSR;
	if (FAILED(hr = pNAContext->get_SpatialReference(&ipNAContextSR))) return hr;

	IProjectedCoordinateSystemPtr ipNAContextPC;
	ISpatialReferenceFactoryPtr pSpatRefFact = ISpatialReferenceFactoryPtr(CLSID_SpatialReferenceEnvironment);
	if (FAILED(hr = pSpatRefFact->CreateProjectedCoordinateSystem(esriSRProjCS_WGS1984WorldMercator, &ipNAContextPC))) return hr;
	ISpatialReferencePtr ipSpatialRef = ipNAContextPC;
	std::vector<EvcPathPtr> * tempPathList = new DEBUG_NEW_PLACEMENT std::vector<EvcPathPtr>(Evacuees->size());
	tempPathList->clear();

	for(eit = Evacuees->begin(); eit != Evacuees->end(); eit++)
	{
		// get all points from the stack and make one polyline from them. this will be the path.
		currentEvacuee = *eit;

		if (currentEvacuee->paths->empty())
		{
			*pIsPartialSolution = VARIANT_TRUE;
		}
		else
		{
			for (tpit = currentEvacuee->paths->begin(); tpit != currentEvacuee->paths->end(); tpit++) tempPathList->push_back(*tpit);			
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
			if (FAILED(hr = ipStepProgressor->put_MaxRange((long)(ecache->Size() + tempPathList->size())))) return hr;
		}
		else
		{
			if (FAILED(hr = ipStepProgressor->put_MaxRange((long)(tempPathList->size())))) return hr;
		}
		if (FAILED(hr = ipStepProgressor->put_StepValue(1))) return hr;
		if (FAILED(hr = ipStepProgressor->put_Position(0))) return hr;
	}

	std::sort(tempPathList->begin(), tempPathList->end(), EvcPath::LessThan);

	// Get the "Routes" NAClass feature class
	IFeatureClassPtr ipRoutesFC(ipRoutesNAClass);
	IFeatureClassPtr ipRouteEdgesFC(ipRouteEdgesNAClass);	

	// Create an insert cursor and feature buffer from the "Routes" feature class to be used to write routes
	IFeatureCursorPtr ipFeatureCursorR, ipFeatureCursorE, ipFeatureCursorU;
	if (FAILED(hr = ipRoutesFC->Insert(VARIANT_TRUE, &ipFeatureCursorR))) return hr;
	if (!DoNotExportRouteEdges) if (FAILED(hr = ipRouteEdgesFC->Insert(VARIANT_TRUE, &ipFeatureCursorE))) return hr;

	IFeatureBufferPtr ipFeatureBufferR, ipFeatureBufferE;
	if (FAILED(hr = ipRoutesFC->CreateFeatureBuffer(&ipFeatureBufferR))) return hr;
	if (!DoNotExportRouteEdges) if (FAILED(hr = ipRouteEdgesFC->CreateFeatureBuffer(&ipFeatureBufferE))) return hr;

	// Query for the appropriate field index values in the "routes" feature class
	long evNameFieldIndex = -1, evacTimeFieldIndex = -1, orgTimeFieldIndex = -1, RIDFieldIndex = -1, ERRouteFieldIndex = -1, EREdgeFieldIndex = -1,
		ERSeqFieldIndex = -1, ERFromPosFieldIndex = -1, ERToPosFieldIndex = -1, ERCostFieldIndex = -1, EREdgeDirFieldIndex = -1;
	if (FAILED(hr = ipRoutesFC->FindField(CComBSTR(CS_FIELD_EVC_NAME), &evNameFieldIndex))) return hr;
	if (FAILED(hr = ipRoutesFC->FindField(CComBSTR(CS_FIELD_E_TIME), &evacTimeFieldIndex))) return hr;
	if (FAILED(hr = ipRoutesFC->FindField(CComBSTR(CS_FIELD_E_ORG), &orgTimeFieldIndex))) return hr;
	if (FAILED(hr = ipRoutesFC->FindField(CComBSTR(CS_FIELD_E_POP), &popFieldIndex))) return hr;
	if (FAILED(hr = ipRoutesFC->FindField(CComBSTR(CS_FIELD_RID), &RIDFieldIndex))) return hr;
	if (!DoNotExportRouteEdges)
	{
		if (FAILED(hr = ipRouteEdgesFC->FindField(CComBSTR(CS_FIELD_RID), &ERRouteFieldIndex))) return hr;
		if (FAILED(hr = ipRouteEdgesFC->FindField(CComBSTR(CS_FIELD_EID), &EREdgeFieldIndex))) return hr;
		if (FAILED(hr = ipRouteEdgesFC->FindField(CComBSTR(CS_FIELD_SEQ), &ERSeqFieldIndex))) return hr;
		if (FAILED(hr = ipRouteEdgesFC->FindField(CComBSTR(CS_FIELD_FromP), &ERFromPosFieldIndex))) return hr;
		if (FAILED(hr = ipRouteEdgesFC->FindField(CComBSTR(CS_FIELD_ToP), &ERToPosFieldIndex))) return hr;
		if (FAILED(hr = ipRouteEdgesFC->FindField(CComBSTR(CS_FIELD_COST), &ERCostFieldIndex))) return hr;
		if (FAILED(hr = ipRouteEdgesFC->FindField(CComBSTR(CS_FIELD_DIR), &EREdgeDirFieldIndex))) return hr;
	}

	for (pit = tempPathList->begin(); pit != tempPathList->end(); pit++)
	{
		if (FAILED(hr = (*pit)->AddPathToFeatureBuffers(pTrackCancel, ipNetworkDataset, ipFeatureClassContainer, sourceNotFoundFlag, ipStepProgressor, globalEvcCost, initDelayCostPerPop, ipFeatureBufferR,
			ipFeatureBufferE, ipFeatureCursorR, ipFeatureCursorE, evNameFieldIndex, evacTimeFieldIndex, orgTimeFieldIndex, popFieldIndex, ERRouteFieldIndex, EREdgeFieldIndex, EREdgeDirFieldIndex, ERSeqFieldIndex,
			ERFromPosFieldIndex, ERToPosFieldIndex, ERCostFieldIndex, predictedCost, DoNotExportRouteEdges))) return hr;
	}

	// flush the insert buffer
	ipFeatureCursorR->Flush();
	if (!DoNotExportRouteEdges) ipFeatureCursorE->Flush();

	// copy all route OIDs to RouteID field
	if (RIDFieldIndex > -1)
	{
		IFeaturePtr routeFeature = NULL;
		long routeID = -1;
		if (FAILED(hr = ipRoutesFC->Update(NULL, VARIANT_TRUE, &ipFeatureCursorU))) return hr;
		if (FAILED(hr = ipFeatureCursorU->NextFeature(&routeFeature))) return hr;
		while (routeFeature)
		{
			if (FAILED(hr = routeFeature->get_OID(&routeID))) return hr;     // get OID
			if (FAILED(hr = routeFeature->put_Value(RIDFieldIndex, CComVariant(routeID)))) return hr;    // put OID as routeID
			if (FAILED(hr = ipFeatureCursorU->UpdateFeature(routeFeature))) return hr;    // put update back in table
			if (FAILED(hr = ipFeatureCursorU->NextFeature(&routeFeature))) return hr;     // for loop next feature
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	///// Exporting EdgeStat data to output featureClass

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
		if (FAILED(hr = ipEdgesFC->FindField(CComBSTR(CS_FIELD_SOURCE_ID), &sourceIDFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(CComBSTR(CS_FIELD_SOURCE_OID), &sourceOIDFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(CComBSTR(CS_FIELD_DIR), &dirFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(CComBSTR(CS_FIELD_ReservPop), &resPopFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(CComBSTR(CS_FIELD_Congestion), &congestionFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(CComBSTR(CS_FIELD_TravCost), &travCostFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(CComBSTR(CS_FIELD_OrgCost), &orgCostFieldIndex))) return hr;
		if (FAILED(hr = ipEdgesFC->FindField(CComBSTR(CS_FIELD_EID), &eidFieldIndex))) return hr;
		
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
			if (FAILED(hr = edge->InsertEdgeToFeatureCursor(ipNetworkDataset, ipFeatureClassContainer, ipFeatureBuffer, eidFieldIndex, sourceIDFieldIndex, sourceOIDFieldIndex, dirFieldIndex, 
				                                            resPopFieldIndex, travCostFieldIndex, orgCostFieldIndex, congestionFieldIndex, sourceNotFoundFlag))) return hr;

			// Insert the feature buffer in the insert cursor
			if (FAILED(hr = ipFeatureCursor->InsertFeature(ipFeatureBuffer, &featureID))) return hr;
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
			if (FAILED(hr = edge->InsertEdgeToFeatureCursor(ipNetworkDataset, ipFeatureClassContainer, ipFeatureBuffer, eidFieldIndex, sourceIDFieldIndex, sourceOIDFieldIndex, dirFieldIndex, 
				                                            resPopFieldIndex, travCostFieldIndex, orgCostFieldIndex, congestionFieldIndex, sourceNotFoundFlag))) return hr;

			// Insert the feature buffer in the insert cursor
			if (FAILED(hr = ipFeatureCursor->InsertFeature(ipFeatureBuffer, &featureID))) return hr;
		}

		// flush the insert buffer
		ipFeatureCursor->Flush();
	
		if (sourceNotFoundFlag) pMessages->AddWarning(CComBSTR(_T("A network source could not be found by source ID.")));	
	}

	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeE, &cpuTimeE);
	tenNanoSec64 = (*((__int64 *) &sysTimeE)) - (*((__int64 *) &sysTimeS));
	outputSecSys = tenNanoSec64 / 10000000.0;
	tenNanoSec64 = (*((__int64 *) &cpuTimeE)) - (*((__int64 *) &cpuTimeS));
	outputSecCpu = tenNanoSec64 / 10000000.0;
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeS, &cpuTimeS);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Perform flocking simulation if requested

	// At this stage we create many evacuee points within a flocking simulation environment to validate the calculated results
	CString collisionMsg, simulationIncompleteEndingMsg;
	std::vector<FlockingLocationPtr> * history = 0;
	std::list<double> * collisionTimes = 0;

	if (flockingEnabled == VARIANT_TRUE)
	{
		// Get the "Flocks" NAClass feature class
		IFeatureCursorPtr ipFeatureCursor;
		IFeatureBufferPtr ipFeatureBuffer;
		PathSegment * pathSegment;
		EvcPath::const_iterator psit;
		IFeatureClassPtr ipFlocksFC(ipFlocksNAClass);
		long nameFieldIndex, timeFieldIndex, traveledFieldIndex, speedXFieldIndex, speedYFieldIndex, idFieldIndex, speedFieldIndex, costFieldIndex, statFieldIndex, ptimeFieldIndex;
		double costPerDay = 1.0, costPerSec = 1.0;
		INetworkAttributePtr costAttrib;
		esriNetworkAttributeUnits unit;
		time_t baseTime = time(NULL), thisTime = 0;
		FlockProfile flockProfile(flockingProfile);
		bool movingObjectLeft;
		wchar_t * thisTimeBuf = new DEBUG_NEW_PLACEMENT wchar_t[25];
		tm local;

		// read cost attribute unit
		if (FAILED(hr = ipNetworkDataset->get_AttributeByID(costAttributeID, &costAttrib))) return hr;
		if (FAILED(hr = costAttrib->get_Units(&unit))) return hr;
		costPerDay = GetUnitPerDay(unit, flockProfile.UsualSpeed);
		costPerSec = costPerDay / (3600.0 * 24.0);

		// project to Mercator for the simulator
		for(eit = Evacuees->begin(); eit < Evacuees->end(); eit++)
		{
			// get all points from the stack and make one polyline from them. this will be the path.
			currentEvacuee = *eit;
			if (!currentEvacuee->paths->empty())
			{
				for (tpit = currentEvacuee->paths->begin(); tpit != currentEvacuee->paths->end(); tpit++)			
				{
					path = *tpit;
					for (psit = path->Begin(); psit != path->End(); psit++)
					{
						pathSegment = *psit;
						if (FAILED(hr = pathSegment->pline->Project(ipNAContextPC))) return hr;
					}
				}
			}
		}

		// init
		if (FAILED(hr = ipStepProgressor->put_Position(0))) return hr;
		if (ipStepProgressor) ipStepProgressor->put_Message(CComBSTR(L"Initializing flocking environment"));
		FlockingEnviroment * flock = new DEBUG_NEW_PLACEMENT FlockingEnviroment(flockingSnapInterval, flockingSimulationInterval, initDelayCostPerPop);
		
		// run simulation
		try
		{
			flock->Init(Evacuees, ipNetworkQuery, &flockProfile, twoWayShareCapacity == VARIANT_TRUE);
			if (ipStepProgressor) ipStepProgressor->put_Message(CComBSTR(L"Running flocking simulation"));
			hr = flock->RunSimulation(ipStepProgressor, pTrackCancel, predictedCost);
		}
		catch(std::exception& e)
		{
			CComBSTR ccombstrErr("Critical error during flocking simulation: ");
			hr = ccombstrErr.Append(e.what());
			pMessages->AddError(-1, ccombstrErr);
		}

		// retrieve results even if it's empty or error
		flock->GetResult(&history, &collisionTimes, &movingObjectLeft);

		if (FAILED(hr))
		{
			// the flocking is canceled or failed. clean up and return.
			delete flock;
			delete [] thisTimeBuf;
			
			// clear and release evacuees and their paths
			for(EvacueeListItr evcItr = Evacuees->begin(); evcItr != Evacuees->end(); evcItr++) delete (*evcItr);
			Evacuees->clear();
			delete Evacuees;

			// releasing all internal objects
			delete vcache;
			delete ecache;
			for (SafeZoneTableItr it = safeZoneList->begin(); it != safeZoneList->end(); it++) delete it->second;
			delete safeZoneList;

			return hr;
		}

		// project back to analysis coordinate system
		for(eit = Evacuees->begin(); eit < Evacuees->end(); eit++)
		{
			currentEvacuee = *eit;
			for (tpit = currentEvacuee->paths->begin(); tpit != currentEvacuee->paths->end(); tpit++)			
			{
				path = *tpit;
				for (psit = path->Begin(); psit != path->End(); psit++)
				{
					pathSegment = *psit;
					if (FAILED(hr = pathSegment->pline->Project(ipNAContextSR))) return hr;
				}
			}			
		}

		// start writing into the feature class
		if (ipStepProgressor)
		{			
			ipStepProgressor->put_Message(CComBSTR(L"Writing flocking results"));
			ipStepProgressor->put_MinRange(0);
			ipStepProgressor->put_MaxRange((long)(history->size()));
			ipStepProgressor->put_StepValue(1);
			ipStepProgressor->put_Position(0);
		}

		// Create an insert cursor and feature buffer from the "Flocks" feature class to be used to write edges
		if (FAILED(hr = ipFlocksFC->Insert(VARIANT_TRUE, &ipFeatureCursor))) return hr;
		if (FAILED(hr = ipFlocksFC->CreateFeatureBuffer(&ipFeatureBuffer))) return hr;

		// Query for the appropriate field index values in the "EdgeStat" feature class
		if (FAILED(hr = ipFlocksFC->FindField(CComBSTR(CS_FIELD_NAME), &nameFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(CComBSTR(CS_FIELD_ID), &idFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(CComBSTR(CS_FIELD_COST), &costFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(CComBSTR(CS_FIELD_TRAVELED), &traveledFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(CComBSTR(CS_FIELD_VelocityX), &speedXFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(CComBSTR(CS_FIELD_VelocityY), &speedYFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(CComBSTR(CS_FIELD_SPEED), &speedFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(CComBSTR(CS_FIELD_TIME), &timeFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(CComBSTR(CS_FIELD_PTIME), &ptimeFieldIndex))) return hr;
		if (FAILED(hr = ipFlocksFC->FindField(CComBSTR(CS_FIELD_STATUS), &statFieldIndex))) return hr;

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
			if (FAILED(hr = ipFeatureBuffer->put_Value(idFieldIndex, CComVariant((*it)->ID)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(nameFieldIndex, (*it)->GroupName))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(costFieldIndex, CComVariant((*it)->MyTime)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(traveledFieldIndex, CComVariant((*it)->Traveled)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(speedXFieldIndex, CComVariant((*it)->Velocity.x)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(speedYFieldIndex, CComVariant((*it)->Velocity.y)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(speedFieldIndex, CComVariant((*it)->Velocity.length())))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(timeFieldIndex, CComVariant(thisTimeBuf)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(ptimeFieldIndex, CComVariant((*it)->GTime / (costPerSec * 60.0))))) return hr;

			// print out the status
			if ((*it)->MyStatus == FLOCK_OBJ_STAT_INIT)
			{
				if (FAILED(hr = ipFeatureBuffer->put_Value(statFieldIndex, CComVariant(_T("I"))))) return hr;
			}
			else if ((*it)->MyStatus == FLOCK_OBJ_STAT_MOVE)
			{
				if (FAILED(hr = ipFeatureBuffer->put_Value(statFieldIndex, CComVariant(_T("M"))))) return hr;
			}
			else if ((*it)->MyStatus == FLOCK_OBJ_STAT_END)
			{
				if (FAILED(hr = ipFeatureBuffer->put_Value(statFieldIndex, CComVariant(_T("E"))))) return hr;
			}
			else if ((*it)->MyStatus == FLOCK_OBJ_STAT_STOP)
			{
				if (FAILED(hr = ipFeatureBuffer->put_Value(statFieldIndex, CComVariant(_T("S"))))) return hr;
			}
			else if ((*it)->MyStatus == FLOCK_OBJ_STAT_COLLID)
			{
				if (FAILED(hr = ipFeatureBuffer->put_Value(statFieldIndex, CComVariant(_T("C"))))) return hr;
			}
			else
			{
				if (FAILED(hr = ipFeatureBuffer->put_Value(statFieldIndex, CComVariant(_T(""))))) return hr;
			}

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
			if (FAILED(hr = ipFeatureBuffer->put_Value(idFieldIndex, CComVariant(0)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(nameFieldIndex, CComVariant("0")))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(costFieldIndex, CComVariant(99999)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(traveledFieldIndex, CComVariant(0)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(speedXFieldIndex, CComVariant(0)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(speedYFieldIndex, CComVariant(0)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(speedFieldIndex, CComVariant(0)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(timeFieldIndex, CComVariant(thisTimeBuf)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(ptimeFieldIndex, CComVariant(99999)))) return hr;

			// print out the status
			if (FAILED(hr = ipFeatureBuffer->put_Value(statFieldIndex, CComVariant(_T("E"))))) return hr;			

			// Insert the feature buffer in the insert cursor
			if (FAILED(hr = ipFeatureCursor->InsertFeature(ipFeatureBuffer, &featureID))) return hr;
		}	
		
		// message about collisions
		collisionMsg.Empty();
		if (collisionTimes && collisionTimes->size() > 0)
		{
			for (std::list<double>::iterator ct = collisionTimes->begin(); ct != collisionTimes->end(); ct++)
			{
				if (collisionMsg.IsEmpty()) collisionMsg.AppendFormat(_T("%.3f"), *ct);
				else collisionMsg.AppendFormat(_T(", %.3f"), *ct);
			}
		}

		// flush the insert buffer and release
		ipFeatureCursor->Flush();
		delete flock;
		delete [] thisTimeBuf;
	}

	// timing
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeE, &cpuTimeE);
	tenNanoSec64 = (*((__int64 *) &sysTimeE)) - (*((__int64 *) &sysTimeS));
	flockSecSys = tenNanoSec64 / 10000000.0;
	tenNanoSec64 = (*((__int64 *) &cpuTimeE)) - (*((__int64 *) &cpuTimeS));
	flockSecCpu = tenNanoSec64 / 10000000.0;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Close it and clean it
	CString performanceMsg, CARMALoopMsg, ExtraInfoMsg, ZeroHurMsg, CARMAExtractsMsg, CacheHitMsg;

	performanceMsg.Format(_T("Timing: Input = %.2f (kernel), %.2f (user); Calculation = %.2f (kernel), %.2f (user); Output = %.2f (kernel), %.2f (user); Flocking = %.2f (kernel), %.2f (user); Total = %.2f"),
		inputSecSys, inputSecCpu, calcSecSys, calcSecCpu, outputSecSys, outputSecCpu, flockSecSys, flockSecCpu,
		inputSecSys + inputSecCpu + calcSecSys + calcSecCpu + flockSecSys + flockSecCpu + outputSecSys + outputSecCpu);

	CARMALoopMsg.Format(_T("The algorithm performed %d CARMA loop(s) in %.2f seconds."), countCARMALoops, carmaSec);

	std::stringstream ss;
	ss.imbue(std::locale(""));
	CARMAExtractsMsg.Format(_T("The following is the number of CARMA heap extracts in each loop: "));
	for (std::vector<unsigned int>::size_type i = 0; i < CARMAExtractCounts.size(); i++)
	{
		if (i == 0) ss << CARMAExtractCounts[0];
		else        ss << " | " << CARMAExtractCounts[i];
	}
	CARMAExtractsMsg.Append(ATL::CString(ss.str().c_str()));

	size_t mem = (peakMemoryUsage - baseMemoryUsage) / 1048576;
	ExtraInfoMsg.Format(_T("Global evacuation cost is %.2f and Peak memory usage (exluding flocking) is %d MB."), globalEvcCost, max(0, mem));
	CacheHitMsg.Format(_T("Traffic model calculation had %.2f%% cache hit."), ecache->GetCacheHitPercentage());

	pMessages->AddMessage(CComBSTR(_T("The routes are generated from the evacuee point(s).")));
	pMessages->AddMessage(CComBSTR(performanceMsg));
	pMessages->AddMessage(CComBSTR(CARMALoopMsg));
	pMessages->AddMessage(CComBSTR(CARMAExtractsMsg));
	pMessages->AddMessage(CComBSTR(ExtraInfoMsg));

	// check version lock: if the evclayer is too old to be solved then add a warning
	if (DoNotExportRouteEdges) pMessages->AddWarning(CComBSTR(_T("This evacuation routing layer is old and does not have the RouteEdges table.")));

	if (EvacueesWithRestrictedSafezone > 0)
	{
		CString RestrictedWarning;
		RestrictedWarning.Format(_T("For %d evacuee(s), a route could not be found because all reachable safe zones were restricted. Make sure you have some non-restricted safe zones with a non-zero capacity."), EvacueesWithRestrictedSafezone);
		pMessages->AddWarning(CComBSTR(RestrictedWarning));
	}

	if (!(simulationIncompleteEndingMsg.IsEmpty())) pMessages->AddWarning(CComBSTR(simulationIncompleteEndingMsg));

	if (!(collisionMsg.IsEmpty()))
	{
		collisionMsg.Insert(0, _T("Some collisions have been reported at the following intervals: "));
		pMessages->AddWarning(CComBSTR(collisionMsg));
	}
	
	// clear and release evacuees and their paths
	for(EvacueeListItr evcItr = Evacuees->begin(); evcItr != Evacuees->end(); evcItr++) delete (*evcItr);
	Evacuees->clear();
	delete Evacuees;

	// releasing all internal objects
	delete vcache;
	delete ecache;
	for (SafeZoneTableItr it = safeZoneList->begin(); it != safeZoneList->end(); it++) delete it->second;
	delete safeZoneList;
	delete tempPathList;

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
