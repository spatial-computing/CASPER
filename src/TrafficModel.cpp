// ===============================================================================================
// Evacuation Solver: Traffic Model Implementation
// Description: Implementation of all traffic models
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#include "StdAfx.h"
#include "TrafficModel.h"

TrafficModel::TrafficModel(EvcTrafficModel Model, double _criticalDensPerCap, double _saturationDensPerCap, double _initDelayCostPerPop)
	: model(Model), CriticalDensPerCap(_criticalDensPerCap), saturationDensPerCap(_saturationDensPerCap), InitDelayCostPerPop(_initDelayCostPerPop)
{
	if (saturationDensPerCap <= CriticalDensPerCap) saturationDensPerCap += CriticalDensPerCap;
	myCache.max_load_factor(0.3);
	cacheHit = 0;
	cacheMiss = 0;
}

double TrafficModel::LeftCapacityOnEdge(double capacity, double reservedFlow, double originalEdgeCost) const
{
	double newPop = 0.0;
	if (model == EvcTrafficModel::STEPModel) newPop = (CriticalDensPerCap * capacity) - reservedFlow;
	else newPop = (CriticalDensPerCap * capacity) - (saturationDensPerCap * capacity);
	if ((InitDelayCostPerPop > 0.0) && (newPop > originalEdgeCost / InitDelayCostPerPop)) newPop = 2147483647.0; // max_int
	newPop = max(newPop, 0.0);
	return newPop;
}

double TrafficModel::GetCongestionPercentage(double capacity, double flow)
{
	double percentage = 1.0;
	if (flow > CriticalDensPerCap * capacity)
	{
		const auto i = myCache.find(TrafficModelCacheNode(capacity, flow));
		if (i != myCache.end())
		{
			percentage = i->second;
			++cacheHit;
		}
		else
		{
			percentage = internalGetCongestionPercentage(capacity, flow);
			myCache.insert(std::pair<TrafficModelCacheNode, double>(TrafficModelCacheNode(capacity, flow), percentage));
			++cacheMiss;
		}
	}
	else
	{
		++cacheHit;
	}
	return percentage;
}

// This is where the actual capacity aware part is happening:
// We take the original values of the edge and recalculate the
// new travel cost based on number of reserved spots by previous evacuees.
double TrafficModel::internalGetCongestionPercentage(double capacity, double flow) const
{
	double a, b, modelRatio, expGamma, speedPercent = 1.0;

	switch (model)
	{
	case EvcTrafficModel::EXPModel:
		/* Exp Model z = exp(-(((flow - 1) / beta) ^ gamma) * log(2))
			a = flow of normal speed, 	b = flow where the speed is dropped to half
			(modelRatio) beta  = b * lane - 1;
			(expGamma)   gamma = (log(log(0.96) / log(0.5))) / log((a * lane - 1) / (b * lane - 1));
		*/
		a = 2.0;
		b = max(a + 1.0, saturationDensPerCap);
		modelRatio  = b * capacity - 1.0;
		expGamma    = (log(log(0.9) / log(0.5))) / log((a * capacity - 1.0) / (b * capacity - 1.0));
		speedPercent = exp(-pow(((flow - 1.0) / modelRatio), expGamma) * log(2.0));
		break;
	case EvcTrafficModel::POWERModel:
		/* Power model z = 1.0 - 0.0202 * sqrt(x) * exp(-0.01127 * y)
			modelRatio = 0.0202 * exp(-0.01127 * reservations->Capacity);
		*/
		modelRatio  = 0.5 / (sqrt(saturationDensPerCap) * exp(-0.01127));
		modelRatio *= exp(-0.01127 * capacity);
		speedPercent = 1.0 - modelRatio * sqrt(flow);
		break;
	case EvcTrafficModel::LINEARModel:
		speedPercent = 1.0 - (flow - CriticalDensPerCap * capacity) / (2.0 * (saturationDensPerCap * capacity - CriticalDensPerCap * capacity));
		break;
	case EvcTrafficModel::STEPModel:
		speedPercent = 0.0;
		break;
	}

	return speedPercent;
}