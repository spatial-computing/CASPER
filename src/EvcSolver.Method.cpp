#include "stdafx.h"
#include "NameConstants.h"
#include "EvcSolver.h"
#include "FibonacciHeap.h"							

HRESULT EvcSolver::SolveMethod(INetworkQueryPtr ipNetworkQuery, IGPMessages* pMessages, ITrackCancel* pTrackCancel, IStepProgressorPtr ipStepProgressor, EvacueeList * Evacuees, NAVertexCache * vcache,
							   NAEdgeCache * ecache, SafeZoneTable * safeZoneList, INetworkForwardStarExPtr ipForwardStar, INetworkForwardStarExPtr ipBackwardStar, VARIANT_BOOL* pIsPartialSolution,
							   double & carmaSec, std::vector<unsigned int> & CARMAExtractCounts, INetworkDatasetPtr ipNetworkDataset)
{	
	// creating the heap for the dijkstra search
	FibonacciHeap * heap = new DEBUG_NEW_PLACEMENT FibonacciHeap(&GetHeapKeyHur);
	NAEdgeMap * closedList = new DEBUG_NEW_PLACEMENT NAEdgeMap();
	NAEdgeMapTwoGen * carmaClosedList = new DEBUG_NEW_PLACEMENT NAEdgeMapTwoGen();
	NAEdgePtr currentEdge;
	std::vector<EvacueePtr>::iterator seit;
	NAVertexPtr neighbor, finalVertex = 0, myVertex;
	SafeZonePtr BetterSafeZone = 0;
	NAEdgePtr myEdge;
	HRESULT hr = S_OK;
	EvacueePtr currentEvacuee;
	VARIANT_BOOL keepGoing;
	double fromPosition, toPosition, populationLeft, population2Route, TimeToBeat = 0.0f, newCost, costLeft = 0.0, globalMinPop2Route = 0.0,
		globalDeltaCost = 0.0, addedCostAsPenalty = 0.0, MaxEvacueeCostSoFar = 0.0;
	std::vector<NAVertexPtr>::iterator vit;
	SafeZoneTableItr iterator;
	long adjacentEdgeCount, i;
	INetworkEdgePtr ipCurrentEdge, lastExteriorEdge, ipTurnCheckEdge;
	INetworkJunctionPtr ipCurrentJunction;
	INetworkElementPtr ipJunctionElement, ipTurnCheckElement, ipEdgeElement;
	bool restricted = false, separationRequired;
	// esriNetworkTurnParticipationType turnType;
	EvacueeList * sortedEvacuees = new DEBUG_NEW_PLACEMENT EvacueeList();
	EvacueeListItr eit;
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
	
	if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipTurnCheckElement))) goto END_OF_FUNC;
	if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipEdgeElement))) goto END_OF_FUNC;
	if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipJunctionElement))) goto END_OF_FUNC;
	ipCurrentJunction = ipJunctionElement;
	ipTurnCheckEdge = ipTurnCheckElement;
	ipCurrentEdge = ipEdgeElement;
	
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

	if (FAILED(hr = DeterminMinimumPop2Route(Evacuees, ipNetworkDataset, globalMinPop2Route, separationRequired))) goto END_OF_FUNC;	

	do
	{
		// indexing all the population by their surrounding vertices this will be used to sort them by network distance to safe zone.
		// only the last 'k'th evacuees will be bucketed to run each round.	 Also time the carma loop and add up.

		dummy = GetProcessTimes(proc, &createTime, &exitTime, &sysTimeS, &cpuTimeS);
		if (FAILED(hr = CARMALoop(ipNetworkQuery, pMessages, pTrackCancel, Evacuees, sortedEvacuees, vcache, ecache, safeZoneList, ipForwardStar, ipBackwardStar, CARMAClosedSize,
			                      carmaClosedList, leafs, CARMAExtractCounts, globalMinPop2Route, separationRequired))) goto END_OF_FUNC;
		dummy = GetProcessTimes(proc, &createTime, &exitTime, &sysTimeE, &cpuTimeE);		
		carmaSec += (*((__int64 *) &cpuTimeE)) - (*((__int64 *) &cpuTimeS)) + (*((__int64 *) &sysTimeE)) - (*((__int64 *) &sysTimeS));

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
			MaxEvacueeCostSoFar = max (MaxEvacueeCostSoFar, currentEvacuee->PredictedCost);
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
				else if (this->solverMethod == CASPERSolver && separationRequired)
				{
					if (populationLeft - globalMinPop2Route < globalMinPop2Route) population2Route = populationLeft;
					else population2Route = globalMinPop2Route;
				}
				else population2Route = populationLeft;

				// populate the heap with vertices associated with the current evacuee
				readyEdges->clear();
				for(std::vector<NAVertexPtr>::iterator h = currentEvacuee->vertices->begin(); h != currentEvacuee->vertices->end(); h++)
					if (FAILED(hr = PrepareVerticesForHeap(*h, vcache, ecache, closedList, readyEdges, population2Route, ipBackwardStar, ipBackwardAdj, ipNetworkQuery, this->solverMethod, this->selfishRatio, MaxEvacueeCostSoFar))) goto END_OF_FUNC;
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
					_ASSERT(!closedList->Exist(myEdge));         // closedList violation happened
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
						if (FAILED(hr = iterator->second->IsRestricted(ipForwardStar, ipForwardAdj, ipTurnCheckEdge, myEdge, restricted))) goto END_OF_FUNC;
						if (!restricted)
						{
							costLeft = iterator->second->SafeZoneCost(population2Route, this->solverMethod, this->costPerDensity, &globalDeltaCost);
							if (TimeToBeat > costLeft + myVertex->GVal + myVertex->GlobalPenaltyCost + globalDeltaCost)
							{
								BetterSafeZone = iterator->second;
								TimeToBeat = costLeft + myVertex->GVal + myVertex->GlobalPenaltyCost + globalDeltaCost;
								finalVertex = myVertex;
							}
						}
					}

					// Query adjacencies from the current junction.
					/*
					if (FAILED(hr = myEdge->NetEdge->get_TurnParticipationType(&turnType))) goto END_OF_FUNC;
					if (turnType == 1) lastExteriorEdge = myEdge->LastExteriorEdge;
					else lastExteriorEdge = 0;
					*/
					if (FAILED(hr = ipForwardStar->QueryAdjacencies(myVertex->Junction, myEdge->NetEdge, 0 /*lastExteriorEdge*/, ipForwardAdj))) goto END_OF_FUNC;
					// if (turnType == 2) lastExteriorEdge = myEdge->NetEdge;

					// Get the adjacent edge count
					// Loop through all adjacent edges and update their cost value
					if (FAILED(hr = ipForwardAdj->get_Count(&adjacentEdgeCount))) goto END_OF_FUNC;

					for (i = 0; i < adjacentEdgeCount; i++)
					{
						if (FAILED(hr = ipForwardAdj->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) goto END_OF_FUNC;			

						/* It turns out, I don't need to check this here. the QueryAdjacencies() function have already did it.
						// check restriction for the recently discovered edge
						if (FAILED(hr = ipForwardStar->get_IsRestricted(ipCurrentEdge, &isRestricted))) goto END_OF_FUNC;						
						if (isRestricted) continue;
						*/
						// if edge has already been discovered then no need to heap it
						currentEdge = ecache->New(ipCurrentEdge, ipNetworkQuery);
						if (closedList->Exist(currentEdge)) continue;

						// multi-part turn restriction flags
						/*
						if (FAILED(hr = ipCurrentEdge->get_TurnParticipationType(&turnType))) goto END_OF_FUNC;
						if (turnType == 1) currentEdge->LastExteriorEdge = lastExteriorEdge;
						else currentEdge->LastExteriorEdge = 0;
						*/
						newCost = myVertex->GVal + currentEdge->GetCost(population2Route, this->solverMethod, &globalDeltaCost);

						if (heap->IsVisited(currentEdge)) // edge has been visited before. update edge and decrease key.
						{
							neighbor = currentEdge->ToVertex;
							addedCostAsPenalty = currentEdge->MaxAddedCostOnReservedPathsWithNewFlow(globalDeltaCost, MaxEvacueeCostSoFar, newCost + neighbor->GetMinHOrZero(), this->selfishRatio);
							if (neighbor->GVal + neighbor->GlobalPenaltyCost > newCost + addedCostAsPenalty + myVertex->GlobalPenaltyCost)
							{
								neighbor->SetBehindEdge(currentEdge);
								neighbor->GVal = newCost;
								neighbor->GlobalPenaltyCost = myVertex->GlobalPenaltyCost + addedCostAsPenalty;
								neighbor->Previous = myVertex;
								if (FAILED(hr = heap->DecreaseKey(currentEdge))) goto END_OF_FUNC;				
							}
						}
						else // unvisited edge. create new and insert in heap
						{
							if (FAILED(hr = ipCurrentEdge->QueryJunctions(0, ipCurrentJunction))) goto END_OF_FUNC;
							neighbor = vcache->New(ipCurrentJunction, ipNetworkQuery);
							neighbor->SetBehindEdge(currentEdge);
							addedCostAsPenalty = currentEdge->MaxAddedCostOnReservedPathsWithNewFlow(globalDeltaCost, MaxEvacueeCostSoFar, newCost + neighbor->GetMinHOrZero(), this->selfishRatio);							
							neighbor->GlobalPenaltyCost = myVertex->GlobalPenaltyCost + addedCostAsPenalty;
							neighbor->GVal = newCost;
							neighbor->Previous = myVertex;

							// Termination Condition: If the new vertex does have a chance to beat the already discovered safe node then add it to the heap.
							if (GetHeapKeyHur(currentEdge) <= TimeToBeat) heap->Insert(currentEdge);
						}
					}
				}

				// collect info for carma
				sumVisitedEdge += closedList->Size();

				// generate path for this evacuee if any was found
				if (FAILED(hr = GeneratePath(BetterSafeZone, finalVertex, populationLeft, pathGenerationCount, currentEvacuee, population2Route, separationRequired))) goto END_OF_FUNC;
				MaxEvacueeCostSoFar = max(MaxEvacueeCostSoFar, currentEvacuee->PredictedCost);

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
	CARMAExtractCounts.pop_back();

END_OF_FUNC:

	_ASSERT(hr >= 0);
	carmaSec = carmaSec / 10000000.0;
	delete readyEdges;
	delete carmaClosedList;
	delete closedList;
	delete heap;
	delete sortedEvacuees;	
	delete leafs;
	return hr;
}

HRESULT EvcSolver::CARMALoop(INetworkQueryPtr ipNetworkQuery, IGPMessages* pMessages, ITrackCancel* pTrackCancel, EvacueeList * Evacuees, EvacueeList * SortedEvacuees, NAVertexCache * vcache,
							 NAEdgeCache * ecache, SafeZoneTable * safeZoneList, INetworkForwardStarExPtr ipForwardStar, INetworkForwardStarExPtr ipBackwardStar, size_t & closedSize,
							 NAEdgeMapTwoGen * closedList, NAEdgeContainer * leafs, std::vector<unsigned int> & CARMAExtractCounts, double globalMinPop2Route, bool separationRequired)
{
	HRESULT hr = S_OK;

	// performing pre-process: Here we will mark each vertex/junction with a heuristic value indicating
	// true distance to closest safe zone using backward traversal and Dijkstra
	long adjacentEdgeCount, index;
	FibonacciHeap * heap = new DEBUG_NEW_PLACEMENT FibonacciHeap(&GetHeapKeyNonHur);	// creating the heap for the dijkstra search
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
	INetworkElementPtr ipEdgeElement, ipJunctionElement, ipTempEdgeElement;
	double fromPosition, toPosition, minPop2Route = 1.0, newCost, lastCost = 0.0, EdgeCostToBeat, SearchRadius = 0.0;
	VARIANT_BOOL keepGoing;
	INetworkEdgePtr ipCurrentEdge, ipTempEdge;
	INetworkJunctionPtr ipCurrentJunction;
	EvacueeList * redundentSortedEvacuees = new DEBUG_NEW_PLACEMENT EvacueeList();	
	redundentSortedEvacuees->reserve(Evacuees->size());	
	std::vector<NAEdgePtr> * readyEdges = new DEBUG_NEW_PLACEMENT std::vector<NAEdgePtr>();
	readyEdges->reserve(safeZoneList->size());
	unsigned int CARMAExtractCount = 0;

	// keeping reachable evacuees in a new hashtable for better access
	// also keep unreachable ones in the redundant list
	NAEvacueeVertexTable * EvacueePairs = new DEBUG_NEW_PLACEMENT NAEvacueeVertexTable();
	EvacueePairs->InsertReachable_KeepOtherWithVertex(Evacuees, redundentSortedEvacuees);
	
	// Create a backward Star Adjacencies object (we need this object to hold traversal queries carried out on the Backward Star)
	INetworkForwardStarAdjacenciesPtr ipBackwardAdj;
	INetworkForwardStarAdjacenciesPtr ipForwardAdj;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipBackwardAdj))) goto END_OF_FUNC;
	if (FAILED(hr = ipNetworkQuery->CreateForwardStarAdjacencies(&ipForwardAdj))) goto END_OF_FUNC;

	if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipEdgeElement))) goto END_OF_FUNC;
	if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipTempEdgeElement))) goto END_OF_FUNC;
    if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipJunctionElement))) goto END_OF_FUNC;
	ipCurrentJunction = ipJunctionElement;
	ipCurrentEdge = ipEdgeElement;
	ipTempEdge = ipTempEdgeElement;

	// if this list is not empty, it means we are going to have another CARMA loop
	if (!EvacueePairs->empty()) 
	{
		countCARMALoops++;
		#ifdef DEBUG
		std::wostringstream os_;
		os_ << "CARMALoop #" << countCARMALoops << std::endl;
		OutputDebugStringW( os_.str().c_str() );
		#endif

		if (ThreeGenCARMA == VARIANT_TRUE) closedList->MarkAllAsOldGen();
		else closedList->Clear(NAEdgeMap_ALLGENS);

		// search for min population on graph evacuees left to be routed. The next if has to be in-tune with what population will be routed next.
		// the h values should always be an underestimation
		minPop2Route = 1.0; // separable CCRPSolver and any case of SPSolver 
		if ((this->solverMethod == CASPERSolver) || (this->solverMethod == CCRPSolver && !separationRequired))
		{
			minPop2Route = FLT_MAX;
			for(EvacueeListItr eit = Evacuees->begin(); eit != Evacuees->end(); eit++)
			{
				if ((*eit)->vertices->empty() || (*eit)->Population <= 0.0) continue;
				minPop2Route = min(minPop2Route, (*eit)->Population);
			}
			if (separationRequired) minPop2Route = min(globalMinPop2Route, minPop2Route);
		}
		minPop2Route = max(minPop2Route, 1.0);

		// This is where the new dynamic CARMA starts. At this point you have to clear the dirty section of the carma tree.
		// also keep the previous leafs only if they are still in closedList. They help re-discover EvacueePairs
		MarkDirtyEdgesAsUnVisited(closedList->oldGen, leafs, minPop2Route, this->solverMethod);

		// prepare and insert safe zone vertices into the heap
		for (SafeZoneTableItr sz = safeZoneList->begin(); sz != safeZoneList->end(); sz++)
			if (FAILED(hr = PrepareVerticesForHeap(sz->second->Vertex, vcache, ecache, closedList->oldGen, readyEdges, minPop2Route, ipForwardStar, ipForwardAdj, ipNetworkQuery, solverMethod, 0.0, 0.0))) goto END_OF_FUNC;	
		for(std::vector<NAEdgePtr>::iterator h = readyEdges->begin(); h != readyEdges->end(); h++) heap->Insert(*h);

		// Now insert leaf edges in heap like the destination edges	
		#ifdef DEBUG
		if (FAILED(hr = PrepareLeafEdgesForHeap(ipNetworkQuery, vcache, ecache, heap, leafs, minPop2Route, this->solverMethod))) goto END_OF_FUNC;
		#else
		if (FAILED(hr = PrepareLeafEdgesForHeap(ipNetworkQuery, vcache, ecache, heap, leafs))) goto END_OF_FUNC;
		#endif

		// we're done with all these leafs. let's clean up and collect new ones for the next round.
		leafs->Clear();

		// Continue traversing the network while the heap has remaining junctions in it
		// this is the actual Dijkstra code with backward network traversal. it will only update h value.
		while (!heap->IsEmpty())
		{
			// Remove the next junction EID from the top of the queue
			myEdge = heap->DeleteMin();
			myVertex = myEdge->ToVertex;
			_ASSERT(!closedList->Exist(myEdge));         // closedList violation happened
			if (FAILED(hr = closedList->Insert(myEdge))) 
			{
				// closedList violation happened
				pMessages->AddError(-myEdge->EID, CComBSTR(L"ClosedList Violation Error."));
				hr = -myEdge->EID;
				goto END_OF_FUNC;
			}

			// this value is being recorded and will be used as the default heuristic value for any future vertex
			lastCost = myVertex->GVal;
			CARMAExtractCount++;

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
			myVertex->UpdateHeuristic(myEdge->EID, myVertex->GVal, countCARMALoops);
			
			pairs = EvacueePairs->Find(myVertex->EID);
			if (pairs)
			{
				for (eitr = pairs->begin(); eitr != pairs->end(); eitr++)
				{
					(*eitr)->PredictedCost = min((*eitr)->PredictedCost, myVertex->GVal);
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

			// this is my new termination condition. let's hope it works.
			// basically i stop inserting new edges if they are above search radius.
			if (EvacueePairs->empty() && SearchRadius <= 0.0) SearchRadius = heap->GetMaxValue();

			// Query adjacencies from the current junction
			if (FAILED(hr = ipBackwardStar->QueryAdjacencies(myVertex->Junction, myEdge->NetEdge, 0, ipBackwardAdj))) goto END_OF_FUNC; 

			// Get the adjacent edge count
			if (FAILED(hr = ipBackwardAdj->get_Count(&adjacentEdgeCount))) goto END_OF_FUNC;

			// Loop through all adjacent edges and update their cost value
			for (index = 0; index < adjacentEdgeCount; index++)
			{
				if (FAILED(hr = ipBackwardAdj->QueryEdge(index, ipCurrentEdge, &fromPosition, &toPosition))) goto END_OF_FUNC;
				if (FAILED(hr = ipCurrentEdge->QueryJunctions(ipCurrentJunction, 0))) goto END_OF_FUNC;

				/* It turns out, I don't need to check this here. the QueryAdjacencies() function have already did it.
				// check restriction for the recently discovered edge
				if (FAILED(hr = ipBackwardStar->get_IsRestricted(ipCurrentEdge, &isRestricted))) goto END_OF_FUNC;
				if (isRestricted) continue;
				*/
				// if node has already been discovered then no need to heap it
				currentEdge = ecache->New(ipCurrentEdge, ipNetworkQuery);
				if (closedList->Exist(currentEdge, NAEdgeMap_OLDGEN)) continue;
				
				newCost = myVertex->GVal + currentEdge->GetCost(minPop2Route, this->solverMethod);
				if (closedList->Exist(currentEdge, NAEdgeMap_NEWGEN))
				{					
					if (FAILED(hr = PrepareUnvisitedVertexForHeap(ipCurrentJunction, currentEdge, myEdge, newCost - myVertex->GVal, myVertex, ecache, closedList, vcache, ipForwardStar, ipForwardAdj, ipNetworkQuery, ipTempEdge, false))) goto END_OF_FUNC;
					EdgeCostToBeat = currentEdge->ToVertex->GetH(currentEdge->EID);
					if(EdgeCostToBeat <= currentEdge->ToVertex->GVal) continue;
					closedList->Erase(currentEdge, NAEdgeMap_NEWGEN);
					heap->Insert(currentEdge);
				}

				if (heap->IsVisited(currentEdge)) // vertex has been visited before. update vertex and decrease key.
				{
					neighbor = currentEdge->ToVertex;
					if (neighbor->GVal > newCost)
					{
						neighbor->SetBehindEdge(currentEdge);
						neighbor->GVal = newCost;
						neighbor->Previous = myVertex;
						if (FAILED(hr = heap->DecreaseKey(currentEdge))) goto END_OF_FUNC;
					}
				}
				else // unvisited vertex. create new and insert into heap
				{
					// termination condition and evacuee discovery
					if (FAILED(hr = PrepareUnvisitedVertexForHeap(ipCurrentJunction, currentEdge, myEdge, newCost - myVertex->GVal, myVertex, ecache, closedList, vcache, ipForwardStar, ipForwardAdj, ipNetworkQuery, ipTempEdge, true))) goto END_OF_FUNC;
					if (!EvacueePairs->empty() || currentEdge->ToVertex->GVal < SearchRadius) heap->Insert(currentEdge);
				}
			}
		}
	}

	// set new default heuristic value
	vcache->UpdateHeuristicForOutsideVertices(lastCost, this->countCARMALoops);
	CARMAExtractCounts.push_back(CARMAExtractCount);

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
		for(std::vector<NAEdgePtr>::iterator i = e->TreeNext.begin(); i != e->TreeNext.end(); i++) 
		{
			(*i)->TreePrevious = NULL;
			subtree.push(*i);
		}
		e->TreeNext.clear();
	}
}

HRESULT InsertLeafEdgeToHeap(INetworkQueryPtr ipNetworkQuery, NAVertexCache * vcache, FibonacciHeap * heap, NAEdge * leaf
							#ifdef DEBUG
							, double minPop2Route, EvcSolverMethod solverMethod
							#endif
							)
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
		fPtr->GVal = tPtr->GetH(leaf->TreePrevious->EID) + leaf->GetCleanCost();    // tPtr->GetH(leaf->TreePrevious->EID) + leaf->GetCost(minPop2Route, solverMethod);
		fPtr->Previous = NULL;
		_ASSERT(fPtr->GVal < FLT_MAX);
		heap->Insert(leaf);
	}
	return hr;
}

HRESULT PrepareLeafEdgesForHeap(INetworkQueryPtr ipNetworkQuery, NAVertexCache * vcache, NAEdgeCache * ecache, FibonacciHeap * heap, NAEdgeContainer * leafs
								#ifdef DEBUG
								, double minPop2Route, EvcSolverMethod solverMethod
								#endif
								)
{
	HRESULT hr = S_OK;
	NAEdgePtr leaf;

	for (NAEdgeIterator i = leafs->begin(); i != leafs->end(); i++)
	{
		if (i->second & 1)
		{
			leaf = ecache->Get(i->first, esriNEDAlongDigitized);
			#ifdef DEBUG
			if (FAILED(hr = InsertLeafEdgeToHeap(ipNetworkQuery, vcache, heap, leaf, minPop2Route, solverMethod))) return hr;
			#else
			if (FAILED(hr = InsertLeafEdgeToHeap(ipNetworkQuery, vcache, heap, leaf))) return hr;
			#endif
		}
		if (i->second & 2)
		{
			leaf = ecache->Get(i->first, esriNEDAgainstDigitized);
			#ifdef DEBUG
			if (FAILED(hr = InsertLeafEdgeToHeap(ipNetworkQuery, vcache, heap, leaf, minPop2Route, solverMethod))) return hr;
			#else
			if (FAILED(hr = InsertLeafEdgeToHeap(ipNetworkQuery, vcache, heap, leaf))) return hr;
			#endif
		}
	}
	return hr;
}

HRESULT EvcSolver::PrepareUnvisitedVertexForHeap(INetworkJunctionPtr junction, NAEdgePtr edge, NAEdgePtr prevEdge, double edgeCost, NAVertexPtr myVertex, NAEdgeCache * ecache, NAEdgeMapTwoGen * closedList,
												 NAVertexCache * vcache, INetworkForwardStarExPtr ipForwardStar, INetworkForwardStarAdjacenciesPtr ipForwardAdj, INetworkQueryPtr ipNetworkQuery,
												 INetworkEdgePtr tempNetEdge, bool checkOldClosedlist) const
{
	HRESULT hr = S_OK;
	long adjacentEdgeCount;
	NAVertexPtr betterMyVertex;
	NAEdgePtr tempEdge, betterEdge = NULL;
	double betterH, tempH, from, to;
	NAVertexPtr tempVertex, neighbor;

	// Dynamic CARMA: at this step we have to check if there is any better previous edge for this new one in closed-list
	tempVertex = vcache->Get(myVertex->EID); // this is the vertex at the center of two edges... we have to check its heuristics to see if the new twempEdge is any better.
	betterH = myVertex->GVal;

	#ifndef DEBUG
	if (checkOldClosedlist && closedList->Size(NAEdgeMap_OLDGEN) > 0)
	#endif
	{
		if (FAILED(hr = ipForwardStar->QueryAdjacencies(myVertex->Junction, edge->NetEdge, 0, ipForwardAdj))) return hr;
		if (FAILED(hr = ipForwardAdj->get_Count(&adjacentEdgeCount))) return hr;

		// Loop through all adjacent edges and update their cost value
		for (long i = 0; i < adjacentEdgeCount; i++)
		{
			if (FAILED(hr = ipForwardAdj->QueryEdge(i, tempNetEdge, &from, &to))) return hr;
		
			// check restriction for the recently discovered edge.
			// if (FAILED(hr = ipForwardStar->get_IsRestricted(tempNetEdge, &isRestricted))) return hr;
			// if (isRestricted) continue;
			tempEdge = ecache->New(tempNetEdge, ipNetworkQuery);
			if (!closedList->Exist(tempEdge, NAEdgeMap_OLDGEN)) continue; // it has to be present in closed list from previous CARMA loop
			if (tempEdge->Direction == prevEdge->Direction && tempEdge->EID == prevEdge->EID) continue; // it cannot be the same parent edge

			// at this point if the new tempEdge satisfied all restrictions and conditions it means it might be a good pick
			// as a previous edge depending on the cost which we shall obtain from vertices heuristic table
			tempH = tempVertex->GetH(tempEdge->EID);
			if (tempH < betterH) { betterEdge = tempEdge; betterH = tempH; }
		}
	}
	if (betterEdge)
	{
		#ifdef DEBUG
		if(!checkOldClosedlist)
		{
			double CostToBeat = edge->ToVertex->GetH(edge->EID);
			_ASSERT(CostToBeat - betterH - edgeCost < FLT_EPSILON);
		}
		#endif

		betterMyVertex = vcache->New(myVertex->Junction);
		betterMyVertex->SetBehindEdge(betterEdge);
		betterMyVertex->Previous = NULL;
		betterMyVertex->GVal = betterH;
	}
	else betterMyVertex = myVertex;

	neighbor = vcache->New(junction, ipNetworkQuery);
	neighbor->SetBehindEdge(edge);
	neighbor->GVal = edgeCost + betterMyVertex->GVal;
	neighbor->Previous = betterMyVertex;
	
	return hr;
}

HRESULT PrepareVerticesForHeap(NAVertexPtr point, NAVertexCache * vcache, NAEdgeCache * ecache, NAEdgeMap * closedList, std::vector<NAEdgePtr> * readyEdges, double pop, INetworkForwardStarExPtr ipStar,
							   INetworkForwardStarAdjacenciesPtr ipStarAdj, INetworkQueryPtr ipNetworkQuery, EvcSolverMethod solverMethod, double selfishRatio, double MaxEvacueeCostSoFar)
{
	HRESULT hr = S_OK;
	NAVertexPtr temp;
	NAEdgePtr edge;
	INetworkEdgePtr ipCurrentEdge;
	INetworkJunctionPtr ipCurrentJunction;
	INetworkElementPtr ipElement;
	long adjacentEdgeCount;
	double fromPosition, toPosition, globalDeltaPenalty = 0.0;

	if(readyEdges)
	{
		temp = vcache->New(point->Junction);
		temp->SetBehindEdge(point->GetBehindEdge());
		temp->GVal = point->GVal;
		temp->GlobalPenaltyCost = point->GlobalPenaltyCost;
		temp->Junction = point->Junction;
		temp->Previous = 0;
		edge = temp->GetBehindEdge();

		// check to see if the edge you're about to insert is not in the closedList
		if (edge) 
		{
			if (!closedList->Exist(edge))
			{
				temp->GVal = point->GVal * edge->GetCost(pop, solverMethod, &globalDeltaPenalty) / edge->OriginalCost;
				temp->GlobalPenaltyCost = edge->MaxAddedCostOnReservedPathsWithNewFlow(globalDeltaPenalty, MaxEvacueeCostSoFar, temp->GVal + temp->GetMinHOrZero(), selfishRatio);
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
				temp->GVal = point->GVal * edge->GetCost(pop, solverMethod, &globalDeltaPenalty) / edge->OriginalCost;
				temp->GlobalPenaltyCost = edge->MaxAddedCostOnReservedPathsWithNewFlow(globalDeltaPenalty, MaxEvacueeCostSoFar, temp->GVal + temp->GetMinHOrZero(), selfishRatio);
				readyEdges->push_back(edge);
			}
		}
	}
	return hr;
}

HRESULT EvcSolver::GeneratePath(SafeZonePtr BetterSafeZone, NAVertexPtr finalVertex, double & populationLeft, int & pathGenerationCount, EvacueePtr currentEvacuee, double population2Route, bool separationRequired) const
{
	double leftCap, fromPosition, toPosition, edgePortion;
	long sourceOID, sourceID;
	HRESULT hr = S_OK;
	EvcPath * path = NULL;

	// generate evacuation route if a destination has been found
	if (BetterSafeZone)
	{
		// First find out about remaining capacity of this path
		NAVertexPtr temp = finalVertex;
		
		if (this->solverMethod == CCRPSolver)
		{
			if (separationRequired)
			{
				population2Route = 0.0;
				while (temp->Previous)
				{
					leftCap = temp->GetBehindEdge()->LeftCapacity();
					if (this->solverMethod == CCRPSolver || leftCap > 0.0) population2Route = min(population2Route, leftCap);
					temp = temp->Previous;
				}
				if (population2Route <= 0.0) population2Route = populationLeft;	
				population2Route = min(population2Route, populationLeft);
			}
			else population2Route = populationLeft;
		}
		populationLeft -= population2Route;

		// create a new path for this portion of the population
		path = new DEBUG_NEW_PLACEMENT EvcPath(population2Route, ++pathGenerationCount, currentEvacuee);

		// special case for the last edge.
		// We have to sub-curve it based on the safe point location along the edge
		if (BetterSafeZone->getBehindEdge())
		{
			if (FAILED(hr = BetterSafeZone->getBehindEdge()->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) goto END_OF_FUNC;
			edgePortion = BetterSafeZone->getPositionAlong();
			if (fromPosition < toPosition) toPosition = fromPosition + edgePortion;
			else toPosition = fromPosition - edgePortion;

			if (edgePortion > 0.0)
			{
				path->AddSegment(population2Route, this->solverMethod, new DEBUG_NEW_PLACEMENT PathSegment(fromPosition, toPosition, sourceOID, sourceID, BetterSafeZone->getBehindEdge(), edgePortion));
				// BetterSafeZone->getBehindEdge()->AddReservation(path, fromPosition, toPosition, population2Route, this->solverMethod);
			}
		}
		edgePortion = 1.0;

		while (finalVertex->Previous)
		{
			if (finalVertex->GetBehindEdge())
			{
				if (FAILED(hr = finalVertex->GetBehindEdge()->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) goto END_OF_FUNC;	
				path->AddSegment(population2Route, this->solverMethod, new DEBUG_NEW_PLACEMENT PathSegment(fromPosition, toPosition, sourceOID, sourceID, finalVertex->GetBehindEdge(), edgePortion));
				// finalVertex->GetBehindEdge()->AddReservation(path, fromPosition, toPosition, population2Route, this->solverMethod);
			}
			finalVertex = finalVertex->Previous;
		}

		// special case for the first edge.
		// We have to curve it based on the evacuee point location along the edge
		if (finalVertex->GetBehindEdge())
		{
			if (FAILED(hr = finalVertex->GetBehindEdge()->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) goto END_OF_FUNC;				
			edgePortion = finalVertex->GVal / finalVertex->GetBehindEdge()->OriginalCost;
			if (fromPosition < toPosition) fromPosition = toPosition - edgePortion;
			else fromPosition = toPosition + edgePortion;

			// path can be empty if the source and destination are the same vertex
			if (!path->Empty() && path->Front()->SourceOID == sourceOID && path->Front()->SourceID == sourceID)
			{				
				PathSegmentPtr lastAdded = path->Front();
				lastAdded->fromPosition = fromPosition;
				lastAdded->EdgePortion = abs(lastAdded->toPosition - lastAdded->fromPosition); // 2.0 - (lastAdded->EdgePortion + edgePortion);
				_ASSERT(lastAdded->EdgePortion > 0.0);
			}
			else if (edgePortion > 0.0)
			{							
				path->AddSegment(population2Route, this->solverMethod, new DEBUG_NEW_PLACEMENT PathSegment(fromPosition, toPosition, sourceOID, sourceID, finalVertex->GetBehindEdge(), edgePortion));
				// finalVertex->GetBehindEdge()->AddReservation(path, fromPosition, toPosition, population2Route, this->solverMethod);
			}
		}
		if (path->Empty()) 
		{
			delete path;
			path = NULL;
		}
		else
		{
			currentEvacuee->paths->push_front(path);
			currentEvacuee->PredictedCost = path->GetEvacuationCost();
			BetterSafeZone->Reserve(path->GetRoutedPop());
		}
	}
	else
	{		
		populationLeft = 0.0; // since no path could be found for this evacuee, we assume the rest of the population at this location have no path as well
	}

END_OF_FUNC:

	if (path && FAILED(hr)) delete path;
	return hr;
}

// This is where i figure out what is the smallest population that I should route (or try to route)
// at each CASPER loop. Obviously this globalMinPop2Route has to be less than the population of any evacuee point.
// Also CASPER and CARMA should be in sync at this number otherwise all the h values are useless.
HRESULT EvcSolver::DeterminMinimumPop2Route(EvacueeList * Evacuees, INetworkDatasetPtr ipNetworkDataset, double & globalMinPop2Route, bool & separationRequired) const
{
	double minPop = FLT_MAX, maxPop = 1.0, CommonCostOfEdgeInUnits = 1.0, avgPop = 0.0;
	HRESULT hr = S_OK;
	INetworkAttributePtr costAttrib;
	esriNetworkAttributeUnits unit;
	size_t count = 0;
	globalMinPop2Route = 0.0;
	separationRequired = this->separable == VARIANT_TRUE;

	if (this->separable && this->solverMethod == CASPERSolver)
	{
		for(EvacueeListItr eit = Evacuees->begin(); eit != Evacuees->end(); eit++)
			if ((*eit)->Population > 0.0)
			{
				count++;
				avgPop += (*eit)->Population;
				if ((*eit)->Population > maxPop) maxPop = (*eit)->Population;
				if ((*eit)->Population < minPop) minPop = (*eit)->Population;
			}
		
		avgPop = avgPop / count;
		
		// read cost attribute unit
		if (SUCCEEDED(hr = ipNetworkDataset->get_AttributeByID(costAttributeID, &costAttrib)))
		{
			if (FAILED(hr = costAttrib->get_Units(&unit))) return hr;
			CommonCostOfEdgeInUnits = GetUnitPerDay(unit, 25.0) / (60.0 * 24.0);
		}
		if ((initDelayCostPerPop > 0.0 && CommonCostOfEdgeInUnits / initDelayCostPerPop <= SaturationPerCap) ||
			maxPop <= SaturationPerCap)
		{
			// the maximum possible flow on common edge is not enough to cause congestion alone so separation is not required.
			separationRequired = false;
			globalMinPop2Route = 0.0;
		}
		else
		{
			separationRequired = true;
			globalMinPop2Route = SaturationPerCap / 2.0;

			// We don't want the minimum routable population to be less than one third of the minimum population of any evacuee point. That just makes CASPER too slow.
			if (globalMinPop2Route * 3.0 < minPop) globalMinPop2Route = minPop / 3.0;
			if (globalMinPop2Route < 1.0) globalMinPop2Route = 1.0;
		}
	}
	return hr;
}

void EvcSolver::UpdatePeakMemoryUsage()
{
	PROCESS_MEMORY_COUNTERS pmc;
	if(!hProcessPeakMemoryUsage) hProcessPeakMemoryUsage = GetCurrentProcess();
	if (GetProcessMemoryInfo(hProcessPeakMemoryUsage, &pmc, sizeof(pmc))) peakMemoryUsage = max(peakMemoryUsage, pmc.PagefileUsage);
}
