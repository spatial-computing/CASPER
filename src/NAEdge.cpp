#include "StdAfx.h"
#include "NAEdge.h"
#include "NAVertex.h"
#include "Evacuee.h"

///////////////////////////////////////////////////////////////////////
// EdgeReservations Methods

EdgeReservations::EdgeReservations(float capacity, float criticalDensPerCap, float saturationDensPerCap, float InitDelayCostPerPop)
{
	//List = new DEBUG_NEW_PLACEMENT std::vector<EdgeReservation>();
	ReservedPop = 0.0;
	Capacity = capacity;	
	CriticalDens = criticalDensPerCap * capacity;
	SaturationDensPerCap = saturationDensPerCap;
	//DirtyFlag = EdgeFlagDirty;
	initDelayCostPerPop = InitDelayCostPerPop;
	isDirty = true;
}

EdgeReservations::EdgeReservations(const EdgeReservations& cpy)
{
	//List = new DEBUG_NEW_PLACEMENT std::vector<EdgeReservation>(*(cpy.List));
	ReservedPop = cpy.ReservedPop;
	Capacity = cpy.Capacity;	
	CriticalDens = cpy.CriticalDens;
	SaturationDensPerCap = cpy.SaturationDensPerCap;
	initDelayCostPerPop = cpy.initDelayCostPerPop;
	isDirty = cpy.isDirty;
}

///////////////////////////////////////////////////////////////////////
// NAEdge Methods

bool NAEdge::LessThanNonHur(NAEdge * n1, NAEdge * n2) { return n1->ToVertex->g < n2->ToVertex->g; }
bool NAEdge::LessThanHur   (NAEdge * n1, NAEdge * n2) { return n1->ToVertex->g + n1->ToVertex->minh() < n2->ToVertex->g + n2->ToVertex->minh(); }

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
	TreePrevious = cpy.TreePrevious;
	TreeNext = cpy.TreeNext;
}

NAEdge::NAEdge(INetworkEdgePtr edge, long capacityAttribID, long costAttribID, float CriticalDensPerCap, float SaturationDensPerCap, NAResTable * resTable, float InitDelayCostPerPop, EvcTrafficModel TrafficModel)
{
	// calcSaved = 0;
	TreePrevious = NULL;
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
//#pragma float_control(precise, off, push)
double NAEdge::GetTrafficSpeedRatio(double allPop) const
{
	if (cachedCost[0] == allPop) { /*calcSaved++;*/ return cachedCost[1]; }
	double speedPercent = 1.0;
	switch (trafficModel)
	{
	case POWERModel:
		// z = 1.0 - 0.0202 * sqrt(x) * exp(-0.01127 * y)
		speedPercent = 1.0 - CASPERRatio * sqrt(allPop);
		break;
	case LINEARModel:
		speedPercent = 1.0 - (allPop - reservations->CriticalDens) / (2.0 * (reservations->SaturationDensPerCap * reservations->Capacity - reservations->CriticalDens));
		break;
	case STEPModel:
		speedPercent = 0.0;
		break;
	}
	cachedCost[0] = allPop; cachedCost[1] = speedPercent;
	return speedPercent;
}
//#pragma float_control(pop)

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

double NAEdge::GetCost(double newPop, EvcSolverMethod method) const
{
	double speedPercent = 1.0;	
	if (reservations->initDelayCostPerPop > 0.0) newPop = min(newPop, OriginalCost / reservations->initDelayCostPerPop);
	newPop += reservations->ReservedPop;

	if (newPop > reservations->CriticalDens)
	{			
		switch (method)
		{
		case CASPERSolver:
			speedPercent = GetTrafficSpeedRatio(newPop);
			break;
		case CCRPSolver:
			speedPercent = 0.0;
			break;
		}
		speedPercent = min(1.0, max(0.0001, speedPercent));
	}
	return OriginalCost / speedPercent;
}

// this function has to cache the answer and it has to be consistent.
bool NAEdge::IsDirty(double minPop2Route, EvcSolverMethod method)
{
	reservations->isDirty |= CleanCost < this->GetCost(minPop2Route, method) * 0.9;
	return reservations->isDirty;
}

void NAEdge::SetClean(double minPop2Route, EvcSolverMethod method)
{
	CleanCost = this->GetCost(minPop2Route, method);
	reservations->isDirty = false;
}

// this function adds the reservation also determines if the new added population makes the edge dirty.
// if this new reservation made the edge change from clean to dirty then the return is true otherwise returns false.
void NAEdge::AddReservation(/* Evacuee * evacuee, double fromCost, double toCost, */ double population, EvcSolverMethod method)
{	
	// actual reservation code
	// reservations->List->insert(reservations->List->end(), EdgeReservation(evacuee, fromCost, toCost));
	float newPop = (float)population;
	if (reservations->initDelayCostPerPop > 0.0f) newPop = min(newPop, (float)(OriginalCost / reservations->initDelayCostPerPop));
	reservations->ReservedPop += newPop;

	// this would mark the edge as dirty if only 1 one person changes it's cost (on top of the already reserved pop)
	IsDirty(1.0, method);
}

void NAEdge::TreeNextEraseFirst(NAEdge * child)
{
	if (child)
	{
		std::vector<NAEdge *>::size_type j = TreeNext.size();
		for(std::vector<NAEdge *>::size_type i = 0; i < TreeNext.size(); i++)		
			if (TreeNext[i]->EID == child->EID && TreeNext[i]->Direction == child->Direction)
			{
				j = i;
				break;
			}
			_ASSERT(j < TreeNext.size());
			if (j < TreeNext.size()) TreeNext.erase(TreeNext.begin() + j);
	}
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
	
void NAEdgeCache::CleanAllEdgesAndRelease(double minPop2Route, EvcSolverMethod solver)
{
	for(NAEdgeTableItr cit = cacheAlong->begin();   cit != cacheAlong->end();   cit++) cit->second->SetClean(minPop2Route, solver);
	for(NAEdgeTableItr cit = cacheAgainst->begin(); cit != cacheAgainst->end(); cit++) cit->second->SetClean(minPop2Route, solver);
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

NAEdgePtr NAEdgeCache::Get(long eid, esriNetworkEdgeDirection dir) const
{
	NAEdgeTable * cache = cacheAgainst;
	if (dir == esriNEDAlongDigitized) cache = cacheAlong;	
	NAEdgeTableItr i = cache->find(eid);
	if (i != cache->end()) return i->second;
	else return NULL;
}

//////////////////////////////////////////////////////////////////
//// NAEdgeMap Methods

bool NAEdgeMap::Exist(long eid, esriNetworkEdgeDirection dir)
{
	NAEdgeTable * cache = 0;

	if (dir == esriNEDAlongDigitized) cache = cacheAlong;
	else cache = cacheAgainst;

	return cache->find(eid) != cache->end();
}

void NAEdgeMap::GetDirtyEdges(std::vector<NAEdgePtr> * dirty, double minPop2Route, EvcSolverMethod method) const
{
	NAEdgeTableItr i;
	if(dirty)
	{
		for (i = cacheAlong->begin();   i != cacheAlong->end();   i++) if (i->second->IsDirty(minPop2Route, method)) dirty->push_back(i->second);
		for (i = cacheAgainst->begin(); i != cacheAgainst->end(); i++) if (i->second->IsDirty(minPop2Route, method)) dirty->push_back(i->second);
	}
}

void NAEdgeMap::Erase(long eid, esriNetworkEdgeDirection dir)
{
	NAEdgeTable * cache = 0;
	if (dir == esriNEDAlongDigitized) cache = cacheAlong;
	else cache = cacheAgainst;

	NAEdgeTableItr i = cache->find(eid);
	if (i != cache->end()) cache->erase(i);
}

HRESULT NAEdgeMap::Insert(NAEdgePtr edge)
{
	NAEdgeTable * cache = 0;

	if (edge->Direction == esriNEDAlongDigitized) cache = cacheAlong;
	else cache = cacheAgainst;

	if (cache->find(edge->EID) != cache->end()) return E_FAIL;
	cache->insert(NAEdgeTablePair(edge));

	return S_OK;
}

HRESULT NAEdgeMap::Insert(NAEdgeMap * edges)
{
	NAEdgeTableItr i;
	for(i = edges->cacheAlong->begin(); i != edges->cacheAlong->end(); i++)	
		if (cacheAlong->find(i->first) == cacheAlong->end()) cacheAlong->insert(NAEdgeTablePair(i->second));
	for(i = edges->cacheAgainst->begin(); i != edges->cacheAgainst->end(); i++)	
		if (cacheAgainst->find(i->first) == cacheAgainst->end()) cacheAgainst->insert(NAEdgeTablePair(i->second));	
	return S_OK;
}

//////////////////////////////////////////////////////////////////
//// NAEdgeMapTwoGen Methods

void NAEdgeMapTwoGen::MarkAllAsOldGen()
{
	oldGen->Insert(newGen);
	newGen->Clear();
}

void NAEdgeMapTwoGen::Clear(UCHAR gen)
{
	if ((gen & NAEdgeMap_OLDGEN) != 0) oldGen->Clear();
	if ((gen & NAEdgeMap_NEWGEN) != 0) newGen->Clear();
}

size_t NAEdgeMapTwoGen::Size(UCHAR gen)
{
	size_t t = 0;
	if ((gen & NAEdgeMap_OLDGEN) != 0) t += oldGen->Size();
	if ((gen & NAEdgeMap_NEWGEN) != 0) t += newGen->Size();
	return t;
}

HRESULT NAEdgeMapTwoGen::Insert(NAEdgePtr edge, UCHAR gen)
{
	HRESULT hr = S_OK;
	if ((gen & NAEdgeMap_OLDGEN) != 0) if (FAILED(hr = oldGen->Insert(edge))) return hr;
	if ((gen & NAEdgeMap_NEWGEN) != 0) if (FAILED(hr = newGen->Insert(edge))) return hr;
	return hr;
}

bool NAEdgeMapTwoGen::Exist(long eid, esriNetworkEdgeDirection dir, UCHAR gen)
{
	bool e = false;
	if ((gen & NAEdgeMap_OLDGEN) != 0) e |= oldGen->Exist(eid, dir);
	if ((gen & NAEdgeMap_NEWGEN) != 0) e |= newGen->Exist(eid, dir);
	return e;
}

void NAEdgeMapTwoGen::Erase(NAEdgePtr edge, UCHAR gen)
{
	if ((gen & NAEdgeMap_OLDGEN) != 0) oldGen->Erase(edge);
	if ((gen & NAEdgeMap_NEWGEN) != 0) newGen->Erase(edge);
}

//////////////////////////////////////////////////////////////////
//// NAEdgeContainer Methods

bool NAEdgeContainer::Exist(INetworkEdgePtr edge)
{
	esriNetworkEdgeDirection dir;
	long eid;
	if (FAILED(edge->get_EID(&eid))) return false;
	if (FAILED(edge->get_Direction(&dir))) return false;
	return Exist(eid, dir);
}

HRESULT NAEdgeContainer::Insert(INetworkEdgePtr edge)
{
	esriNetworkEdgeDirection dir;
	long eid;
	HRESULT hr;
	if (FAILED(hr = edge->get_EID(&eid))) return hr;
	if (FAILED(hr = edge->get_Direction(&dir))) return hr;
	return Insert(eid, dir);
}

void NAEdgeContainer::Insert(NAEdgeContainer * clone)
{
	for(NAEdgeIterator i = clone->cache->begin(); i != clone->cache->end(); i++) cache->insert(NAEdgeContainerPair(i->first, i->second));
}

bool NAEdgeContainer::Exist(long eid, esriNetworkEdgeDirection dir)
{
	stdext::hash_map<long, unsigned char>::iterator i = cache->find(eid);
	bool ret = false;
	unsigned char d = (unsigned char)dir;
	if (i != cache->end() && i->second != 0) ret = (i->second & d) != 0;	
	return ret;
}

HRESULT NAEdgeContainer::Insert(long eid, esriNetworkEdgeDirection dir)
{
	unsigned char d = (unsigned char)dir;
	stdext::hash_map<long, unsigned char>::iterator i = cache->find(eid);
	if (i == cache->end()) cache->insert(NAEdgeContainerPair(eid, d));
	else i->second |= d;
	return S_OK;
}