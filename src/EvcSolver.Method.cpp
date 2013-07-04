#include "stdafx.h"
#include "NameConstants.h"
#include "EvcSolver.h"
#include "FibonacciHeap.h"

HRESULT PrepareLeafEdgesForHeap(INetworkQueryPtr ipNetworkQuery, NAVertexCache * vcache, NAEdgeCache * ecache, FibonacciHeap * heap, NAEdgeContainer * leafs, double minPop2Route, EvcSolverMethod solverMethod);

HRESULT EvcSolver::SolveMethod(INetworkQueryPtr ipNetworkQuery, IGPMessages* pMessages, ITrackCancel* pTrackCancel,
							   IStepProgressorPtr ipStepProgressor, EvacueeList * Evacuees, NAVertexCache * vcache, NAEdgeCache * ecache,
							   NAVertexTable * safeZoneList, INetworkForwardStarExPtr ipForwardStar, INetworkForwardStarExPtr ipBackwardStar, VARIANT_BOOL* pIsPartialSolution, double & carmaSec)
{	
	// creating the heap for the dijkstra search
	FibonacciHeap * heap = new DEBUG_NEW_PLACEMENT FibonacciHeap(&NAEdge::LessThanHur);
	NAEdgeMap * closedList = new DEBUG_NEW_PLACEMENT NAEdgeMap();
	NAEdgeMapTwoGen * carmaClosedList = new DEBUG_NEW_PLACEMENT NAEdgeMapTwoGen();
	NAEdgePtr currentEdge;
	std::vector<EvacueePtr>::iterator seit;
	NAVertexPtr neighbor, BetterSafeZone = 0, finalVertex = 0, myVertex;
	NAEdgePtr myEdge, edge;
	HRESULT hr = S_OK;
	EvacueePtr currentEvacuee;
	VARIANT_BOOL keepGoing, isRestricted;
	double fromPosition, toPosition, populationLeft, population2Route, TimeToBeat = 0.0f, newCost, costLeft = 0.0;
	std::vector<NAVertexPtr>::iterator vit;
	NAVertexTableItr iterator;
	long adjacentEdgeCount, i, eid;
	INetworkEdgePtr ipCurrentEdge, lastExteriorEdge;
	INetworkJunctionPtr ipCurrentJunction;
	INetworkElementPtr ipElement, ipOtherElement;
	esriNetworkEdgeDirection dir;
	bool restricted = false;
	esriNetworkTurnParticipationType turnType;
	EvacueeList * sortedEvacuees = new DEBUG_NEW_PLACEMENT EvacueeList();
	sortedEvacuees->reserve(Evacuees->size());
	unsigned int countEvacueesInOneBucket = 0, countCASPERLoops = 0, sumVisitedDirtyEdge = 0;
	int pathGenerationCount = -1;
	size_t CARMAClosedSize = 0, sumVisitedEdge = 0;
	countCARMALoops = 0;
	NAEdgeContainer * leafs = new DEBUG_NEW_PLACEMENT NAEdgeContainer();
	std::vector<NAEdgePtr> * readyEdges = new DEBUG_NEW_PLACEMENT std::vector<NAEdgePtr>();
	HANDLE proc = GetCurrentProcess();
	BOOL dummy;
	FILETIME cpuTimeS, cpuTimeE, sysTimeS, sysTimeE, createTime, exitTime;

	// Create a Forward Star Adjacencies object (we need this object to hold traversal queries carried out on the Forward Star)
	INetworkForwardStarAdjacenciesPtr ipForwardAdj;
	INetworkForwardStarAdjacenciesPtr ipBackwardAdj;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipForwardAdj))) goto END_OF_FUNC;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipBackwardAdj))) goto END_OF_FUNC; 

	///////////////////////////////////////
	// Setup a message on our step progress bar indicating that we are traversing the network
	if (ipStepProgressor)
	{
		// Setup our progress bar based on the number of Evacuee points
		if (FAILED(hr = ipStepProgressor->put_MinRange(0))) goto END_OF_FUNC;
		if (FAILED(hr = ipStepProgressor->put_MaxRange((long)(Evacuees->size())))) goto END_OF_FUNC;
		if (FAILED(hr = ipStepProgressor->put_StepValue(1))) goto END_OF_FUNC;
		if (FAILED(hr = ipStepProgressor->put_Position(0))) goto END_OF_FUNC;

		if (this->solverMethod == CASPERSolver)
		{
			if (FAILED(hr = ipStepProgressor->put_Message(CComBSTR(L"Performing CASPER search")))) goto END_OF_FUNC;
		}
		else if (this->solverMethod == SPSolver)
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
		// indexing all the population by their surrounding vertices this will be used to sort them by network distance to safe zone.
		// only the last 'k'th evacuees will be bucketed to run each round.		
		// time the carma loop and add up

		dummy = GetProcessTimes(proc, &createTime, &exitTime, &sysTimeS, &cpuTimeS);
		if (FAILED(hr = CARMALoop(ipNetworkQuery, pMessages, pTrackCancel, Evacuees, sortedEvacuees, vcache, ecache, safeZoneList, ipForwardStar, ipBackwardStar, CARMAClosedSize, carmaClosedList, leafs))) goto END_OF_FUNC;
		dummy = GetProcessTimes(proc, &createTime, &exitTime, &sysTimeE, &cpuTimeE);		
		carmaSec += ((*((__int64 *) &cpuTimeE)) - (*((__int64 *) &cpuTimeS)) + (*((__int64 *) &sysTimeE)) - (*((__int64 *) &sysTimeS))) / 10000000.0;

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

			// Step the progress bar before continuing to the next Evacuee point
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
				if (this->solverMethod == CCRPSolver) population2Route = 1.0;
				else population2Route = populationLeft;

				// populate the heap with vertices associated with the current evacuee
				readyEdges->clear();
				for(std::vector<NAVertexPtr>::iterator h = currentEvacuee->vertices->begin(); h != currentEvacuee->vertices->end(); h++)
					if (FAILED(hr = PrepareVerticesForHeap(*h, vcache, ecache, closedList, readyEdges, population2Route, ipBackwardStar,
						ipBackwardAdj, ipNetworkQuery, this->solverMethod))) goto END_OF_FUNC;
				for(std::vector<NAEdgePtr>::iterator h = readyEdges->begin(); h != readyEdges->end(); h++) heap->Insert(*h);

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

					if (myEdge->IsDirty(1.0, this->solverMethod)) sumVisitedDirtyEdge++;

					// Check for destinations. If a new destination has been found then we should
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
							if (FAILED(hr = ipForwardStar->QueryAdjacencies(myVertex->Junction, myEdge->NetEdge , 0, ipForwardAdj))) goto END_OF_FUNC;
							if (FAILED(hr = ipForwardAdj->get_Count(&adjacentEdgeCount))) goto END_OF_FUNC;
							for (i = 0; i < adjacentEdgeCount; i++)
							{
								if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) goto END_OF_FUNC;
								ipCurrentEdge = ipElement;
								if (FAILED(hr = ipForwardAdj->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) goto END_OF_FUNC;
								if (FAILED(hr = ipForwardStar->get_IsRestricted(ipCurrentEdge, &isRestricted))) goto END_OF_FUNC;
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

					// Query adjacencies from the current junction.
					if (FAILED(hr = myEdge->NetEdge->get_TurnParticipationType(&turnType))) goto END_OF_FUNC;
					if (turnType == 1) lastExteriorEdge = myEdge->LastExteriorEdge;
					else lastExteriorEdge = 0;
					if (FAILED(hr = ipForwardStar->QueryAdjacencies(myVertex->Junction, myEdge->NetEdge, 0 /*lastExteriorEdge*/, ipForwardAdj))) goto END_OF_FUNC;
					if (turnType == 2) lastExteriorEdge = myEdge->NetEdge;

					// Get the adjacent edge count
					// Loop through all adjacent edges and update their cost value
					if (FAILED(hr = ipForwardAdj->get_Count(&adjacentEdgeCount))) goto END_OF_FUNC;

					for (i = 0; i < adjacentEdgeCount; i++)
					{
						if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) goto END_OF_FUNC;
						if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) goto END_OF_FUNC;					
						ipCurrentEdge = ipElement;
						ipCurrentJunction = ipOtherElement;

						if (FAILED(hr = ipForwardAdj->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) goto END_OF_FUNC;
						if (FAILED(hr = ipCurrentEdge->QueryJunctions(0, ipCurrentJunction))) goto END_OF_FUNC;					

						// check restriction for the recently discovered edge
						if (FAILED(hr = ipForwardStar->get_IsRestricted(ipCurrentEdge, &isRestricted))) goto END_OF_FUNC;
						if (isRestricted) continue;

						// if edge has already been discovered then no need to heap it
						currentEdge = ecache->New(ipCurrentEdge);
						if (closedList->Exist(currentEdge)) continue;

						// multi-part turn restriction flags
						if (FAILED(hr = ipCurrentEdge->get_TurnParticipationType(&turnType))) goto END_OF_FUNC;
						if (turnType == 1) currentEdge->LastExteriorEdge = lastExteriorEdge;
						else currentEdge->LastExteriorEdge = 0;

						newCost = myVertex->g + currentEdge->GetCost(population2Route, this->solverMethod);
						if (heap->IsVisited(currentEdge)) // edge has been visited before. update edge and decrease key.
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

							// Termination Condition: If the new vertex does have a chance to beat the already discovered safe node then add it to the heap.
							if (1.1 * (neighbor->g + neighbor->minh()) <= TimeToBeat) heap->Insert(currentEdge);
						}
					}
				}

				// collect info for carma
				sumVisitedEdge += closedList->Size();

				// generate path for this evacuee if any was found
				GeneratePath(BetterSafeZone, finalVertex, populationLeft, pathGenerationCount, currentEvacuee);

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

				// cleanup search heap and closed-list
				UpdatePeakMemoryUsage();
				heap->Clear();
				closedList->Clear();
			} // end of while loop for multiple routes single evacuee

			// cleanup vertices of this evacuee since all its population is routed.
			for(vit = currentEvacuee->vertices->begin(); vit != currentEvacuee->vertices->end(); vit++) delete (*vit);	
			currentEvacuee->vertices->clear();

			// determine if the previous round of DJs where fast enough and if not break out of the loop and have CARMALoop do something about it
			if (this->solverMethod == CASPERSolver && sumVisitedDirtyEdge > this->CARMAPerformanceRatio * sumVisitedEdge) break;

		} // end of for loop over sortedEvacuees
	}
	while (!sortedEvacuees->empty());

	UpdatePeakMemoryUsage();

END_OF_FUNC:

	_ASSERT(hr >= 0);
	delete readyEdges;
	delete carmaClosedList;
	delete closedList;
	delete heap;
	delete sortedEvacuees;	
	delete leafs;
	return hr;
}

HRESULT EvcSolver::CARMALoop(INetworkQueryPtr ipNetworkQuery, IGPMessages* pMessages, ITrackCancel* pTrackCancel, EvacueeList * Evacuees, EvacueeList * SortedEvacuees, NAVertexCache * vcache,
							 NAEdgeCache * ecache, NAVertexTable * safeZoneList, INetworkForwardStarExPtr ipForwardStar, INetworkForwardStarExPtr ipBackwardStar, size_t & closedSize, NAEdgeMapTwoGen * closedList, NAEdgeContainer * leafs)
{
	HRESULT hr = S_OK;

	// performing pre-process: Here we will mark each vertex/junction with a heuristic value indicating
	// true distance to closest safe zone using backward traversal and Dijkstra

	long adjacentEdgeCount, i;
	FibonacciHeap * heap = new DEBUG_NEW_PLACEMENT FibonacciHeap(&NAEdge::LessThanNonHur);	// creating the heap for the dijkstra search
	NAEdge * currentEdge;
	NAVertexPtr neighbor;
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
	double fromPosition, toPosition, minPop2Route = 1.0, newCost, lastCost = 0.0, EdgeCostToBeat;
	VARIANT_BOOL keepGoing, isRestricted;
	INetworkEdgePtr ipCurrentEdge;
	INetworkJunctionPtr ipCurrentJunction;
	EvacueeList * redundentSortedEvacuees = new DEBUG_NEW_PLACEMENT EvacueeList();	
	redundentSortedEvacuees->reserve(Evacuees->size());	
	std::vector<NAEdgePtr> * readyEdges = new DEBUG_NEW_PLACEMENT std::vector<NAEdgePtr>();
	readyEdges->reserve(safeZoneList->size());

	// keeping reachable evacuees in a new hashtable for better access
	NAEvacueeVertexTable * EvacueePairs = new DEBUG_NEW_PLACEMENT NAEvacueeVertexTable();
	EvacueePairs->InsertReachable(Evacuees);

	// also keep unreachable ones in the redundant list	
	for(std::vector<EvacueePtr>::iterator i = Evacuees->begin(); i != Evacuees->end(); i++)	if (!((*i)->Reachable)) redundentSortedEvacuees->push_back(*i);

	ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement);
	ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement);

	// Create a backward Star Adjacencies object (we need this object to hold traversal queries carried out on the Backward Star)
	INetworkForwardStarAdjacenciesPtr ipBackwardAdj;
	INetworkForwardStarAdjacenciesPtr ipForwardAdj;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipBackwardAdj))) goto END_OF_FUNC;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipForwardAdj))) goto END_OF_FUNC;

	// closedList->Clear();        // with this line commented we go to dynamic carma
	closedList->MarkAllAsOldGen(); // this is what we have to do instead of cleaning

	// search for min population on graph evacuees left to be routed. The next if has to be in-tune with what population will be routed next.
	// the h values should always be an underestimation
	if (this->solverMethod != CCRPSolver && !separable)
	{
		minPop2Route = FLT_MAX;
		for(EvacueeListItr eit = Evacuees->begin(); eit != Evacuees->end(); eit++)
		{
			if ((*eit)->vertices->empty() || (*eit)->Population <= 0.0) continue;
			minPop2Route = min(minPop2Route, (*eit)->Population);
		}
		minPop2Route = max(minPop2Route, 1.0);
	}

	// This is where the new dynamic CARMA starts. At this point you have to clear the dirty section of the carma tree.
	// also keep the previous leafs only if they are still in closedList. They help re-discover EvacueePairs
	MarkDirtyEdgesAsUnVisited(closedList->oldGen, leafs, minPop2Route, this->solverMethod);

	// prepare and insert safe zone vertices into the heap
	for (NAVertexTableItr i = safeZoneList->begin(); i != safeZoneList->end(); i++)
		if (FAILED(hr = PrepareVerticesForHeap(i->second, vcache, ecache, closedList->oldGen, readyEdges, minPop2Route, ipForwardStar, ipForwardAdj, ipNetworkQuery, solverMethod))) goto END_OF_FUNC;	
	for(std::vector<NAEdgePtr>::iterator h = readyEdges->begin(); h != readyEdges->end(); h++) heap->Insert(*h);

	// Now insert leaf edges in heap like the destination edges
	if (FAILED(hr = PrepareLeafEdgesForHeap(ipNetworkQuery, vcache, ecache, heap, leafs, minPop2Route, this->solverMethod))) goto END_OF_FUNC;	

	// we're done with all these leafs. let's clean up and collect new ones for the next round.
	leafs->Clear();

	// if this list is not empty, it means we are going to have another CARMA loop
	if (!EvacueePairs->empty()) 
	{
		countCARMALoops++;
#ifdef DEBUG
				std::wostringstream os_;
				os_ << "CARMALoop #" << countCARMALoops << std::endl;
				OutputDebugStringW( os_.str().c_str() );
#endif

		// Continue traversing the network while the heap has remaining junctions in it
		// this is the actual Dijkstra code with backward network traversal. it will only update h value.
		while (!heap->IsEmpty())
		{
			// Remove the next junction EID from the top of the stack
			myEdge = heap->DeleteMin();
			myVertex = myEdge->ToVertex;
			if (closedList->Exist(myEdge))	// closedList violation happened		
			{				
				pMessages->AddError(myEdge->EID, CComBSTR(L"ClosedList Violation Error."));
				hr = -myEdge->EID;				
				goto END_OF_FUNC;
			}
			if (FAILED(hr = closedList->Insert(myEdge))) goto END_OF_FUNC;

			// this value is being recorded and will be used as the default heuristic value for any future vertex
			lastCost = myVertex->g;

			// Code to build the CARMA Tree
			if (myVertex->Previous) 
			{
				if(myEdge->TreePrevious) myEdge->TreePrevious->TreeNextEraseFirst(myEdge);				
				myEdge->TreePrevious = myVertex->Previous->GetBehindEdge();
				myEdge->TreePrevious->TreeNext.push_back(myEdge);
			}

			// part to check if this branch of DJ tree needs expanding to update heuristics
			// This update should know if this is the first time this vertex is coming out
			// in this 'CARMALoop' round. Only then we can be sure whether to update to min
			// or update absolutely to this new value.
			myVertex->UpdateHeuristic(myEdge->EID, myVertex->g, countCARMALoops);
			
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
				leafs->Insert(myEdge); // because this edge helped us find a new evacuee, we save it as a leaf for the next carma loop
			}

			// Query adjacencies from the current junction
			if (FAILED(hr = ipBackwardStar->QueryAdjacencies(myVertex->Junction, myEdge->NetEdge, 0, ipBackwardAdj))) goto END_OF_FUNC; 

			// Get the adjacent edge count
			if (FAILED(hr = ipBackwardAdj->get_Count(&adjacentEdgeCount))) goto END_OF_FUNC;

			// Loop through all adjacent edges and update their cost value
			for (i = 0; i < adjacentEdgeCount; i++)
			{
				if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipOtherElement))) goto END_OF_FUNC;
				if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) goto END_OF_FUNC;
				ipCurrentEdge = ipElement;
				ipCurrentJunction = ipOtherElement;

				if (FAILED(hr = ipBackwardAdj->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) goto END_OF_FUNC;
				if (FAILED(hr = ipCurrentEdge->QueryJunctions(ipCurrentJunction, 0))) goto END_OF_FUNC;

				// check restriction for the recently discovered edge
				if (FAILED(hr = ipBackwardStar->get_IsRestricted(ipCurrentEdge, &isRestricted))) goto END_OF_FUNC;
				if (isRestricted) continue;

				// if node has already been discovered then no need to heap it
				currentEdge = ecache->New(ipCurrentEdge);
				if (closedList->Exist(currentEdge, NAEdgeMap_OLDGEN)) continue;
				
				newCost = myVertex->g + currentEdge->GetCost(minPop2Route, this->solverMethod);
				if (closedList->Exist(currentEdge, NAEdgeMap_NEWGEN))
				{					
					if (FAILED(hr = PrepareUnvisitedVertexForHeap(ipCurrentJunction, currentEdge, myEdge, newCost - myVertex->g, myVertex, ecache,
						                                          closedList, vcache, ipForwardStar, ipForwardAdj, ipNetworkQuery))) goto END_OF_FUNC;
					EdgeCostToBeat = currentEdge->ToVertex->GetH(currentEdge->EID);
					if(EdgeCostToBeat <= currentEdge->ToVertex->g) continue;
					closedList->Erase(currentEdge, NAEdgeMap_NEWGEN);
					heap->Insert(currentEdge);
				}

				if (heap->IsVisited(currentEdge)) // vertex has been visited before. update vertex and decrease key.
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
					// termination condition and evacuee discovery
					#pragma message (__FILE__ "(" STRING(__LINE__) "): warning : [TODO] I still don't know how to terminate properly")
					// if (!EvacueePairs->empty())
					{
						if (FAILED(hr = PrepareUnvisitedVertexForHeap(ipCurrentJunction, currentEdge, myEdge, newCost - myVertex->g, myVertex, ecache,
																	  closedList, vcache, ipForwardStar, ipForwardAdj, ipNetworkQuery))) goto END_OF_FUNC;
						heap->Insert(currentEdge);
					}
				}
			}
		}
	}
	/// Check the frequency of H counts for each vertex
#ifdef DEBUG
	vcache->PrintVertexHeuristicFeq();
#endif

	// set new default heuristic value
	vcache->UpdateHeuristicForOutsideVertices(lastCost, this->countCARMALoops);

	// load evacuees into sorted list from the redundant list in reverse
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
	closedSize = closedList->Size();	
	ecache->CleanAllEdgesAndRelease(minPop2Route, this->solverMethod); // set graph as having all clean edges

#ifdef TRACE
	std::ofstream f;
	f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
	f << "CARMA visited edges = " << closedSize << std::endl;
	f.close();
#endif

	// variable cleanup
END_OF_FUNC:

	delete readyEdges;
	delete redundentSortedEvacuees;	
	delete heap;
	delete EvacueePairs;
	return hr;
}

void EvcSolver::MarkDirtyEdgesAsUnVisited(NAEdgeMap * closedList, NAEdgeContainer * oldLeafs, double minPop2Route, EvcSolverMethod method) const
{
	std::vector<NAEdgePtr> * dirtyVisited = new DEBUG_NEW_PLACEMENT std::vector<NAEdgePtr>();
	std::vector<NAEdgePtr>::iterator i;
	NAEdgeIterator j;
	dirtyVisited->reserve(closedList->Size());
	NAEdgeContainer * tempLeafs = new DEBUG_NEW_PLACEMENT NAEdgeContainer();

	closedList->GetDirtyEdges(dirtyVisited, minPop2Route, method);
	
	for(i = dirtyVisited->begin(); i != dirtyVisited->end(); i++)	
		if (closedList->Exist(*i))
		{
			NAEdgePtr leaf = *i;
			while (leaf->TreePrevious && leaf->IsDirty(minPop2Route, method)) leaf = leaf->TreePrevious;
			NonRecursiveMarkAndRemove(leaf, closedList);

			// What is the definition of a leaf edge? An edge that has a previous (so it's not a destination edge) and has at least one dirty child edge.
			// So the usual for loop is going to insert destination dirty edges and the rest are in the leafs list.
			tempLeafs->Insert(leaf->EID, leaf->Direction);
		}		

	// removing previously identified leafs from closedList
	for (j = oldLeafs->begin(); j != oldLeafs->end(); j++)
	{
		if ((j->second & 1) && closedList->Exist(j->first, esriNEDAlongDigitized))
		{
			closedList->Erase(j->first, esriNEDAlongDigitized);
			tempLeafs->Insert(j->first, esriNEDAlongDigitized);
		}
		if ((j->second & 2) && closedList->Exist(j->first, esriNEDAgainstDigitized))
		{
			closedList->Erase(j->first, esriNEDAgainstDigitized);
			tempLeafs->Insert(j->first, esriNEDAgainstDigitized);
		}
	}
	oldLeafs->Clear();
	oldLeafs->Insert(tempLeafs);

	delete dirtyVisited;
	delete tempLeafs;
}

// this recursive call would be better as a loop ... possible stack overflow in feature
void EvcSolver::RecursiveMarkAndRemove(NAEdgePtr e, NAEdgeMap * closedList) const
{
	closedList->Erase(e);
	e->SetDirty();
	for(std::vector<NAEdgePtr>::iterator i = e->TreeNext.begin(); i != e->TreeNext.end(); i++) 
	{
		(*i)->TreePrevious = NULL;
		RecursiveMarkAndRemove(*i, closedList);
	}
	e->TreeNext.clear();
}

void EvcSolver::NonRecursiveMarkAndRemove(NAEdgePtr head, NAEdgeMap * closedList) const
{
	NAEdgePtr e = NULL;
	std::stack<NAEdgePtr> subtree;
	subtree.push(head);
	while (!subtree.empty())
	{
		e = subtree.top();
		subtree.pop();
		closedList->Erase(e);
		e->SetDirty();
		for(std::vector<NAEdgePtr>::iterator i = e->TreeNext.begin(); i != e->TreeNext.end(); i++) 
		{
			(*i)->TreePrevious = NULL;
			subtree.push(*i);
		}
		e->TreeNext.clear();
	}
}

HRESULT InsertLeafEdgeToHeap(INetworkQueryPtr ipNetworkQuery, NAVertexCache * vcache, NAEdgeCache * ecache, FibonacciHeap * heap, NAEdge * leaf, double minPop2Route, EvcSolverMethod solverMethod)
{
	HRESULT hr = S_OK;
	INetworkElementPtr fe, te;
	INetworkJunctionPtr f, t;

	// if it does not have a previous, then it's not a leaf ... it's a destination edge and it will be added to the heap at 'PrepareVerticesForHeap'
	if (leaf->TreePrevious)
	{
		// leaf by definition has to be a clean edge with a positive clean cost
		_ASSERT(leaf->GetCleanCost() > 0.0);
		_ASSERT(!leaf->IsDirty(minPop2Route, solverMethod));

		if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &fe))) return hr;
		if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &te))) return hr;
		f = fe; t = te;
		if (FAILED(hr = leaf->NetEdge->QueryJunctions(f, t))) return hr;
		NAVertexPtr fPtr = vcache->New(f);
		NAVertexPtr tPtr = vcache->Get(t);

		fPtr->SetBehindEdge(leaf);
		fPtr->g = tPtr->GetH(leaf->TreePrevious->EID) + leaf->GetCleanCost();    // tPtr->GetH(leaf->TreePrevious->EID) + leaf->GetCost(minPop2Route, solverMethod);
		fPtr->Previous = NULL;
		_ASSERT(fPtr->g < FLT_MAX);
		heap->Insert(leaf);
	}
	return hr;
}

HRESULT PrepareLeafEdgesForHeap(INetworkQueryPtr ipNetworkQuery, NAVertexCache * vcache, NAEdgeCache * ecache, FibonacciHeap * heap, NAEdgeContainer * leafs, double minPop2Route, EvcSolverMethod solverMethod)
{
	HRESULT hr = S_OK;
	NAEdgePtr leaf;

	for (NAEdgeIterator i = leafs->begin(); i != leafs->end(); i++)
	{
		if (i->second & 1)
		{
			leaf = ecache->Get(i->first, esriNEDAlongDigitized);
			if (FAILED(hr = InsertLeafEdgeToHeap(ipNetworkQuery, vcache, ecache, heap, leaf, minPop2Route, solverMethod))) return hr;
		}
		if (i->second & 2)
		{
			leaf = ecache->Get(i->first, esriNEDAgainstDigitized);
			if (FAILED(hr = InsertLeafEdgeToHeap(ipNetworkQuery, vcache, ecache, heap, leaf, minPop2Route, solverMethod))) return hr;
		}
	}
	return hr;
}

HRESULT EvcSolver::PrepareUnvisitedVertexForHeap(INetworkJunctionPtr junction, NAEdgePtr edge, NAEdgePtr prevEdge, double edgeCost, NAVertexPtr myVertex, NAEdgeCache * ecache, NAEdgeMapTwoGen * closedList,
												 NAVertexCache * vcache, INetworkForwardStarExPtr ipForwardStar, INetworkForwardStarAdjacenciesPtr ipForwardAdj, INetworkQueryPtr ipNetworkQuery, bool checkOldClosedlist) const
{
	HRESULT hr = S_OK;
	long adjacentEdgeCount;
	INetworkElementPtr elementE/*, elementJ*/;
	INetworkEdgePtr tempNetEdge;
	// INetworkJunctionPtr INbetterMyVertex;
	NAVertexPtr betterMyVertex;
	NAEdgePtr tempEdge, betterEdge = NULL;
	VARIANT_BOOL isRestricted;
	double betterH, tempH, from, to;
	NAVertexPtr tempVertex, neighbor;

	// Dynamic CARMA: at this step we have to check if there is any better previous edge for this new one in closed-list
	tempVertex = vcache->Get(myVertex->EID); // this is the vertex at the center of two edges... we have to check its heuristics to see if the new twempEdge is any better.
	betterH = myVertex->g;
	
	#pragma message (__FILE__ "(" STRING(__LINE__) "): warning : [TODO] so is it really called only for newly added edges?")
	/// if (checkOldClosedlist)
	{
		if (FAILED(hr = ipForwardStar->QueryAdjacencies(myVertex->Junction, edge->NetEdge, 0, ipForwardAdj))) return hr;
		if (FAILED(hr = ipForwardAdj->get_Count(&adjacentEdgeCount))) return hr;

		// Loop through all adjacent edges and update their cost value
		for (long i = 0; i < adjacentEdgeCount; i++)
		{
			if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &elementE))) return hr;
			tempNetEdge = elementE;
			if (FAILED(hr = ipForwardAdj->QueryEdge(i, tempNetEdge, &from, &to))) return hr;
		
			// check restriction for the recently discovered edge
			if (FAILED(hr = ipForwardStar->get_IsRestricted(tempNetEdge, &isRestricted))) return hr;
			if (isRestricted) continue;
			tempEdge = ecache->New(tempNetEdge);
			if (!closedList->Exist(tempEdge, NAEdgeMap_OLDGEN)) continue; // it has to be present in closed list from previous CARMA loop
			if (tempEdge->Direction == prevEdge->Direction && tempEdge->EID == prevEdge->EID) continue; // it cannot be the same parent edge

			// at this point if the new tempEdge satisfied all restrictions and conditions it means it might be a good pick
			// as a previous edge depending on the cost which we shall obtain from vertices heuristic table
			#pragma message (__FILE__ "(" STRING(__LINE__) "): warning : [TODO] is this cost calculation general enough or I should lookup upper vertex minimum H?")
			tempH = tempVertex->GetH(tempEdge->EID);
			if (tempH < betterH) { betterEdge = tempEdge; betterH = tempH; }
		}
	}
	if (betterEdge)
	{
		_ASSERT(checkOldClosedlist);
		betterMyVertex = vcache->New(myVertex->Junction);
		betterMyVertex->SetBehindEdge(betterEdge);
		betterMyVertex->Previous = NULL;
		betterMyVertex->g = betterH;
	}
	else betterMyVertex = myVertex;

	neighbor = vcache->New(junction);
	neighbor->SetBehindEdge(edge);
	neighbor->g = edgeCost + betterMyVertex->g;
	neighbor->Previous = betterMyVertex;
	
	return hr;
}

HRESULT PrepareVerticesForHeap(NAVertexPtr point, NAVertexCache * vcache, NAEdgeCache * ecache, NAEdgeMap * closedList, std::vector<NAEdgePtr> * readyEdges, double pop,
							   INetworkForwardStarExPtr ipStar, INetworkForwardStarAdjacenciesPtr ipStarAdj, INetworkQueryPtr ipNetworkQuery, EvcSolverMethod solverMethod)
{
	HRESULT hr = S_OK;
	NAVertexPtr temp;
	NAEdgePtr edge;
	INetworkEdgePtr ipCurrentEdge;
	INetworkJunctionPtr ipCurrentJunction;
	INetworkElementPtr ipElement;
	long adjacentEdgeCount;
	double fromPosition, toPosition;

	if(readyEdges)
	{
		temp = vcache->New(point->Junction);
		temp->SetBehindEdge(point->GetBehindEdge());
		temp->g = point->g;
		temp->Junction = point->Junction;
		temp->Previous = 0;
		edge = temp->GetBehindEdge();

		// check to see if the edge you're about to insert is not in the closedList
		if (edge) 
		{
			if (!closedList->Exist(edge))
			{
				#pragma message (__FILE__ "(" STRING(__LINE__) "): warning : [TODO] Why do I need to reset the h value for the destination edges?")
				// vcache->Get(temp->EID)->ResetHValues();
				temp->g = point->g * edge->GetCost(pop, solverMethod) / edge->OriginalCost;
				readyEdges->push_back(edge);
			}
		}
		else
		{
			// if the start point was a single junction, then all the adjacent edges can be start edges
			if (FAILED(hr = ipStar->QueryAdjacencies(temp->Junction, 0, 0, ipStarAdj))) return hr; 
			if (FAILED(hr = ipStarAdj->get_Count(&adjacentEdgeCount))) return hr;
			for (long i = 0; i < adjacentEdgeCount; i++)
			{
				if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement))) return hr;
				ipCurrentEdge = ipElement;
				if (FAILED(hr = ipStarAdj->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) return hr;
				edge = ecache->New(ipCurrentEdge);
				if (closedList->Exist(edge)) continue; // dynamic carma condition .... only dirty destination edges are inserted.
				temp = vcache->New(point->Junction);
				temp->Previous = 0;
				temp->SetBehindEdge(edge);
				temp->g = point->g * edge->GetCost(pop, solverMethod) / edge->OriginalCost;
				// vcache->Get(temp->EID)->ResetHValues();
				readyEdges->push_back(edge);
			}
		}
	}
	return hr;
}

HRESULT EvcSolver::GeneratePath(NAVertexPtr BetterSafeZone, NAVertexPtr finalVertex, double & populationLeft, int & pathGenerationCount, EvacueePtr currentEvacuee) const
{
	double population2Route = 0.0, leftCap, fromPosition, toPosition, edgePortion;
	long sourceOID, sourceID;
	HRESULT hr = S_OK;
	PathSegmentPtr lastAdded;
	EvcPath * path = NULL;

	// generate evacuation route if a destination has been found
	if (BetterSafeZone)
	{
		// First find out about remaining capacity of this path
		NAVertexPtr temp = finalVertex;
		population2Route = populationLeft;
		if (separable)				
		{
			while (temp->Previous)
			{
				leftCap = temp->GetBehindEdge()->LeftCapacity();
				if (this->solverMethod == CCRPSolver || leftCap > 0.0) population2Route = min(population2Route, leftCap);
				temp = temp->Previous;
			}
			if (population2Route <= 0.0) population2Route = populationLeft;	
			population2Route = min(population2Route, populationLeft);			
		}
		populationLeft -= population2Route;

		// create a new path for this portion of the population
		path = new DEBUG_NEW_PLACEMENT EvcPath(population2Route, ++pathGenerationCount, currentEvacuee);

		// special case for the last edge.
		// We have to curve it based on the safe point location along the edge
		if (BetterSafeZone->GetBehindEdge())
		{
			if (FAILED(hr = BetterSafeZone->GetBehindEdge()->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) goto END_OF_FUNC;
			edgePortion = BetterSafeZone->g / BetterSafeZone->GetBehindEdge()->OriginalCost;
			if (fromPosition < toPosition) toPosition = fromPosition + edgePortion;
			else toPosition = fromPosition - edgePortion;

			if (edgePortion > 0.0)
			{
				path->push_front(new DEBUG_NEW_PLACEMENT PathSegment(fromPosition, toPosition, sourceOID, sourceID, BetterSafeZone->GetBehindEdge(), edgePortion));
				BetterSafeZone->GetBehindEdge()->AddReservation(population2Route, this->solverMethod);
			}
		}
		edgePortion = 1.0;

		while (finalVertex->Previous)
		{
			if (finalVertex->GetBehindEdge())
			{
				if (FAILED(hr = finalVertex->GetBehindEdge()->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) goto END_OF_FUNC;	
				path->push_front(new DEBUG_NEW_PLACEMENT PathSegment(fromPosition, toPosition, sourceOID, sourceID, finalVertex->GetBehindEdge(), edgePortion));
				finalVertex->GetBehindEdge()->AddReservation(population2Route, this->solverMethod);
			}
			finalVertex = finalVertex->Previous;
		}

		// special case for the first edge.
		// We have to curve it based on the evacuee point location along the edge
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
				path->push_front(new DEBUG_NEW_PLACEMENT PathSegment(fromPosition, toPosition, sourceOID, sourceID, finalVertex->GetBehindEdge(), edgePortion));
				finalVertex->GetBehindEdge()->AddReservation(population2Route, this->solverMethod);
			}
		}
		currentEvacuee->paths->push_front(path);
	}
	else
	{		
		populationLeft = 0.0; // since no path could be found for this evacuee, we assume the rest of the population at this location have no path as well
	}

END_OF_FUNC:

	if (path && FAILED(hr)) delete path;
	return hr;
}

void EvcSolver::UpdatePeakMemoryUsage()
{
	PROCESS_MEMORY_COUNTERS pmc;
	if(!hProcessPeakMemoryUsage) hProcessPeakMemoryUsage = GetCurrentProcess();
	if (GetProcessMemoryInfo(hProcessPeakMemoryUsage, &pmc, sizeof(pmc))) peakMemoryUsage = max(peakMemoryUsage, pmc.PagefileUsage);
}
