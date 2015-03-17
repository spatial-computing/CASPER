// ===============================================================================================
// Evacuation Solver: CASPER implementation
// Description: Core of the CASPER algorithm implementation
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

HRESULT EvcSolver::SolveMethod(INetworkQueryPtr ipNetworkQuery, IGPMessages* pMessages, ITrackCancel* pTrackCancel, IStepProgressorPtr ipStepProgressor, std::shared_ptr<EvacueeList> AllEvacuees,
	std::shared_ptr<NAVertexCache> vcache, std::shared_ptr<NAEdgeCache> ecache, std::shared_ptr<SafeZoneTable> safeZoneList, double & carmaSec, std::vector<unsigned int> & CARMAExtractCounts,
	INetworkDatasetPtr ipNetworkDataset, unsigned int & EvacueesWithRestrictedSafezone, std::vector<double> & GlobalEvcCostAtIteration,
	std::vector<double> & EffectiveIterationRatio, std::shared_ptr<DynamicDisaster> dynamicDisasters)
{
	// creating the heap for the Dijkstra search
	MyFibonacciHeap<NAEdgePtr, NAEdgePtrHasher, NAEdgePtrEqual> heap(NAEdge::GetHeapKeyHur);
	NAEdgeMap closedList;
	auto carmaClosedList = std::shared_ptr<NAEdgeMapTwoGen>(new DEBUG_NEW_PLACEMENT NAEdgeMapTwoGen());
	NAVertexPtr neighbor = nullptr, finalVertex = nullptr, myVertex = nullptr;
	SafeZonePtr BetterSafeZone = nullptr;
	NAEdgePtr myEdge = nullptr;
	HRESULT hr = S_OK;
	VARIANT_BOOL keepGoing;
	double populationLeft, population2Route, TimeToBeat = 0.0f, newCost, globalMinPop2Route = 0.0, minPop2Route = 1.0, globalDeltaCost = 0.0, MaxPathCostSoFar = 0.0, addedCostAsPenalty = 0.0, EvcStartTime = 0.0;
	std::vector<NAVertexPtr>::const_iterator vit;
	INetworkJunctionPtr ipCurrentJunction = nullptr;
	INetworkElementPtr ipJunctionElement = nullptr;
	bool separationRequired, foundRestrictedSafezone;
	auto sortedEvacuees = std::shared_ptr<std::vector<EvacueePtr>>(new DEBUG_NEW_PLACEMENT std::vector<EvacueePtr>());
	unsigned int countEvacueesInOneBucket = 0, countCASPERLoops = 0, sumVisitedDirtyEdge = 0;
	int pathGenerationCount = -1, EvacueeProcessOrder = -1;
	size_t CARMAClosedSize = 0, sumVisitedEdge = 0, NumberOfEvacueesInIteration = 0, LocalIteration = 0;
	long progressBaseValue = 0l;
	auto leafs = std::shared_ptr<NAEdgeContainer>(new DEBUG_NEW_PLACEMENT NAEdgeContainer(200));
	std::vector<NAEdgePtr> readyEdges;
	HANDLE proc = GetCurrentProcess();
	BOOL dummy;
	FILETIME cpuTimeS, cpuTimeE, sysTimeS, sysTimeE, createTime, exitTime;
	ArrayList<NAEdgePtr> * adj = nullptr;
	ATL::CString statusMsg, AlgName;
	CARMASort RevisedCarmaSortCriteria = this->CarmaSortCriteria;
	auto detachedPaths = std::shared_ptr<std::vector<EvcPathPtr>>(new DEBUG_NEW_PLACEMENT std::vector<EvcPathPtr>());
	CARMAExtractCounts.clear();

	switch (solverMethod)
	{
	case EvcSolverMethod::CASPERSolver:
		AlgName = _T("CASPER");
		break;
	case EvcSolverMethod::SPSolver:
		AlgName = _T("SP");
		break;
	case EvcSolverMethod::CCRPSolver:
		AlgName = _T("CCRP");
		break;
	default:
		AlgName = _T("the");
		break;
	}

	// initialize all dynamic changes and prepare for loop
	size_t countDynamic = dynamicDisasters->ResetDynamicChanges();

	// Setup a message on our step progress bar indicating that we are traversing the network
	if (ipStepProgressor)
	{
		// Setup our progress bar based on the number of Evacuee points
		if (FAILED(hr = ipStepProgressor->put_MinRange(0))) goto END_OF_FUNC;
		if (FAILED(hr = ipStepProgressor->put_MaxRange((long)(countDynamic * AllEvacuees->size())))) goto END_OF_FUNC;
		if (FAILED(hr = ipStepProgressor->put_StepValue(1))) goto END_OF_FUNC;
	}

	sortedEvacuees->reserve(AllEvacuees->size());
	EvacueesWithRestrictedSafezone = 0;
	if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipJunctionElement))) goto END_OF_FUNC;
	ipCurrentJunction = ipJunctionElement;

	if (FAILED(hr = DeterminMinimumPop2Route(AllEvacuees, ipNetworkDataset, globalMinPop2Route, separationRequired))) goto END_OF_FUNC;

	// dynamic CASPER loop
	for (NumberOfEvacueesInIteration = dynamicDisasters->NextDynamicChange(AllEvacuees, ecache, EvcStartTime); NumberOfEvacueesInIteration > 0;
		 NumberOfEvacueesInIteration = dynamicDisasters->NextDynamicChange(AllEvacuees, ecache, EvcStartTime))
	{
		LocalIteration = 0;
		RevisedCarmaSortCriteria = this->CarmaSortCriteria;
		do // iteration loop
		{
			if (ipStepProgressor)
			{
				if (FAILED(hr = ipStepProgressor->put_Position(progressBaseValue + (long)(AllEvacuees->size() - NumberOfEvacueesInIteration)))) goto END_OF_FUNC;
				statusMsg.Format(_T("Performing %s search (time %.2f, pass %d)"), AlgName, EvcStartTime, GlobalEvcCostAtIteration.size() + 1);
			}
			do
			{
				// Indexing all the population by their surrounding vertices this will be used to sort them by network distance to safe zone. Also time the carma loops.
				dummy = GetProcessTimes(proc, &createTime, &exitTime, &sysTimeS, &cpuTimeS);
				if (FAILED(hr = CARMALoop(ipNetworkQuery, ipStepProgressor, pMessages, pTrackCancel, AllEvacuees, sortedEvacuees, vcache, ecache, safeZoneList, CARMAClosedSize,
					carmaClosedList, leafs, CARMAExtractCounts, globalMinPop2Route, minPop2Route, separationRequired, RevisedCarmaSortCriteria))) goto END_OF_FUNC;
				dummy = GetProcessTimes(proc, &createTime, &exitTime, &sysTimeE, &cpuTimeE);
				carmaSec += (*((__int64 *)&cpuTimeE)) - (*((__int64 *)&cpuTimeS)) + (*((__int64 *)&sysTimeE)) - (*((__int64 *)&sysTimeS));

				if (ipStepProgressor) { if (FAILED(hr = ipStepProgressor->put_Message(ATL::CComBSTR(statusMsg)))) goto END_OF_FUNC; }
				countEvacueesInOneBucket = 0;
				sumVisitedDirtyEdge = 0;
				sumVisitedEdge = 0;

				for (const auto currentEvacuee : *sortedEvacuees)
				{
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
					_ASSERT_EXPR(currentEvacuee->Status != EvacueeStatus::CARMALooking, L"CARMA did not make up his mind on this evacuee");
					if (currentEvacuee->Status != EvacueeStatus::Unprocessed) continue;

					// Step the progress bar before continuing to the next Evacuee point
					if (ipStepProgressor) ipStepProgressor->Step();
					currentEvacuee->ProcessOrder = ++EvacueeProcessOrder;
					MaxPathCostSoFar = max(MaxPathCostSoFar, currentEvacuee->PredictedCost);
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
						if (this->solverMethod == EvcSolverMethod::CCRPSolver) population2Route = 1.0;
						else if (this->solverMethod == EvcSolverMethod::CASPERSolver && separationRequired)
						{
							if (populationLeft - globalMinPop2Route < globalMinPop2Route) population2Route = populationLeft;
							else population2Route = globalMinPop2Route;
						}
						else population2Route = populationLeft;

						// populate the heap with vertices associated with the current evacuee
						readyEdges.clear();
						for (auto const & v : *(currentEvacuee->VerticesAndRatio))
							if (FAILED(hr = PrepareVerticesForHeap(v, vcache, ecache, &closedList, readyEdges, population2Route, solverMethod, selfishRatio, MaxPathCostSoFar, QueryDirection::Backward))) goto END_OF_FUNC;
						for (const auto & e : readyEdges) heap.Insert(e);

						TimeToBeat = CASPER_INFINITY;
						BetterSafeZone = nullptr;
						finalVertex = nullptr;
						foundRestrictedSafezone = false;

						// reduce the effect of previous CASPER loop heap extract for the purpose of dirty edge ratio. This will encourage more CARMA loops.
						sumVisitedDirtyEdge = (unsigned int)(sumVisitedDirtyEdge * 0.9);
						sumVisitedEdge = (size_t)(sumVisitedEdge * 0.9);

						// Continue traversing the network while the heap has remaining junctions in it
						// this is the actual Dijkstra code with the Fibonacci Heap
						while (!heap.empty())
						{
							// Remove the next junction EID from the top of the stack
							myEdge = heap.DeleteMin();
							myVertex = myEdge->ToVertex;
							_ASSERT_EXPR(!closedList.Exist(myEdge), L"closedList violation happened");
							if (FAILED(hr = closedList.Insert(myEdge)))
							{
								// closedList violation happened
								pMessages->AddError(-myEdge->EID, ATL::CComBSTR(L"ClosedList Violation Error."));
								hr = ATL::AtlReportError(this->GetObjectCLSID(), _T("ClosedList Violation Error."), IID_INASolver);
								goto END_OF_FUNC;
							}

							if (myEdge->GetDirtyState() != EdgeDirtyState::CleanState) sumVisitedDirtyEdge++;

							// Check for destinations. If a new destination has been found then we should
							// first flag this so later we can use to generate route. Also we should
							// update the new TimeToBeat value for proper termination.
							if (safeZoneList->CheckDiscoveredSafePoint(ecache, myVertex, myEdge, finalVertex, TimeToBeat, BetterSafeZone, costPerDensity,
								population2Route, solverMethod, globalDeltaCost, foundRestrictedSafezone)) UpdatePeakMemoryUsage();

							if (FAILED(hr = ecache->QueryAdjacencies(myVertex, myEdge, QueryDirection::Forward, &adj))) goto END_OF_FUNC;

							for (const auto & currentEdge : *adj)
							{
								// if edge has already been discovered then no need to heap it
								if (closedList.Exist(currentEdge)) continue;

								newCost = myVertex->GVal + currentEdge->GetCost(population2Route, this->solverMethod, &globalDeltaCost);
								if (newCost >= CASPER_INFINITY) continue;

								if (heap.IsVisited(currentEdge)) // edge has been visited before. update edge and decrease key.
								{
									neighbor = currentEdge->ToVertex;
									addedCostAsPenalty = currentEdge->MaxAddedCostOnReservedPathsWithNewFlow(globalDeltaCost, MaxPathCostSoFar, newCost + neighbor->GetMinHOrZero(), this->selfishRatio);
									if (neighbor->GVal + neighbor->GlobalPenaltyCost > newCost + addedCostAsPenalty + myVertex->GlobalPenaltyCost)
									{
										neighbor->SetBehindEdge(currentEdge);
										neighbor->GVal = newCost;
										neighbor->GlobalPenaltyCost = myVertex->GlobalPenaltyCost + addedCostAsPenalty;
										neighbor->Previous = myVertex;
										heap.UpdateKey(currentEdge);
									}
								}
								else // unvisited edge. create new and insert in heap
								{
									if (FAILED(hr = currentEdge->NetEdge->QueryJunctions(nullptr, ipCurrentJunction))) goto END_OF_FUNC;
									neighbor = vcache->New(ipCurrentJunction, ipNetworkQuery);
									neighbor->SetBehindEdge(currentEdge);
									addedCostAsPenalty = currentEdge->MaxAddedCostOnReservedPathsWithNewFlow(globalDeltaCost, MaxPathCostSoFar, newCost + neighbor->GetMinHOrZero(), this->selfishRatio);
									neighbor->GlobalPenaltyCost = myVertex->GlobalPenaltyCost + addedCostAsPenalty;
									neighbor->GVal = newCost;
									neighbor->Previous = myVertex;

									// Termination Condition: If the new vertex does have a chance to beat the already discovered safe node then add it to the heap.
									if (NAEdge::GetHeapKeyHur(currentEdge) <= TimeToBeat) heap.Insert(currentEdge);
								}
							}
						}

						// collect info for Carma
						sumVisitedEdge += closedList.Size();

						// Find a path despite the fact that a safe zone (restricted) was found
						// Address issue number 4: http://github.com/spatial-computing/CASPER/issues/4
						if (!BetterSafeZone && foundRestrictedSafezone) ++EvacueesWithRestrictedSafezone;

						// Generate path for this evacuee if any found
						if (GeneratePath(BetterSafeZone, finalVertex, populationLeft, pathGenerationCount, currentEvacuee, population2Route, separationRequired))
							MaxPathCostSoFar = max(MaxPathCostSoFar, currentEvacuee->Paths->front()->GetReserveEvacuationCost());
 						else currentEvacuee->Status = EvacueeStatus::Unreachable;

						#ifdef DEBUG
						std::wostringstream os_;
						os_.precision(3);
						os_ << "CARMALoop stat " << countEvacueesInOneBucket << ": " << (int)sumVisitedEdge << ',' << (int)sumVisitedDirtyEdge << ',' << sumVisitedDirtyEdge / (CARMAPerformanceRatio * sumVisitedEdge) << std::endl;
						OutputDebugStringW(os_.str().c_str());
						#endif
						#ifdef TRACE
						std::ofstream f;
						f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
						f.precision(3);
						f << "CARMALoop stat " << countEvacueesInOneBucket << ": " << (int)sumVisitedEdge << ',' << (int)sumVisitedDirtyEdge << ',' << sumVisitedDirtyEdge / (CARMAPerformanceRatio * sumVisitedEdge) << std::endl;
						f.close();
						#endif

						// cleanup search heap and closed-list
						UpdatePeakMemoryUsage();
						heap.Clear();
						closedList.Clear();
					} // end of while loop for multiple routes single evacuee

					if (currentEvacuee->Status == EvacueeStatus::Unprocessed) currentEvacuee->Status = EvacueeStatus::Processed;

					// determine if the previous round of DJs where fast enough and if not break out of the loop and have CARMALoop do something about it
					if (this->solverMethod == EvcSolverMethod::CASPERSolver && sumVisitedDirtyEdge > this->CARMAPerformanceRatio * sumVisitedEdge) break;

				} // end of for loop over sortedEvacuees
			} while (!sortedEvacuees->empty());

			UpdatePeakMemoryUsage();

			// figure out how may of paths need to be detached and process again
			NumberOfEvacueesInIteration = FindPathsThatNeedToBeProcessedInIteration(AllEvacuees, detachedPaths, GlobalEvcCostAtIteration, LocalIteration);
			if (NumberOfEvacueesInIteration > 0)
			{
				RevisedCarmaSortCriteria = CARMASort::ReverseFinalCost;
				EffectiveIterationRatio.push_back(pow(double(NumberOfEvacueesInIteration) / AllEvacuees->size(), 1.0 / GlobalEvcCostAtIteration.size()));
			}
		} while (NumberOfEvacueesInIteration > 0);
		progressBaseValue += (long)AllEvacuees->size();
	}

END_OF_FUNC:

	_ASSERT_EXPR(hr >= 0 || hr == E_ABORT, L"SolveMethod function exit with error");
	#ifdef TRACE
	std::ofstream f;
	f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
	f << "Search exit: " << hr << std::endl;
	f.close();
	#endif
	carmaSec = carmaSec / 10000000.0;
	return hr;
}

size_t EvcSolver::FindPathsThatNeedToBeProcessedInIteration(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<std::vector<EvcPathPtr>> detachedPaths,
	std::vector<double> & GlobalEvcCostAtIteration, size_t & LocalIteration) const
{
	std::vector<EvcPathPtr> allPaths;
	std::vector<EvacueePtr> EvacueesForNextIteration;
	std::unordered_set<NAEdgePtr, NAEdgePtrHasher, NAEdgePtrEqual> touchededges;

	// Recalculate all path costs and then list them in a sorted manner by descending final cost
	for (const auto & evc : *AllEvacuees)
		if (evc->Status != EvacueeStatus::Unreachable)
		{
			evc->FinalCost = 0.0;
			for (const auto & path : *evc->Paths)
				if (path->IsActive())
				{
					path->CalculateFinalEvacuationCost(initDelayCostPerPop, EvcSolverMethod::CASPERSolver);
					allPaths.push_back(path);
				}
		}

	std::sort(allPaths.begin(), allPaths.end(), EvcPath::MoreThanFinalCost);

	// setting up the best ratios
	const double minRatioOfLongestPath = allPaths.front()->GetMinCostRatio();
	const double ThreasholdForCost = max(0.15, minRatioOfLongestPath);
	const double ThreasholdForPathOverlap = 0.4;

	// collect what is the global evacuation time at each iteration and check that we're not getting worse
	GlobalEvcCostAtIteration.push_back(allPaths.front()->GetFinalEvacuationCost());

	// instead of assuming iteration is equal size of GlobalEvcCostAtIteration, we now ask that from the solver function.
	// it is garanteed that at least 'Iteration' many loops happened before and hence GlobalEvcCostAtIteration.size() >= Iteration
	// the number 'Iteration' referes to the loops that happened since the last dynamic change
	++LocalIteration;
	size_t GolbalIteration = GlobalEvcCostAtIteration.size();
	size_t MaxEvacueesInIteration = size_t(AllEvacuees->size() / (pow(1.0 / iterateRatio, LocalIteration)));

	if (LocalIteration > 1)
	{
		// check if it got worse and then undo it
		if (GlobalEvcCostAtIteration[GolbalIteration - 1] >  GlobalEvcCostAtIteration[GolbalIteration - 2] ||
		  ((GlobalEvcCostAtIteration[GolbalIteration - 1] == GlobalEvcCostAtIteration[GolbalIteration - 2]) && iterateRatio >= 1.0f))

		{
			std::sort(detachedPaths->begin(), detachedPaths->end(), EvcPath::LessThanPathOrder2);
			for (const auto & path : *detachedPaths) path->CleanYourEvacueePaths(solverMethod, touchededges);
			for (const auto & path : *detachedPaths) path->ReattachToEvacuee(solverMethod, touchededges);
			NAEdge::HowDirtyExhaustive(touchededges.begin(), touchededges.end(), solverMethod, 1.0);
			detachedPaths->clear();
			GlobalEvcCostAtIteration.pop_back();
			return 0;
		}

		// since we're not going to undo then we don't need the detached paths. Let's cleanup and collect new paths.
		for (const auto & path : *detachedPaths) delete path;
		detachedPaths->clear();
	}

	// And the next step is to find 'bad' paths and detach them so that the next iteration can find new paths for these evacuees.
	// If no `bad` paths where found then we leave `EvacueesForNextIteration` empty so that the solver terminates and returns.
	for (const auto & path : allPaths)
	{
		if (EvacueesForNextIteration.size() >= MaxEvacueesInIteration) break;
		path->DoesItNeedASecondChance(ThreasholdForCost, ThreasholdForPathOverlap, EvacueesForNextIteration, GlobalEvcCostAtIteration[GolbalIteration - 1], solverMethod);
	}

	// Now that we know which evacuees are going to be processed again, let's reset their values and detach their paths.
	std::sort(EvacueesForNextIteration.begin(), EvacueesForNextIteration.end(), EvcPath::MoreThanPathOrder1);
	for (const auto & evc : EvacueesForNextIteration) EvcPath::DetachPathsFromEvacuee(evc, solverMethod, touchededges, detachedPaths);
	NAEdge::HowDirtyExhaustive(touchededges.begin(), touchededges.end(), solverMethod, 1.0);

	return EvacueesForNextIteration.size();
}

HRESULT EvcSolver::CARMALoop(INetworkQueryPtr ipNetworkQuery, IStepProgressorPtr ipStepProgressor, IGPMessages* pMessages, ITrackCancel* pTrackCancel, std::shared_ptr<EvacueeList> Evacuees, std::shared_ptr<std::vector<EvacueePtr>> SortedEvacuees, std::shared_ptr<NAVertexCache> vcache,
	std::shared_ptr<NAEdgeCache> ecache, std::shared_ptr<SafeZoneTable> safeZoneList, size_t & closedSize, std::shared_ptr<NAEdgeMapTwoGen> closedList, std::shared_ptr<NAEdgeContainer> leafs,
	std::vector<unsigned int> & CARMAExtractCounts, double globalMinPop2Route, double & minPop2Route, bool separationRequired, CARMASort RevisedCarmaSortCriteria)
{
	HRESULT hr = S_OK;

	// performing pre-process: Here we will mark each vertex/junction with a heuristic value indicating
	// true distance to closest safe zone using backward traversal and Dijkstra
	MyFibonacciHeap<NAEdgePtr, NAEdgePtrHasher, NAEdgePtrEqual> heap(NAEdge::GetHeapKeyNonHur);	// creating the heap for the dijkstra search
	NAVertexPtr neighbor = nullptr;
	INetworkElementPtr ipElementEdge = nullptr;
	VARIANT val;
	val.vt = VT_R8; // double variant value
	NAVertexPtr myVertex = nullptr;
	NAEdgePtr myEdge = nullptr;
	INetworkElementPtr ipJunctionElement = nullptr;
	double newCost, SearchRadius, prevMinPop2Route = minPop2Route;
	VARIANT_BOOL keepGoing;
	INetworkJunctionPtr ipCurrentJunction = nullptr;
	std::vector<NAEdgePtr> readyEdges;
	readyEdges.reserve(safeZoneList->size());
	unsigned int CARMAExtractCount = 0;
	ArrayList<NAEdgePtr> * adj = nullptr;
	ATL::CString statusMsg;
	bool ShouldCARMACheckForDecreasedCost = false, FullSPTSelected = false;
	std::vector<NAEdgePtr> removedDirty; removedDirty.reserve(10000);
	const std::function<bool(EvacueePtr, EvacueePtr)> SortFunctions[7] =
		{ Evacuee::LessThanObjectID, Evacuee::LessThan, Evacuee::LessThan, Evacuee::MoreThan, Evacuee::MoreThan, Evacuee::ReverseFinalCost, Evacuee::ReverseEvacuationCost };

	// keeping reachable evacuees in a new hashtable for better access
	// also keep unreachable ones in the redundant list

	/// TODO what heppens here is that some evacuee may change location (DynamicCASPER) and hence the previous leafs
	/// may not be the same edges to discover them again. In case of a DSPT this may mislead CARMA.
	/// Also this is even more interesting when the evacuee is stuck
	NAEvacueeVertexTable EvacueePairs;
	EvacueePairs.InsertReachable(Evacuees, CarmaSortCriteria, leafs); // this is very important to be 'CarmaSortCriteria' with capital 'C'
	SortedEvacuees->clear();

	#ifdef TRACE
	std::ofstream f;
	#endif

	if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipJunctionElement))) return hr;
	ipCurrentJunction = ipJunctionElement;

	// if this list is not empty, it means we are going to have another CARMA loop
	if (!EvacueePairs.empty())
	{
		if (ipStepProgressor)
		{
			statusMsg.Format(_T("CARMA Loop %d: ..."), CARMAExtractCounts.size() + 1);
			if (FAILED(hr = ipStepProgressor->put_Message(ATL::CComBSTR(statusMsg)))) return hr;
		}

		// search for min population on graph evacuees left to be routed. The next if has to be in-tune with what population will be routed next.
		// the h values should always be an underestimation
		minPop2Route = 1.0; // separable CCRPSolver and any case of SPSolver
		if ((this->solverMethod == EvcSolverMethod::CASPERSolver) || (this->solverMethod == EvcSolverMethod::CCRPSolver && !separationRequired))
		{
			minPop2Route = CASPER_INFINITY;
			for (const auto & e : *Evacuees)
			{
				if (e->Status != EvacueeStatus::CARMALooking || e->Population <= 0.0) continue;
				minPop2Route = min(minPop2Route, e->Population);
			}
			if (separationRequired) minPop2Route = min(globalMinPop2Route, minPop2Route);
		}
		minPop2Route = max(minPop2Route, 1.0);

		// generally speaking we use FullSPT if the user wants it or if the mimPop2Route has changed.
		// if the minPop has changed it means pretty much all edges are dirty and there is no point checking them or do DSPT.
		// later in the code we also check if there are too many dirty edges and in that case we also revert back to FullSPT.
		FullSPTSelected = ThreeGenCARMA == VARIANT_FALSE || minPop2Route != prevMinPop2Route;
		if (FullSPTSelected) closedList->Clear(NAEdgeMapGeneration::AllGens); // Full SPT
		else closedList->MarkAllAsOldGen(); // DSPT option

		// This is where the new dynamic CARMA starts. At this point you have to clear the dirty section of the carma tree.
		// also keep the previous leafs only if they are still in closedList. They help re-discover EvacueePairs
		MarkDirtyEdgesAsUnVisited(closedList->oldGen, leafs, removedDirty, ShouldCARMACheckForDecreasedCost);

		if (ipStepProgressor)
		{
			if (FullSPTSelected) statusMsg.Format(_T("CARMA Loop %d: Full SPT"), CARMAExtractCounts.size() + 1);
			else if (ShouldCARMACheckForDecreasedCost) statusMsg.Format(_T("CARMA Loop %d: Fully-Dynamic SPT"), CARMAExtractCounts.size() + 1);
			else statusMsg.Format(_T("CARMA Loop %d: Semi-Dynamic SPT"), CARMAExtractCounts.size() + 1);
			if (FAILED(hr = ipStepProgressor->put_Message(ATL::CComBSTR(statusMsg)))) return hr;
		}
		#ifdef DEBUG
		std::wostringstream os_;
		os_ << statusMsg.GetBuffer(statusMsg.GetLength()) << std::endl;
		OutputDebugStringW(os_.str().c_str());
		statusMsg.ReleaseBuffer();
		#endif

		// pre-add dirty edges to heap with their old clean parents instead of checking it during loop
		if (FAILED(hr = FindDirtyEdgesWithACleanParent(ecache, vcache, ipNetworkQuery, closedList, leafs, removedDirty))) return hr;

		// prepare and insert safe zone vertices into the heap
		for (const auto & z : *safeZoneList)
		{
			if (FAILED(hr = PrepareVerticesForHeap(z.second->VertexAndRatio, vcache, ecache, closedList->oldGen, readyEdges, minPop2Route, solverMethod, 0.0, 0.0, QueryDirection::Forward))) return hr;
		}
		for (const auto & h : readyEdges)
		{
			// since this turns out to be a safe zone edge we should force the previous edge to be null in the tree
			h->TreePrevious = nullptr;
			heap.Insert(h);
		}

		// Now insert leaf edges in heap like the destination edges
		// do I have to insert leafs even if DSPT is off? It does not matter cause closedList is cleaned and hence all leafs will be removed anyway.
		if (FAILED(hr = InsertLeafEdgesToHeap(ipNetworkQuery, vcache, ecache, heap, leafs))) return hr;

		// we're done with all these leafs. let's clean up and collect new ones for the next round.
		leafs->Clear();
		SearchRadius = CASPER_INFINITY;

		// Continue traversing the network while the heap has remaining junctions in it
		// this is the actual Dijkstra code with backward network traversal. it will only update h value.
		while (!heap.empty())
		{
			// Remove the next junction EID from the top of the queue
			myEdge = heap.DeleteMin();
			_ASSERT_EXPR(!closedList->Exist(myEdge), L"CARMA closedList violation");
			if (FAILED(hr = closedList->Insert(myEdge)))
			{
				// closedList violation happened
				pMessages->AddError(-myEdge->EID, ATL::CComBSTR(L"CARMA ClosedList Violation."));
				return ATL::AtlReportError(this->GetObjectCLSID(), _T("CARMA ClosedList Violation."), IID_INASolver);
			}
			myVertex = myEdge->ToVertex;

			// Check to see if the user wishes to continue or cancel the solve
			if (pTrackCancel)
			{
				if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
				if (keepGoing == VARIANT_FALSE) return E_ABORT;
			}

			// check if this edge decreased its cost
			CARMAExtractCount++;

			// Code to build the CARMA Tree
			if (myVertex->Previous)
			{
				if (myEdge->TreePrevious) myEdge->TreePrevious->TreeNext.unordered_erase(myEdge, NAEdge::IsEqualNAEdgePtr);
				myEdge->TreePrevious = myVertex->Previous->GetBehindEdge();
				myEdge->TreePrevious->TreeNext.push_back(myEdge);
			}

			// part to check if this branch of DJ tree needs expanding to update heuristics. This update should know if this is the first time this vertex is coming out
			// in this 'CARMALoop' round. Only then we can be sure whether to update to min or update absolutely to this new value.
			myVertex->UpdateYourHeuristic();
			myEdge->SetClean(this->solverMethod, minPop2Route);

			// termination condition and evacuee discovery
			// if we've found all evacuees and we're beyond the search radius then instead of adding to the heap, we add it to the leafs list so that the next carma
			// loop we can use it to expand the rest of the tree ... if this branch was needed. Not adding the edge to the heap will basically render this edge invisible to the
			// future carma loops and can cause problems / inconsistancies. This is an attempt to solve the bug in issue 8: http://github.com/spatial-computing/CASPER/issues/8
			if (EvacueePairs.empty())
			{
				UpdatePeakMemoryUsage();
				leafs->Insert(myEdge);
				SearchRadius = min(SearchRadius, myVertex->GVal);
				continue;
			}

			EvacueePairs.RemoveDiscoveredEvacuees(myVertex, myEdge, SortedEvacuees, minPop2Route, solverMethod);

			if (FAILED(hr = ecache->QueryAdjacencies(myVertex, myEdge, QueryDirection::Backward, &adj))) return hr;

			for (const auto & currentEdge : *adj)
			{
				if (FAILED(hr = currentEdge->NetEdge->QueryJunctions(ipCurrentJunction, nullptr))) return hr;
				newCost = myVertex->GVal + currentEdge->GetCost(minPop2Route, solverMethod);
				if (newCost >= CASPER_INFINITY) continue;

				if (closedList->Exist(currentEdge, NAEdgeMapGeneration::OldGen))
				{
					if (ShouldCARMACheckForDecreasedCost)
					{
						neighbor = vcache->New(ipCurrentJunction, ipNetworkQuery);
						if (newCost < neighbor->GetH(currentEdge->EID))
						{
							neighbor->SetBehindEdge(currentEdge);
							neighbor->GVal = newCost;
							neighbor->Previous = myVertex;
							closedList->Erase(currentEdge, NAEdgeMapGeneration::OldGen);
							heap.Insert(currentEdge);
						}
					}
				}
				else
				{
					if (!closedList->Exist(currentEdge, NAEdgeMapGeneration::NewGen))
					{
						if (heap.IsVisited(currentEdge)) // vertex has been visited before. update vertex and decrease key.
						{
							neighbor = currentEdge->ToVertex;
							if (neighbor->GVal > newCost)
							{
								neighbor->SetBehindEdge(currentEdge);
								neighbor->GVal = newCost;
								neighbor->Previous = myVertex;
								heap.UpdateKey(currentEdge);
							}
						}
						else // unvisited vertex. create new and insert into heap
						{
							neighbor = vcache->New(ipCurrentJunction, ipNetworkQuery);
							neighbor->SetBehindEdge(currentEdge);
							neighbor->GVal = newCost;
							neighbor->Previous = myVertex;
							heap.Insert(currentEdge);
						}
					}
				}
			}
		}
		#ifdef DEBUG
		std::wostringstream os2;
		os2 << "CARMA Extract Count = " << CARMAExtractCount << std::endl;
		OutputDebugStringW(os2.str().c_str());
		#endif

		_ASSERT_EXPR(EvacueePairs.empty(), L"Carma loop ended after scanning all the graph");

		// set new default heuristic value
		vcache->UpdateHeuristicForOutsideVertices(SearchRadius, CARMAExtractCounts.empty());
		CARMAExtractCounts.push_back(CARMAExtractCount);
	}

	// load discovered evacuees into sorted list
	EvacueePairs.LoadSortedEvacuees(SortedEvacuees);

	std::sort(SortedEvacuees->begin(), SortedEvacuees->end(), SortFunctions[RevisedCarmaSortCriteria]);
	UpdatePeakMemoryUsage();
	closedSize = closedList->Size();
	
	#ifdef TRACE
	f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
	f << "CARMA visited edges = " << closedSize << std::endl;
	f.close();
	#endif

	return hr;
}

void EvcSolver::MarkDirtyEdgesAsUnVisited(NAEdgeMap * closedList, std::shared_ptr<NAEdgeContainer> oldLeafs, std::vector<NAEdgePtr> & removedDirty, bool & ShouldCARMACheckForDecreasedCost) const
{
	std::vector<NAEdgePtr> dirtyVisited;
	NAEdgeIterator j;
	NAEdgePtr leaf = nullptr;
	dirtyVisited.reserve(closedList->Size() / 2);
	auto tempLeafs = std::shared_ptr<NAEdgeContainer>(new DEBUG_NEW_PLACEMENT NAEdgeContainer(1000));
	removedDirty.clear();
	ShouldCARMACheckForDecreasedCost = false;

	// the assumption here is that the minPop2Route has not changhes since last carma loop
	// so only the edges that have recently been used in a path are dirty. so no need for a howdirty call. just a getdirtystate call is enough.
	closedList->GetDirtyEdges(dirtyVisited);

	for (const auto & d : dirtyVisited)
	{
		ShouldCARMACheckForDecreasedCost |= d->GetDirtyState() == EdgeDirtyState::CostDecreased;
		if (closedList->Exist(d))
		{
			leaf = d;
			while (leaf->TreePrevious && leaf->GetDirtyState() != EdgeDirtyState::CleanState) leaf = leaf->TreePrevious;
			NonRecursiveMarkAndRemove(leaf, closedList, removedDirty);

			// What is the definition of a leaf edge? An edge that has a previous (so it's not a destination edge) and has at least one dirty child edge.
			// So the usual for loop is going to insert destination dirty edges and the rest are in the leafs list.
			tempLeafs->Insert(leaf);
		}
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
}

void EvcSolver::NonRecursiveMarkAndRemove(NAEdgePtr head, NAEdgeMap * closedList, std::vector<NAEdgePtr> & removedDirty) const
{
	NAEdgePtr e = nullptr;
	std::stack<NAEdgePtr> subtree;
	subtree.push(head);
	while (!subtree.empty())
	{
		e = subtree.top();
		subtree.pop();
		closedList->Erase(e);
		removedDirty.push_back(e);
		for (const auto & i : e->TreeNext)
		{
			i->TreePrevious = nullptr;
			subtree.push(i);
		}
		e->TreeNext.clear();
	}
}

HRESULT InsertLeafEdgeToHeap(INetworkQueryPtr ipNetworkQuery, std::shared_ptr<NAVertexCache> vcache, MyFibonacciHeap<NAEdgePtr, NAEdgePtrHasher, NAEdgePtrEqual> & heap, NAEdge * leaf)
{
	HRESULT hr = S_OK;
	INetworkElementPtr fe, te;
	INetworkJunctionPtr f, t;

	// if it does not have a previous, then it's not a leaf ... it's a destination edge and it will be added to the heap at 'PrepareVerticesForHeap'
	if (leaf->TreePrevious)
	{
		// leaf by definition has to be a clean edge with a positive clean cost
		_ASSERT(leaf->GetCleanCost() > 0.0);
		_ASSERT(leaf->GetDirtyState() == EdgeDirtyState::CleanState);

		if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &fe))) return hr;
		if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &te))) return hr;
		f = fe; t = te;
		if (FAILED(hr = leaf->NetEdge->QueryJunctions(f, t))) return hr;
		NAVertexPtr fPtr = vcache->New(f);
		NAVertexPtr tPtr = vcache->Get(t);

		fPtr->SetBehindEdge(leaf);
		fPtr->GVal = tPtr->GetH(leaf->TreePrevious->EID) + leaf->GetCleanCost();
		fPtr->Previous = nullptr;
		_ASSERT(fPtr->GVal < CASPER_INFINITY);
		heap.Insert(leaf);
	}
	return hr;
}

HRESULT InsertLeafEdgesToHeap(INetworkQueryPtr ipNetworkQuery, std::shared_ptr<NAVertexCache> vcache, std::shared_ptr<NAEdgeCache> ecache, MyFibonacciHeap<NAEdgePtr, NAEdgePtrHasher, NAEdgePtrEqual> & heap,
								std::shared_ptr<NAEdgeContainer> leafs)
{
	HRESULT hr = S_OK;
	NAEdgePtr leaf;

	for (NAEdgeIterator i = leafs->begin(); i != leafs->end(); i++)
	{
		if (i->second & 1)
		{
			leaf = ecache->Get(i->first, esriNEDAlongDigitized);
			if (FAILED(hr = InsertLeafEdgeToHeap(ipNetworkQuery, vcache, heap, leaf))) return hr;
		}
		if (i->second & 2)
		{
			leaf = ecache->Get(i->first, esriNEDAgainstDigitized);
			if (FAILED(hr = InsertLeafEdgeToHeap(ipNetworkQuery, vcache, heap, leaf))) return hr;
		}
	}
	return hr;
}

HRESULT FindDirtyEdgesWithACleanParent(std::shared_ptr<NAEdgeCache> ecache, std::shared_ptr<NAVertexCache> vcache, INetworkQueryPtr ipNetworkQuery, std::shared_ptr<NAEdgeMapTwoGen> closedList,
	std::shared_ptr<NAEdgeContainer> Leafs, std::vector<NAEdgePtr> & removedDirty)
{
	HRESULT hr = S_OK;
	NAVertexPtr toVertex = nullptr;
	ArrayList<NAEdgePtr> * adj = nullptr;
	double betterH, tempH;
	INetworkElementPtr te;
	NAEdgePtr betterParent = nullptr;

	if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &te))) return hr;
	INetworkJunctionPtr t = te;

	for (const auto e : removedDirty)
	{
		betterParent = nullptr;
		betterH = CASPER_INFINITY;

		if (FAILED(hr = e->NetEdge->QueryJunctions(nullptr, t))) return hr;
		toVertex = vcache->New(t, ipNetworkQuery);
		if (FAILED(hr = ecache->QueryAdjacencies(toVertex, e, QueryDirection::Forward, &adj))) return hr;

		// Loop through all adjacent edges and update their cost value
		for (const auto & tempEdge : *adj)
		{
			// it has to be present in closed list from previous CARMA loop
			// it has to have a previous otherwise it cannot be a leaf
			if (tempEdge->TreePrevious && closedList->Exist(tempEdge, NAEdgeMapGeneration::OldGen))
			{
				tempH = toVertex->GetH(tempEdge->EID);
				if (!betterParent || tempH < betterH) { betterParent = tempEdge; betterH = tempH; }
			}
		}
		// for this border dirty edge we add the best parent it has
		if (betterParent)
		{
			Leafs->Insert(betterParent);
			closedList->Erase(betterParent, NAEdgeMapGeneration::OldGen);
		}
	}
	return hr;
}

HRESULT PrepareVerticesForHeap(NAVertexPtr point, std::shared_ptr<NAVertexCache> vcache, std::shared_ptr<NAEdgeCache> ecache, NAEdgeMap * closedList, std::vector<NAEdgePtr> & readyEdges, double pop,
	EvcSolverMethod solverMethod, double selfishRatio, double MaxEvacueeCostSoFar, QueryDirection dir)
{
	HRESULT hr = S_OK;
	NAVertexPtr temp;
	NAEdgePtr edge;
	double globalDeltaPenalty = 0.0, edgeCost = 0.0;
	ArrayList<NAEdgePtr> * adj = nullptr;

	// Remember vertex gval is now only ratio along edge
	temp = vcache->New(point->Junction);
	temp->SetBehindEdge(point->GetBehindEdge());
	temp->GVal = point->GVal;
	temp->GlobalPenaltyCost = point->GlobalPenaltyCost;
	temp->Junction = point->Junction;
	temp->Previous = nullptr;
	edge = temp->GetBehindEdge();

	// check to see if the edge you're about to insert is not in the closedList
	if (edge)
	{
		if (!closedList->Exist(edge))
		{
			edgeCost = edge->GetCost(pop, solverMethod, &globalDeltaPenalty) /* / edge->OriginalCost*/;
			if (edgeCost >= CASPER_INFINITY) temp->GVal = CASPER_INFINITY;
			else temp->GVal = point->GVal * edgeCost;
			temp->GlobalPenaltyCost = edge->MaxAddedCostOnReservedPathsWithNewFlow(globalDeltaPenalty, MaxEvacueeCostSoFar, temp->GVal + temp->GetMinHOrZero(), selfishRatio);
			readyEdges.push_back(edge);
		}
		else _ASSERT(false);
	}
	else
	{
		// if the start point was a single junction, then all the adjacent edges can be start edges
		if (FAILED(hr = ecache->QueryAdjacencies(temp, nullptr, dir, &adj))) return hr;
		for (const auto & edge : *adj)
		{
			if (closedList->Exist(edge)) continue; // DSPT condition .... only dirty destination edges are inserted.
			temp = vcache->New(point->Junction);
			temp->Previous = nullptr;
			temp->SetBehindEdge(edge);
			edgeCost = edge->GetCost(pop, solverMethod, &globalDeltaPenalty) /* / edge->OriginalCost*/;
			if (edgeCost >= CASPER_INFINITY) temp->GVal = CASPER_INFINITY;
			else temp->GVal = point->GVal * edgeCost;
			temp->GlobalPenaltyCost = edge->MaxAddedCostOnReservedPathsWithNewFlow(globalDeltaPenalty, MaxEvacueeCostSoFar, temp->GVal + temp->GetMinHOrZero(), selfishRatio);
			readyEdges.push_back(edge);
		}
	}
	return hr;
}

bool EvcSolver::GeneratePath(SafeZonePtr BetterSafeZone, NAVertexPtr finalVertex, double & populationLeft, int & pathGenerationCount, EvacueePtr currentEvacuee, double population2Route, bool separationRequired) const
{
	double leftCap, edgePortion;
	EvcPath * path = nullptr;

	// generate evacuation route if a destination has been found
	if (BetterSafeZone)
	{
		// First find out about remaining capacity of this path
		NAVertexPtr temp = finalVertex;

		if (this->solverMethod == EvcSolverMethod::CCRPSolver)
		{
			if (separationRequired)
			{
				population2Route = 0.0;
				while (temp->Previous)
				{
					leftCap = temp->GetBehindEdge()->LeftCapacity();
					if (this->solverMethod == EvcSolverMethod::CCRPSolver || leftCap > 0.0) population2Route = min(population2Route, leftCap);
					temp = temp->Previous;
				}
				if (population2Route <= 0.0) population2Route = populationLeft;
				population2Route = min(population2Route, populationLeft);
			}
			else population2Route = populationLeft;
		}
		populationLeft -= population2Route;

		// create a new path for this portion of the population
		path = new DEBUG_NEW_PLACEMENT EvcPath(initDelayCostPerPop, population2Route, ++pathGenerationCount, currentEvacuee, BetterSafeZone);

		// special case for the last edge. We have to sub-curve it based on the safe point location along the edge
		if (BetterSafeZone->getBehindEdge())
		{
			edgePortion = BetterSafeZone->getPositionAlong();
			if (edgePortion > 0.0) path->AddSegment(solverMethod, new DEBUG_NEW_PLACEMENT PathSegment(BetterSafeZone->getBehindEdge(), 0.0, edgePortion));
		}

		while (finalVertex->Previous)
		{
			if (finalVertex->GetBehindEdge()) path->AddSegment(solverMethod, new DEBUG_NEW_PLACEMENT PathSegment(finalVertex->GetBehindEdge()));
			finalVertex = finalVertex->Previous;
		}

		// special case for the first edge. We have to curve it based on the evacuee point location along the edge
		if (finalVertex->GetBehindEdge())
		{
			// search for mother vertex and its position along the edge
			edgePortion = 1.0;
			for (const auto & v : *currentEvacuee->VerticesAndRatio)
				if (v->EID == finalVertex->EID)
				{
					edgePortion = v->GVal;
					break;
				}

			// path can be empty if the source and destination are the same vertex
			PathSegmentPtr lastAdded = path->front();
			if (!path->empty() && NAEdge::IsEqualNAEdgePtr(lastAdded->Edge, finalVertex->GetBehindEdge()))
			{
				lastAdded->SetFromRatio(1.0 - edgePortion);
			}
			else if (edgePortion > 0.0)
			{
				path->AddSegment(solverMethod, new DEBUG_NEW_PLACEMENT PathSegment(finalVertex->GetBehindEdge(), 1.0 - edgePortion, 1.0));
			}
		}
		if (path->empty())
		{
			delete path;
			path = nullptr;
		}
		else
		{
			path->shrink_to_fit();
			currentEvacuee->Paths->push_front(path);
			BetterSafeZone->Reserve(path->GetRoutedPop());
		}
	}
	else
	{
		populationLeft = 0.0; // since no path could be found for this evacuee, we assume the rest of the population at this location have no path as well
	}
	return path != nullptr;
}

// This is where i figure out what is the smallest population that I should route (or try to route)
// at each CASPER loop. Obviously this globalMinPop2Route has to be less than the population of any evacuee point.
// Also CASPER and CARMA should be in sync at this number otherwise all the h values are useless.
HRESULT EvcSolver::DeterminMinimumPop2Route(std::shared_ptr<EvacueeList> Evacuees, INetworkDatasetPtr ipNetworkDataset, double & globalMinPop2Route, bool & separationRequired) const
{
	double minPop = CASPER_INFINITY, maxPop = 1.0, CommonCostOfEdgeInUnits = 1.0, avgPop = 0.0;
	HRESULT hr = S_OK;
	INetworkAttributePtr costAttrib;
	esriNetworkAttributeUnits unit;
	size_t count = 0;
	globalMinPop2Route = 0.0;
	separationRequired = Evacuees->IsSeperable();

	if (separationRequired && this->solverMethod == EvcSolverMethod::CASPERSolver)
	{
		for (const auto & e : *Evacuees)
			if (e->Population > 0.0)
			{
				count++;
				avgPop += e->Population;
				if (e->Population > maxPop) maxPop = e->Population;
				if (e->Population < minPop) minPop = e->Population;
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
			globalMinPop2Route = SaturationPerCap / 3.0;

			// We don't want the minimum routable population to be less than one-forth of the minimum population of any evacuee point. That just makes CASPER too slow.
			if (globalMinPop2Route * 4.0 < minPop) globalMinPop2Route = minPop / 4.0;
			if (globalMinPop2Route < 1.0) globalMinPop2Route = 1.0;
		}
	}
	return hr;
}

void EvcSolver::UpdatePeakMemoryUsage()
{
	_ASSERTE(_CrtCheckMemory());
	PROCESS_MEMORY_COUNTERS pmc;
	if (!hProcessPeakMemoryUsage) hProcessPeakMemoryUsage = GetCurrentProcess();
	if (GetProcessMemoryInfo(hProcessPeakMemoryUsage, &pmc, sizeof(pmc))) peakMemoryUsage = max(peakMemoryUsage, pmc.PagefileUsage);
}

HRESULT PrepareUnvisitedVertexForHeap(INetworkJunctionPtr junction, NAEdgePtr edge, NAEdgePtr prevEdge, double edgeCost, NAVertexPtr myVertex, std::shared_ptr<NAEdgeCache> ecache,
	std::shared_ptr<NAEdgeMapTwoGen> closedList, std::shared_ptr<NAVertexCache> vcache, INetworkQueryPtr ipNetworkQuery, bool checkOldClosedlist)
{
	HRESULT hr = S_OK;
	NAVertexPtr betterMyVertex;
	NAEdgePtr betterEdge = nullptr;
	double betterH, tempH;
	NAVertexPtr tempVertex, neighbor;
	ArrayList<NAEdgePtr> * adj = nullptr;

	// Dynamic CARMA: at this step we have to check if there is any better previous edge for this new one in closed-list
	// this is the vertex at the center of two edges... we have to check its heuristics to see if the new twempEdge is any better.
	tempVertex = vcache->Get(myVertex->EID);
	betterH = myVertex->GVal;

	if (checkOldClosedlist)
	{
		if (FAILED(hr = ecache->QueryAdjacencies(myVertex, edge, QueryDirection::Forward, &adj))) return hr;

		// Loop through all adjacent edges and update their cost value
		for (const auto & tempEdge : *adj)
		{
			if (!closedList->Exist(tempEdge, NAEdgeMapGeneration::OldGen)) continue; // it has to be present in closed list from previous CARMA loop
			if (NAEdge::IsEqualNAEdgePtr(tempEdge, prevEdge)) continue; // it cannot be the same parent edge

			// at this point if the new tempEdge satisfied all restrictions and conditions it means it might be a good pick
			// as a previous edge depending on the cost which we shall obtain from vertices heuristic table
			tempH = tempVertex->GetH(tempEdge->EID);
			if (tempH < betterH) { betterEdge = tempEdge; betterH = tempH; }
		}
	}
	if (betterEdge)
	{
		#ifdef DEBUG
		if (!checkOldClosedlist)
		{
			double CostToBeat = edge->ToVertex->GetH(edge->EID);
			_ASSERT(CostToBeat - betterH - edgeCost < FLT_EPSILON);
		}
		#endif

		betterMyVertex = vcache->New(myVertex->Junction, ipNetworkQuery);
		betterMyVertex->SetBehindEdge(betterEdge);
		betterMyVertex->Previous = nullptr;
		betterMyVertex->GVal = betterH;
	}
	else betterMyVertex = myVertex;

	neighbor = vcache->New(junction, ipNetworkQuery);
	neighbor->SetBehindEdge(edge);
	neighbor->GVal = edgeCost + betterMyVertex->GVal;
	neighbor->Previous = betterMyVertex;

	return hr;
}
