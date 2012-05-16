// Copyright 2010 ESRI
// 
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
// 
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
// 
// See the use restrictions at http://help.arcgis.com/en/sdk/10.0/usageRestrictions.htm

#pragma once

#include <vector>
#include <stack>
#include <hash_map>

class NAVertex;
class NAEdge;
typedef NAVertex * NAVertexPtr;

class PathSegment
{
public:
  double fromPosition;
  double toPosition;
  long SourceOID;
  long SourceID;
  NAEdge * Edge;
  double EdgePortion;
  IPolylinePtr pline;

  PathSegment(double from, double to, long sourceOID, long sourceID, NAEdge * edge, double edgePortion)
  {
	  fromPosition = from;
	  toPosition = to;
	  SourceOID = sourceOID;
	  SourceID = sourceID;
	  Edge = edge;
	  EdgePortion = edgePortion;
	  pline = 0;
  }
};

typedef PathSegment * PathSegmentPtr;

class EvcPath : public std::list<PathSegmentPtr>
{
public:
	double RoutedPop;
	double EvacuationCost;
	double OrginalCost;

	EvcPath(double routedPop) : std::list<PathSegmentPtr>()
	{
		RoutedPop = routedPop; 
		EvacuationCost = -1.0;
		OrginalCost = -1.0;
	}

	~EvcPath(void)
	{
		for(iterator it = begin(); it != end(); it++) delete (*it);
		clear();
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

	Evacuee(VARIANT name, double pop)
	{
		Name = name;
		vertices = new std::vector<NAVertexPtr>();
		paths = new std::list<EvcPathPtr>();
		Population = pop;
	}

	~Evacuee(void)
	{
		for(std::list<EvcPathPtr>::iterator it = paths->begin(); it != paths->end(); it++) delete (*it);
		paths->clear();
		vertices->clear();
		delete vertices;
		delete paths;
	}
};

typedef Evacuee * EvacueePtr;
typedef std::vector<EvacueePtr> EvacueeList;
typedef std::vector<EvacueePtr>::iterator EvacueeListItr;
typedef std::pair<long, std::vector<EvacueePtr> *> _NAEvacueeVertexTablePair;
typedef stdext::hash_map<long, std::vector<EvacueePtr> *>::iterator NAEvacueeVertexTableItr;

class NAEvacueeVertexTable : private stdext::hash_map<long, std::vector<EvacueePtr> *>
{
public:
	~NAEvacueeVertexTable();

	void Insert(EvacueeList * list);
	std::vector<EvacueePtr> * Find(long junctionEID);
	void Erase(long junctionEID);
	bool IsEmpty() const { return this->empty(); }
};
