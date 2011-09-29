#include "StdAfx.h"
#include "NAGraph.h"
#include "Evacuee.h"
#include <math.h>

///////////////////////////////////////////////////////////////////////
// NAEdge Methods

NAEdge::NAEdge(const NAEdge& cpy)
{
	reservations = new std::vector<EdgeReservation>(*(cpy.reservations));
	NetEdge = cpy.NetEdge;	
	ReservedPop = cpy.ReservedPop;
	originalCost = cpy.originalCost;	
	criticalDens = cpy.criticalDens;
	saturationDens = cpy.saturationDens;
	Direction = cpy.Direction;
	EID = cpy.EID;
	ToVertex = cpy.ToVertex;
}

NAEdge::NAEdge(INetworkEdgePtr edge, long capacityAttribID, long costAttribID, double CriticalDensPerCap,
	double SaturationDensPerCap)
{
	reservations = new std::vector<EdgeReservation>();
	this->NetEdge = edge;
	VARIANT vcost, vcap;
	ReservedPop = 0.0;
	// cachedCost = originalCost;

	if (FAILED(edge->get_AttributeValue(capacityAttribID, &vcap)) ||	
		FAILED(edge->get_AttributeValue(costAttribID, &vcost)) ||	
		FAILED(edge->get_EID(&EID)) ||	
		FAILED(edge->get_Direction(&Direction)))
	{
		NetEdge = 0;
		criticalDens = -1;
		saturationDens = -1;
		originalCost = -1;
		EID = -1;
		// Direction = 0;
	}
	else
	{
		originalCost = max(0.0, vcost.dblVal);
		double Capacity = 1.0;
		if (vcap.vt == VT_R8) Capacity = max(1.0, vcap.dblVal);
		else if (vcap.vt == VT_I4) Capacity = max(1, vcap.intVal);
		criticalDens = CriticalDensPerCap * Capacity;
		saturationDens = SaturationDensPerCap * Capacity;
	}
}

HRESULT NAEdge::QuerySourceStuff(long * sourceOID, long * sourceID, double * fromPosition, double * toPosition) const
{
	HRESULT hr;
	if (FAILED(hr = NetEdge->get_OID(sourceOID))) return hr;
	if (FAILED(hr = NetEdge->get_SourceID(sourceID))) return hr;
	if (FAILED(hr = NetEdge->QueryPositions(fromPosition, toPosition))) return hr;
	return S_OK;
}

// Special function for CCRP: to check how much capacity id left on this edge.
// Will be used to get max capacity available on a path
double NAEdge::CapacityLeft() const
{
	return criticalDens - ReservedPop;
}

// This is where the actual capacity aware part is happening:
// We take the original values of the edge and recalculate the
// new travel cost based on number of reserved spots by previous evacuees.
double NAEdge::GetCost(double newPop, char method) const
{
	double newCost = originalCost, speedPercent = 1.0;
	switch (method)
	{
	case 0x2: // casper method
		newPop = newPop + ReservedPop - criticalDens;
		if (newPop > 0)
		{
			speedPercent = exp(-newPop / saturationDens);
			speedPercent = min(1.0, max(0.001, speedPercent));
			newCost = originalCost / speedPercent;
		}
		break;
	case 0x1: // CCRP method
		newPop = newPop + ReservedPop - criticalDens;
		if (newPop > 0) newCost *= 1000.0;		
		break;
	}
	return newCost;
}

void NAEdge::AddReservation(Evacuee * evacuee, double fromCost, double toCost, double population)
{
	reservations->insert(reservations->end(), EdgeReservation(evacuee, fromCost, toCost));
	ReservedPop += population;
}

/////////////////////////////////////////////////////////////
// NAEdgeCache

// Creates a new edge pointer based on the given NetworkEdge. If one exist in the cache, it will be sent out.
/*
NAEdgePtr NAEdgeCache::New(INetworkEdgePtr edge, bool replace)
{
	NAEdgePtr n = 0;
	long EID;
	NAEdgeTable * cache = 0;
	esriNetworkEdgeDirection dir;

	if (FAILED(edge->get_EID(&EID))) return 0;
	if (FAILED(edge->get_Direction(&dir))) return 0;

	if (dir == esriNEDAlongDigitized) cache = cacheAlong;
	else cache = cacheAgainst;

	NAEdgeTableItr it = cache->find(EID);

	if (it == cache->end())
	{
		n = new NAEdge(edge, capacityAttribID, costAttribID, criticalDensPerCap, saturationPerCap);
		cache->insert(NAEdgeTablePair(n));
	}
	else
	{
		n = new NAEdge(*(it->second));
		sideCache->insert(sideCache->end(), n);
	}
	return n;
}
*/
NAEdgePtr NAEdgeCache::New(INetworkEdgePtr edge, bool replace)
{
	NAEdgePtr n = 0;
	long EID;
	NAEdgeTable * cache = 0;
	esriNetworkEdgeDirection dir;

	if (FAILED(edge->get_EID(&EID))) return 0;
	if (FAILED(edge->get_Direction(&dir))) return 0;

	if (dir == esriNEDAlongDigitized) cache = cacheAlong;
	else cache = cacheAgainst;

	NAEdgeTableItr it = cache->find(EID);	

	if (it == cache->end())
	{
		n = new NAEdge(edge, capacityAttribID, costAttribID, criticalDensPerCap, saturationPerCap);
		cache->insert(NAEdgeTablePair(n));
	}
	else
	{
		if (replace)
		{
			delete it->second;
			it->second = new NAEdge(edge, capacityAttribID, costAttribID, criticalDensPerCap, saturationPerCap);
		}
		else it->second->NetEdge = edge;
		n = it->second;
	}
	return n;
}

void NAEdgeCache::Clear()
{
	for(NAEdgeTableItr cit = cacheAlong->begin(); cit != cacheAlong->end(); cit++) delete (*cit).second;
	cacheAlong->clear();
	for(NAEdgeTableItr cit = cacheAgainst->begin(); cit != cacheAgainst->end(); cit++) delete (*cit).second;
	cacheAgainst->clear();
	for(std::vector<NAEdgePtr>::iterator i = sideCache->begin(); i != sideCache->end(); i++) delete (*i);
	sideCache->clear();
}

///////////////////////////////////////////////
// NAVertex methods

NAVertex::NAVertex(const NAVertex& cpy)
{
	g = cpy.g;
	h = cpy.h;
	Junction = cpy.Junction;
	BehindEdge = cpy.BehindEdge;
	Previous = cpy.Previous;
	EID = cpy.EID;
}

NAVertex::NAVertex(void)
{
	EID = -1;
	Junction = 0;
	BehindEdge = 0;
	Previous = 0;
	g = 0;
	h = -1;
}

NAVertex::NAVertex(INetworkJunctionPtr junction, NAEdge * behindEdge)
{
	Previous = 0;
	g = 0;
	h = MAX_COST;
	BehindEdge = behindEdge;

	if (!FAILED(junction->get_EID(&EID)))
	{		
		Junction = junction;
	}
	else
	{
		EID = -1;
		Junction = 0;
	}
}

void NAVertex::SetBehindEdge(NAEdge * behindEdge)
{
	BehindEdge = behindEdge;
	BehindEdge->ToVertex = this;
}

void NAVertexCache::UdateHeuristic(NAVertexPtr vertex)
{
	NAVertexPtr a = Get(vertex->EID);
	a->h = min(a->h, vertex->h);
}

NAVertexPtr NAVertexCache::New(INetworkJunctionPtr junction)
{
	NAVertexPtr n = 0;
	long JunctionEID;
	if (FAILED(junction->get_EID(&JunctionEID))) return 0;
	NAVertexTableItr it = cache->find(JunctionEID);

	if (it == cache->end())
	{
		n = new NAVertex(junction, 0);
		cache->insert(NAVertexTablePair(n));
	}
	else
	{
		n = new NAVertex(*(it->second));
		sideCache->insert(sideCache->end(), n);
	}
	return n;
}

NAVertexPtr NAVertexCache::Get(long eid)
{
	NAVertexPtr n = 0;
	NAVertexTableItr it = cache->find(eid);
	if (it != cache->end()) n = it->second;
	return n;
}

void NAVertexCache::Clear()
{
	for(NAVertexTableItr cit = cache->begin(); cit != cache->end(); cit++) delete cit->second;
	cache->clear();
	for(std::vector<NAVertexPtr>::iterator i = sideCache->begin(); i != sideCache->end(); i++) delete (*i);
	sideCache->clear();
}

NAVertexPtr NAVertexCollector::New(INetworkJunctionPtr junction)
{
	NAVertexPtr n = new NAVertex(junction, 0);
	cache->insert(cache->end(), n);
	return n;
}

void NAVertexCollector::Clear()
{
	for(std::vector<NAVertexPtr>::iterator cit = cache->begin(); cit != cache->end(); cit++) delete (*cit);
	cache->clear();
}

//////////////////////////////////////////////////////////////////
//// NAEdgeClosed Methods

bool NAEdgeClosed::IsClosed(NAEdgePtr edge)
{
	NAEdgePtr n = 0;
	NAEdgeTable * cache = 0;

	if (edge->Direction == esriNEDAlongDigitized) cache = cacheAlong;
	else cache = cacheAgainst;

	return cache->find(edge->EID) != cache->end();
}

HRESULT NAEdgeClosed::Insert(NAEdgePtr edge)
{
	NAEdgePtr n = 0;
	NAEdgeTable * cache = 0;

	if (edge->Direction == esriNEDAlongDigitized) cache = cacheAlong;
	else cache = cacheAgainst;

	if (cache->find(edge->EID) != cache->end()) return E_FAIL;
	cache->insert(NAEdgeTablePair(edge));

	return S_OK;
}

//////////////////////////////////////////////////////////////////
//// NAEdgeContainer Methods

bool NAEdgeContainer::Exist(INetworkEdgePtr edge)
{
	esriNetworkEdgeDirection dir;
	long eid;
	stdext::hash_map<long, char> * cache = 0;

	if (FAILED(edge->get_EID(&eid))) return false;
	if (FAILED(edge->get_Direction(&dir))) return false;

	if (dir == esriNEDAlongDigitized) cache = cacheAlong;
	else cache = cacheAgainst;

	return cache->find(eid) != cache->end();
}

HRESULT NAEdgeContainer::Insert(INetworkEdgePtr edge)
{
	esriNetworkEdgeDirection dir;
	long eid;
	HRESULT hr;

	if (FAILED(hr = edge->get_EID(&eid))) return hr;
	if (FAILED(hr = edge->get_Direction(&dir))) return hr;

	if (dir == esriNEDAlongDigitized) cacheAlong->insert(NAEdgeContainerPair(eid));
	else  cacheAgainst->insert(NAEdgeContainerPair(eid));
	return S_OK;
}
