// ===============================================================================================
// Evacuation Solver: Dynamic classes definition
// Description: Contains definition for dynamic CASPER solver
//
// Copyright (C) 2015 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#pragma once

#include "StdAfx.h"
#include "utils.h"
#include "Evacuee.h"
#include "NAEdge.h"

struct DynamicChange
{
	EdgeDirection DisasterDirection;
	IPolygonPtr MyGeometry;
	double      StartTime;
	double      EndTime;
	double      AffectedCostPercentage;
	double      AffectedCapacityPercentage;
	bool        EvacueesAreStuck;
	bool        Applied;

	std::vector<EvacueePtr> EnclosedEvacuees;
	std::vector<NAEdgePtr>  EnclosedEdges;

	DynamicChange(EdgeDirection disasterDirection, IPolygonPtr myGeometry, double startTime, double endTime, double affectedCostPercentage, double affectedCapacityPercentage, bool evacueesAreStuck) :
		DisasterDirection(disasterDirection), MyGeometry(myGeometry), StartTime(startTime), EndTime(endTime), AffectedCostPercentage(affectedCostPercentage),
		AffectedCapacityPercentage(affectedCapacityPercentage), EvacueesAreStuck(evacueesAreStuck), Applied(false) { }

	DynamicChange() : DisasterDirection(EdgeDirection::None), MyGeometry(nullptr), StartTime(0.0), EndTime(-1.0), AffectedCostPercentage(0.0),
		AffectedCapacityPercentage(0.0), EvacueesAreStuck(true), Applied(false) { }
};

class DynamicDisaster
{
private:
	std::vector<DynamicChange> allChanges;
	double currentTime;
	DynamicMode myDynamicMode;

	void LoadAffectedGraph(DynamicChange & item, std::shared_ptr<EvacueeList> Evacuees, std::shared_ptr<NAEdgeCache> ecache);

public:
	DynamicDisaster(IFeatureClassPtr DynamicChangesTable, std::shared_ptr<EvacueeList> Evacuees, std::shared_ptr<NAEdgeCache> ecache, DynamicMode dynamicMode);
	virtual ~DynamicDisaster() { }
};
