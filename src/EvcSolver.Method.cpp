#include "stdafx.h"
#include "NameConstants.h"
#include "EvcSolver.h"
#include "FibonacciHeap.h"

HRESULT EvcSolver::SolveMethod(INetworkQueryPtr ipNetworkQuery, IGPMessages* pMessages, ITrackCancel* pTrackCancel,
							   IStepProgressorPtr ipStepProgressor, EvacueeList * Evacuees, NAVertexCache * vcache, NAEdgeCache * ecache,
							   NAVertexTable * safeZoneList, INetworkForwardStarExPtr ipNetworkForwardStarEx, INetworkForwardStarExPtr ipNetworkBackwardStarEx, VARIANT_BOOL* pIsPartialSolution)
{	
	// creating the heap for the dijkstra search
	FibonacciHeap * heap = new DEBUG_NEW_PLACEMENT FibonacciHeap(&NAEdge::LessThanHur);
	NAEdgeClosed * closedList = new DEBUG_NEW_PLACEMENT NAEdgeClosed();
	NAEdgeClosed * carmaClosedList = new DEBUG_NEW_PLACEMENT NAEdgeClosed();
	NAEdgePtr currentEdge;
	std::vector<EvacueePtr>::iterator seit;
	NAVertexPtr neighbor, evc, tempEvc, BetterSafeZone = 0, finalVertex = 0, myVertex, temp;
	NAEdgePtr myEdge, edge;
	HRESULT hr = S_OK;
	EvacueePtr currentEvacuee;
	VARIANT_BOOL keepGoing, isRestricted;
	double fromPosition, toPosition, edgePortion = 1.0, populationLeft, population2Route, leftCap, TimeToBeat = 0.0f, newCost, costLeft = 0.0;
	std::vector<NAVertexPtr>::iterator vit;
	NAVertexTableItr iterator;
	long adjacentEdgeCount, i, sourceOID, sourceID, eid;
	INetworkEdgePtr ipCurrentEdge, lastExteriorEdge;
	INetworkJunctionPtr ipCurrentJunction;
	INetworkElementPtr ipElement, ipOtherElement;
	PathSegment * lastAdded = 0;
	EvcPathPtr path;
	esriNetworkEdgeDirection dir;
	bool restricted = false;
	esriNetworkTurnParticipationType turnType;
	EvacueeList * sortedEvacuees = new DEBUG_NEW_PLACEMENT EvacueeList();
	sortedEvacuees->reserve(Evacuees->size());
	unsigned int countEvacueesInOneBucket = 0, countCASPERLoops = 0, sumVisitedDirtyEdge = 0, sumVisitedEdge = 0;
	int pathGenerationCount = -1;
	size_t CARMAClosedSize = 0;
	countCARMALoops = 0;
	
	// Create a Forward Star Adjacencies object (we need this object to hold traversal queries carried out on the Forward Star)
	INetworkForwardStarAdjacenciesPtr ipNetworkForwardStarAdjacencies;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipNetworkForwardStarAdjacencies))) goto END_OF_FUNC; 

	///////////////////////////////////////
	// Setup a message on our step progressor indicating that we are traversing the network
	if (ipStepProgressor)
	{
		// Setup our progressor based on the number of Evacuee points
		if (FAILED(hr = ipStepProgressor->put_MinRange(0))) goto END_OF_FUNC;
		if (FAILED(hr = ipStepProgressor->put_MaxRange((long)(Evacuees->size())))) goto END_OF_FUNC;
		if (FAILED(hr = ipStepProgressor->put_StepValue(1))) goto END_OF_FUNC;
		if (FAILED(hr = ipStepProgressor->put_Position(0))) goto END_OF_FUNC;

		if (this->solverMethod == EVC_SOLVER_METHOD_CASPER)
		{
			if (FAILED(hr = ipStepProgressor->put_Message(CComBSTR(L"Performing CASPER search")))) goto END_OF_FUNC;
		}
		else if (this->solverMethod == EVC_SOLVER_METHOD_SP)
		{
			if (FAILED(hr = ipStepProgressor->put_Message(CComBSTR(L"Performing SP search")))) goto END_OF_FUNC;
		}
		else
		{
			if (FAILED(hr = ipStepProgressor->put_Message(CComBSTR(L"Performing CCRP search")))) goto END_OF_FUNC;
		}		
	}

	do
	{
		// indexing all the population by their sorrounding vertices this will be used to sort them by network distance to safe zone.
		// only the last 'k'th evacuees will be bucketed to run each round.
		if (FAILED(hr = CARMALoop(ipNetworkQuery, pMessages, pTrackCancel, Evacuees, sortedEvacuees, vcache, ecache, safeZoneList, ipNetworkForwardStarEx, ipNetworkBackwardStarEx, CARMAClosedSize, carmaClosedList))) goto END_OF_FUNC;		

		countEvacueesInOneBucket = 0;
		sumVisitedDirtyEdge = 0;
		sumVisitedEdge = 0;

		for(seit = sortedEvacuees->begin(); seit != sortedEvacuees->end(); seit++)
		{
			currentEvacuee = *seit;	

			// Check to see if the user wishes to continue or cancel the solve (i.e., check whether or not the user has hit the ESC key to stop processing)
			if (pTrackCancel)
			{
				if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) goto END_OF_FUNC;
				if (keepGoing == VARIANT_FALSE)
				{
					hr = E_ABORT;
					goto END_OF_FUNC;
				}
			}

			// if the CARMALoop decided this evacuee is not reachable, then we're not even going to try solving it.
			if (!(currentEvacuee->Reachable))
			{				
				// cleanup vertices of this evacuee
				for(vit = currentEvacuee->vertices->begin(); vit != currentEvacuee->vertices->end(); vit++) delete (*vit);	
				currentEvacuee->vertices->clear();
				*pIsPartialSolution = VARIANT_TRUE;
			}

			if (currentEvacuee->vertices->size() == 0) continue;

			// Step the progressor before continuing to the next Evacuee point
			if (ipStepProgressor) ipStepProgressor->Step();	
			countEvacueesInOneBucket++;
			countCASPERLoops++;
			populationLeft = currentEvacuee->Population;
			
			while (populationLeft > 0.0)
			{
				// It's now safe to collect-n-clean on the graph (ecache & vcache).
				// clean used-up vertices from GC
				vcache->CollectAndRelease();

				// the next 'if' is a distinctive feature by CASPER that CCRP does not have
				// and can actually improve routes even with a STEP traffic model
				if (this->solverMethod == EVC_SOLVER_METHOD_CCRP) population2Route = 1.0;
				else population2Route = populationLeft;

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

					if (myEdge)
					{
						tempEvc->g = evc->g * myEdge->GetCost(population2Route, this->solverMethod) / myEdge->OriginalCost;
						heap->Insert(myEdge);
					}
					else
					{
						// if the start point was a single junction, then all the adjacent edges can be start edges
						if (FAILED(hr = ipNetworkBackwardStarEx->QueryAdjacencies(tempEvc->Junction, 0, 0, ipNetworkForwardStarAdjacencies))) goto END_OF_FUNC; 
						if (FAILED(hr = ipNetworkForwardStarAdjacencies->get_Count(&adjacentEdgeCount))) goto END_OF_FUNC;
						for (i = 0; i < adjacentEdgeCount; i++)
						{
							if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) goto END_OF_FUNC;
							ipCurrentEdge = ipElement;
							if (FAILED(hr = ipNetworkForwardStarAdjacencies->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) goto END_OF_FUNC;
							myEdge = ecache->New(ipCurrentEdge);
							tempEvc = vcache->New(evc->Junction);
							tempEvc->Previous = 0;
							tempEvc->SetBehindEdge(myEdge);
							tempEvc->g = evc->g * myEdge->GetCost(population2Route, this->solverMethod) / myEdge->OriginalCost;
							heap->Insert(myEdge);
						}
					}
				}

				TimeToBeat = FLT_MAX;
				BetterSafeZone = 0;
				finalVertex = 0;
				
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
						hr = -myEdge->EID;
						goto END_OF_FUNC;
					}

					if (myEdge->IsDirty()) sumVisitedDirtyEdge++;

					// Check for destinations. If a new destination has been found then we sould
					// first flag this so later we can use to generate route. Also we should
					// update the new TimeToBeat value for proper termination.
					iterator = safeZoneList->find(myVertex->EID);
					if (iterator != safeZoneList->end())
					{
						// Handle the last turn restriction here ... and the remaining capacity-aware cost.
						edge = iterator->second->GetBehindEdge();
						costLeft = iterator->second->g;
						restricted = false;

						if (edge)
						{
							costLeft *= edge->GetCost(population2Route, this->solverMethod) / edge->OriginalCost;

							restricted = true;
							if (FAILED(hr = ipNetworkForwardStarEx->QueryAdjacencies(myVertex->Junction, myEdge->NetEdge , 0, ipNetworkForwardStarAdjacencies))) goto END_OF_FUNC;
							if (FAILED(hr = ipNetworkForwardStarAdjacencies->get_Count(&adjacentEdgeCount))) goto END_OF_FUNC;
							for (i = 0; i < adjacentEdgeCount; i++)
							{
								if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) goto END_OF_FUNC;
								ipCurrentEdge = ipElement;
								if (FAILED(hr = ipNetworkForwardStarAdjacencies->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) goto END_OF_FUNC;
								if (FAILED(hr = ipNetworkForwardStarEx->get_IsRestricted(ipCurrentEdge, &isRestricted))) goto END_OF_FUNC;
								if (isRestricted) continue;
								if (FAILED(hr = ipCurrentEdge->get_EID(&eid))) goto END_OF_FUNC;
								if (FAILED(hr = ipCurrentEdge->get_Direction(&dir))) goto END_OF_FUNC;
								if (edge->Direction == dir && edge->EID == eid) restricted = false;
							}
						}
						if (!restricted)
						{
							if (TimeToBeat > costLeft + myVertex->g)
							{
								BetterSafeZone = iterator->second;
								TimeToBeat = costLeft + myVertex->g;
								finalVertex = myVertex;
							}
						}
					}

					// termination condition is when we reach the TimeToBeat radius.
					// this could either be the maximum allowed or it could mean that
					// the already discovered safe zone is the closest.
					// I'm going to experiment with a new termination condition at the heap->insert line
					// if (myVertex->g > TimeToBeat) continue;

					// Query adjacencies from the current junction.
					if (FAILED(hr = myEdge->NetEdge->get_TurnParticipationType(&turnType))) goto END_OF_FUNC;
					if (turnType == 1) lastExteriorEdge = myEdge->LastExteriorEdge;
					else lastExteriorEdge = 0;
					if (FAILED(hr = ipNetworkForwardStarEx->QueryAdjacencies(myVertex->Junction, myEdge->NetEdge, 0 /*lastExteriorEdge*/, ipNetworkForwardStarAdjacencies))) goto END_OF_FUNC;
					if (turnType == 2) lastExteriorEdge = myEdge->NetEdge;

					// Get the adjacent edge count
					// Loop through all adjacent edges and update their cost value
					if (FAILED(hr = ipNetworkForwardStarAdjacencies->get_Count(&adjacentEdgeCount))) goto END_OF_FUNC;

					for (i = 0; i < adjacentEdgeCount; i++)
					{
						if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) goto END_OF_FUNC;
						if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) goto END_OF_FUNC;					
						ipCurrentEdge = ipElement;
						ipCurrentJunction = ipOtherElement;

						if (FAILED(hr = ipNetworkForwardStarAdjacencies->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) goto END_OF_FUNC;
						if (FAILED(hr = ipCurrentEdge->QueryJunctions(0, ipCurrentJunction))) goto END_OF_FUNC;					

						// check restriction for the recently discovered edge
						if (FAILED(hr = ipNetworkForwardStarEx->get_IsRestricted(ipCurrentEdge, &isRestricted))) goto END_OF_FUNC;
						if (isRestricted) continue;

						// if edge has already been discovered then no need to heap it
						currentEdge = ecache->New(ipCurrentEdge);
						if (closedList->IsClosed(currentEdge)) continue;

						// multi-part turn restriction flags
						if (FAILED(hr = ipCurrentEdge->get_TurnParticipationType(&turnType))) goto END_OF_FUNC;
						if (turnType == 1) currentEdge->LastExteriorEdge = lastExteriorEdge;
						else currentEdge->LastExteriorEdge = 0;

						newCost = myVertex->g + currentEdge->GetCost(population2Route, this->solverMethod);
						if (heap->IsVisited(currentEdge)) // edge has been visited before. update edge and decrese key.
						{
							neighbor = currentEdge->ToVertex;
							if (neighbor->g > newCost)
							{
								neighbor->SetBehindEdge(currentEdge);
								neighbor->g = newCost;
								neighbor->Previous = myVertex;
								if (FAILED(hr = heap->DecreaseKey(currentEdge))) goto END_OF_FUNC;				
							}
						}
						else // unvisited edge. create new and insert in heap
						{						
							neighbor = vcache->New(ipCurrentJunction);
							neighbor->SetBehindEdge(currentEdge);
							neighbor->g = newCost;
							neighbor->Previous = myVertex;

							// if the new vertex does have a chance to beat the already
							// discovered safe node then add it to the heap
							if (1.1 * (neighbor->g + neighbor->minh()) <= TimeToBeat) heap->Insert(currentEdge);
						}
					}
				}

				sumVisitedEdge += closedList->Size();

				population2Route = 0.0;
				// generate evacuation route if a destination has been found
				if (BetterSafeZone)
				{
					// First find out about remaining capacity of this path
					temp             = finalVertex;
					population2Route = populationLeft;
					if (separable)				
					{
						while (temp->Previous)
						{
							leftCap = temp->GetBehindEdge()->LeftCapacity();
							if (this->solverMethod == EVC_SOLVER_METHOD_CCRP || leftCap > 0.0) population2Route = min(population2Route, leftCap);
							temp = temp->Previous;
						}
						if (population2Route <= 0.0) population2Route = populationLeft;	
						population2Route = min(population2Route, populationLeft);			
					}
					populationLeft -= population2Route;

					// create a new path for this portion of the population
					path = new DEBUG_NEW_PLACEMENT EvcPath(population2Route, ++pathGenerationCount, currentEvacuee);

					// special case for the last edge.
					// We have to subcurve it based on the safezone point location along the edge
					if (BetterSafeZone->GetBehindEdge())
					{
						if (FAILED(hr = BetterSafeZone->GetBehindEdge()->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) goto END_OF_FUNC;
						edgePortion = BetterSafeZone->g / BetterSafeZone->GetBehindEdge()->OriginalCost;
						if (fromPosition < toPosition) toPosition = fromPosition + edgePortion;
						else toPosition = fromPosition - edgePortion;

						if (edgePortion > 0.0)
						{
							path->push_front(new PathSegment(fromPosition, toPosition, sourceOID, sourceID, BetterSafeZone->GetBehindEdge(), edgePortion));
							BetterSafeZone->GetBehindEdge()->AddReservation(population2Route);
						}
					}

					edgePortion = 1.0;

					while (finalVertex->Previous)
					{
						if (finalVertex->GetBehindEdge())
						{
							if (FAILED(hr = finalVertex->GetBehindEdge()->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) goto END_OF_FUNC;	
							path->push_front(new PathSegment(fromPosition, toPosition, sourceOID, sourceID, finalVertex->GetBehindEdge(), edgePortion));
							finalVertex->GetBehindEdge()->AddReservation(population2Route);
						}
						finalVertex = finalVertex->Previous;
					}

					// special case for the first edge.
					// We have to subcurve it based on the evacuee point location along the edge
					if (finalVertex->GetBehindEdge())
					{
						if (FAILED(hr = finalVertex->GetBehindEdge()->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) goto END_OF_FUNC;				
						edgePortion = finalVertex->g / finalVertex->GetBehindEdge()->OriginalCost;
						if (fromPosition < toPosition) fromPosition = toPosition - edgePortion;
						else fromPosition = toPosition + edgePortion;

						lastAdded = path->front();
						if (lastAdded->SourceOID == sourceOID && lastAdded->SourceID == sourceID)
						{
							lastAdded->fromPosition = fromPosition;
							lastAdded->EdgePortion = abs(lastAdded->toPosition - lastAdded->fromPosition); // 2.0 - (lastAdded->EdgePortion + edgePortion);
							_ASSERT(lastAdded->EdgePortion > 0.0);
						}
						else if (edgePortion > 0.0)
						{							
							path->push_front(new PathSegment(fromPosition, toPosition, sourceOID, sourceID, finalVertex->GetBehindEdge(), edgePortion));
							finalVertex->GetBehindEdge()->AddReservation(population2Route);
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

				#ifdef DEBUG
				std::wostringstream os_;
				os_.precision(3);
				os_ << "CARMALoop stat " << countEvacueesInOneBucket << ": " << (int)sumVisitedEdge << ',' << (int)sumVisitedDirtyEdge << ','
				    << sumVisitedDirtyEdge / (CARMAPerformanceRatio * sumVisitedEdge) << std::endl;
				OutputDebugStringW( os_.str().c_str() );
				#endif
				#ifdef TRACE
				std::ofstream f;
				f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
				f.precision(3);
				f << "CARMALoop stat " << countEvacueesInOneBucket << ": " << (int)sumVisitedEdge << ',' << (int)sumVisitedDirtyEdge << ','
				  << sumVisitedDirtyEdge / (CARMAPerformanceRatio * sumVisitedEdge) << std::endl;
				f.close();
				#endif

				// cleanup search heap and closedlist
				UpdatePeakMemoryUsage();
				heap->Clear();
				closedList->Clear();
			} // end of while loop for multiple routes single evacuee

			// cleanup vertices of this evacuee since all its population is routed.
			for(vit = currentEvacuee->vertices->begin(); vit != currentEvacuee->vertices->end(); vit++) delete (*vit);	
			currentEvacuee->vertices->clear();

			// determine if the previous round of DJs where fast enough and if not break out of the loop and have CARMALoop do something about it
			if (this->solverMethod == EVC_SOLVER_METHOD_CASPER && sumVisitedDirtyEdge > this->CARMAPerformanceRatio * sumVisitedEdge) break;

		} // end of for loop over sortedEvacuees
	}
	while (!sortedEvacuees->empty());

	UpdatePeakMemoryUsage();

END_OF_FUNC:

	delete carmaClosedList;
	delete closedList;
	delete heap;
	delete sortedEvacuees;
	return hr;
}

HRESULT EvcSolver::CARMALoop(INetworkQueryPtr ipNetworkQuery, IGPMessages* pMessages, ITrackCancel* pTrackCancel, EvacueeList * Evacuees, EvacueeList * SortedEvacuees, NAVertexCache * vcache,
	NAEdgeCache * ecache, NAVertexTable * safeZoneList, INetworkForwardStarExPtr ipNetworkForwardStarEx, INetworkForwardStarExPtr ipNetworkBackwardStarEx, size_t & closedSize, NAEdgeClosed * closedList)
{
	HRESULT hr = S_OK;

	// performing pre-process
	// here we will mark each vertex/junction with a heuristic value indicating
	// true distance to closest safe zone using backward traversal and Dijkstra

	// creating the heap for the dijkstra search
	long adjacentEdgeCount, i;
	FibonacciHeap * heap = new DEBUG_NEW_PLACEMENT FibonacciHeap(&NAEdge::LessThanNonHur);	
	NAEdge * currentEdge;
	NAVertexPtr neighbor, zone, tempZone;
	std::vector<EvacueePtr>::iterator eit;
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
	double fromPosition, toPosition, minPop2Route = 1.0, newCost, lastCost = 0.0;
	VARIANT_BOOL keepGoing, isRestricted;
	INetworkEdgePtr ipCurrentEdge;
	INetworkJunctionPtr ipCurrentJunction;
	EvacueeList * redundentSortedEvacuees = new DEBUG_NEW_PLACEMENT EvacueeList();
	redundentSortedEvacuees->reserve(Evacuees->size());

	// keeping reachable evacuees in a new hashtable for better access
	NAEvacueeVertexTable * EvacueePairs = new DEBUG_NEW_PLACEMENT NAEvacueeVertexTable();
	EvacueePairs->InsertReachable(Evacuees);

	// also keep unreachable ones in the redundent list	
	for(std::vector<EvacueePtr>::iterator i = Evacuees->begin(); i != Evacuees->end(); i++)	if (!((*i)->Reachable)) redundentSortedEvacuees->push_back(*i);

	ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement);
	ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement);

	// Create a backward Star Adjacencies object (we need this object to hold traversal queries carried out on the Backward Star)
	INetworkForwardStarAdjacenciesPtr ipNetworkBackwardStarAdjacencies;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipNetworkBackwardStarAdjacencies))) goto END_OF_FUNC;

	// search for min population on graph evacuees left to be routed
	// The next if has to be in-tune with what population will be routed next.
	// the h values should always be an underestimation
	if (this->solverMethod != EVC_SOLVER_METHOD_CCRP && !separable)
	{
		minPop2Route = FLT_MAX;
		for(EvacueeListItr eit = Evacuees->begin(); eit != Evacuees->end(); eit++)
		{
			if ((*eit)->vertices->empty() || (*eit)->Population <= 0.0) continue;
			minPop2Route = min(minPop2Route, (*eit)->Population);
		}
		minPop2Route = max(minPop2Route, 1.0);
	}

	for(iterator = safeZoneList->begin(); iterator != safeZoneList->end(); iterator++)
	{
		zone = iterator->second;
		tempZone = vcache->New(zone->Junction);
		tempZone->SetBehindEdge(zone->GetBehindEdge());
		tempZone->g = zone->g;
		tempZone->Junction = zone->Junction;
		tempZone->Previous = 0;
		myEdge = tempZone->GetBehindEdge();
		vcache->Get(tempZone->EID)->ResetHValues();

		if (myEdge) 
		{
			tempZone->g = zone->g * myEdge->GetCost(minPop2Route, this->solverMethod) / myEdge->OriginalCost;
			heap->Insert(myEdge);
		}
		else
		{
			// if the start point was a single junction, then all the adjacent edges can be start edges
			if (FAILED(hr = ipNetworkForwardStarEx->QueryAdjacencies(tempZone->Junction, 0, 0, ipNetworkBackwardStarAdjacencies))) goto END_OF_FUNC; 
			if (FAILED(hr = ipNetworkBackwardStarAdjacencies->get_Count(&adjacentEdgeCount))) goto END_OF_FUNC;
			for (i = 0; i < adjacentEdgeCount; i++)
			{
				if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) goto END_OF_FUNC;
				ipCurrentEdge = ipElement;
				if (FAILED(hr = ipNetworkBackwardStarAdjacencies->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) goto END_OF_FUNC;
				myEdge = ecache->New(ipCurrentEdge);
				tempZone = vcache->New(zone->Junction);
				tempZone->Previous = 0;
				tempZone->SetBehindEdge(myEdge);
				tempZone->g = zone->g * myEdge->GetCost(minPop2Route, this->solverMethod) / myEdge->OriginalCost;
				heap->Insert(myEdge);
			}
		}
	}

	closedList->Clear();

	// if this list is not empty, it means we are going to have another CARMA loop
	if (!EvacueePairs->empty()) 
	{
		countCARMALoops++;

		// Continue traversing the network while the heap has remaining junctions in it
		// this is the actual dijkstra code with backward network traversal. it will only update h value.
		while (!heap->IsEmpty())
		{
			// Remove the next junction EID from the top of the stack
			myEdge = heap->DeleteMin();
			myVertex = myEdge->ToVertex;
			if (FAILED(hr = closedList->Insert(myEdge)))
			{
				// closedList violation happened
				pMessages->AddError(-myEdge->EID, CComBSTR(L"ClosedList Violation Error."));
				hr = -myEdge->EID;
				goto END_OF_FUNC;
			}

			// this value is being recorded and will be used as the default
			// heuristic value for any future vertex
			lastCost = myVertex->g;

			// part to check if this branch of DJ tree needs expanding to update hueristics
			// This update should know if this is the first time this vertex is coming out
			// in this 'CARMALoop' round. Only then we can be sure whether to update to min
			// or update absolutely to this new value.
			vcache->UpdateHeuristic(myEdge->EID, myVertex);

			// termination condition and evacuee discovery
			if (EvacueePairs->empty()) continue;

			pairs = EvacueePairs->Find(myVertex->EID);
			if (pairs)
			{
				for (eitr = pairs->begin(); eitr != pairs->end(); eitr++)
				{
					(*eitr)->PredictedCost = min((*eitr)->PredictedCost, myVertex->g);
					redundentSortedEvacuees->push_back(*eitr);
				}
				EvacueePairs->Erase(myVertex->EID);

				// Check to see if the user wishes to continue or cancel the solve
				if (pTrackCancel)
				{
					if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) goto END_OF_FUNC;
					if (keepGoing == VARIANT_FALSE)
					{
						hr = E_ABORT;			
						goto END_OF_FUNC;
					}
				}
			}

			// Query adjacencies from the current junction
			if (FAILED(hr = ipNetworkBackwardStarEx->QueryAdjacencies(myVertex->Junction, myEdge->NetEdge, 0, ipNetworkBackwardStarAdjacencies))) goto END_OF_FUNC; 

			// Get the adjacent edge count
			if (FAILED(hr = ipNetworkBackwardStarAdjacencies->get_Count(&adjacentEdgeCount))) goto END_OF_FUNC;

			// Loop through all adjacent edges and update their cost value
			for (i = 0; i < adjacentEdgeCount; i++)
			{
				if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) goto END_OF_FUNC;
				if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) goto END_OF_FUNC;
				ipCurrentEdge = ipElement;
				ipCurrentJunction = ipOtherElement;

				if (FAILED(hr = ipNetworkBackwardStarAdjacencies->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) goto END_OF_FUNC;
				if (FAILED(hr = ipCurrentEdge->QueryJunctions(ipCurrentJunction, 0))) goto END_OF_FUNC;

				// check restriction for the recently discovered edge
				if (FAILED(hr = ipNetworkBackwardStarEx->get_IsRestricted(ipCurrentEdge, &isRestricted))) goto END_OF_FUNC;
				if (isRestricted) continue;

				// if node has already been discovered then no need to heap it
				currentEdge = ecache->New(ipCurrentEdge);
				if (closedList->IsClosed(currentEdge)) continue;
				newCost = myVertex->g + currentEdge->GetCost(minPop2Route, this->solverMethod);

				if (heap->IsVisited(currentEdge)) // vertex has been visited before. update vertex and decrese key.
				{
					neighbor = currentEdge->ToVertex;
					if (neighbor->g > newCost)
					{
						neighbor->SetBehindEdge(currentEdge);
						neighbor->g = newCost;
						neighbor->Previous = myVertex;
						if (FAILED(hr = heap->DecreaseKey(currentEdge))) goto END_OF_FUNC;
					}
				}
				else // unvisited vertex. create new and insert in heap
				{
					neighbor = vcache->New(ipCurrentJunction);
					neighbor->SetBehindEdge(currentEdge);
					neighbor->g = newCost;
					neighbor->Previous = myVertex;
					heap->Insert(currentEdge);
				}
			}
		}
	}

	// set new default heurisitc value
	vcache->UpdateHeuristicForOutsideVertices(lastCost, this->countCARMALoops == 1);

	// load evacuees into sorted list from the redundent list in reverse
	SortedEvacuees->clear();
	for (NAEvacueeVertexTableItr evcItr = EvacueePairs->begin(); evcItr != EvacueePairs->end(); evcItr++)
	{
		for(eit = evcItr->second->begin(); eit != evcItr->second->end(); eit++)
		{
			if ((*eit)->PredictedCost >= FLT_MAX)
			{
				(*eit)->Reachable = false;
				#ifdef TRACE
				std::ofstream f;
				f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
				f << "Evacuee " << (*eit)->Name.bstrVal << " is unreachable." << std::endl;
				f.close();
				#endif
			}
			redundentSortedEvacuees->push_back(*eit);
		}
	}
	if (!redundentSortedEvacuees->empty())
	{
		std::sort(redundentSortedEvacuees->begin(), redundentSortedEvacuees->end(), Evacuee::LessThan);		
		SortedEvacuees->insert(SortedEvacuees->begin(), redundentSortedEvacuees->rbegin(), redundentSortedEvacuees->rend());
	}
	UpdatePeakMemoryUsage();
	{
		closedSize = closedList->Size();
		#ifdef TRACE
		std::ofstream f;
		f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
		f << "CARMA visited edges = " << closedSize << std::endl;
		f.close();
		#endif
	}
	// set graph as having all clean edges
	ecache->CleanAllEdgesAndRelease(lastCost);

END_OF_FUNC:

	// variable cleanup
	delete redundentSortedEvacuees;	
	delete heap;
	delete EvacueePairs;

	return hr;
}

void EvcSolver::UpdatePeakMemoryUsage()
{
    PROCESS_MEMORY_COUNTERS pmc;
	if(!hProcessPeakMemoryUsage) hProcessPeakMemoryUsage = GetCurrentProcess();
	if (GetProcessMemoryInfo(hProcessPeakMemoryUsage, &pmc, sizeof(pmc))) peakMemoryUsage = max(peakMemoryUsage, pmc.PagefileUsage);
}
