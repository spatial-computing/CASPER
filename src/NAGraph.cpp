#include "StdAfx.h"
#include "NAGraph.h"
#include "Evacuee.h"
#include <cmath>
#include <algorithm>

///////////////////////////////////////////////////////////////////////
// EdgeReservations Methods

EdgeReservations::EdgeReservations(float capacity, float criticalDensPerCap, float saturationDensPerCap, float InitDelayCostPerPop)
{
	//List = new DEBUG_NEW_PLACEMENT std::vector<EdgeReservation>();
	ReservedPop = 0.0;
	Capacity = capacity;	
	CriticalDens = criticalDensPerCap * capacity;
	SaturationDensPerCap = saturationDensPerCap;
	DirtyFlag = false;
	initDelayCostPerPop = InitDelayCostPerPop;
}

EdgeReservations::EdgeReservations(const EdgeReservations& cpy)
{
	//List = new DEBUG_NEW_PLACEMENT std::vector<EdgeReservation>(*(cpy.List));
	ReservedPop = cpy.ReservedPop;
	Capacity = cpy.Capacity;	
	CriticalDens = cpy.CriticalDens;
	SaturationDensPerCap = cpy.SaturationDensPerCap;
	DirtyFlag = cpy.DirtyFlag;
	initDelayCostPerPop = cpy.initDelayCostPerPop;
}

///////////////////////////////////////////////////////////////////////
// NAEdge Methods

NAEdge::NAEdge(const NAEdge& cpy)
{
	reservations = cpy.reservations;
	NetEdge = cpy.NetEdge;
	OriginalCost = cpy.OriginalCost;
	Direction = cpy.Direction;
	EID = cpy.EID;
	ToVertex = cpy.ToVertex;
	LastExteriorEdge = cpy.LastExteriorEdge;
	trafficModel = cpy.trafficModel;
	CASPERRatio = cpy.CASPERRatio;
	CleanCost = cpy.CleanCost;
	cachedCost[0] = cpy.cachedCost[0]; cachedCost[1] = cpy.cachedCost[1];
	// calcSaved = cpy.calcSaved;
}

NAEdge::NAEdge(INetworkEdgePtr edge, long capacityAttribID, long costAttribID, float CriticalDensPerCap, float SaturationDensPerCap, NAResTable * resTable, float InitDelayCostPerPop, EVC_TRAFFIC_MODEL TrafficModel)
{
	// calcSaved = 0;
	CleanCost = -1.0;
	ToVertex = 0;
	trafficModel = TrafficModel;
	this->NetEdge = edge;
	LastExteriorEdge = 0;
	VARIANT vcost, vcap;
	float capacity = 1.0;
	cachedCost[0] = FLT_MAX; cachedCost[1] = 0.0;
	HRESULT hr = 0;

	if (FAILED(hr = edge->get_AttributeValue(capacityAttribID, &vcap)) ||	
		FAILED(hr = edge->get_AttributeValue(costAttribID, &vcost)) ||	
		FAILED(hr = edge->get_EID(&EID)) ||	
		FAILED(hr = edge->get_Direction(&Direction)))
	{
		_ASSERT(hr == 0);
		NetEdge = 0;
		reservations = 0;
		EID = -1;
		throw std::logic_error("Something bad happened while looking up a network edge");
	}
	else
	{
		_ASSERT(vcost.dblVal >= 0.0);
		OriginalCost = max(0.0, vcost.dblVal);
		if (vcap.vt == VT_R8) capacity = max(1.0f, (float)(vcap.dblVal));
		else if (vcap.vt == VT_I4) capacity = max(1.0f, (float)(vcap.intVal));

		NAResTableItr it = resTable->find(EID);
		if (it == resTable->end())
		{
			reservations = new DEBUG_NEW_PLACEMENT EdgeReservations(capacity, CriticalDensPerCap, SaturationDensPerCap, InitDelayCostPerPop);
			resTable->insert(NAResTablePair(EID, reservations));
		}
		else
		{
			reservations = it->second;
		}
		
		// z = 1.0 - 0.0202 * sqrt(x) * exp(-0.01127 * y)
		// CASPERRatio = 0.0202 * exp(-0.01127 * reservations->Capacity);
		CASPERRatio  = 0.5 / (sqrt(reservations->SaturationDensPerCap) * exp(-0.01127));
		CASPERRatio *= exp(-0.01127 * reservations->Capacity);
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

// Special function for CCRP: to check how much capacity is left on this edge.
// Will be used to get max capacity available on a path
double NAEdge::LeftCapacity() const
{
	double newPop = reservations->CriticalDens - reservations->ReservedPop;
	if ((reservations->initDelayCostPerPop > 0.0) && (newPop > OriginalCost / reservations->initDelayCostPerPop)) newPop = 2147483647.0; // max_int	
	newPop = max(newPop, 0.0);
	return newPop;
}

// This is where the actual capacity aware part is happening:
// We take the original values of the edge and recalculate the
// new travel cost based on number of reserved spots by previous evacuees.
#pragma float_control(precise, off, push)
double NAEdge::GetTrafficSpeedRatio(double allPop) const
{
	if (cachedCost[0] == allPop) { /*calcSaved++;*/ return cachedCost[1]; }
	double speedPercent = 1.0;
	switch (trafficModel)
	{
	case EVC_TRAFFIC_MODEL_CASPER:
		// z = 1.0 - 0.0202 * sqrt(x) * exp(-0.01127 * y)
		speedPercent = 1.0 - CASPERRatio * sqrt(allPop);
		break;
	case EVC_TRAFFIC_MODEL_LINEAR:
		speedPercent = 1.0 - (allPop - reservations->CriticalDens) / (2.0 * (reservations->SaturationDensPerCap * reservations->Capacity - reservations->CriticalDens));
		break;
	case EVC_TRAFFIC_MODEL_STEP:
		speedPercent = 0.0;
		break;
	}
	cachedCost[0] = allPop; cachedCost[1] = speedPercent;
	return speedPercent;
}
#pragma float_control(pop)

double NAEdge::GetCurrentCost() const
{
	double speedPercent = 1.0;
	if (reservations->ReservedPop > reservations->CriticalDens)
	{
		speedPercent = GetTrafficSpeedRatio(reservations->ReservedPop);
		speedPercent = min(1.0, max(0.0001, speedPercent));
	}
	return OriginalCost / speedPercent;
}

double NAEdge::GetCost(double newPop, EVC_SOLVER_METHOD method) const
{
	double speedPercent = 1.0;	
	if (reservations->initDelayCostPerPop > 0.0) newPop = min(newPop, OriginalCost / reservations->initDelayCostPerPop);
	newPop += reservations->ReservedPop;

	if (newPop > reservations->CriticalDens)
	{			
		switch (method)
		{
		case EVC_SOLVER_METHOD_CASPER:
			speedPercent = GetTrafficSpeedRatio(newPop);
			break;
		case EVC_SOLVER_METHOD_CCRP:
			speedPercent = 0.0;
			break;
		}
		speedPercent = min(1.0, max(0.0001, speedPercent));
	}
	return OriginalCost / speedPercent;
}

// this function addes the resrvation also determines if the new added population makes the edge dirty.
// if this new reservation made the edge change from clean to dirty then the return is true otherwise returns false.
bool NAEdge::AddReservation(/* Evacuee * evacuee, double fromCost, double toCost, */ double population)
{
	if (CleanCost < 0.0) CleanCost = this->GetCurrentCost();	
	
	// actual reservation code
	// reservations->List->insert(reservations->List->end(), EdgeReservation(evacuee, fromCost, toCost));
	float newPop = (float)population;
	if (reservations->initDelayCostPerPop > 0.0f) newPop = min(newPop, (float)(OriginalCost / reservations->initDelayCostPerPop));
	reservations->ReservedPop += newPop;

	bool ret = (!reservations->DirtyFlag) && (CleanCost / this->GetCurrentCost() < 0.9);	
	reservations->DirtyFlag |= ret;
	return ret;
}

/////////////////////////////////////////////////////////////
// NAEdgeCache

// Creates a new edge pointer based on the given NetworkEdge. If one exist in the cache, it will be sent out.
NAEdgePtr NAEdgeCache::New(INetworkEdgePtr edge, bool replace)
{
	NAEdgePtr n = 0;
	long EID;
	NAEdgeTable * cache = 0;
	NAResTable  * resTable = 0;
	esriNetworkEdgeDirection dir;

	if (FAILED(edge->get_EID(&EID))) return 0;
	if (FAILED(edge->get_Direction(&dir))) return 0;

	if (dir == esriNEDAlongDigitized)
	{
		cache = cacheAlong;
		resTable = resTableAlong;
	}
	else
	{
		cache = cacheAgainst;
		resTable = resTableAgainst;
	}

	NAEdgeTableItr it = cache->find(EID);	

	if (it == cache->end())
	{
		n = new DEBUG_NEW_PLACEMENT NAEdge(edge, capacityAttribID, costAttribID, criticalDensPerCap, saturationPerCap, resTable, initDelayCostPerPop, trafficModel);
		cache->insert(NAEdgeTablePair(n));
	}
	else
	{
		if (replace)
		{
			delete it->second;
			it->second = new DEBUG_NEW_PLACEMENT NAEdge(edge, capacityAttribID, costAttribID, criticalDensPerCap, saturationPerCap, resTable, initDelayCostPerPop, trafficModel);
		}
		else it->second->NetEdge = edge;
		n = it->second;
	}
	return n;
}

void NAEdgeCache::Clear()
{
	for(NAEdgeTableItr cit = cacheAlong->begin(); cit != cacheAlong->end(); cit++) delete (*cit).second;
	for(NAEdgeTableItr cit = cacheAgainst->begin(); cit != cacheAgainst->end(); cit++) delete (*cit).second;
	for(NAResTableItr ires = resTableAlong->begin(); ires != resTableAlong->end(); ires++) delete (*ires).second;

	cacheAlong->clear();
	cacheAgainst->clear();
	resTableAlong->clear();

	if (!twoWayRoadsShareCap)
	{
		for(NAResTableItr ires = resTableAgainst->begin(); ires != resTableAgainst->end(); ires++) delete (*ires).second;
		resTableAgainst->clear();
	}
}

///////////////////////////////////////////////
// NAVertex methods

NAVertex::NAVertex(const NAVertex& cpy)
{
	g = cpy.g;
	h = new DEBUG_NEW_PLACEMENT std::vector<HValue>(*(cpy.h));
	Junction = cpy.Junction;
	BehindEdge = cpy.BehindEdge;
	Previous = cpy.Previous;
	EID = cpy.EID;
	// posAlong = cpy.posAlong;
}

NAVertex::NAVertex(void)
{
	EID = -1;
	Junction = 0;
	BehindEdge = 0;
	Previous = 0;
	g = 0.0f;
	h = new DEBUG_NEW_PLACEMENT std::vector<HValue>();
	ResetHValues();
	// posAlong = 0.0f;
}

NAVertex::NAVertex(INetworkJunctionPtr junction, NAEdge * behindEdge)
{
	Previous = 0;
	// posAlong = 0.0f;
	g = 0.0f;
	h = new DEBUG_NEW_PLACEMENT std::vector<HValue>();
	ResetHValues();
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

inline void NAVertex::SetBehindEdge(NAEdge * behindEdge) 
{
	BehindEdge = behindEdge;
	if (BehindEdge != NULL) BehindEdge->ToVertex = this;
}

// return true if update was unnesecery
bool NAVertex::UpdateHeuristic(long edgeid, double hur)
{
	bool unnesecery = false;
	for(std::vector<HValue>::iterator i = h->begin(); i != h->end(); i++)
	{
		if (i->EdgeID == edgeid)
		{
			_ASSERT(i->Value <= hur);
			unnesecery = i->Value == hur;
			i->Value = hur;
			if (!unnesecery) std::sort(h->begin(), h->end(), HValue::LessThan);
			return unnesecery;
		}
	}
	h->push_back(HValue(edgeid, hur));
	std::sort(h->begin(), h->end(), HValue::LessThan);
	return unnesecery;	
}

// return true if update was unnesecery
bool NAVertexCache::UpdateHeuristic(long edgeid, NAVertex * n)
{
	NAVertexPtr a = Get(n->EID);
	return a->UpdateHeuristic(edgeid, n->g);
}

void NAVertexCache::UpdateHeuristicForOutsideVertices(double hur, bool goDeep)
{
	if (heuristicForOutsideVertices < hur)
	{
		heuristicForOutsideVertices = hur;
		if (goDeep)
			for(NAVertexTableItr it = cache->begin(); it != cache->end(); it++) it->second->UpdateHeuristic(-1, hur);			
	}
}

NAVertexPtr NAVertexCache::New(INetworkJunctionPtr junction/*, int loopCount */)
{
	NAVertexPtr n = 0;
	long JunctionEID;
	if (FAILED(junction->get_EID(&JunctionEID))) return 0;
	NAVertexTableItr it = cache->find(JunctionEID);

	if (it == cache->end())
	{
		n = new DEBUG_NEW_PLACEMENT NAVertex(junction, 0);
		n->UpdateHeuristic(-1, heuristicForOutsideVertices);
		cache->insert(NAVertexTablePair(n));
	}
	else
	{
		n = new DEBUG_NEW_PLACEMENT NAVertex(*(it->second));
		sideCache->push_back(n);
	}
	/*
	if (loopCount > 0 && n->IsHEmpty())
	{
		// we have found a node without an H value
		if (noHVertices->find(loopCount) == noHVertices->end())
		{
			noHVertices->insert(NAVertexLoopCountListPair(loopCount, new std::list<long>()));
		}
		noHVertices->at(loopCount)->push_back(n->EID);
	}
	*/

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
	CollectAndRelease();
}

void NAVertexCache::CollectAndRelease()
{	
	int count = 0;
	for(std::vector<NAVertexPtr>::iterator i = sideCache->begin(); i != sideCache->end(); i++)
	{
		(*i)->SetBehindEdge(0);
		delete (*i);
		count++; 
	}
	sideCache->clear();
	// sideCache->shrink_to_fit();
	/*
	#ifdef TRACE
	std::ofstream f;
	f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
	f << "Vertex Cleared: " << count << std::endl;
	f.close();
	#endif
	*/
}

NAVertexPtr NAVertexCollector::New(INetworkJunctionPtr junction)
{
	NAVertexPtr n = new DEBUG_NEW_PLACEMENT NAVertex(junction, 0);
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
	NAEdgeTable * cache = 0;

	if (edge->Direction == esriNEDAlongDigitized) cache = cacheAlong;
	else cache = cacheAgainst;

	return cache->find(edge->EID) != cache->end();
}

HRESULT NAEdgeClosed::Insert(NAEdgePtr edge)
{
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
