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

DynamicDisaster::DynamicDisaster(ITablePtr DynamicChangesTable, DynamicMode dynamicMode) :
	myDynamicMode(dynamicMode)
{
	HRESULT hr = S_OK;
	long count, EdgeDirIndex, StartTimeIndex, EndTimeIndex, CostIndex, CapacityIndex, EvcIndex;
	ICursorPtr ipCursor = nullptr;
	IRowESRI * ipRow = nullptr;
	VARIANT var;
	INALocationRangesPtr range = nullptr;
	SingleDynamicChangePtr item = nullptr;
	long EdgeCount, JunctionCount, EID, JID;
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
	if (FAILED(hr = DynamicChangesTable->FindField(CS_FIELD_DYNEVCSTUCK, &EvcIndex))) goto END_OF_FUNC;
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
		if (FAILED(hr = ipRow->get_Value(EvcIndex, &var))) goto END_OF_FUNC;
		item->EvacueesAreStuck = var.lVal == 1l;

		// load associated edge and junctions
		INALocationRangesObjectPtr blob(ipRow);
		if (!blob) continue; // we only want valid location objects
		if (FAILED(hr = blob->get_NALocationRanges(&range))) goto END_OF_FUNC;
		if (FAILED(hr = range->get_EdgeRangeCount(&EdgeCount))) goto END_OF_FUNC;
		if (FAILED(hr = range->get_JunctionCount(&JunctionCount))) goto END_OF_FUNC;
		for (long i = 0; i < EdgeCount; ++i)
		{
			if (FAILED(hr = range->QueryEdgeRange(i, &EID, &dir, &fromPosition, &toPosition))) continue;
			item->EnclosedEdges.insert(std::pair<long, esriNetworkEdgeDirection>(EID, dir));
		}
		for (long i = 0; i < JunctionCount; ++i)
		{
			if (FAILED(hr = range->QueryJunction(i, &JID))) continue;
			item->EnclosedVertices.insert(JID);
		}

		// we're done with this item and no error happened. we'll push it to the set and continue to the other one
		item->check();
		allChanges.push_back(item);
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
		if (p->StartTime >= 0.0)
		{
			auto i = dynamicTimeFrame.emplace(CriticalTime(p->StartTime));
			i.first->AddApplyChange(p);
		}
		if (p->EndTime >= 0.0)
		{
			auto j = dynamicTimeFrame.emplace(CriticalTime(p->EndTime));
			j.first->AddUnapplyChange(p);
		}
	}
	currentTime = dynamicTimeFrame.begin();
}

bool DynamicDisaster::NextDynamicChange(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<NAVertexCache> vcache, std::shared_ptr<NAEdgeCache> ecache)
{
	if (currentTime == dynamicTimeFrame.end()) return false;
	const CriticalTime & currentChangeGroup = *currentTime;
	currentChangeGroup.ProcessAllChanges(AllEvacuees, vcache, ecache, OriginalEdgeSettings);
	++currentTime;
	return true;
}

void CriticalTime::ProcessAllChanges(std::shared_ptr<EvacueeList> AllEvacuees, std::shared_ptr<NAVertexCache> vcache, std::shared_ptr<NAEdgeCache> ecache,
	std::unordered_map<NAEdgePtr, std::pair<double, double>, NAEdgePtrHasher, NAEdgePtrEqual> & OriginalEdgeSettings) const
{
	// first undo previous changes using the backup map 'OriginalEdgeSettings'

	// then clean the edges only if they are no longer affected

	// next apply new changes to enclosed edges. backup original settings into the 'OriginalEdgeSettings' map

	// for each evacuee, find the edge that the evacuee is likely to be their based on the time of this event

	// Now we have to figure out if any evacuee is enclosed and if so is it going to be stuck or moving

	// now it's time to move all evacuees along their paths based on current time of this event and evacuee stuck policy
}
