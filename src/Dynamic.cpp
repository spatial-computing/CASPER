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

DynamicDisaster::DynamicDisaster(ITablePtr DynamicChangesTable, DynamicMode dynamicMode, bool & flagBadDynamicChangeSnapping) :
	myDynamicMode(dynamicMode)
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
	currentTime = dynamicTimeFrame.end();

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
		item = nullptr;
	}

END_OF_FUNC:
	if (FAILED(hr))
	{
		if (item) delete item;
		for (auto p : allChanges) delete p;
		allChanges.clear();
		myDynamicMode = DynamicMode::Disabled;
	}
}

void DynamicDisaster::ResetDynamicChanges()
{
	dynamicTimeFrame.clear();
	dynamicTimeFrame.emplace(CriticalTime(0.0));

	for (const auto & p : allChanges)
	{	
		auto i = dynamicTimeFrame.emplace(CriticalTime(p->StartTime));
		i.first->AddIntersectedChange(p);		
		if (p->EndTime < FLT_MAX) dynamicTimeFrame.emplace(CriticalTime(p->EndTime));
	}
	CriticalTime::MergeWithPreviousTimeFrame(dynamicTimeFrame);
	currentTime = dynamicTimeFrame.begin();
}

void CriticalTime::MergeWithPreviousTimeFrame(std::set<CriticalTime> & dynamicTimeFrame)
{
	auto thisTime = dynamicTimeFrame.cbegin();
	std::vector<SingleDynamicChangePtr> * previousTimeFrame = &(thisTime->Intersected);
	
	for (++thisTime; thisTime != dynamicTimeFrame.cend(); ++thisTime)
	{
		for (auto prevChange : *previousTimeFrame) if (prevChange->EndTime > thisTime->Time) thisTime->AddIntersectedChange(prevChange);
		previousTimeFrame = &(thisTime->Intersected);
	}
}

bool DynamicDisaster::NextDynamicChange(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<NAEdgeCache> ecache)
{
	if (currentTime == dynamicTimeFrame.end()) return false;
	const CriticalTime & currentChangeGroup = *currentTime;
	currentChangeGroup.ProcessAllChanges(AllEvacuees, ecache, OriginalEdgeSettings, this->myDynamicMode);
	++currentTime;
	return true;
}

void CriticalTime::ProcessAllChanges(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<NAEdgeCache> ecache,
	 std::unordered_map<NAEdgePtr, EdgeOriginalData, NAEdgePtrHasher, NAEdgePtrEqual> & OriginalEdgeSettings, DynamicMode myDynamicMode) const
{
	// for each evacuee, find the edge that the evacuee is likely to be their based on the time of this event
	// now it's time to move all evacuees along their paths based on current time of this event and evacuee stuck policy
	if (myDynamicMode == DynamicMode::Full) for (auto evc : *AllEvacuees) evc->DynamicStep_MoveOnPath(this->Time);
	else;///

	// first undo previous changes using the backup map 'OriginalEdgeSettings'
	for (auto & pair : OriginalEdgeSettings) pair.second.ResetRatios();	

	// next apply new changes to enclosed edges. backup original settings into the 'OriginalEdgeSettings' map
	NAEdgePtr edge = nullptr;
	for (auto polygon : Intersected)
		for (auto EID : polygon->EnclosedEdges)
		{
			if (CheckFlag(polygon->DisasterDirection, EdgeDirection::Along))
			{
				edge = ecache->New(EID, esriNetworkEdgeDirection::esriNEDAlongDigitized);
				auto i = OriginalEdgeSettings.emplace(std::pair<NAEdgePtr, EdgeOriginalData>(edge, EdgeOriginalData(edge)));
				i.first->second.CapacityRatio *= polygon->AffectedCapacityRate;
				i.first->second.CostRatio *= polygon->AffectedCostRate;
			}
			if (CheckFlag(polygon->DisasterDirection, EdgeDirection::Against))
			{
				edge = ecache->New(EID, esriNetworkEdgeDirection::esriNEDAgainstDigitized);
				auto i = OriginalEdgeSettings.emplace(std::pair<NAEdgePtr, EdgeOriginalData>(edge, EdgeOriginalData(edge)));
				i.first->second.CapacityRatio *= polygon->AffectedCapacityRate;
				i.first->second.CostRatio *= polygon->AffectedCostRate;
			}
		}

	// apply to the graph
	for (auto & pair : OriginalEdgeSettings) pair.second.ApplyNewOriginalCostAndCapacity(pair.first);
	
	// then clean the edges only if they are no longer affected
	std::unordered_map<NAEdgePtr, EdgeOriginalData, NAEdgePtrHasher, NAEdgePtrEqual> clone(OriginalEdgeSettings);
	OriginalEdgeSettings.clear();
	for (const auto pair : clone) if (pair.second.IsAffectedEdge()) OriginalEdgeSettings.insert(pair);
}
