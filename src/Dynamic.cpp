// ===============================================================================================
// Evacuation Solver: Dynamic classes definition Implementation
// Description: Contains implementation of Dynamic CASPER solver. Hold information about affected
// paths, moves the evacuees, runs DSPT, then runs CASPER on affected cars, then iterate to correct.
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#include "stdafx.h"
#include "Dynamic.h"
#include "NameConstants.h"
#include "Evacuee.h"
#include "NAVertex.h"

// constants for range of valid ratio
const double EdgeOriginalData::MaxCostRatio     = 1000.0;
const double EdgeOriginalData::MinCostRatio     = 1.0 / 1000.0;
const double EdgeOriginalData::MaxCapacityRatio = 1000.0;
const double EdgeOriginalData::MinCapacityRatio = 1.0 / 100.0;

DynamicDisaster::DynamicDisaster(ITablePtr DynamicChangesTable, DynamicMode dynamicMode, bool & flagBadDynamicChangeSnapping, EvcSolverMethod solverMethod) : 
	myDynamicMode(dynamicMode), SolverMethod(solverMethod)
{
	HRESULT hr = S_OK;
	long count, EdgeDirIndex, StartTimeIndex, EndTimeIndex, CostIndex, CapacityIndex;
	ICursorPtr ipCursor = nullptr;
	IRowESRI * ipRow = nullptr;
	VARIANT var;
	INALocationRangesPtr range = nullptr;
	SingleDynamicChangePtr item = nullptr;
	long EdgeCount, JunctionCount, EID;
	esriNetworkEdgeDirection dir;
	double fromPosition, toPosition;
	std::set<CriticalTime>::_Pairib fr, bc;
	currentTime = dynamicTimeFrame.end();

	if (dynamicMode == DynamicMode::Disabled)
	{
		dynamicTimeFrame.emplace(CriticalTime(0.0));
		dynamicTimeFrame.emplace(CriticalTime(CASPER_INFINITY));
		goto END_OF_FUNC;
	}

	if (!DynamicChangesTable)
	{
		hr = E_POINTER;
		goto END_OF_FUNC;
	}

	if (FAILED(hr = DynamicChangesTable->FindField(CS_FIELD_DYNROADDIR, &EdgeDirIndex))) goto END_OF_FUNC;
	if (FAILED(hr = DynamicChangesTable->FindField(CS_FIELD_DYNSTARTTIME, &StartTimeIndex))) goto END_OF_FUNC;
	if (FAILED(hr = DynamicChangesTable->FindField(CS_FIELD_DYNENDTIME, &EndTimeIndex))) goto END_OF_FUNC;
	if (FAILED(hr = DynamicChangesTable->FindField(CS_FIELD_DYNCOST, &CostIndex))) goto END_OF_FUNC;
	if (FAILED(hr = DynamicChangesTable->FindField(CS_FIELD_DYNCAPACITY, &CapacityIndex))) goto END_OF_FUNC;
	if (FAILED(hr = DynamicChangesTable->Search(nullptr, VARIANT_TRUE, &ipCursor))) goto END_OF_FUNC;
	if (FAILED(hr = DynamicChangesTable->RowCount(nullptr, &count))) goto END_OF_FUNC;
	allChanges.reserve(count);

	while (ipCursor->NextRow(&ipRow) == S_OK)
	{
		item = new DEBUG_NEW_PLACEMENT SingleDynamicChange();
		
		if (FAILED(hr = ipRow->get_Value(EdgeDirIndex, &var))) goto END_OF_FUNC;
		item->DisasterDirection = EdgeDirection(var.lVal);
		if (FAILED(hr = ipRow->get_Value(StartTimeIndex, &var))) goto END_OF_FUNC;
		item->StartTime = var.dblVal;
		if (FAILED(hr = ipRow->get_Value(EndTimeIndex, &var))) goto END_OF_FUNC;
		item->EndTime = var.dblVal;
		if (FAILED(hr = ipRow->get_Value(CostIndex, &var))) goto END_OF_FUNC;
		item->AffectedCostRate = var.dblVal;
		if (FAILED(hr = ipRow->get_Value(CapacityIndex, &var))) goto END_OF_FUNC;
		item->AffectedCapacityRate = var.dblVal;

		// load associated edge and junctions
		INALocationRangesObjectPtr blob(ipRow);
		if (!blob) continue; // we only want valid location objects
		if (FAILED(hr = blob->get_NALocationRanges(&range))) goto END_OF_FUNC;
		if (FAILED(hr = range->get_EdgeRangeCount(&EdgeCount))) goto END_OF_FUNC;
		if (FAILED(hr = range->get_JunctionCount(&JunctionCount))) goto END_OF_FUNC;
		for (long i = 0; i < EdgeCount; ++i)
		{
			if (FAILED(hr = range->QueryEdgeRange(i, &EID, &dir, &fromPosition, &toPosition))) continue;
			item->EnclosedEdges.insert(EID);
		}
		flagBadDynamicChangeSnapping |= JunctionCount != 0 && EdgeCount == 0;

		// we're done with this item and no error happened. we'll push it to the set and continue to the other one
		if (item->IsValid()) allChanges.push_back(item);
		else delete item;
		item = nullptr;
	}

	// can i model the good old static barrier layer using my DynbamicChanges layer?
	dynamicTimeFrame.clear();
	fr = dynamicTimeFrame.emplace(CriticalTime(0.0));
	bc = dynamicTimeFrame.emplace(CriticalTime(CASPER_INFINITY));

	if (myDynamicMode == DynamicMode::Simple)
	{
		// if we are in simp[le mode, we ignore all times and apply all changes at time 0, untill infinity
		for (const auto & p : allChanges)
		{
			fr.first->AddIntersectedChange(p);
			bc.first->AddIntersectedChange(p);
		}
	}
	else if (myDynamicMode == DynamicMode::Smart || myDynamicMode == DynamicMode::Full)
	{
		for (const auto & p : allChanges)
		{
			auto i = dynamicTimeFrame.emplace(CriticalTime(p->StartTime));
			dynamicTimeFrame.emplace(CriticalTime(p->EndTime));
			i.first->AddIntersectedChange(p);
		}
		CriticalTime::MergeWithPreviousTimeFrame(dynamicTimeFrame);

		// check if we can downgrade the time frame to Simple mode
		if (dynamicTimeFrame.size() == 2) myDynamicMode = DynamicMode::Simple;
	}

END_OF_FUNC:
	if (FAILED(hr))
	{
		if (item) delete item;
		myDynamicMode = DynamicMode::Disabled;
		Flush();
	}
}

size_t DynamicDisaster::ResetDynamicChanges()
{
	currentTime = dynamicTimeFrame.begin();
	return dynamicTimeFrame.size() - 1;
}

void CriticalTime::MergeWithPreviousTimeFrame(std::set<CriticalTime> & dynamicTimeFrame)
{
	auto thisTime = dynamicTimeFrame.cbegin();
	std::vector<SingleDynamicChangePtr> * previousTimeFrame = &(thisTime->Intersected);
	
	for (++thisTime; thisTime != dynamicTimeFrame.cend(); ++thisTime)
	{
		for (auto prevChange : *previousTimeFrame) 
			if (prevChange->EndTime > thisTime->Time || prevChange->EndTime >= CASPER_INFINITY) thisTime->AddIntersectedChange(prevChange);
		previousTimeFrame = &(thisTime->Intersected);
	}
}

size_t DynamicDisaster::NextDynamicChange(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<NAEdgeCache> ecache, double & EvcStartTime)
{
	size_t EvcCount = 0;
	_ASSERT_EXPR(currentTime != dynamicTimeFrame.end(), L"NextDynamicChange function called on invalid iterator");
	if (currentTime == dynamicTimeFrame.end()) return 0;
	EvcCount = currentTime->ProcessAllChanges(AllEvacuees, ecache, EvcStartTime, OriginalEdgeSettings, this->myDynamicMode, SolverMethod);
	++currentTime;
	return EvcCount;
}

size_t CriticalTime::ProcessAllChanges(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<NAEdgeCache> ecache, double & EvcStartTime,
	std::unordered_map<NAEdgePtr, EdgeOriginalData, NAEdgePtrHasher, NAEdgePtrEqual> & OriginalEdgeSettings, DynamicMode myDynamicMode, EvcSolverMethod solverMethod) const
{
	size_t CountPaths = AllEvacuees->size();
	EvcStartTime = this->Time;

	// first undo previous changes using the backup map 'OriginalEdgeSettings'
	for (auto & pair : OriginalEdgeSettings) pair.second.ResetRatios();

	// next apply new changes to enclosed edges. backup original settings into the 'OriginalEdgeSettings' map
	NAEdgePtr edge = nullptr;
	std::unordered_map<NAEdgePtr, EdgeOriginalData, NAEdgePtrHasher, NAEdgePtrEqual>::_Pairib i;
	std::unordered_set<NAEdgePtr, NAEdgePtrHasher, NAEdgePtrEqual> DynamicallyAffectedEdges;

	for (auto polygon : this->Intersected)
	{
		if (CheckFlag(polygon->DisasterDirection, EdgeDirection::Along))
		{
			for (auto EID : polygon->EnclosedEdges)
			{
				edge = ecache->New(EID, esriNetworkEdgeDirection::esriNEDAlongDigitized);
				i = OriginalEdgeSettings.emplace(std::pair<NAEdgePtr, EdgeOriginalData>(edge, EdgeOriginalData(edge)));
				i.first->second.CapacityRatio *= polygon->AffectedCapacityRate;
				i.first->second.CostRatio     *= polygon->AffectedCostRate;
			}
		}
		if (CheckFlag(polygon->DisasterDirection, EdgeDirection::Against))
		{
			for (auto EID : polygon->EnclosedEdges)
			{
				edge = ecache->New(EID, esriNetworkEdgeDirection::esriNEDAgainstDigitized);
				i = OriginalEdgeSettings.emplace(std::pair<NAEdgePtr, EdgeOriginalData>(edge, EdgeOriginalData(edge)));
				i.first->second.CapacityRatio *= polygon->AffectedCapacityRate;
				i.first->second.CostRatio     *= polygon->AffectedCostRate;
			}
		}
	}
	if (this->Time < CASPER_INFINITY)
	{
		// extract affected edges and use it to identify affected evacuee paths
		for (auto & pair : OriginalEdgeSettings) if (pair.second.IsAffectedEdge(pair.first)) DynamicallyAffectedEdges.insert(pair.first);

		// for each evacuee, find the edge that the evacuee is likely to be their based on the time of this event
		// now it's time to move all evacuees along their paths based on current time of this event and evacuee stuck policy
		if (this->Time > 0.0)
		{
			if (myDynamicMode == DynamicMode::Full)
			{
				std::vector<EvcPathPtr> allPaths;
				allPaths.reserve(AllEvacuees->size());
				for (auto e : *AllEvacuees) for (auto p : *(e->Paths)) if (p->IsActive()) allPaths.push_back(p);
				CountPaths = EvcPath::DynamicStep_MoveOnPath(allPaths.begin(), allPaths.end(), DynamicallyAffectedEdges, this->Time, solverMethod, ecache->GetNetworkQuery());
			}
			else if (myDynamicMode == DynamicMode::Smart)
			{
				std::vector<EvcPathPtr> AffectedPaths;
				AffectedPaths.reserve(min(AllEvacuees->size(), DynamicallyAffectedEdges.size()));
				NAEdge::DynamicStep_ExtractAffectedPaths(AffectedPaths, DynamicallyAffectedEdges);
				CountPaths = EvcPath::DynamicStep_MoveOnPath(AffectedPaths.begin(), AffectedPaths.end(), DynamicallyAffectedEdges, this->Time, solverMethod, ecache->GetNetworkQuery());
			}
			CountPaths += EvcPath::DynamicStep_UnreachableEvacuees(AllEvacuees, this->Time);

			// what if all paths are OK and non are affected and there are no unreachable evacuees?
			// In that case we still need to call CASPER and allow for at least of iterative loop to
			// execute just to make sure this round of graph changes is also considered.
			if (CountPaths == 0) CountPaths = 1;
		}
	}
	// now apply changes to the graph
	for (auto & pair : OriginalEdgeSettings) pair.second.ApplyNewOriginalCostAndCapacity(pair.first);

	if (this->Time >= CASPER_INFINITY)
	{
		// merge paths together only if we are in a non-simple mode
		if (myDynamicMode == DynamicMode::Smart || myDynamicMode == DynamicMode::Full) EvcPath::DynamicStep_MergePaths(AllEvacuees);
		CountPaths = 0;
		OriginalEdgeSettings.clear();
	}
	else
	{
		// re-calculate edges' dirtyness state
		NAEdge::HowDirtyExhaustive(DynamicallyAffectedEdges.begin(), DynamicallyAffectedEdges.end(), solverMethod, 1.0);

		// then clean the edges from backup map only if they are no longer affected
		std::unordered_map<NAEdgePtr, EdgeOriginalData, NAEdgePtrHasher, NAEdgePtrEqual> clone(OriginalEdgeSettings);
		OriginalEdgeSettings.clear();
		for (const auto pair : clone) if (pair.second.IsRatiosNonOne()) OriginalEdgeSettings.insert(pair);
	}
	return CountPaths;
}
