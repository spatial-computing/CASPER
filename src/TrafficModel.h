// ===============================================================================================
// Evacuation Solver: Traffic model class definition
// Description:
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#pragma once

#include "StdAfx.h"
#include "utils.h"

struct TrafficModelCacheNode
{
public:
	double Capacity;
	double Flow;
	TrafficModelCacheNode(double capacity, double flow) : Capacity(capacity), Flow(flow) { }

	inline bool friend operator<(const TrafficModelCacheNode& lhs, const TrafficModelCacheNode& rhs)
	{
		if (lhs.Capacity == rhs.Capacity) return lhs.Flow < rhs.Flow;
		else return lhs.Capacity < rhs.Capacity;
	}

	inline bool friend operator==(const TrafficModelCacheNode& lhs, const TrafficModelCacheNode& rhs)
	{
		if (lhs.Capacity == rhs.Capacity) return lhs.Flow == rhs.Flow;
		else return lhs.Capacity == rhs.Capacity;
	}

	inline bool friend operator!=(const TrafficModelCacheNode& lhs, const TrafficModelCacheNode& rhs){ return !(lhs == rhs); }
	inline bool friend operator> (const TrafficModelCacheNode& lhs, const TrafficModelCacheNode& rhs){ return rhs < lhs;     }
	inline bool friend operator<=(const TrafficModelCacheNode& lhs, const TrafficModelCacheNode& rhs){ return !(lhs > rhs);  }
	inline bool friend operator>=(const TrafficModelCacheNode& lhs, const TrafficModelCacheNode& rhs){ return !(lhs < rhs);  }

	struct Hasher : public std::hash<double>
	{
		size_t operator()(const TrafficModelCacheNode & e) const
		{
			return std::hash<double>::operator()(e.Capacity * 100.0 * e.Flow);
		}
	};
};

class TrafficModel
{
private:
	EvcTrafficModel model;
	double saturationDensPerCap;
	std::unordered_map<TrafficModelCacheNode, double, TrafficModelCacheNode::Hasher> myCache;
	unsigned int cacheMiss;
	unsigned int cacheHit;

	double internalGetCongestionPercentage(double capacity, double flow) const;

public:
	double InitDelayCostPerPop;
	double CriticalDensPerCap;

	TrafficModel(EvcTrafficModel Model, double _criticalDensPerCap, double _saturationDensPerCap, double _initDelayCostPerPop);
	virtual ~TrafficModel(void) { }
	TrafficModel(const TrafficModel & that) = delete;
	TrafficModel & operator=(const TrafficModel &) = delete;
	double GetCongestionPercentage(double capacity, double flow);
	double LeftCapacityOnEdge(double capacity, double reservedFlow, double originalEdgeCost) const;
	double GetCacheHitPercentage() const { return 100.0 * cacheHit / (cacheHit + cacheMiss); }
};

