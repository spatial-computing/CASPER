#include "StdAfx.h"
#include "Evacuee.h"
#include "NAVertex.h"
#include "NAEdge.h"

void NAEvacueeVertexTable::InsertReachable_KeepOtherWithVertex(EvacueeList * list, EvacueeList * redundentSortedEvacuees)
{
	std::vector<EvacueePtr> * p = 0;
	std::vector<EvacueePtr>::iterator i;
	std::vector<NAVertexPtr>::iterator v;

	for(i = list->begin(); i != list->end(); i++)
	{
		if ((*i)->Reachable && (*i)->Population > 0.0)
		{
			// reset evacuation prediction
			// (*i)->PredictedCost = FLT_MAX;

			for (v = (*i)->vertices->begin(); v != (*i)->vertices->end(); v++)
			{
				p = Find((*v)->EID);
				if (!p)
				{
					p = new DEBUG_NEW_PLACEMENT std::vector<EvacueePtr>();
					insert(_NAEvacueeVertexTablePair((*v)->EID, p));
				}
				p->push_back(*i);
			}
		}
		else if (!(*i)->vertices->empty())
		{
			redundentSortedEvacuees->push_back(*i);
		}
	}
}

std::vector<EvacueePtr> * NAEvacueeVertexTable::Find(long junctionEID)
{
	std::vector<EvacueePtr> * p = 0;
	NAEvacueeVertexTableItr i = find(junctionEID);
	if (i != end()) p = i->second;
	return p;
}

NAEvacueeVertexTable::~NAEvacueeVertexTable()
{
	for (NAEvacueeVertexTableItr i = begin(); i != end(); i++) delete i->second;
	this->clear();
}

void NAEvacueeVertexTable::Erase(long junctionEID)
{
	std::vector<NAVertexPtr>::iterator vi;
	NAEvacueeVertexTableItr evcItr1, evcItr2 = this->find(junctionEID);

	// this for loop is going to list all discovered evacuees and will
	// remove them from the entire hash_map... not just this one junction point.
	/*
	for(std::vector<EvacueePtr>::iterator i = evcItr2->second->begin(); i != evcItr2->second->end(); i++)
	{
	for (vi = (*i)->vertices->begin(); vi != (*i)->vertices->end(); vi++)
	{
	if ((*vi)->EID == junctionEID) continue;
	evcItr1 = find((*vi)->EID);
	evcItr1->second->erase(std::find(evcItr1->second->begin(), evcItr1->second->end(), *i));
	}
	}
	*/
	delete evcItr2->second;
	erase(junctionEID); 
}

SafeZone::~SafeZone() { delete Vertex; }

SafeZone::SafeZone(INetworkJunctionPtr _junction, NAEdge * _behindEdge, double posAlong, VARIANT cap) : junction(_junction), behindEdge(_behindEdge), positionAlong(posAlong), capacity(0.0)
{
	reservedPop = 0.0;
	Vertex = new DEBUG_NEW_PLACEMENT NAVertex(junction,behindEdge);
	if (cap.vt == VT_R8) capacity = cap.dblVal;
	else if (cap.vt == VT_BSTR) swscanf_s(cap.bstrVal, L"%lf", &capacity);
}

double SafeZone::SafeZoneCost(double population2Route, EvcSolverMethod solverMethod, double costPerDensity)
{
	double cost = 0.0;
	double totalPop = population2Route + reservedPop;
	if (capacity == 0.0) return FLT_MAX;
	if (totalPop > capacity && capacity > 0.0) cost += costPerDensity * ((totalPop / capacity) - 1.0);
	if (behindEdge) cost += behindEdge->GetCost(population2Route, solverMethod) * positionAlong;
	return cost;
}

HRESULT SafeZone::IsRestricted(INetworkForwardStarExPtr ipForwardStar, INetworkForwardStarAdjacenciesPtr ipForwardAdj, INetworkEdgePtr ipTurnCheckEdge, NAEdge * leadingEdge, bool & restricted)
{	
	HRESULT hr = S_OK;
	long adjacentEdgeCount, i, eid;
	double fromPosition, toPosition;
	esriNetworkEdgeDirection dir;
	restricted = false;

	if (behindEdge)
	{
		restricted = true;
		if (FAILED(hr = ipForwardStar->QueryAdjacencies(Vertex->Junction, leadingEdge->NetEdge , 0, ipForwardAdj))) return hr;
		if (FAILED(hr = ipForwardAdj->get_Count(&adjacentEdgeCount))) return hr;
		for (i = 0; i < adjacentEdgeCount; i++)
		{
			if (FAILED(hr = ipForwardAdj->QueryEdge(i, ipTurnCheckEdge, &fromPosition, &toPosition))) return hr;								
			if (FAILED(hr = ipTurnCheckEdge->get_EID(&eid))) return hr;
			if (FAILED(hr = ipTurnCheckEdge->get_Direction(&dir))) return hr;
			if (behindEdge->Direction == dir && behindEdge->EID == eid) restricted = false;
		}
	}
	return hr;
}