// ===============================================================================================
// Evacuation Solver: Evacuee class Implementation
// Description: Contains code for evacuee, evacuee path, evacuee path segment (which wrappes graph
// edge) and other useful container types.
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#include "StdAfx.h"
#include "Evacuee.h"
#include "NAVertex.h"
#include "NAEdge.h"

HRESULT PathSegment::GetGeometry(INetworkDatasetPtr ipNetworkDataset, IFeatureClassContainerPtr ipFeatureClassContainer, bool & sourceNotFoundFlag, IGeometryPtr & geometry)
{
	HRESULT hr = S_OK;
	ICurvePtr subCurve;
	if (FAILED(hr = Edge->GetGeometry(ipNetworkDataset, ipFeatureClassContainer, sourceNotFoundFlag, geometry))) return hr;
	if (fromRatio != 0.0 || toRatio != 1.0)
	{
		// We must use only a curve of the line geometry
		ICurve3Ptr ipCurve(geometry);
		if (FAILED(hr = ipCurve->GetSubcurve(fromRatio, toRatio, VARIANT_TRUE, &subCurve))) return hr;
		geometry = subCurve;
	}
	return hr;
}

double PathSegment::GetCurrentCost(EvcSolverMethod method) const { return Edge->GetCurrentCost(method) * abs(GetEdgePortion()); }
bool EvcPath::MoreThanPathOrder1(const Evacuee * e1, const Evacuee * e2) { return e1->Paths->front()->Order > e2->Paths->front()->Order; }

// first i have to move the evacuee. then cut the path and back it up. mark the evacuee to be processed again.
size_t EvcPath::DynamicStep_MoveOnPath(const DoubleGrowingArrayList<EvcPath *, size_t>::iterator & begin, const DoubleGrowingArrayList<EvcPath *, size_t>::iterator & end,
	std::unordered_set<NAEdge *, NAEdgePtrHasher, NAEdgePtrEqual> & DynamicallyAffectedEdges, double CurrentTime, EvcSolverMethod method, double initDelayPerPop)
{
	size_t count = 0, segment = 0;
	double pathCost = 0.0, segCost = 0.0, segRatio = 0.0;
	EvcPathPtr path = nullptr;

	if (CurrentTime > 0.0)
	{
		std::sort(begin, end, EvcPath::MoreThanPathOrder2);
		for (auto p = begin; p != end; ++p)
		{
			path = *p;
			if (path->FinalEvacuationCost > CurrentTime && path->myEvc->Status != EvacueeStatus::Unreachable && !path->empty())
			{
				segCost = 0.0;
				pathCost = 0.0;
				for (segment = 0; pathCost < CurrentTime && segment < path->size(); ++segment)
				{
					segCost = path->at(segment)->GetCurrentCost(method);
					pathCost += segCost;
				}
				segRatio = (pathCost - CurrentTime) / segCost;

				for (size_t i = path->size() - 1; i >= segment; --i)
				{
					path->at(i)->Edge->RemoveReservation(path, method, true);
					DynamicallyAffectedEdges.insert(path->at(i)->Edge);
					delete path->at(i);
				}
				path->Frozen = true;
				path->erase(path->begin() + segment, path->end());

				path->myEvc->Status = EvacueeStatus::Unprocessed;
				path->MySafeZone->Reserve(-path->RoutedPop);
				++count;
			}
		}
	}
	return count;
}

void EvcPath::DetachPathsFromEvacuee(Evacuee * evc, EvcSolverMethod method, std::unordered_set<NAEdgePtr, NAEdgePtrHasher, NAEdgePtrEqual> * touchedEdges, std::shared_ptr<std::vector<EvcPathPtr>> detachedPaths)
{
	// It's time to clean up the evacuee object and reset it for the next iteration
	// To do this we first collect all its paths, take away all edge reservations, and then reset some of its fields.
	// at the end keep a record of touched edges for a 'HowDirty' call
	for (const auto & p : *evc->Paths)
	{
		for (auto s = p->crbegin(); s != p->crend(); ++s) (*s)->Edge->RemoveReservation(p, method, true);
		if (touchedEdges) { for (const auto & s : *p) touchedEdges->insert(s->Edge); }
		if (detachedPaths) detachedPaths->push_back(p); else delete p;
	}
	evc->Paths->clear();
}

void EvcPath::ReattachToEvacuee(EvcSolverMethod method)
{
	for (const auto & s : *this) s->Edge->AddReservation(this, method, true);
	myEvc->Paths->push_back(this);
}

double EvcPath::GetMinCostRatio(double MaxEvacuationCost) const
{
	if (MaxEvacuationCost <= 0.0) MaxEvacuationCost = FinalEvacuationCost;
	double PredictionCostRatio = (ReserveEvacuationCost - myEvc->PredictedCost) / MaxEvacuationCost;
	double EvacuationCostRatio = (FinalEvacuationCost - ReserveEvacuationCost) / MaxEvacuationCost;
	return min(PredictionCostRatio, EvacuationCostRatio);
}

double EvcPath::GetAvgCostRatio(double MaxEvacuationCost) const
{
	if (MaxEvacuationCost <= 0.0) MaxEvacuationCost = FinalEvacuationCost;
	return (FinalEvacuationCost - myEvc->PredictedCost) / (2.0 * MaxEvacuationCost);
}

void EvcPath::DoesItNeedASecondChance(double ThreasholdForCost, double ThreasholdForPathOverlap, std::vector<EvacueePtr> & AffectingList, double ThisIterationMaxCost, EvcSolverMethod method)
{
	double PredictionCostRatio = (ReserveEvacuationCost - myEvc->PredictedCost) / ThisIterationMaxCost;
	double EvacuationCostRatio = (FinalEvacuationCost - ReserveEvacuationCost) / ThisIterationMaxCost;

	if (PredictionCostRatio >= ThreasholdForCost || EvacuationCostRatio >= ThreasholdForCost)
	{
		if (myEvc->Status == EvacueeStatus::Processed)
		{
			// since the prediction was bad it probably means the evacuee has more than average vehicles so it should be processed sooner
			AffectingList.push_back(myEvc);
			myEvc->Status = EvacueeStatus::Unprocessed;
		}

		// We have to add the affecting list to be re-routed as well
		// We do this by selecting the paths that have some overlap
		std::vector<EvcPathPtr> crossing;
		Histogram<EvcPathPtr> FreqOfOverlaps;
		crossing.reserve(50);

		for (const auto & seg : *this)
		{
			seg->Edge->GetUniqeCrossingPaths(crossing, true);
			FreqOfOverlaps.WeightedAdd(crossing, seg->GetCurrentCost(method));
		}

		double cutOffWeight = ThreasholdForPathOverlap * FreqOfOverlaps.maxWeight;
		for (const auto & pair : FreqOfOverlaps)
		{
			if (pair.first->myEvc->Status == EvacueeStatus::Processed && pair.second > cutOffWeight)
			{
				AffectingList.push_back(pair.first->myEvc);
				pair.first->myEvc->Status = EvacueeStatus::Unprocessed;
			}
		}
	}
}

void EvcPath::AddSegment(EvcSolverMethod method, PathSegmentPtr segment)
{
	this->push_front(segment);
	segment->Edge->AddReservation(this, method);
	double p = abs(segment->GetEdgePortion());
	ReserveEvacuationCost += segment->Edge->GetCurrentCost(method) * p;
	OrginalCost += segment->Edge->OriginalCost * p;
}

void EvcPath::CalculateFinalEvacuationCost(double initDelayCostPerPop, EvcSolverMethod method)
{
	FinalEvacuationCost = 0.0;
	for (const auto & pathSegment : *this) FinalEvacuationCost += pathSegment->GetCurrentCost(method);
	FinalEvacuationCost += RoutedPop * initDelayCostPerPop;
	myEvc->FinalCost = max(myEvc->FinalCost, FinalEvacuationCost);
}

HRESULT EvcPath::AddPathToFeatureBuffers(ITrackCancel * pTrackCancel, INetworkDatasetPtr ipNetworkDataset, IFeatureClassContainerPtr ipFeatureClassContainer, bool & sourceNotFoundFlag,
	IStepProgressorPtr ipStepProgressor, double & globalEvcCost, double initDelayCostPerPop, IFeatureBufferPtr ipFeatureBufferR, IFeatureCursorPtr ipFeatureCursorR,
	long evNameFieldIndex, long evacTimeFieldIndex, long orgTimeFieldIndex, long popFieldIndex,
	long ERRouteFieldIndex, long EREdgeFieldIndex, long EREdgeDirFieldIndex, long ERSeqFieldIndex, long ERFromPosFieldIndex, long ERToPosFieldIndex, long ERCostFieldIndex)
{
	HRESULT hr = S_OK;
	OrginalCost = 0.0;
	FinalEvacuationCost = 0.0;
	IPointCollectionPtr pline = IPointCollectionPtr(CLSID_Polyline);
	long pointCount = -1;
	VARIANT_BOOL keepGoing;
	IGeometryPtr ipGeometry;
	esriGeometryType type;
	IPointCollectionPtr pcollect;
	IPointPtr p;
	VARIANT RouteOID;

	for (const auto & pathSegment : *this)
	{
		// Check to see if the user wishes to continue or cancel the solve (i.e., check whether or not the user has hit the ESC key to stop processing)
		if (pTrackCancel)
		{
			if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
			if (keepGoing == VARIANT_FALSE) return E_ABORT;
		}

		// take a path segment from the stack
		pointCount = -1;
		_ASSERT(pathSegment->GetEdgePortion() > 0.0);
		if (FAILED(hr = pathSegment->GetGeometry(ipNetworkDataset, ipFeatureClassContainer, sourceNotFoundFlag, ipGeometry))) return hr;

		ipGeometry->get_GeometryType(&type);

		// get all the points from this polyline and store it in the point stack
		_ASSERT(type == esriGeometryPolyline);

		if (type == esriGeometryPolyline)
		{
			pathSegment->pline = (IPolylinePtr)ipGeometry;
			pcollect = pathSegment->pline;
			if (FAILED(hr = pcollect->get_PointCount(&pointCount))) return hr;

			// if this is not the last path segment then the last point is redundant.
			pointCount--;
			for (long i = 0; i < pointCount; i++)
			{
				if (FAILED(hr = pcollect->get_Point(i, &p))) return hr;
				if (FAILED(hr = pline->AddPoint(p))) return hr;
			}
		}
		// Final cost calculations
		double p = abs(pathSegment->GetEdgePortion());
		FinalEvacuationCost += pathSegment->Edge->GetCurrentCost() * p;
		OrginalCost         += pathSegment->Edge->OriginalCost     * p;
	}

	// Add the last point of the last path segment to the polyline
	if (pointCount > -1)
	{
		pcollect->get_Point(pointCount, &p);
		pline->AddPoint(p);
	}

	// add the initial delay cost
	FinalEvacuationCost += RoutedPop * initDelayCostPerPop;
	globalEvcCost = max(globalEvcCost, FinalEvacuationCost);

	// Store the feature values on the feature buffer
	if (FAILED(hr = ipFeatureBufferR->putref_Shape((IPolylinePtr)pline))) return hr;
	if (FAILED(hr = ipFeatureBufferR->put_Value(evNameFieldIndex, myEvc->Name))) return hr;
	if (FAILED(hr = ipFeatureBufferR->put_Value(evacTimeFieldIndex, ATL::CComVariant(FinalEvacuationCost)))) return hr;
	if (FAILED(hr = ipFeatureBufferR->put_Value(orgTimeFieldIndex, ATL::CComVariant(OrginalCost)))) return hr;
	if (FAILED(hr = ipFeatureBufferR->put_Value(popFieldIndex, ATL::CComVariant(RoutedPop)))) return hr;

	// Insert the feature buffer in the insert cursor
	if (FAILED(hr = ipFeatureCursorR->InsertFeature(ipFeatureBufferR, &RouteOID))) return hr;

	#ifdef DEBUG
	std::wostringstream os_;
	os_.precision(3);
	os_ << RouteOID.intVal << ',' << myEvc->PredictedCost << ',' << ReserveEvacuationCost << ',' << FinalEvacuationCost << std::endl;
	OutputDebugStringW(os_.str().c_str());
	#endif
	#ifdef TRACE
	std::ofstream f;
	f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
	f << RouteOID.intVal << ',' << myEvc->PredictedCost << ',' << ReserveEvacuationCost << ',' << FinalEvacuationCost << std::endl;
	f.close();
	#endif
	
	// Step the progress bar before continuing to the next Evacuee point
	if (ipStepProgressor) ipStepProgressor->Step();

	// Check to see if the user wishes to continue or cancel the solve (i.e., check whether or not the user has hit the ESC key to stop processing)
	if (pTrackCancel)
	{
		if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
		if (keepGoing == VARIANT_FALSE) return E_ABORT;
	}
	return hr;
}

Evacuee::Evacuee(VARIANT name, double pop, UINT32 objectID)
{
	ObjectID = objectID;
	Name = name;
	Vertices = new DEBUG_NEW_PLACEMENT std::vector<NAVertexPtr>();
	Paths = new DEBUG_NEW_PLACEMENT std::list<EvcPathPtr>();
	Population = pop;
	PredictedCost = FLT_MAX;
	Status = EvacueeStatus::Unprocessed;
	ProcessOrder = -1;
	FinalCost = FLT_MAX;
}

Evacuee::~Evacuee(void)
{
	for (auto & p : *Paths) delete p;
	for (auto v : *Vertices) delete v;
	Paths->clear();
	Vertices->clear();
	delete Vertices;
	delete Paths;
}

EvacueeList::~EvacueeList()
{
	for (const auto & e : *this) delete e;
	clear();
}

void MergeEvacueeClusters(std::unordered_map<long, std::list<EvacueePtr>> & EdgeEvacuee, std::vector<EvacueePtr> & ToErase, double OKDistance)
{
	for (const auto & l : EdgeEvacuee)
	{
		EvacueePtr left = nullptr;
		for (const auto & i : l.second)
		{
			if (left && abs(i->Vertices->front()->GVal - left->Vertices->front()->GVal) <= OKDistance)
			{
				// merge i with left
				ToErase.push_back(i);
				left->Population += i->Population;
			}
			else left = i;
		}
	}
}

void SortedInsertIntoMapOfLists(std::unordered_map<long, std::list<EvacueePtr>> & EdgeEvacuee, long eid, EvacueePtr evc)
{
	auto i = EdgeEvacuee.find(eid);
	if (i == EdgeEvacuee.end())
	{
		EdgeEvacuee.insert(std::pair<long, std::list<EvacueePtr>>(eid, std::list<EvacueePtr>()));
		i = EdgeEvacuee.find(eid);
	}
	auto j = i->second.begin();
	while (j != i->second.end() && evc->Vertices->front()->GVal > (*j)->Vertices->front()->GVal) ++j;
	i->second.insert(j, evc);
}

void EvacueeList::FinilizeGroupings(double OKDistance, bool DynamicCASPEREnabled)
{
	// turn off seperation flag is dynamic capser is enabled
	if (DynamicCASPEREnabled)
	{
		SeperationDisabledForDynamicCASPER = CheckFlag(groupingOption, EvacueeGrouping::Separate);
		groupingOption &= ~EvacueeGrouping::Separate;
	}

	if (CheckFlag(groupingOption, EvacueeGrouping::Merge))
	{
		std::unordered_map<long, EvacueePtr> VertexEvacuee;
		std::unordered_map<long, std::list<EvacueePtr>> EdgeAlongEvacuee, EdgeAgainstEvacuee, DoubleEdgeEvacuee;
		std::vector<EvacueePtr> ToErase;
		NAVertexPtr v1;
		NAEdgePtr e1;
		
		for (const auto & evc : *this)
		{
			v1 = evc->Vertices->front();
			e1 = v1->GetBehindEdge();
			if (!e1) // evacuee mapped to intersection
			{
				auto i = VertexEvacuee.find(v1->EID);
				if (i == VertexEvacuee.end()) VertexEvacuee.insert(std::pair<long, EvacueePtr>(v1->EID, evc));
				else
				{
					ToErase.push_back(evc);
					i->second->Population += evc->Population;
				}
			}
			else if (evc->Vertices->size() == 2) SortedInsertIntoMapOfLists(DoubleEdgeEvacuee, e1->EID, evc);  // evacuee mapped to both side of the street segment
			else if (e1->Direction == esriNetworkEdgeDirection::esriNEDAlongDigitized) SortedInsertIntoMapOfLists(EdgeAlongEvacuee, e1->EID, evc);
			else SortedInsertIntoMapOfLists(EdgeAgainstEvacuee, e1->EID, evc);
		}

		MergeEvacueeClusters(EdgeAgainstEvacuee, ToErase, OKDistance);
		MergeEvacueeClusters(EdgeAlongEvacuee,   ToErase, OKDistance);
		MergeEvacueeClusters(DoubleEdgeEvacuee,  ToErase, OKDistance);

		for (const auto & e : ToErase)
		{
			unordered_erase(e, [&](const EvacueePtr & evc1, const EvacueePtr & evc2)->bool { return evc1->ObjectID == evc2->ObjectID; });
			delete e;
		}
	}

	shrink_to_fit();
}

void NAEvacueeVertexTable::InsertReachable(std::shared_ptr<EvacueeList> list, CARMASort sortDir)
{
	for(const auto & evc : *list)
	{
		if (evc->Status == EvacueeStatus::Unprocessed && evc->Population > 0.0)
		{
			// reset evacuation prediction for continues carma sort
			if (sortDir == CARMASort::BWCont || sortDir == CARMASort::FWCont) evc->PredictedCost = FLT_MAX;

			for (const auto & v : *evc->Vertices)
			{
				if (find(v->EID) == end()) insert(_NAEvacueeVertexTablePair(v->EID, std::vector<EvacueePtr>()));
				at(v->EID).push_back(evc);
			}
		}
	}
}

void NAEvacueeVertexTable::RemoveDiscoveredEvacuees(NAVertex * myVertex, NAEdge * myEdge, std::shared_ptr<std::vector<EvacueePtr>> SortedEvacuees, std::shared_ptr<NAEdgeContainer> leafs, double pop, EvcSolverMethod method)
{
	const auto & pair = find(myVertex->EID);
	NAVertexPtr foundVertex;
	bool AtLeastOneEvacueeFound = false;
	double newPredictedCost = 0.0;

	if (pair != end())
	{
		for (const auto & evc : pair->second)
		{
			foundVertex = nullptr;
			for (const auto & v : *evc->Vertices)
			{
				if (v->EID == myVertex->EID)
				{
					foundVertex = v;
					break;
				}
			}
			if (foundVertex)
			{
				newPredictedCost = myVertex->GVal;
				NAEdgePtr behindEdge = foundVertex->GetBehindEdge();
				if (behindEdge) newPredictedCost += foundVertex->GVal * behindEdge->GetCost(pop, method) / behindEdge->OriginalCost;
				evc->PredictedCost = min(evc->PredictedCost, newPredictedCost);
				SortedEvacuees->push_back(evc);
				AtLeastOneEvacueeFound = true;
			}
		}
		erase(myVertex->EID);
		// because this edge helped us find a new evacuee, we save it as a leaf for the next carma loop
		if (AtLeastOneEvacueeFound) leafs->Insert(myEdge); 
	}
}

void NAEvacueeVertexTable::LoadSortedEvacuees(std::shared_ptr<std::vector<EvacueePtr>> SortedEvacuees) const
{
	#ifdef TRACE
	std::ofstream f;
	f.open("c:\\evcsolver.log", std::ios_base::out | std::ios_base::app);
	f << "List of unreachable evacuees =";
	#endif
	for (const auto & evc : *this)
		for (const auto & e : evc.second)
		{
			if (e->PredictedCost >= FLT_MAX)
			{
				e->Status = EvacueeStatus::Unreachable;
				#ifdef TRACE
				f << ' ' << ATL::CW2A(e->Name.bstrVal);
				#endif
			}
			else SortedEvacuees->push_back(e);
		}
	#ifdef TRACE
	f << std::endl;
	f.close();
	#endif
}

SafeZone::~SafeZone() { delete Vertex; }

SafeZone::SafeZone(INetworkJunctionPtr _junction, NAEdge * _behindEdge, double posAlong, VARIANT cap) : junction(_junction), behindEdge(_behindEdge), positionAlong(posAlong), capacity(0.0)
{
	reservedPop = 0.0;
	Vertex = new DEBUG_NEW_PLACEMENT NAVertex(junction,behindEdge);
	if (cap.vt == VT_R8) capacity = cap.dblVal;
	else if (cap.vt == VT_BSTR) swscanf_s(cap.bstrVal, L"%lf", &capacity);
}

double SafeZone::SafeZoneCost(double population2Route, EvcSolverMethod solverMethod, double costPerDensity, double * globalDeltaCost)
{
	double cost = 0.0;
	double totalPop = population2Route + reservedPop;
	if (capacity == 0.0 && costPerDensity > 0.0) return FLT_MAX;
	if (totalPop > capacity && capacity > 0.0) cost += costPerDensity * ((totalPop / capacity) - 1.0);
	if (behindEdge) cost += behindEdge->GetCost(population2Route, solverMethod, globalDeltaCost) * positionAlong;
	return cost;
}

bool SafeZone::IsRestricted(std::shared_ptr<NAEdgeCache> ecache, NAEdge * leadingEdge, double costPerDensity)
{
	HRESULT hr = S_OK;
	bool restricted = capacity == 0.0 && costPerDensity > 0.0;
	ArrayList<NAEdgePtr> * adj = nullptr;

	if (behindEdge && !restricted)
	{
		restricted = true;
		if (SUCCEEDED(hr = ecache->QueryAdjacencies(Vertex, leadingEdge, QueryDirection::Forward, &adj)))
		{
			for (const auto & currentEdge : *adj) if (NAEdge::IsEqualNAEdgePtr(behindEdge, currentEdge)) restricted = false;
		}
	}
	return restricted;
}

bool SafeZoneTable::insert(SafeZonePtr z)
{
	auto insertRet = std::unordered_map<long, SafeZonePtr>::insert(std::pair<long, SafeZonePtr>(z->Vertex->EID, z));
	if (!insertRet.second) delete z;
	return insertRet.second;
}

bool SafeZoneTable::CheckDiscoveredSafePoint(std::shared_ptr<NAEdgeCache> ecache, NAVertexPtr myVertex, NAEdgePtr myEdge, NAVertexPtr & finalVertex, double & TimeToBeat, SafeZonePtr & BetterSafeZone, double costPerDensity,
	double population2Route, EvcSolverMethod solverMethod, double & globalDeltaCost, bool & foundRestrictedSafezone) const
{
	bool found = false;
	auto i = find(myVertex->EID);

	if (i != end())
	{
		// Handle the last turn restriction here ... and the remaining capacity-aware cost.
		if (!i->second->IsRestricted(ecache, myEdge, costPerDensity))
		{
			double costLeft = i->second->SafeZoneCost(population2Route, solverMethod, costPerDensity, &globalDeltaCost);
			if (TimeToBeat > costLeft + myVertex->GVal + myVertex->GlobalPenaltyCost + globalDeltaCost)
			{
				BetterSafeZone = i->second;
				TimeToBeat = costLeft + myVertex->GVal + myVertex->GlobalPenaltyCost + globalDeltaCost;
				finalVertex = myVertex;
			}
		}
		else
		{
			// found a safe zone but it was restricted
			foundRestrictedSafezone = true;
		}
		found = true;
	}
	return found;
}
