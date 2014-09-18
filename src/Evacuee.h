// Copyright 2010 ESRI
// 
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
// 
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
// 
// See the use restrictions at http://help.arcgis.com/en/sdk/10.1/usageRestrictions.htm

#pragma once

#include "StdAfx.h"

class NAVertex;
class NAEdge;
class NAEdgeCache;
typedef NAVertex * NAVertexPtr;
enum EvcSolverMethod : unsigned char;

// enum for carma sort setting
[export, uuid("AAC29CC5-80A9-454A-984B-43525917E53B")] enum CARMASort : unsigned char { None = 0x0, FWSingle = 0x1, FMCont = 0x2, BWSingle = 0x3, BWCont = 0x4 };

class PathSegment
{
private:
	double fromRatio;
	double toRatio;

public:
    NAEdge * Edge;
    IPolylinePtr pline;

    double GetEdgePortion() const { return toRatio - fromRatio; }
	HRESULT GetGeometry(INetworkDatasetPtr ipNetworkDataset, IFeatureClassContainerPtr ipFeatureClassContainer, bool & sourceNotFoundFlag, IGeometryPtr & geometry);

    void SetFromRatio(double FromRatio)
    {
	    fromRatio = FromRatio;
	    if (fromRatio == toRatio) fromRatio = toRatio - 0.001;
    }

	double GetFromRatio() const { return fromRatio; }
	double GetToRatio()   const { return toRatio;   }

    PathSegment(NAEdge * edge, double FromRatio = 0.0, double ToRatio = 1.0)
    {
	    fromRatio = FromRatio;
	    toRatio = ToRatio;
		_ASSERT(FromRatio < ToRatio);
	    Edge = edge;
	    pline = 0;
    }
};

typedef PathSegment * PathSegmentPtr;
class Evacuee;

class EvcPath : private std::list<PathSegmentPtr>
{
private:
	double  RoutedPop;
	double  EvacuationCost;
	double  OrginalCost;
	int     Order;
	Evacuee * myEvc;

public:
	typedef std::list<PathSegmentPtr>::const_iterator  const_iterator;
	typedef std::list<PathSegmentPtr>::const_reference const_reference;

	const_reference Front()           const { return this->front();  }
	const_iterator Begin()            const { return this->begin();  }
	const_iterator End()              const { return this->end();    }
	inline double GetRoutedPop()      const { return RoutedPop;      }
	inline double GetEvacuationCost() const { return EvacuationCost; }

	EvcPath(double routedPop, int order, Evacuee * evc) : std::list<PathSegmentPtr>()
	{
		RoutedPop = routedPop; 
		EvacuationCost = 0.0;
		OrginalCost = 0.0;
		Order = order;
		myEvc = evc;
	}

	void AddSegment(double population2Route, EvcSolverMethod method, PathSegmentPtr segment);
	HRESULT AddPathToFeatureBuffers(ITrackCancel * , INetworkDatasetPtr , IFeatureClassContainerPtr , bool & , IStepProgressorPtr , double & , double , IFeatureBufferPtr , IFeatureBufferPtr ,
									IFeatureCursorPtr , IFeatureCursorPtr , long , long , long , long ,	long , long , long , long , long , long , double &, bool);

	bool           Empty() const { return std::list<PathSegmentPtr>::empty(); }
	PathSegmentPtr Front()       { return std::list<PathSegmentPtr>::front(); }
	PathSegmentPtr Back()        { return std::list<PathSegmentPtr>::back();  }

	~EvcPath(void)
	{
		for(const_iterator it = begin(); it != end(); it++) delete (*it);
		clear();
	}

	static bool LessThan(EvcPath * p1, EvcPath * p2)
	{
		return p1->Order < p2->Order;
	}
};

typedef EvcPath * EvcPathPtr;

class Evacuee
{
public:
	std::vector<NAVertexPtr> * vertices;
	std::list<EvcPathPtr> * paths;
	VARIANT Name;
	double Population;
	double PredictedCost;
	bool Reachable;
	UINT32 ObjectID;

	Evacuee(VARIANT name, double pop, UINT32 objectID)
	{
		ObjectID = objectID;
		Name = name;
		vertices = new DEBUG_NEW_PLACEMENT std::vector<NAVertexPtr>();
		paths = new DEBUG_NEW_PLACEMENT std::list<EvcPathPtr>();
		Population = pop;
		PredictedCost = FLT_MAX;
		Reachable = true;
	}

	~Evacuee(void)
	{
		for(std::list<EvcPathPtr>::iterator it = paths->begin(); it != paths->end(); it++) delete (*it);
		paths->clear();
		vertices->clear();
		delete vertices;
		delete paths;
	}

	static bool LessThan(Evacuee * e1, Evacuee * e2)
	{
		if (e1->PredictedCost == e2->PredictedCost) return e1->Population < e2->Population;
		else return e1->PredictedCost < e2->PredictedCost;
	}

	static bool LessThanObjectID(Evacuee * e1, Evacuee * e2)
	{
		return e1->ObjectID < e2->ObjectID;
	}
};

typedef Evacuee * EvacueePtr;
typedef std::vector<EvacueePtr> EvacueeList;
typedef std::vector<EvacueePtr>::iterator EvacueeListItr;
typedef std::pair<long, std::vector<EvacueePtr> *> _NAEvacueeVertexTablePair;
typedef stdext::hash_map<long, std::vector<EvacueePtr> *>::iterator NAEvacueeVertexTableItr;

class NAEvacueeVertexTable : public stdext::hash_map<long, std::vector<EvacueePtr> *>
{
public:
	~NAEvacueeVertexTable();

	void InsertReachable_KeepOtherWithVertex(EvacueeList * list, EvacueeList * redundentSortedEvacuees, CARMASort sortDir);
	std::vector<EvacueePtr> * Find(long junctionEID);
	void Erase(long junctionEID);
};

class SafeZone
{
private:
	INetworkJunctionPtr junction;
	NAEdge * behindEdge;
	double   positionAlong;
	double   capacity;
	double   reservedPop;

public:
	NAVertex * Vertex;

	inline void   Reserve(double pop)      { reservedPop += pop;   }
	inline double getPositionAlong() const { return positionAlong; }
	inline NAEdge * getBehindEdge()        { return behindEdge;    }
	
	~SafeZone();
	SafeZone(INetworkJunctionPtr _junction, NAEdge * _behindEdge, double posAlong, VARIANT cap);
	HRESULT IsRestricted(NAEdgeCache * ecache, NAEdge * leadingEdge, bool & restricted, double costPerDensity);
	double SafeZoneCost(double population2Route, EvcSolverMethod solverMethod, double costPerDensity, double * globalDeltaCost = NULL);
};

typedef SafeZone * SafeZonePtr;
typedef stdext::hash_map<long, SafeZonePtr> SafeZoneTable;
typedef stdext::hash_map<long, SafeZonePtr>::_Pairib SafeZoneTableInsertReturn;
typedef stdext::hash_map<long, SafeZonePtr>::iterator SafeZoneTableItr;
typedef std::pair<long, SafeZonePtr> _SafeZoneTablePair;
#define SafeZoneTablePair(a) _SafeZoneTablePair(a->Vertex->EID, a)
