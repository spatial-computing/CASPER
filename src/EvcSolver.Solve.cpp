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
#include "float.h"  // for FLT_MAX, etc.
#include <cmath>   // for HUGE_VAL
#include <ctime>
#include "EvcSolver.h"
#include "FibonacciHeap.h"
#include "Flocking.h"
#include <ATLComTime.h>

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

	HRESULT hr;
	long i;

	// Check for null parameter variables (the track cancel variable is typically considered optional)
	if (!pNAContext || !pMessages) return E_POINTER;

	// The partialSolution variable is used to indicate to the caller of this method whether or not we were only able to 
	// find a partial solution to the problem. We initialize this variable to false and set it to true only in certain
	// conditional cases (e.g., some stops/points are unreachable, a Evacuee point is unlocated, etc.)
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

	// Check for a Step Progressor on the track cancel parameter variable.
	// This can be used to indicate progress and output messages to the client throughout the solve
	CancelTrackerHelper cancelTrackerHelper;
	IStepProgressorPtr ipStepProgressor;
	if (pTrackCancel)
	{
		IProgressorPtr ipProgressor;
		if (FAILED(hr = pTrackCancel->get_Progressor(&ipProgressor))) return hr;

		ipStepProgressor = ipProgressor;

		// We use the cancel tracker helper object to disassociate and reassociate the cancel tracker from the progressor
		// during and after the Solve, respectively. This prevents calls to ITrackCancel::Continue from stepping our progressor
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
#if defined(_FLOCK)
	if (FAILED(hr = ipNAClasses->get_ItemByName(CComBSTR(CS_FLOCKS_NAME), &ipUnk))) return hr;
	INAClassPtr ipFlocksNAClass(ipUnk);
	if (FAILED(hr = ipFlocksNAClass->DeleteAllRows())) return hr;
#else
	INAClassPtr ipFlocksNAClass;
#endif

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Setup the Network Forward Star for traversal
	// Create a Forward Star object from the INetworkQuery interface of the network dataset

	// QI the Forward Star to INetworkForwardStarEx
	// This interface can be used to setup necessary traversal constraints on the Forward Star before proceeding
	// This typically includes:
	// 1) setting the traversal direction (through INetworkForwardStarSetup::IsForwardTraversal)
	// 2) setting any restrictions on the forward star (e.g., oneway restrictions, restricted turns, etc.)
	// 3) setting the U-turn policy (through INetworkForwardStarSetup::Backtrack)
	// 4) setting the hierarchy (through INetworkForwardStarSetup::HierarchyAttribute)
	// 5) setting up traversable/non-traversable elements (in our case, we will be setting up barriers as non-traversable)
	INetworkQueryPtr ipNetworkQuery(ipNetworkDataset);
	INetworkForwardStarPtr ipNetworkForwardStar;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStar(&ipNetworkForwardStar))) return hr;
	INetworkForwardStarExPtr ipNetworkForwardStarEx(ipNetworkForwardStar);
	if (FAILED(hr = ipNetworkForwardStarEx->put_BacktrackPolicy(backtrack))) return hr;

	// this will create the backward traversal object to query for adjacencies during pre-processing
	INetworkForwardStarPtr ipNetworkBackwardStar;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStar(&ipNetworkBackwardStar))) return hr;
	INetworkForwardStarExPtr ipNetworkBackwardStarEx(ipNetworkBackwardStar);
	if (FAILED(hr = ipNetworkBackwardStarEx->put_IsForwardTraversal(VARIANT_FALSE))) return hr;
	if (FAILED(hr = ipNetworkBackwardStarEx->put_BacktrackPolicy(backtrack))) return hr;

	// Get the "Barriers" NAClass table (we need the NALocation objects from this NAClass to push barriers into the Forward Star)
	ITablePtr ipBarriersTable;
	if (FAILED(hr = GetNAClassTable(pNAContext, CComBSTR(CS_BARRIERS_NAME), &ipBarriersTable))) return hr;

	// Load the barriers
	if (FAILED(hr = LoadBarriers(ipBarriersTable, ipNetworkQuery, ipNetworkForwardStarEx))) return hr;
	if (FAILED(hr = LoadBarriers(ipBarriersTable, ipNetworkQuery, ipNetworkBackwardStarEx))) return hr;

	// loading restriction attributes into the forward star. this will enforce all available restrictions.
	for (std::vector<INetworkAttribute2Ptr>::iterator Iter = turnAttribs.begin(); Iter != turnAttribs.end(); Iter++)
	{
		if (FAILED(hr = ipNetworkForwardStarEx->AddRestrictionAttribute(*Iter))) return hr;
		if (FAILED(hr = ipNetworkBackwardStarEx->AddRestrictionAttribute(*Iter))) return hr;
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
	NAVertexPtr myVertex;

	// Initialize Caches
	// This cache will maintain a list of all created vertices/edges. You can retrieve
	// them later using EID. The benefit of using this cache is that we
	// can maintain one-to-one relationship between network junctions and vertices.
	// This will particularly be helpful with the heuristic calculator part of the algorithm.
	// if (FAILED(hr = heuristicAttribs[heuristicAttribIndex]->get_ID(&capAttributeID))) return hr;  	
	NAVertexCache * vcache = new NAVertexCache();
	NAEdgeCache   * ecache = new NAEdgeCache(capAttributeID, costAttributeID, SaturationPerCap, CriticalDensPerCap,
		twoWayShareCapacity == VARIANT_TRUE, initDelayCostPerPop, trafficModel);

	// Vertex table structures
	NAVertexTable * safeZoneList = new NAVertexTable();
	INetworkEdgePtr ipEdge(ipElement);
	INetworkEdgePtr ipOtherEdge(ipOtherElement);

	if (ipStepProgressor) ipStepProgressor->put_Message(CComBSTR(L"Collecting input point(s)")); // add more specific information here if appropriate

	///////////////////////////
	// here we begin collecting safe zone points for all the evacuees

	// Get a cursor on the zones table to loop through each row
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
		if (!isLocated)
		{
			// If this point is unlocated, we will only be able to find a partial solution
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

			// We must loop through the returned elements, looking for an appropriate ending point
			ipEnumNetworkElement->Reset();

			while (ipEnumNetworkElement->Next(&ipElement) == S_OK)
			{
				// We must then check the returned element type
				ipElement->get_ElementType(&elementType);

				// If the element is a junction, then it is the starting point of traversal 
				// We simply add its EID to the heap and break out of the enumerating loop
				if (elementType == esriNETJunction)
				{
					if (FAILED(hr = ipNetworkForwardStarEx->get_IsRestricted(ipElement, &isRestricted))) return hr;
					if (!isRestricted)
					{
						myVertex = new NAVertex(ipElement, 0);
						safeZoneList->insert(NAVertexTablePair(myVertex));
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

						if (FAILED(hr = ipNetworkForwardStarEx->get_IsRestricted(ipEdge, &isRestricted))) return hr;
						if (!isRestricted)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
							ipCurrentJunction = ipOtherElement;
							if (FAILED(hr = ipEdge->QueryJunctions(ipCurrentJunction, 0))) return hr;

							myVertex = new NAVertex(ipCurrentJunction, ecache->New(ipEdge));
							myVertex->posAlong = posAlong * myVertex->GetBehindEdge()->OriginalCost; // / (toPosition - fromPosition);

							safeZoneList->insert(NAVertexTablePair(myVertex));
						}
					}

					if (FAILED(hr = ipEdge->QueryEdgeInOtherDirection(ipOtherEdge))) return hr;		  
					if (FAILED(hr = ipOtherEdge->QueryPositions(&toPosition, &fromPosition))) return hr;

					if (fromPosition <= posAlong && posAlong <= toPosition)
					{
						if (FAILED(hr = ipNetworkForwardStarEx->get_IsRestricted(ipOtherEdge, &isRestricted))) return hr;
						if (!isRestricted)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
							ipCurrentJunction = ipOtherElement;
							if (FAILED(hr = ipOtherEdge->QueryJunctions(ipCurrentJunction, 0))) return hr;						

							myVertex = new NAVertex(ipCurrentJunction, ecache->New(ipOtherEdge));
							myVertex->posAlong = (toPosition - posAlong) * myVertex->GetBehindEdge()->OriginalCost; // / (toPosition - fromPosition);
							safeZoneList->insert(NAVertexTablePair(myVertex));
						}
					}
				}
			}
		}
	}

	// Get a cursor on the Evacuee points table to loop through each row
	if (FAILED(hr = ipEvacueePointsTable->Search(0, VARIANT_TRUE, &ipCursor))) return hr;
	EvacueeList * Evacuees = new EvacueeList();
	Evacuee * currentEvacuee;
	VARIANT evName, pop;
	long nameFieldIndex = 0l, popFieldIndex = 0l;

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
			currentEvacuee = new Evacuee(evName, pop.dblVal);

			while (ipEnumNetworkElement->Next(&ipElement) == S_OK)
			{
				// We must then check the returned element type
				ipElement->get_ElementType(&elementType);

				// If the element is a junction, then it is the starting point of traversal 
				// We simply add its EID to the heap and break out of the enumerating loop
				if (elementType == esriNETJunction)
				{
					if (FAILED(hr = ipNetworkForwardStarEx->get_IsRestricted(ipElement, &isRestricted))) return hr;
					if (!isRestricted)
					{
						myVertex = new NAVertex(ipElement, 0);
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

						if (FAILED(hr = ipNetworkForwardStarEx->get_IsRestricted(ipEdge, &isRestricted))) return hr;
						if (!isRestricted)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
							ipCurrentJunction = ipOtherElement;
							if (FAILED(hr = ipEdge->QueryJunctions(0, ipCurrentJunction))) return hr;

							myVertex = new NAVertex(ipCurrentJunction, ecache->New(ipEdge));
							myVertex->posAlong = (toPosition - posAlong) * myVertex->GetBehindEdge()->OriginalCost;
							currentEvacuee->vertices->insert(currentEvacuee->vertices->end(), myVertex);
						}
					}

					if (FAILED(hr = ipEdge->QueryEdgeInOtherDirection(ipOtherEdge))) return hr;		  
					if (FAILED(hr = ipOtherEdge->QueryPositions(&toPosition, &fromPosition))) return hr;

					if (fromPosition <= posAlong && posAlong <= toPosition)
					{
						if (FAILED(hr = ipNetworkForwardStarEx->get_IsRestricted(ipOtherEdge, &isRestricted))) return hr;
						if (!isRestricted)
						{							
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
							ipCurrentJunction = ipOtherElement;
							if (FAILED(hr = ipOtherEdge->QueryJunctions(0, ipCurrentJunction))) return hr;

							myVertex = new NAVertex(ipCurrentJunction, ecache->New(ipOtherEdge));
							myVertex->posAlong = posAlong * myVertex->GetBehindEdge()->OriginalCost;
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

	///////////////////////////////////////
	// this will call the core part of the algorithm.
	if (FAILED(hr = SolveMethod(ipNetworkQuery, pMessages, pTrackCancel, ipStepProgressor, Evacuees, vcache, ecache,
		safeZoneList, ipNetworkForwardStarEx, ipNetworkBackwardStarEx))) return hr;	

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

	// Setup a message on our step progressor indicating that we are outputting feature information
	if (ipStepProgressor) ipStepProgressor->put_Message(CComBSTR(L"Writing output features")); 

	// If we reach this point, we have some features to output to the Routes NAClass
	// Reset the progressor based on the number of features that we must output
	if (ipStepProgressor)
	{
		// Step progressor range = 0 through numberOfOutputSteps
		if (FAILED(hr = ipStepProgressor->put_MinRange(0))) return hr;
		if (exportEdgeStat)
		{
			if (FAILED(hr = ipStepProgressor->put_MaxRange(ecache->Size() + Evacuees->size()))) return hr;
		}
		else
		{
			if (FAILED(hr = ipStepProgressor->put_MaxRange(Evacuees->size()))) return hr;
		}
		if (FAILED(hr = ipStepProgressor->put_StepValue(1))) return hr;
		if (FAILED(hr = ipStepProgressor->put_Position(0))) return hr;
	}

	// looping through processed evacuees and generate routes in output featureclass
	PathSegment * pathSegment;
	IFeaturePtr ipSourceFeature;
	IGeometryPtr ipGeometry;
	ICurvePtr  ipSubCurve;
	IPointCollectionPtr pcollect = NULL;	
	esriGeometryType * type = new esriGeometryType();
	long pointCount = 0;
	IPointPtr p = NULL;
	CComVariant featureID(0);
	IPointCollectionPtr pline = 0;
	EvcPathPtr path;
	std::list<PathSegmentPtr>::iterator psit;
	std::list<EvcPathPtr>::iterator pit;
	double predictedCost = 0.0;
	bool sourceNotFoundFlag = false;
	IFeatureClassContainerPtr ipFeatureClassContainer(ipNetworkDataset);
	IFeatureClassPtr ipNetworkSourceFC;
	INetworkSourcePtr ipNetworkSource;
	BSTR sourceName;
	std::vector<EvacueePtr>::iterator eit;
	
	// load the mercator projection and analysis projection
	ISpatialReferencePtr ipNAContextSR;
	if (FAILED(hr = pNAContext->get_SpatialReference(&ipNAContextSR))) return hr;

	IProjectedCoordinateSystemPtr ipNAContextPC;
	ISpatialReferenceFactoryPtr pSpatRefFact = ISpatialReferenceFactoryPtr(CLSID_SpatialReferenceEnvironment);
	if (FAILED(hr = pSpatRefFact->CreateProjectedCoordinateSystem(esriSRProjCS_WGS1984WorldMercator, &ipNAContextPC))) return hr;
	ISpatialReferencePtr ipSpatialRef = ipNAContextPC;

	// Get the "Routes" NAClass feature class
	IFeatureClassPtr ipRoutesFC(ipRoutesNAClass);

	// Create an insert cursor and feature buffer from the "Routes" feature class to be used to write routes
	IFeatureCursorPtr ipFeatureCursor;
	if (FAILED(hr = ipRoutesFC->Insert(VARIANT_TRUE, &ipFeatureCursor))) return hr;

	IFeatureBufferPtr ipFeatureBuffer;
	if (FAILED(hr = ipRoutesFC->CreateFeatureBuffer(&ipFeatureBuffer))) return hr;

	// Query for the appropriate field index values in the "routes" feature class
	long evNameFieldIndex, evacTimeFieldIndex, orgTimeFieldIndex;
	if (FAILED(hr = ipRoutesFC->FindField(CComBSTR(CS_FIELD_EVC_NAME), &evNameFieldIndex))) return hr;
	if (FAILED(hr = ipRoutesFC->FindField(CComBSTR(CS_FIELD_E_TIME), &evacTimeFieldIndex))) return hr;
	if (FAILED(hr = ipRoutesFC->FindField(CComBSTR(CS_FIELD_E_ORG), &orgTimeFieldIndex))) return hr;
	if (FAILED(hr = ipRoutesFC->FindField(CComBSTR(CS_FIELD_E_POP), &popFieldIndex))) return hr;

	for(eit = Evacuees->begin(); eit < Evacuees->end(); eit++)
	{
		// Check to see if the user wishes to continue or cancel the solve (i.e., check whether or not the user has hit the ESC key to stop processing)
		if (pTrackCancel)
		{
			if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
			if (keepGoing == VARIANT_FALSE) return E_ABORT;			
		}

		// get all points from the stack and make one polyline from them. this will be the path.
		currentEvacuee = *eit;

		// Step the progressor before continuing to the next Evacuee point
		if (ipStepProgressor) ipStepProgressor->Step();	

		if (currentEvacuee->paths->empty())
		{
			*pIsPartialSolution = VARIANT_TRUE;
		}
		else
		{
			for (pit = currentEvacuee->paths->begin(); pit != currentEvacuee->paths->end(); pit++)			
			{
				path = *pit;
				path->OrginalCost = 0.0;
				path->EvacuationCost = 0.0;
				pline = IPointCollectionPtr(CLSID_Polyline);
				pointCount = -1;

				for (psit = path->begin(); psit != path->end(); psit++)
				{
					// take a path segment from the stack
					pathSegment = *psit;
					pointCount = -1;
					_ASSERT(pathSegment->EdgePortion > 0.0);

					// retrive street shape for this segment
					if (FAILED(hr = ipNetworkDataset->get_SourceByID(pathSegment->SourceID, &ipNetworkSource))) return hr;
					if (FAILED(hr = ipNetworkSource->get_Name(&sourceName))) return hr;
					if (FAILED(hr = ipFeatureClassContainer->get_ClassByName(sourceName, &ipNetworkSourceFC))) return hr;
					if (!ipNetworkSourceFC)
					{
						if (!sourceNotFoundFlag)
						{
							sourceNotFoundFlag = true;
							pMessages->AddWarning(CComBSTR(_T("A network source could not be found by source ID.")));							
						}
						continue;
					}
					if (FAILED(hr = ipNetworkSourceFC->GetFeature(pathSegment->SourceOID, &ipSourceFeature))) return hr;
					if (FAILED(hr = ipSourceFeature->get_Shape(&ipGeometry))) return hr;

					// Check to see how much of the line geometry we can copy over
					if (pathSegment->fromPosition != 0.0 || pathSegment->toPosition != 1.0)
					{
						// We must use only a subcurve of the line geometry
						ICurve3Ptr ipCurve(ipGeometry);
						if (FAILED(hr = ipCurve->GetSubcurve(pathSegment->fromPosition, pathSegment->toPosition, VARIANT_TRUE, &ipSubCurve))) return hr;
						ipGeometry = ipSubCurve;
					}
					ipGeometry->get_GeometryType(type);			

					// get all the points from this polyline and store it in the point stack
					_ASSERT(*type == esriGeometryPolyline);

					if (*type == esriGeometryPolyline)
					{
						pathSegment->pline = (IPolylinePtr)ipGeometry;
						pcollect = pathSegment->pline;
						if (FAILED(hr = pcollect->get_PointCount(&pointCount))) return hr;

						// if this is not the last path segment then the last point is redundent.
						pointCount--;						
						for (i = 0; i < pointCount; i++)
						{
							if (FAILED(hr = pcollect->get_Point(i, &p))) return hr;
							if (FAILED(hr = pline->AddPoint(p))) return hr;
						}
						// if (FAILED(hr = pathSegment->pline->Project(ipSpatialRef))) return hr;
					}			
					// Final cost calculations
					path->EvacuationCost += pathSegment->Edge->GetCurrentCost() * pathSegment->EdgePortion;
					path->OrginalCost    += pathSegment->Edge->OriginalCost * pathSegment->EdgePortion;
				}

				// Add the last point of the last path segment to the polyline
				if (pointCount > -1)
				{
					pcollect->get_Point(pointCount, &p);
					pline->AddPoint(p);
				}

				// att the initial delay cost
				path->EvacuationCost += path->RoutedPop * initDelayCostPerPop;

				// Store the feature values on the feature buffer
				if (FAILED(hr = ipFeatureBuffer->putref_Shape((IPolylinePtr)pline))) return hr;
				if (FAILED(hr = ipFeatureBuffer->put_Value(evNameFieldIndex, currentEvacuee->Name))) return hr;
				if (FAILED(hr = ipFeatureBuffer->put_Value(evacTimeFieldIndex, CComVariant(path->EvacuationCost)))) return hr;
				if (FAILED(hr = ipFeatureBuffer->put_Value(orgTimeFieldIndex, CComVariant(path->OrginalCost)))) return hr;
				if (FAILED(hr = ipFeatureBuffer->put_Value(popFieldIndex, CComVariant(path->RoutedPop)))) return hr;

				// Insert the feature buffer in the insert cursor
				if (FAILED(hr = ipFeatureCursor->InsertFeature(ipFeatureBuffer, &featureID))) return hr;

				predictedCost = max(predictedCost, path->EvacuationCost);
			}
		}
	}
	
	// fluch the insert buffer
	ipFeatureCursor->Flush();

	////////////////////////////////////////////////////////////////////////////////
	///// Exporting EdgeStat data to output featureClass

	if (exportEdgeStat)
	{
		// Get the "Routes" NAClass feature class
		IFeatureClassPtr ipEdgesFC(ipEdgesNAClass);
		NAEdgePtr edge;
		double resPop;
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

		BSTR dir = L"Along";

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
			resPop = edge->GetReservedPop();
			if (resPop <= 0.0) continue;

			// retrive street shape for this edge
			if (FAILED(hr = edge->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) return hr;
			if (FAILED(hr = ipNetworkDataset->get_SourceByID(sourceID, &ipNetworkSource))) return hr;
			if (FAILED(hr = ipNetworkSource->get_Name(&sourceName))) return hr;
			if (FAILED(hr = ipFeatureClassContainer->get_ClassByName(sourceName, &ipNetworkSourceFC))) return hr;
			if (!ipNetworkSourceFC)
			{
				if (!sourceNotFoundFlag)
				{
					sourceNotFoundFlag = true;
					pMessages->AddWarning(CComBSTR(_T("A network source could not be found by source ID.")));							
				}
				continue;
			}
			if (FAILED(hr = ipNetworkSourceFC->GetFeature(sourceOID, &ipSourceFeature))) return hr;
			if (FAILED(hr = ipSourceFeature->get_Shape(&ipGeometry))) return hr;

			// Check to see how much of the line geometry we can copy over
			if (fromPosition != 0.0 || toPosition != 1.0)
			{
				// We must use only a subcurve of the line geometry
				ICurve3Ptr ipCurve(ipGeometry);
				if (FAILED(hr = ipCurve->GetSubcurve(fromPosition, toPosition, VARIANT_TRUE, &ipSubCurve))) return hr;
				ipGeometry = ipSubCurve;
			}

			// Store the feature values on the feature buffer
			if (FAILED(hr = ipFeatureBuffer->putref_Shape(ipGeometry))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(eidFieldIndex, CComVariant(edge->EID)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(sourceIDFieldIndex, CComVariant(sourceID)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(sourceOIDFieldIndex, CComVariant(sourceOID)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(dirFieldIndex, CComVariant(dir)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(resPopFieldIndex, CComVariant(resPop)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(travCostFieldIndex, CComVariant(edge->GetCurrentCost())))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(orgCostFieldIndex, CComVariant(edge->OriginalCost)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(congestionFieldIndex, CComVariant(edge->GetCurrentCost() / edge->OriginalCost)))) return hr;

			// Insert the feature buffer in the insert cursor
			if (FAILED(hr = ipFeatureCursor->InsertFeature(ipFeatureBuffer, &featureID))) return hr;
		}

		dir = L"Against";

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
			resPop = edge->GetReservedPop();
			if (resPop <= 0.0) continue;

			// retrive street shape for this edge
			if (FAILED(hr = edge->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) return hr;
			if (FAILED(hr = ipNetworkDataset->get_SourceByID(sourceID, &ipNetworkSource))) return hr;
			if (FAILED(hr = ipNetworkSource->get_Name(&sourceName))) return hr;
			if (FAILED(hr = ipFeatureClassContainer->get_ClassByName(sourceName, &ipNetworkSourceFC))) return hr;
			if (!ipNetworkSourceFC)
			{
				if (!sourceNotFoundFlag)
				{
					sourceNotFoundFlag = true;
					pMessages->AddWarning(CComBSTR(_T("A network source could not be found by source ID.")));							
				}
				continue;
			}
			if (FAILED(hr = ipNetworkSourceFC->GetFeature(sourceOID, &ipSourceFeature))) return hr;
			if (FAILED(hr = ipSourceFeature->get_Shape(&ipGeometry))) return hr;

			// Check to see how much of the line geometry we can copy over
			if (fromPosition != 0.0 || toPosition != 1.0)
			{
				// We must use only a subcurve of the line geometry
				ICurve3Ptr ipCurve(ipGeometry);
				if (FAILED(hr = ipCurve->GetSubcurve(fromPosition, toPosition, VARIANT_TRUE, &ipSubCurve))) return hr;
				ipGeometry = ipSubCurve;
			}

			// Store the feature values on the feature buffer
			if (FAILED(hr = ipFeatureBuffer->putref_Shape(ipGeometry))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(eidFieldIndex, CComVariant(edge->EID)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(sourceIDFieldIndex, CComVariant(sourceID)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(sourceOIDFieldIndex, CComVariant(sourceOID)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(dirFieldIndex, CComVariant(dir)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(resPopFieldIndex, CComVariant(resPop)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(travCostFieldIndex, CComVariant(edge->GetCurrentCost())))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(orgCostFieldIndex, CComVariant(edge->OriginalCost)))) return hr;
			if (FAILED(hr = ipFeatureBuffer->put_Value(congestionFieldIndex, CComVariant(edge->GetCurrentCost() / edge->OriginalCost)))) return hr;

			// Insert the feature buffer in the insert cursor
			if (FAILED(hr = ipFeatureCursor->InsertFeature(ipFeatureBuffer, &featureID))) return hr;
		}

		// fluch the insert buffer
		ipFeatureCursor->Flush();
	}

	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeE, &cpuTimeE);
	tenNanoSec64 = (*((__int64 *) &sysTimeE)) - (*((__int64 *) &sysTimeS));
	outputSecSys = tenNanoSec64 / 10000000.0;
	tenNanoSec64 = (*((__int64 *) &cpuTimeE)) - (*((__int64 *) &cpuTimeS));
	outputSecCpu = tenNanoSec64 / 10000000.0;
	c = GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &sysTimeS, &cpuTimeS);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Perform flocking simulation if requested

	// At this stage we create many evacuee points with in an flocking simulation enviroment to validate the calculated results
	CString colMsgString, ending;
	std::vector<FlockingLocationPtr> * history = 0;
	std::list<double> * collisionTimes = 0;

#ifndef _FLOCK
	this->flockingEnabled = false;
#endif

	if (this->flockingEnabled)
	{
		// Get the "Flocks" NAClass feature class
		IFeatureClassPtr ipFlocksFC(ipFlocksNAClass);
		long nameFieldIndex, timeFieldIndex, traveledFieldIndex, speedXFieldIndex, speedYFieldIndex, idFieldIndex, speedFieldIndex, costFieldIndex, statFieldIndex, ptimeFieldIndex;
		double costPerDay = 1.0, costPerSec = 1.0;
		INetworkAttributePtr costAttrib;
		esriNetworkAttributeUnits unit;
		time_t baseTime = time(NULL), thisTime = 0;
		FlockProfile flockProfile(flockingProfile);
		bool movingObjectLeft;
		wchar_t * thisTimeBuf = new wchar_t[25];
		tm local;

		// read cost attribute unit
		if (FAILED(hr = ipNetworkDataset->get_AttributeByID(costAttributeID, &costAttrib))) return hr;
		if (FAILED(hr = costAttrib->get_Units(&unit))) return hr;
		costPerDay = GetUnitPerDay(unit, flockProfile.UsualSpeed);
		costPerSec = costPerDay / (3600.0 * 24.0);

		// project to mercator for the simulator
		for(eit = Evacuees->begin(); eit < Evacuees->end(); eit++)
		{
			// get all points from the stack and make one polyline from them. this will be the path.
			currentEvacuee = *eit;
			if (!currentEvacuee->paths->empty())
			{
				for (pit = currentEvacuee->paths->begin(); pit != currentEvacuee->paths->end(); pit++)			
				{
					path = *pit;
					for (psit = path->begin(); psit != path->end(); psit++)
					{
						pathSegment = *psit;
						if (FAILED(hr = pathSegment->pline->Project(ipNAContextPC))) return hr;
					}
				}
			}
		}

		// init
		if (FAILED(hr = ipStepProgressor->put_Position(0))) return hr;
		if (ipStepProgressor) ipStepProgressor->put_Message(CComBSTR(L"Initializing flocking enviroment"));
		FlockingEnviroment * flock = new FlockingEnviroment(flockingSnapInterval, flockingSimulationInterval, initDelayCostPerPop);
		flock->Init(Evacuees, ipNetworkQuery, costPerSec, &flockProfile, twoWayShareCapacity == VARIANT_TRUE);
		
		// run simulation
		try
		{
			if (ipStepProgressor) ipStepProgressor->put_Message(CComBSTR(L"Running flocking simulation"));
			if (FAILED(hr = flock->RunSimulation(ipStepProgressor, pTrackCancel, predictedCost))) return hr;
		}
		catch(std::exception& e)
		{
			CComBSTR ccombstrErr("Critical error during flocking simulation: ");
			ccombstrErr.Append(e.what());
			pMessages->AddError(-1, ccombstrErr);
		}

		// retrive results even if it's empty or errored
		flock->GetResult(&history, &collisionTimes, &movingObjectLeft);

		// project back to analysis coordinate system
		for(eit = Evacuees->begin(); eit < Evacuees->end(); eit++)
		{
			currentEvacuee = *eit;
			for (pit = currentEvacuee->paths->begin(); pit != currentEvacuee->paths->end(); pit++)			
			{
				path = *pit;
				for (psit = path->begin(); psit != path->end(); psit++)
				{
					pathSegment = *psit;
					if (FAILED(hr = pathSegment->pline->Project(ipNAContextSR))) return hr;
				}
			}			
		}

		// start writing into the featureclass
		if (ipStepProgressor)
		{			
			ipStepProgressor->put_Message(CComBSTR(L"Writing flocking results"));
			ipStepProgressor->put_MinRange(0);
			ipStepProgressor->put_MaxRange(history->size());
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

			// generate time as unicode string
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

		// collision messaging
		colMsgString.Empty();
		ending.Empty();

		// message about simulation time
		if (movingObjectLeft) ending = _T("Max simulation time reached therefore not all objects get to a safe area. Probebly the predicted evacuation time was too low.");

		// message about collisions
		if (collisionTimes && collisionTimes->size() > 0)
		{
			for (std::list<double>::iterator ct = collisionTimes->begin(); ct != collisionTimes->end(); ct++)
			{
				if (colMsgString.IsEmpty()) colMsgString.AppendFormat(_T("%.3f"), *ct);
				else colMsgString.AppendFormat(_T(", %.3f"), *ct);
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
	CString formatString;
	CString msgString;
#ifdef _FLOCK
	formatString = _T("Timeing: Input = %.2f (kernel), %.2f (user); Calculation = %.2f (kernel), %.2f (user); Output = %.2f (kernel), %.2f (user); Flocking = %.2f (kernel), %.2f (user); Total = %.2f");
	msgString.Format(formatString, inputSecSys, inputSecCpu, calcSecSys, calcSecCpu, outputSecSys, outputSecCpu, flockSecSys, flockSecCpu,
		inputSecSys + inputSecCpu + calcSecSys + calcSecCpu + flockSecSys + flockSecCpu + outputSecSys + outputSecCpu);
#else
	formatString = _T("Timeing: Input = %.2f (kernel), %.2f (user); Calculation = %.2f (kernel), %.2f (user); Output = %.2f (kernel), %.2f (user); Total = %.2f");
	msgString.Format(formatString, inputSecSys, inputSecCpu, calcSecSys, calcSecCpu, outputSecSys, outputSecCpu,
		inputSecSys + inputSecCpu + calcSecSys + calcSecCpu + flockSecSys + flockSecCpu + outputSecSys + outputSecCpu);
#endif
	pMessages->AddMessage(CComBSTR(_T("The routes are generated from the evacuee point(s).")));
	pMessages->AddMessage(CComBSTR(msgString));

	if (!(ending.IsEmpty())) pMessages->AddWarning(CComBSTR(ending));

	if (!(colMsgString.IsEmpty()))
	{
		pMessages->AddWarning(CComBSTR(_T("Some collisions have been reported at the following intervals:")));
		pMessages->AddWarning(CComBSTR(colMsgString));
	}

	// clear and release evacuees and their paths
	for(EvacueeListItr evcItr = Evacuees->begin(); evcItr != Evacuees->end(); evcItr++) delete (*evcItr);
	Evacuees->clear();
	delete Evacuees;

	// releasing all internal objects
	delete vcache;
	delete ecache;
	for (NAVertexTableItr it = safeZoneList->begin(); it != safeZoneList->end(); it++) delete it->second;
	delete safeZoneList;
	return S_OK;
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
