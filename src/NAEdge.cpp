#include "StdAfx.h"
#include "NAEdge.h"
#include "NAVertex.h"
#include "Evacuee.h"

///////////////////////////////////////////////////////////////////////
// EdgeReservations Methods

EdgeReservations::EdgeReservations(float capacity, TrafficModel * trafficModel)
{
	ReservedPop = 0.0;
	Capacity = capacity;
	myTrafficModel = trafficModel;
	dirtyState = EdgeDirtyState::CostIncreased;
}

EdgeReservations::EdgeReservations(const EdgeReservations& cpy)
{
	ReservedPop = cpy.ReservedPop;
	Capacity = cpy.Capacity;
	myTrafficModel = cpy.myTrafficModel;
	dirtyState = cpy.dirtyState;
}

void EdgeReservations::AddReservation(double newFlow, EvcPathPtr path)
{
	this->insert(std::pair<int, EvcPathPtr>(path->GetKey(), path));
	ReservedPop += (float)newFlow;
}

void EdgeReservations::RemoveReservation(double flow, EvcPathPtr path)
{
	erase(path->GetKey());
	ReservedPop -= (float)flow;
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
	CleanCost = cpy.CleanCost;
	TreePrevious = cpy.TreePrevious;
	TreeNext = cpy.TreeNext;
	AdjacentForward = cpy.AdjacentForward;
	AdjacentBackward = cpy.AdjacentBackward;
	myGeometry = cpy.myGeometry;
}

NAEdge::NAEdge(INetworkEdgePtr edge, long capacityAttribID, long costAttribID, NAResTable * resTable, TrafficModel * model)
{
	myGeometry = NULL;
	TreePrevious = NULL;
	CleanCost = -1.0;
	ToVertex = 0;
	this->NetEdge = edge;
	VARIANT vcost, vcap;
	float capacity = 1.0;
	HRESULT hr = 0;
	AdjacentForward = NULL;
	AdjacentBackward = NULL;

	if (FAILED(hr = edge->get_AttributeValue(capacityAttribID, &vcap)))
	{
		vcap.vt = VT_R8;
		vcap.dblVal = 1.0;
	}
	if (FAILED(hr = edge->get_AttributeValue(costAttribID, &vcost)))
	{
		vcost.vt = VT_R8;
		vcost.dblVal = 1.0;
	}

	if (FAILED(hr = edge->get_EID(&EID)) ||	FAILED(hr = edge->get_Direction(&Direction)))
	{
		_ASSERT(hr == 0);
		NetEdge = 0;
		reservations = 0;
		EID = -1;
		throw std::exception("Something bad happened while looking up a network edge");
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
			reservations = new DEBUG_NEW_PLACEMENT EdgeReservations(capacity, model);
			resTable->insert(NAResTablePair(EID, reservations));
		}
		else
		{
			reservations = it->second;
		}
	}
}

HRESULT NAEdge::QuerySourceStuff(long * sourceOID, long * sourceID, double * fromPosition, double * toPosition) const
{
	HRESULT hr = S_OK;
	if (FAILED(hr = NetEdge->get_OID(sourceOID))) return hr;
	if (FAILED(hr = NetEdge->get_SourceID(sourceID))) return hr;
	if (FAILED(hr = NetEdge->QueryPositions(fromPosition, toPosition))) return hr;
	return hr;
}

HRESULT NAEdge::GetGeometry(INetworkDatasetPtr ipNetworkDataset, IFeatureClassContainerPtr ipFeatureClassContainer, bool & sourceNotFoundFlag, IGeometryPtr & geometry)
{
	HRESULT hr = S_OK;
	long sourceOID, sourceID;
	double fromPosition, toPosition;
	INetworkSourcePtr ipNetworkSource;
	BSTR sourceName;
	IFeatureClassPtr ipNetworkSourceFC;
	IFeaturePtr ipSourceFeature;
	ICurvePtr ipSubCurve;

	if (!myGeometry)
	{
		// retrieve street shape for this edge
		if (FAILED(hr = this->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) return hr;
		if (FAILED(hr = ipNetworkDataset->get_SourceByID(sourceID, &ipNetworkSource))) return hr;
		if (FAILED(hr = ipNetworkSource->get_Name(&sourceName))) return hr;
		if (FAILED(hr = ipFeatureClassContainer->get_ClassByName(sourceName, &ipNetworkSourceFC))) return hr;
		if (!ipNetworkSourceFC)
		{
			sourceNotFoundFlag = true;
			return hr;
		}
		if (FAILED(hr = ipNetworkSourceFC->GetFeature(sourceOID, &ipSourceFeature))) return hr;
		if (FAILED(hr = ipSourceFeature->get_Shape(&myGeometry))) return hr;

		// Check to see how much of the line geometry we can copy over
		if (fromPosition != 0.0 || toPosition != 1.0)
		{
			// We must use only a curve of the line geometry
			ICurve3Ptr ipCurve(myGeometry);
			if (FAILED(hr = ipCurve->GetSubcurve(fromPosition, toPosition, VARIANT_TRUE, &ipSubCurve))) return hr;
			myGeometry = ipSubCurve;
		}
	}
	geometry = myGeometry;
	return hr;
}

HRESULT NAEdge::InsertEdgeToFeatureCursor(INetworkDatasetPtr ipNetworkDataset, IFeatureClassContainerPtr ipFeatureClassContainer, IFeatureBufferPtr ipFeatureBuffer, IFeatureCursorPtr ipFeatureCursor,
										   long eidFieldIndex, long sourceIDFieldIndex, long sourceOIDFieldIndex, long dirFieldIndex, long resPopFieldIndex, long travCostFieldIndex,
										   long orgCostFieldIndex, long congestionFieldIndex, bool & sourceNotFoundFlag)
{
	HRESULT hr = S_OK;
	ATL::CComVariant featureID(0);
	long sourceOID, sourceID;
	double fromPosition, toPosition;
	IGeometryPtr ipGeometry;
	BSTR dir = Direction == esriNEDAgainstDigitized ? L"Against" : L"Along";

	float resPop = this->GetReservedPop();
	if (resPop <= 0.0) return hr;

	// retrieve street shape for this edge
	if (FAILED(hr = this->GetGeometry(ipNetworkDataset, ipFeatureClassContainer, sourceNotFoundFlag, ipGeometry))) return hr;
	if (FAILED(hr = this->QuerySourceStuff(&sourceOID, &sourceID, &fromPosition, &toPosition))) return hr;

	// Store the feature values on the feature buffer
	if (FAILED(hr = ipFeatureBuffer->putref_Shape(ipGeometry))) return hr;
	if (FAILED(hr = ipFeatureBuffer->put_Value(eidFieldIndex, ATL::CComVariant(EID)))) return hr;
	if (FAILED(hr = ipFeatureBuffer->put_Value(sourceIDFieldIndex, ATL::CComVariant(sourceID)))) return hr;
	if (FAILED(hr = ipFeatureBuffer->put_Value(sourceOIDFieldIndex, ATL::CComVariant(sourceOID)))) return hr;
	if (FAILED(hr = ipFeatureBuffer->put_Value(dirFieldIndex, ATL::CComVariant(dir)))) return hr;
	if (FAILED(hr = ipFeatureBuffer->put_Value(resPopFieldIndex, ATL::CComVariant(resPop)))) return hr;
	if (FAILED(hr = ipFeatureBuffer->put_Value(travCostFieldIndex, ATL::CComVariant(GetCurrentCost())))) return hr;
	if (FAILED(hr = ipFeatureBuffer->put_Value(orgCostFieldIndex, ATL::CComVariant(OriginalCost)))) return hr;
	if (FAILED(hr = ipFeatureBuffer->put_Value(congestionFieldIndex, ATL::CComVariant(GetCurrentCost() / OriginalCost)))) return hr;

	// Insert the feature buffer in the insert cursor
	if (FAILED(hr = ipFeatureCursor->InsertFeature(ipFeatureBuffer, &featureID))) return hr;
	return hr;
}

// Special function for CCRP: to check how much capacity is left on this edge.
// Will be used to get max capacity available on a path
double NAEdge::LeftCapacity() const
{
	return reservations->myTrafficModel->LeftCapacityOnEdge(reservations->Capacity, reservations->ReservedPop, OriginalCost);
}

// This is where the actual capacity aware part is happening:
// We take the original values of the edge and recalculate the
// new travel cost based on number of reserved spots by previous evacuees.
double NAEdge::GetTrafficSpeedRatio(double allPop, EvcSolverMethod method) const
{
	double speedPercent = 1.0;
	if (method == CASPERSolver) speedPercent = reservations->myTrafficModel->GetCongestionPercentage(reservations->Capacity, allPop);
	else if (method == CCRPSolver) speedPercent = allPop > reservations->myTrafficModel->CriticalDensPerCap * reservations->Capacity ? 0.0 : 1.0;
	speedPercent = min(1.0, max(0.0001, speedPercent));
	return speedPercent;
}

double NAEdge::GetCurrentCost(EvcSolverMethod method) const
{
	return OriginalCost / GetTrafficSpeedRatio(reservations->ReservedPop, method);
}

double NAEdge::GetCost(double newPop, EvcSolverMethod method, double * globalDeltaCost) const
{
	double speedPercent = 1.0;
	if (reservations->myTrafficModel->InitDelayCostPerPop > 0.0) newPop = min(newPop, OriginalCost / reservations->myTrafficModel->InitDelayCostPerPop);
	newPop += reservations->ReservedPop;

	speedPercent = GetTrafficSpeedRatio(newPop, method);

	// this extra output tells CASPER how much will this edge reservation affects the cost according to the traffic model
	if (globalDeltaCost)
	{
		double globalDeltaCostPercentage = 0.0;
		globalDeltaCostPercentage = GetTrafficSpeedRatio(reservations->ReservedPop, method) - speedPercent;
		_ASSERT(globalDeltaCostPercentage >= 0.0);
		*globalDeltaCost = OriginalCost * globalDeltaCostPercentage;
	}
	return OriginalCost / speedPercent;
}

double NAEdge::MaxAddedCostOnReservedPathsWithNewFlow(double deltaCostOfNewFlow, double longestPathSoFar, double currentPathSoFar, double selfishRatio) const
{
	double AddedGlobalCost = 0.0;
	double cutoffCost = max(longestPathSoFar, currentPathSoFar);

	if (deltaCostOfNewFlow > 0.0 && selfishRatio > 0.0)
		for(const auto & p: *reservations) AddedGlobalCost = max(AddedGlobalCost, p.second->GetReserveEvacuationCost() + deltaCostOfNewFlow - cutoffCost);		
	else return 0.0;

	return selfishRatio * min(AddedGlobalCost, deltaCostOfNewFlow);
}

// this function has to cache the answer and it has to be consistent.
EdgeDirtyState NAEdge::HowDirty(EvcSolverMethod method, double minPop2Route, bool exhaustive)
{
	if (CleanCost <= 0.0) reservations->dirtyState = EdgeDirtyState::CostIncreased;
	else
	{
		if (exhaustive || reservations->dirtyState == EdgeDirtyState::CleanState)
		{
			reservations->dirtyState = EdgeDirtyState::CleanState;
			double costchange = (GetCost(minPop2Route, method) / CleanCost) - 1.0;
			if (costchange > 0.02) reservations->dirtyState = EdgeDirtyState::CostIncreased;
			if (costchange < -FLT_EPSILON) reservations->dirtyState = EdgeDirtyState::CostDecreased;
		}
	}
	return reservations->dirtyState;
}

void NAEdge::SetClean(EvcSolverMethod method, double minPop2Route)
{
	CleanCost = this->GetCost(minPop2Route, method);
	reservations->dirtyState = EdgeDirtyState::CleanState;
}

// this function adds the reservation also determines if the new added population makes the edge dirty.
// if this new reservation made the edge change from clean to dirty then the return is true otherwise returns false.
void NAEdge::AddReservation(EvcPath * path, EvcSolverMethod method, bool delayedDirtyState)
{
	// actual reservation code
	double population = path->GetRoutedPop();
	if (reservations->myTrafficModel->InitDelayCostPerPop > 0.0) population = min(population, OriginalCost / reservations->myTrafficModel->InitDelayCostPerPop);
	reservations->AddReservation(population, path);

	// this would mark the edge as dirty if only 1 one person changes it's cost (on top of the already reserved pop)
	if (!delayedDirtyState) HowDirty(method);
}

void NAEdge::RemoveReservation(EvcPathPtr path, EvcSolverMethod method, bool delayedDirtyState)
{
	double flow = path->GetRoutedPop();
	if (reservations->myTrafficModel->InitDelayCostPerPop > 0.0) flow = min(flow, OriginalCost / reservations->myTrafficModel->InitDelayCostPerPop);
	reservations->RemoveReservation(flow, path);
	// this would mark the edge as dirty if only 1 one person changes it's cost (on top of the already reserved pop)
	if (!delayedDirtyState) HowDirty(method);
}

void NAEdge::TreeNextEraseFirst(NAEdge * child)
{
	if (child)
	{
		std::vector<NAEdge *>::size_type j = TreeNext.size();
		for(std::vector<NAEdge *>::size_type i = 0; i < TreeNext.size(); i++)
			if (IsEqual(TreeNext[i], child))
			{
				j = i;
				break;
			}
		_ASSERT(j < TreeNext.size());
		if (j < TreeNext.size()) TreeNext.erase(TreeNext.begin() + j);
	}
}

double GetHeapKeyHur   (const NAEdge * e)                     { return e->ToVertex->GVal + e->ToVertex->GlobalPenaltyCost + e->ToVertex->GetMinHOrZero(); }
double GetHeapKeyNonHur(const NAEdge * e)                     { return e->ToVertex->GVal; }
bool   IsEqual         (const NAEdge * n1, const NAEdge * n2) { return n1->EID == n2->EID && n1->Direction == n2->Direction; }

/////////////////////////////////////////////////////////////
// NAEdgeCache
// Creates a new edge pointer based on the given NetworkEdge. If one exist in the cache, it will be sent out.
NAEdgePtr NAEdgeCache::New(INetworkEdgePtr edge, bool reuseEdgeElement)
{
	NAEdgePtr n = 0;
	long EID;
	NAEdgeTable * cache = 0;
	NAResTable  * resTable = 0;
	esriNetworkEdgeDirection dir;
	INetworkElementPtr ipEdgeElement;
	INetworkEdgePtr edgeClone;

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

	std::unordered_map<long, NAEdgePtr>::const_iterator it = cache->find(EID);

	if (it == cache->end())
	{
		if (!reuseEdgeElement)
		{
			if (FAILED(ipNetworkQuery->CreateNetworkElement(esriNETEdge, &ipEdgeElement))) return 0;
			edgeClone = ipEdgeElement;
			if (FAILED(ipNetworkQuery->QueryEdge(EID, dir, edgeClone))) return 0;
		}
		else
		{
			edgeClone = edge;
		}
		n = new DEBUG_NEW_PLACEMENT NAEdge(edgeClone, capacityAttribID, costAttribID, resTable, myTrafficModel);
		cache->insert(NAEdgeTablePair(n));
	}
	else
	{
		n = it->second;
	}
	return n;
}

void NAEdgeCache::CleanAllEdgesAndRelease(double minPop2Route, EvcSolverMethod solver)
{
	for (NAEdgeTableItr cit = cacheAlong->begin(); cit != cacheAlong->end(); cit++) cit->second->SetClean(solver, minPop2Route);
	for (NAEdgeTableItr cit = cacheAgainst->begin(); cit != cacheAgainst->end(); cit++) cit->second->SetClean(solver, minPop2Route);
}

void NAEdgeCache::Clear()
{
	for(NAEdgeTableItr cit = cacheAlong->begin(); cit != cacheAlong->end(); cit++)
	{
		NAEdgePtr e = (*cit).second;
		if (e->AdjacentForward) delete e->AdjacentForward;
		if (e->AdjacentBackward) delete e->AdjacentBackward;
		delete e;
	}
	for(NAEdgeTableItr cit = cacheAgainst->begin(); cit != cacheAgainst->end(); cit++)
	{
		NAEdgePtr e = (*cit).second;
		if (e->AdjacentForward) delete e->AdjacentForward;
		if (e->AdjacentBackward) delete e->AdjacentBackward;
		delete e;
	}
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

HRESULT NAEdgeCache::QueryAdjacencies(NAVertexPtr ToVertex, NAEdgePtr Edge, QueryDirection dir, vector_NAEdgePtr_Ptr & neighbors)
{
	HRESULT hr = S_OK;
	long adjacentEdgeCount;
	double fromPosition, toPosition;
	INetworkForwardStarExPtr star;
	neighbors = NULL;
	INetworkEdgePtr netEdge = NULL;

	if (Edge)
	{
		neighbors = dir == QueryDirection::Forward ? Edge->AdjacentForward : Edge->AdjacentBackward;
		netEdge = Edge->NetEdge;
	}
	if (neighbors) return hr;
	neighbors = new DEBUG_NEW_PLACEMENT std::vector<NAEdgePtr>();

	if (dir == QueryDirection::Forward)
	{
		star = ipForwardStar;
		if (Edge) Edge->AdjacentForward = neighbors;
	}
	else
	{
		star = ipBackwardStar;
		if (Edge) Edge->AdjacentBackward = neighbors;
	}

	if (FAILED(hr = star->QueryAdjacencies(ToVertex->Junction, netEdge, 0 /*lastExteriorEdge*/, ipAdjacencies))) return hr;
	if (FAILED(hr = ipAdjacencies->get_Count(&adjacentEdgeCount))) return hr;
	neighbors->reserve(adjacentEdgeCount);
	for (long i = 0; i < adjacentEdgeCount; i++)
	{
		if (FAILED(hr = ipAdjacencies->QueryEdge(i, ipCurrentEdge, &fromPosition, &toPosition))) return hr;
		neighbors->push_back(this->New(ipCurrentEdge, false));
	}

	return hr;
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
		for (i = cacheAlong->begin();   i != cacheAlong->end();   i++) if (i->second->HowDirty(method, minPop2Route) != EdgeDirtyState::CleanState) dirty->push_back(i->second);
		for (i = cacheAgainst->begin(); i != cacheAgainst->end(); i++) if (i->second->HowDirty(method, minPop2Route) != EdgeDirtyState::CleanState) dirty->push_back(i->second);
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

void NAEdgeMap::CallHowDirty(EvcSolverMethod method, double minPop2Route, bool exhaustive)
{
	NAEdgeTableItr i;
	for (i = cacheAlong->begin(); i != cacheAlong->end(); i++) i->second->HowDirty(method, minPop2Route, exhaustive);
	for (i = cacheAgainst->begin(); i != cacheAgainst->end(); i++) i->second->HowDirty(method, minPop2Route, exhaustive);
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

void NAEdgeMapTwoGen::Clear(NAEdgeMapGeneration gen)
{
	if (CheckFlag(gen, NAEdgeMapGeneration::OldGen)) oldGen->Clear();
	if (CheckFlag(gen, NAEdgeMapGeneration::NewGen)) newGen->Clear();
}

size_t NAEdgeMapTwoGen::Size(NAEdgeMapGeneration gen)
{
	size_t t = 0;
	if (CheckFlag(gen, NAEdgeMapGeneration::OldGen)) t += oldGen->Size();
	if (CheckFlag(gen, NAEdgeMapGeneration::NewGen)) t += newGen->Size();
	return t;
}

HRESULT NAEdgeMapTwoGen::Insert(NAEdgePtr edge, NAEdgeMapGeneration gen)
{
	HRESULT hr = S_OK;
	if (CheckFlag(gen, NAEdgeMapGeneration::OldGen)) if (FAILED(hr = oldGen->Insert(edge))) return hr;
	if (CheckFlag(gen, NAEdgeMapGeneration::NewGen)) if (FAILED(hr = newGen->Insert(edge))) return hr;
	return hr;
}

bool NAEdgeMapTwoGen::Exist(long eid, esriNetworkEdgeDirection dir, NAEdgeMapGeneration gen)
{
	bool e = false;
	if (CheckFlag(gen, NAEdgeMapGeneration::OldGen)) e |= oldGen->Exist(eid, dir);
	if (CheckFlag(gen, NAEdgeMapGeneration::NewGen)) e |= newGen->Exist(eid, dir);
	return e;
}

void NAEdgeMapTwoGen::Erase(NAEdgePtr edge, NAEdgeMapGeneration gen)
{
	if (CheckFlag(gen, NAEdgeMapGeneration::OldGen)) oldGen->Erase(edge);
	if (CheckFlag(gen, NAEdgeMapGeneration::NewGen)) newGen->Erase(edge);
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

void NAEdgeContainer::Insert(NAEdgeContainer * clone)
{
	for (NAEdgeIterator i = clone->cache->begin(); i != clone->cache->end(); i++) cache->insert(NAEdgeContainerPair(i->first, i->second));
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

HRESULT NAEdgeContainer::Insert(long eid, unsigned char dir)
{
	std::unordered_map<long, unsigned char>::iterator i = cache->find(eid);
	if (i == cache->end()) cache->insert(NAEdgeContainerPair(eid, dir));
	else i->second |= dir;
	return S_OK;
}

HRESULT NAEdgeContainer::Insert(long eid, esriNetworkEdgeDirection dir)
{
	unsigned char d = (unsigned char)dir;
	return Insert(eid, d);
}

bool NAEdgeContainer::Exist(long eid, esriNetworkEdgeDirection dir)
{
	std::unordered_map<long, unsigned char>::const_iterator i = cache->find(eid);
	bool ret = false;
	unsigned char d = (unsigned char)dir;
	if (i != cache->end() && i->second != 0) ret = (i->second & d) != 0;
	return ret;
}

HRESULT NAEdgeContainer::Remove(INetworkEdgePtr edge)
{
	esriNetworkEdgeDirection dir;
	long eid;
	HRESULT hr;
	if (FAILED(hr = edge->get_EID(&eid))) return hr;
	if (FAILED(hr = edge->get_Direction(&dir))) return hr;
	return Remove(eid, dir);
}

HRESULT NAEdgeContainer::Remove(long eid, unsigned char dir)
{
	std::unordered_map<long, unsigned char>::iterator i = cache->find(eid);
	if (i != cache->end())
	{
		i->second &= ~dir;
		if (i->second == 0) cache->erase(i->first);
	}
	return S_OK;
}

HRESULT NAEdgeContainer::Remove(long eid, esriNetworkEdgeDirection dir)
{
	unsigned char d = (unsigned char)dir;
	return Remove(eid, d);
}
