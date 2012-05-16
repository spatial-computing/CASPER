#include "stdafx.h"
#include "NameConstants.h"
#include "float.h"  // for FLT_MAX, etc.
#include <cmath>   // for HUGE_VAL
#include "EvcSolver.h"
#include "FibonacciHeap.h"

#define EvacueeBucketSize 5

HRESULT EvcSolver::SolveMethod(INetworkQueryPtr ipNetworkQuery, IGPMessages* pMessages, ITrackCancel* pTrackCancel,
							   IStepProgressorPtr ipStepProgressor, EvacueeList * Evacuees, NAVertexCache * vcache, NAEdgeCache * ecache,
							   NAVertexTable * safeZoneList, INetworkForwardStarExPtr ipNetworkForwardStarEx, INetworkForwardStarExPtr ipNetworkBackwardStarEx)
{	
	// creating the heap for the dijkstra search
	FibonacciHeap * heap = new FibonacciHeap();
	NAEdgeClosed * closedList = new NAEdgeClosed();
	NAEdgePtr currentEdge;
	std::vector<EvacueePtr>::reverse_iterator rseit;
	NAVertexPtr neighbor, evc, tempEvc, BetterSafeZone = 0, finalVertex = 0, myVertex, temp;
	NAEdgePtr myEdge, edge;
	HRESULT hr;
	EvacueePtr currentEvacuee;
	VARIANT_BOOL keepGoing, isRestricted;
	double fromPosition, toPosition, TimeToBeat = 0.0, edgePortion = 1.0, newCost, populationLeft, population2Route;
	std::vector<NAVertexPtr>::iterator vit;
	NAVertexTableItr iterator;
	long adjacentEdgeCount, i, sourceOID, sourceID;
	INetworkEdgePtr ipCurrentEdge, lastExteriorEdge;
	INetworkJunctionPtr ipCurrentJunction;
	INetworkElementPtr ipElement, ipOtherElement;
	PathSegment * lastAdded = 0;
	EvcPathPtr path;
	long eid;
	esriNetworkEdgeDirection dir;
	bool restricted = false;
	esriNetworkTurnParticipationType turnType;
	EvacueeList * sortedEvacuees = new EvacueeList();
	sortedEvacuees->reserve(EvacueeBucketSize);

	///////////////////////////////////////
	if (ipStepProgressor)
	{
		// Step progressor range = 0 through numberOfOutputSteps
		if (solvermethod == EVC_SOLVER_METHOD_CASPER)
		{
			if (FAILED(hr = ipStepProgressor->put_Message(CComBSTR(L"Performing CASPER search")))) return hr;
		}
		else if (solvermethod == EVC_SOLVER_METHOD_SP)
		{
			if (FAILED(hr = ipStepProgressor->put_Message(CComBSTR(L"Performing SP search")))) return hr;
		}
		else
		{
			if (FAILED(hr = ipStepProgressor->put_Message(CComBSTR(L"Performing CCRP search")))) return hr;
		}		
		if (FAILED(hr = ipStepProgressor->put_MinRange(0))) return hr;
		if (FAILED(hr = ipStepProgressor->put_MaxRange(Evacuees->size()))) return hr;
		if (FAILED(hr = ipStepProgressor->put_StepValue(1))) return hr;
		if (FAILED(hr = ipStepProgressor->put_Position(0))) return hr;
	}

	// Create a Forward Star Adjacencies object (we need this object to hold traversal queries carried out on the Forward Star)
	INetworkForwardStarAdjacenciesPtr ipNetworkForwardStarAdjacencies;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipNetworkForwardStarAdjacencies))) return hr; 

	do
	{
		// indexing all the population by their srrounding vertices this will be used to sort them by network distance to safe zone.
		// only the last 'k'th evacuees will be bucketed to run each round.
		if (FAILED(hr = RunHeuristic(ipNetworkQuery, pMessages, pTrackCancel, Evacuees, sortedEvacuees, vcache, ecache,
			safeZoneList, ipNetworkBackwardStarEx, EvacueeBucketSize))) return hr;

		for(rseit = sortedEvacuees->rbegin(); rseit != sortedEvacuees->rend(); rseit++)
		{
			currentEvacuee = *rseit;

			// Step the progressor before continuing to the next Evacuee point
			if (ipStepProgressor) ipStepProgressor->Step();		

			// Check to see if the user wishes to continue or cancel the solve (i.e., check whether or not the user has hit the ESC key to stop processing)
			if (pTrackCancel)
			{
				if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
				if (keepGoing == VARIANT_FALSE) return E_ABORT;			
			}
			if (currentEvacuee->vertices->size() == 0) continue;
			populationLeft = currentEvacuee->Population;

			while (populationLeft > 0.0)
			{
				// populate the heap with vextices asociated with the current evacuee
				for(vit = currentEvacuee->vertices->begin(); vit != currentEvacuee->vertices->end(); vit++)
				{
					evc = *vit;
					tempEvc = vcache->New(evc->Junction);
					tempEvc->SetBehindEdge(evc->GetBehindEdge());
					tempEvc->g = evc->g;
					tempEvc->Junction = evc->Junction;
					tempEvc->Previous = 0;
					myEdge = tempEvc->GetBehindEdge();

					if (myEdge) heap->Insert(myEdge);
					else
					{
						// if the start point was a single junction, then all the adjacent edges can be start edges
						if (FAILED(hr = ipNetworkBackwardStarEx->QueryAdjacencies(tempEvc->Junction, 0, 0, ipNetworkForwardStarAdjacencies))) return hr; 
						if (FAILED(hr = ipNetworkForwardStarAdjacencies->get_Count(&adjacentEdgeCount))) return hr;
						for (i = 0; i < adjacentEdgeCount; i++)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) return hr;
							ipCurrentEdge = ipElement;
							if (FAILED(hr = ipNetworkForwardStarAdjacencies->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) return hr;
							myEdge = ecache->New(ipCurrentEdge);
							tempEvc->SetBehindEdge(myEdge);
							heap->Insert(myEdge);
						}
					}
				}

				TimeToBeat = MAX_COST;
				BetterSafeZone = 0;
				finalVertex = 0;
				population2Route = populationLeft;

				// Continue traversing the network while the heap has remaining junctions in it
				// this is the actual dijkstra code with the Fibonacci Heap
				while (!heap->IsEmpty())
				{
					// Remove the next junction EID from the top of the stack
					myEdge = heap->DeleteMin();
					myVertex = myEdge->ToVertex;
					if (FAILED(hr = closedList->Insert(myEdge)))
					{
						// closedList violation happened
						pMessages->AddError(-myEdge->EID, CComBSTR(L"ClosedList Violation Error."));
						return -myEdge->EID;
					}

					// Check for destinations. If a new destination has been found then we sould
					// first flag this so later we can use to generate route. Also we should
					// update the new TimeToBeat value for proper termination.
					iterator = safeZoneList->find(myVertex->EID);
					if (iterator != safeZoneList->end())
					{
						// I should handle the last turn restriction here					
						edge = iterator->second->GetBehindEdge();
						restricted = false;

						if (edge)
						{
							restricted = true;		
							if (FAILED(hr = ipNetworkForwardStarEx->QueryAdjacencies(myVertex->Junction, myEdge->NetEdge
								, 0, ipNetworkForwardStarAdjacencies))) return hr;
							if (FAILED(hr = ipNetworkForwardStarAdjacencies->get_Count(&adjacentEdgeCount))) return hr;
							for (i = 0; i < adjacentEdgeCount; i++)
							{
								if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) return hr;
								ipCurrentEdge = ipElement;
								if (FAILED(hr = ipNetworkForwardStarAdjacencies->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) return hr;
								if (FAILED(hr = ipNetworkForwardStarEx->get_IsRestricted(ipCurrentEdge, &isRestricted))) return hr;
								if (isRestricted) continue;
								if (FAILED(hr = ipCurrentEdge->get_EID(&eid))) return hr;
								if (FAILED(hr = ipCurrentEdge->get_Direction(&dir))) return hr;
								if (edge->Direction == dir && edge->EID == eid) restricted = false;
							}
						}
						if (!restricted)
						{
							if (TimeToBeat >  iterator->second->h + myVertex->g)
							{
								BetterSafeZone = iterator->second;
								TimeToBeat = iterator->second->h + myVertex->g;
								finalVertex = myVertex;
							}
						}
					}

					// termination condition is when we reach the TimeToBeat radius.
					// this could either be the maximum allowed or it could mean that
					// the already discovered safe zone is the closest.
					if (myVertex->g > TimeToBeat) break;

					// Query adjacencies from the current junction.
					if (FAILED(hr = myEdge->NetEdge->get_TurnParticipationType(&turnType))) return hr;
					if (turnType == 1) lastExteriorEdge = myEdge->LastExteriorEdge;
					else lastExteriorEdge = 0;
					if (FAILED(hr = ipNetworkForwardStarEx->QueryAdjacencies(myVertex->Junction, myEdge->NetEdge, lastExteriorEdge, ipNetworkForwardStarAdjacencies))) return hr;
					if (turnType == 2) lastExteriorEdge = myEdge->NetEdge;

					// Get the adjacent edge count
					// Loop through all adjacent edges and update their cost value
					if (FAILED(hr = ipNetworkForwardStarAdjacencies->get_Count(&adjacentEdgeCount))) return hr;

					for (i = 0; i < adjacentEdgeCount; i++)
					{
						if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
						if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) return hr;					
						ipCurrentEdge = ipElement;
						ipCurrentJunction = ipOtherElement;

						if (FAILED(hr = ipNetworkForwardStarAdjacencies->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) return hr;
						if (FAILED(hr = ipCurrentEdge->QueryJunctions(0, ipCurrentJunction))) return hr;					

						// check restriction for the recently discovered edge
						if (FAILED(hr = ipNetworkForwardStarEx->get_IsRestricted(ipCurrentEdge, &isRestricted))) return hr;
						if (isRestricted) continue;

						// if edge has already been discovered then no need to heap it
						currentEdge = ecache->New(ipCurrentEdge);
						if (closedList->IsClosed(currentEdge)) continue;

						// multi-part turn restriction flags
						if (FAILED(hr = ipCurrentEdge->get_TurnParticipationType(&turnType))) return hr;
						if (turnType == 1) currentEdge->LastExteriorEdge = lastExteriorEdge;
						else currentEdge->LastExteriorEdge = 0;

						newCost = myVertex->g + currentEdge->GetCost(population2Route, solvermethod);
						if (heap->IsVisited(currentEdge)) // edge has been visited before. update edge and decrese key.
						{
							neighbor = vcache->New(ipCurrentJunction);
							if (neighbor->g > newCost)
							{
								neighbor->SetBehindEdge(currentEdge);
								neighbor->g = newCost;
								neighbor->Previous = myVertex;
								if (FAILED(hr = heap->DecreaseKey(currentEdge))) return hr;				
							}
						}
						else // unvisited edge. create new and insert in heap
						{						
							neighbor = vcache->New(ipCurrentJunction);
							neighbor->SetBehindEdge(currentEdge);
							neighbor->g = newCost;
							neighbor->Previous = myVertex;
							heap->Insert(currentEdge);
						}
					}
				}

				// TODO: some features of CCRP is not yet implemented
				// 1. CCRP considers vertex/junction capacity as well.

				population2Route = 0.0;
				// generate evacuation route if a destination has been found
				if (BetterSafeZone)
				{
					// First find out about remaining limit of this path
					temp             = finalVertex;
					population2Route = populationLeft;
					if (separable)				
					{
						while (temp->Previous)
						{
							population2Route = min(population2Route, temp->GetBehindEdge()->LeftCapacity());
							temp = temp->Previous;
						}
						if (population2Route <= 0.0) population2Route = populationLeft;	
						population2Route = min(population2Route, populationLeft);			
					}
					populationLeft  -= population2Route;

					// create a new path for this portion of the population
					path = new EvcPath(population2Route);

					// special case for the last edge.
					// We have to subcurve it based on the safezone point location along the edge
					if (BetterSafeZone->GetBehindEdge())
					{
						if (FAILED(hr = BetterSafeZone->GetBehindEdge()->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) return hr;
						edgePortion = BetterSafeZone->h / BetterSafeZone->GetBehindEdge()->OriginalCost;
						if (fromPosition < toPosition) toPosition = fromPosition + edgePortion;
						else toPosition = fromPosition - edgePortion;

						path->push_front(new PathSegment(fromPosition, toPosition, sourceOID, sourceID, BetterSafeZone->GetBehindEdge(), edgePortion));
						BetterSafeZone->GetBehindEdge()->AddReservation(currentEvacuee, 0.0, 0.0, population2Route);
					}

					edgePortion = 1.0;

					while (finalVertex->Previous)
					{
						if (finalVertex->GetBehindEdge())
						{
							if (FAILED(hr = finalVertex->GetBehindEdge()->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) return hr;	
							path->push_front(new PathSegment(fromPosition, toPosition, sourceOID, sourceID, finalVertex->GetBehindEdge(), edgePortion));
							finalVertex->GetBehindEdge()->AddReservation(currentEvacuee, 0.0, 0.0, population2Route);
						}
						finalVertex = finalVertex->Previous;
					}

					// special case for the first edge.
					// We have to subcurve it based on the evacuee point location along the edge
					if (finalVertex->GetBehindEdge())
					{
						if (FAILED(hr = finalVertex->GetBehindEdge()->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) return hr;				
						edgePortion = finalVertex->g / finalVertex->GetBehindEdge()->OriginalCost;
						if (fromPosition < toPosition) fromPosition = toPosition - edgePortion;
						else fromPosition = toPosition + edgePortion;

						lastAdded = path->front();
						if (lastAdded->SourceOID == sourceOID && lastAdded->SourceID == sourceID)
						{
							lastAdded->fromPosition = fromPosition;
							lastAdded->EdgePortion = abs(lastAdded->toPosition - lastAdded->fromPosition); // 2.0 - (lastAdded->EdgePortion + edgePortion);
						}
						else
						{
							path->push_front(new PathSegment(fromPosition, toPosition, sourceOID, sourceID, finalVertex->GetBehindEdge(), edgePortion));
							finalVertex->GetBehindEdge()->AddReservation(currentEvacuee, 0.0, 0.0, population2Route);
						}
					}
					currentEvacuee->paths->push_front(path);
				}
				else
				{
					// since no path could be found for this evacuee, we assume the rest of the
					// population at this location have no path as well
					populationLeft = 0.0;
				}

				// cleanup search heap and closedlist
				heap->Clear();
				closedList->Clear();
			} // end of while loop for multiple routes single evacuee

			// cleanup vertices of this evacuee since all its population is routed.
			for(vit = currentEvacuee->vertices->begin(); vit != currentEvacuee->vertices->end(); vit++) delete (*vit);	
			currentEvacuee->vertices->clear();
		} // end of for loop over sortedEvacuees (reverse)
	}
	while (!sortedEvacuees->empty());

	delete closedList;
	delete sortedEvacuees;
	return S_OK;
}

HRESULT EvcSolver::RunHeuristic(INetworkQueryPtr ipNetworkQuery, IGPMessages* pMessages, ITrackCancel* pTrackCancel, EvacueeList * Evacuees, EvacueeList * SortedEvacuees,
								NAVertexCache * vcache, NAEdgeCache * ecache, NAVertexTable * safeZoneList, INetworkForwardStarExPtr ipNetworkBackwardStarEx, int CountReturnEvacuees)
{
	HRESULT hr;

	// performing pre-process
	// here we will mark each vertex/junction with a heuristic value indicating
	// true distance to closest safe zone using backward traversal and Dijkstra

	// creating the heap for the dijkstra search
	long adjacentEdgeCount, i, currentJunctionEID;
	FibonacciHeap * heap = new FibonacciHeap();
	NAEdgeClosed * closedList = new NAEdgeClosed();
	NAEdge * currentEdge;
	NAVertexPtr neighbor, evc, tempEvc;
	std::vector<EvacueePtr>::iterator eit;
	std::vector<EvacueePtr>::reverse_iterator rseit;
	std::vector<NAVertexPtr>::iterator vit;
	NAVertexTableItr cit;
	INetworkElementPtr ipElementEdge;
	std::vector<EvacueePtr> * pairs;
	std::vector<EvacueePtr>::iterator eitr;
	VARIANT val;
	val.vt = VT_R8; // double variant value
	NAVertexTableItr iterator;
	NAVertexPtr myVertex;
	NAEdgePtr myEdge;
	INetworkElementPtr ipElement, ipOtherElement;
	double fromPosition, toPosition, newh;
	VARIANT_BOOL keepGoing, isRestricted;
	INetworkEdgePtr ipCurrentEdge;
	INetworkJunctionPtr ipCurrentJunction;
	EvacueeList * redundentSortedEvacuees = new EvacueeList();
	redundentSortedEvacuees->reserve(Evacuees->size());

	NAEvacueeVertexTable * EvacueePairs = new NAEvacueeVertexTable();
	EvacueePairs->Insert(Evacuees);

	ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement);
	ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement);

	// Create a backward Star Adjacencies object (we need this object to hold traversal queries carried out on the Backward Star)
	INetworkForwardStarAdjacenciesPtr ipNetworkBackwardStarAdjacencies;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipNetworkBackwardStarAdjacencies))) return hr;

	for(iterator = safeZoneList->begin(); iterator != safeZoneList->end(); iterator++)
	{
		evc = iterator->second;
		tempEvc = vcache->New(evc->Junction);
		tempEvc->SetBehindEdge(evc->GetBehindEdge());
		tempEvc->h = evc->h;
		tempEvc->Junction = evc->Junction;
		tempEvc->Previous = 0;
		myEdge = tempEvc->GetBehindEdge();

		if (myEdge) heap->Insert(myEdge);
		else
		{
			// if the start point was a single junction, then all the adjacent edges can be start edges
			if (FAILED(hr = ipNetworkBackwardStarEx->QueryAdjacencies(tempEvc->Junction, 0, 0, ipNetworkBackwardStarAdjacencies))) return hr; 
			if (FAILED(hr = ipNetworkBackwardStarAdjacencies->get_Count(&adjacentEdgeCount))) return hr;
			for (i = 0; i < adjacentEdgeCount; i++)
			{
				if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) return hr;
				ipCurrentEdge = ipElement;
				if (FAILED(hr = ipNetworkBackwardStarAdjacencies->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) return hr;
				myEdge = ecache->New(ipCurrentEdge);
				tempEvc->SetBehindEdge(myEdge);
				heap->Insert(myEdge);
			}
		}
	}

	// Continue traversing the network while the heap has remaining junctions in it
	// this is the actual dijkstra code with backward network traversal. it will only update h value.
	while (!(heap->IsEmpty() || EvacueePairs->IsEmpty()))
	{
		// Remove the next junction EID from the top of the stack
		myEdge = heap->DeleteMin();
		myVertex = myEdge->ToVertex;
		if (FAILED(hr = closedList->Insert(myEdge)))
		{
			// closedList violation happened
			pMessages->AddError(-myEdge->EID, CComBSTR(L"ClosedList Violation Error."));
			return -myEdge->EID;
		}

		// The step progressor is based on discovered evacuee vertices
		pairs = EvacueePairs->Find(myVertex->EID);
		if (pairs)
		{
			for (eitr = pairs->begin(); eitr != pairs->end(); eitr++) redundentSortedEvacuees->push_back(*eitr);
			EvacueePairs->Erase(myVertex->EID);
			// Check to see if the user wishes to continue or cancel the solve
			if (pTrackCancel)
			{
				if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
				if (keepGoing == VARIANT_FALSE) return E_ABORT;			
			}
		}

		// Query adjacencies from the current junction
		if (FAILED(hr = ipNetworkBackwardStarEx->QueryAdjacencies
			(myVertex->Junction, myEdge->NetEdge, 0, ipNetworkBackwardStarAdjacencies))) return hr; 

		// Get the adjacent edge count
		if (FAILED(hr = ipNetworkBackwardStarAdjacencies->get_Count(&adjacentEdgeCount))) return hr;

		// Loop through all adjacent edges and update their cost value
		for (i = 0; i < adjacentEdgeCount; i++)
		{
			if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) return hr;
			if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) return hr;
			ipCurrentEdge = ipElement;
			ipCurrentJunction = ipOtherElement;

			if (FAILED(hr = ipNetworkBackwardStarAdjacencies->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) return hr;
			if (FAILED(hr = ipCurrentEdge->QueryJunctions(ipCurrentJunction, 0))) return hr; 
			if (FAILED(hr = ipCurrentJunction->get_EID(&currentJunctionEID))) return hr;

			// check restriction for the recently discovered edge
			if (FAILED(hr = ipNetworkBackwardStarEx->get_IsRestricted(ipCurrentEdge, &isRestricted))) return hr;
			if (isRestricted) continue;

			// if node has already been discovered then no need to heap it
			currentEdge = ecache->New(ipCurrentEdge);
			if (closedList->IsClosed(currentEdge)) continue;
			newh = myVertex->h + currentEdge->GetCurrentCost();

			if (heap->IsVisited(currentEdge)) // vertex has been visited before. update vertex and decrese key.
			{
				neighbor = vcache->Get(currentJunctionEID);
				if (neighbor->h > newh)
				{
					neighbor->SetBehindEdge(currentEdge);
					neighbor->h = newh;
					neighbor->Previous = myVertex;
					if (FAILED(hr = heap->DecreaseKey(currentEdge))) return hr;	
					vcache->UpdateHeuristic(neighbor);			
				}
			}
			else // unvisited vertex. create new and insert in heap
			{
				neighbor = vcache->New(ipCurrentJunction);
				if (neighbor->h >= MAX_COST || neighbor->h < newh)
				{
					neighbor->SetBehindEdge(currentEdge);
					neighbor->h = newh;
					neighbor->Previous = myVertex;
					heap->Insert(currentEdge);
					vcache->UpdateHeuristic(neighbor);
				}
			}
		}
	}

	// load last 'k'th evacuees into sorted list from the redundent list
	//CountReturnEvacuees
	SortedEvacuees->clear();

	// variable cleanup
	redundentSortedEvacuees->clear();
	delete redundentSortedEvacuees;
	delete closedList;
	delete heap;
	delete EvacueePairs;

	return S_OK;
}
