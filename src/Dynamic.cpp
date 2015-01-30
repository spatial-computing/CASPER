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

DynamicDisaster::DynamicDisaster(IFeatureClassPtr DynamicChangesTable, std::shared_ptr<EvacueeList> Evacuees, std::shared_ptr<NAEdgeCache> ecache, DynamicMode dynamicMode) :
	myDynamicMode(dynamicMode), currentTime(0.0)
{
	HRESULT hr = S_OK;
	long count, EdgeDirIndex, StartTimeIndex, EndTimeIndex, CostIndex, CapacityIndex, EvcIndex;
	IFeatureCursorPtr ipCursor = nullptr;
	IFeaturePtr ipRow = nullptr;
	VARIANT var;
	esriGeometryType geoType;

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
	if (FAILED(hr = DynamicChangesTable->FeatureCount(nullptr, &count))) goto END_OF_FUNC;
	allChanges.reserve(count);

	while (ipCursor->NextFeature(&ipRow) == S_OK)
	{
		DynamicChange item;
		IGeometryPtr shape;
		
		if (FAILED(hr = ipRow->get_Value(EdgeDirIndex, &var))) goto END_OF_FUNC;
		item.DisasterDirection = EdgeDirection(var.lVal);
		if (FAILED(hr = ipRow->get_Value(StartTimeIndex, &var))) goto END_OF_FUNC;
		item.StartTime = var.dblVal;
		if (FAILED(hr = ipRow->get_Value(EndTimeIndex, &var))) goto END_OF_FUNC;
		item.EndTime = var.dblVal;
		if (FAILED(hr = ipRow->get_Value(CostIndex, &var))) goto END_OF_FUNC;
		item.AffectedCostPercentage = var.dblVal;
		if (FAILED(hr = ipRow->get_Value(CapacityIndex, &var))) goto END_OF_FUNC;
		item.AffectedCapacityPercentage = var.dblVal;
		if (FAILED(hr = ipRow->get_Value(EvcIndex, &var))) goto END_OF_FUNC;
		item.EvacueesAreStuck = var.lVal == 1l;

		if (FAILED(hr = ipRow->get_ShapeCopy(&shape))) goto END_OF_FUNC;
		if (FAILED(hr = shape->get_GeometryType(&geoType))) goto END_OF_FUNC;
		if (geoType != esriGeometryType::esriGeometryPolygon)
		{
			hr = E_POINTER;
			goto END_OF_FUNC;
		}
		item.MyGeometry = shape;
		allChanges.push_back(item);
	}

	for (auto & item : allChanges) LoadAffectedGraph(item, Evacuees, ecache);

END_OF_FUNC:
	if (FAILED(hr))
	{
		allChanges.clear();
		myDynamicMode = DynamicMode::Disabled;
	}
}

void DynamicDisaster::LoadAffectedGraph(DynamicChange & item, std::shared_ptr<EvacueeList> Evacuees, std::shared_ptr<NAEdgeCache> ecache)
{

}
