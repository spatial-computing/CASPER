#pragma once

#include "StdAfx.h"

[ export, uuid("096CB996-9144-4CC3-BB69-FCFAA5C273FC") ] enum EvcSolverMethod : unsigned char { SPSolver = 0x0, CCRPSolver = 0x1, CASPERSolver = 0x2 };
[ export, uuid("BFDD2DB3-DA25-42CA-8021-F67BF7D14948") ] enum EvcTrafficModel : unsigned char { FLATModel = 0x0, STEPModel = 0x1, LINEARModel = 0x2, POWERModel = 0x3, EXPModel = 0x4 };

// Hash map for cached calculated congestions
typedef public stdext::hash_map<double, double> FlowCongestionMap;
typedef stdext::hash_map<double, double>::iterator FlowCongestionMapItr;
typedef std::pair<double, double> FlowCongestionMapPair;

typedef FlowCongestionMap * FlowCongestionMapPtr;
typedef public stdext::hash_map<double, FlowCongestionMapPtr> CapacityFlowsMap;
typedef stdext::hash_map<double, FlowCongestionMapPtr>::iterator CapacityFlowsMapItr;
typedef std::pair<double, FlowCongestionMapPtr> CapacityFlowsMapPair;

class TrafficModel
{
private:
	EvcTrafficModel model;
	double saturationDensPerCap;
	CapacityFlowsMap * myCache;

	double internalGetCongestionPercentage(double capacity, double flow) const;

public:
	double InitDelayCostPerPop;
	double CriticalDensPerCap;

	TrafficModel(EvcTrafficModel Model, double _criticalDensPerCap, double _saturationDensPerCap, double _initDelayCostPerPop);
	~TrafficModel(void);
	double GetCongestionPercentage(double capacity, double flow);
	double LeftCapacityOnEdge(double capacity, double reservedFlow, double originalEdgeCost) const;
};

