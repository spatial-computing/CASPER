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

DynamicDisaster::DynamicDisaster(ITablePtr DynamicChangesTable, DynamicMode dynamicMode) :
	myDynamicMode(dynamicMode), currentTime(0.0)
{
	HRESULT hr = S_OK;
	long count, EdgeDirIndex, StartTimeIndex, EndTimeIndex, CostIndex, CapacityIndex, EvcIndex;
	ICursorPtr ipCursor = nullptr;
	IRowESRI * ipRow = nullptr;
	VARIANT var;
	INALocationRangesPtr range = nullptr;
	DynamicChange * item = nullptr;
	long EdgeCount, JunctionCount, EID, JID;
	esriNetworkEdgeDirection dir;
	double fromPosition, toPosition;

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
		item = new DEBUG_NEW_PLACEMENT DynamicChange();
		
		if (FAILED(hr = ipRow->get_Value(EdgeDirIndex, &var))) goto END_OF_FUNC;
		item->DisasterDirection = EdgeDirection(var.lVal);
		if (FAILED(hr = ipRow->get_Value(StartTimeIndex, &var))) goto END_OF_FUNC;
		item->StartTime = var.dblVal;
		if (FAILED(hr = ipRow->get_Value(EndTimeIndex, &var))) goto END_OF_FUNC;
		item->EndTime = var.dblVal;
		if (FAILED(hr = ipRow->get_Value(CostIndex, &var))) goto END_OF_FUNC;
		item->AffectedCostPercentage = var.dblVal;
		if (FAILED(hr = ipRow->get_Value(CapacityIndex, &var))) goto END_OF_FUNC;
		item->AffectedCapacityPercentage = var.dblVal;
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
