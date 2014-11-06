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

bool EvcPath::MoreThanPathOrder(const Evacuee * e1, const Evacuee * e2)
{
	return e1->Paths->front()->Order > e2->Paths->front()->Order;
}

void EvcPath::DetachPathsFromEvacuee(Evacuee * evc, EvcSolverMethod method, std::vector<EvcPathPtr> * detachedPaths, NAEdgeMap * touchedEdges)
{
	// It's time to clean up the evacuee object and reset it for the next iteration
	// To do this we first collect all its paths, take away all edge reservations, and then reset some of its fields.
	// at the end keep a record of touched edges for a 'HowDirty' call
	for (const auto & p : *evc->Paths)
	{
		for (auto s = p->crbegin(); s != p->crend(); ++s) (*s)->Edge->RemoveReservation(p, method, true);
		if (touchedEdges) { for (const auto & s : *p) touchedEdges->Insert(s->Edge); }
		if (detachedPaths) detachedPaths->push_back(p); else delete p;
	}
	evc->Paths->clear();
}

void EvcPath::ReattachToEvacuee(EvcSolverMethod method)
{
	for (const auto & s : *this) s->Edge->AddReservation(this, method, true);
	myEvc->Paths->push_back(this);
}

bool EvcPath::DoesItNeedASecondChance(double ThreasholdForFinalCost, std::vector<EvacueePtr> & AffectingList, double ThisIterationMaxCost, EvcSolverMethod method)
{
	double PredictionCostRatio = (ReserveEvacuationCost - myEvc->PredictedCost ) / ThisIterationMaxCost;
	double EvacuationCostRatio = (FinalEvacuationCost   - ReserveEvacuationCost) / ThisIterationMaxCost;
	bool NeedsAChance = PredictionCostRatio > ThreasholdForFinalCost || EvacuationCostRatio > ThreasholdForFinalCost;

	if (NeedsAChance && myEvc->Status == EvacueeStatus::Processed)
	{
		// since the prediction was bad it probably means the evacuee has more than average vehicles so it should be processed sooner
		AffectingList.push_back(myEvc);
		myEvc->Status = EvacueeStatus::Unprocessed;
	}

	if (EvacuationCostRatio > ThreasholdForFinalCost)
	{
		// we have to add the affecting list to be re-routed as well
		// we do this by selecxting the highly congestied and most costly path segment and then extract all the evacuees that share the same segments (edges)
		PathSegmentPtr maxCong = front(), maxCost = front();
		for (const auto & seg : *this)
		{
			if (NAEdge::CostLessThan(maxCost->Edge, seg->Edge, method)) maxCost = seg;
			if (NAEdge::CongestionLessThan(maxCost->Edge, seg->Edge, method)) maxCong = seg;
		}
		std::vector<EvcPathPtr> crossing;
		crossing.reserve(50);
		maxCong->Edge->GetCrossingPaths(crossing);
		if (maxCong != maxCost) maxCost->Edge->GetCrossingPaths(crossing);
		for (const auto & p : crossing)
			if (p->myEvc->Status == EvacueeStatus::Processed)
			{
				AffectingList.push_back(p->myEvc);
				p->myEvc->Status = EvacueeStatus::Unprocessed;
			}
	}

	return NeedsAChance;
}

void EvcPath::AddSegment(EvcSolverMethod method, PathSegmentPtr segment)
{
	this->push_front(segment);
	segment->Edge->AddReservation(this, method);
	double p = abs(segment->GetEdgePortion());
	ReserveEvacuationCost += segment->Edge->GetCurrentCost(method) * p;
	OrginalCost    += segment->Edge->OriginalCost     * p;
}

void EvcPath::CalculateFinalEvacuationCost(double initDelayCostPerPop, EvcSolverMethod method)
{
	FinalEvacuationCost = 0.0;
	for (const auto & pathSegment : *this) FinalEvacuationCost += pathSegment->Edge->GetCurrentCost(method) * abs(pathSegment->GetEdgePortion());
	FinalEvacuationCost += RoutedPop * initDelayCostPerPop;
	myEvc->FinalCost = max(myEvc->FinalCost, FinalEvacuationCost);
}

HRESULT EvcPath::AddPathToFeatureBuffers(ITrackCancel * pTrackCancel, INetworkDatasetPtr ipNetworkDataset, IFeatureClassContainerPtr ipFeatureClassContainer, bool & sourceNotFoundFlag,
	IStepProgressorPtr ipStepProgressor, double & globalEvcCost, double initDelayCostPerPop, IFeatureBufferPtr ipFeatureBufferR, IFeatureBufferPtr ipFeatureBufferE, IFeatureCursorPtr ipFeatureCursorR,
	IFeatureCursorPtr ipFeatureCursorE, long evNameFieldIndex, long evacTimeFieldIndex, long orgTimeFieldIndex, long popFieldIndex,
	long ERRouteFieldIndex, long EREdgeFieldIndex, long EREdgeDirFieldIndex, long ERSeqFieldIndex, long ERFromPosFieldIndex, long ERToPosFieldIndex, long ERCostFieldIndex, bool DoNotExportRouteEdges)
{
	HRESULT hr = S_OK;
	OrginalCost = 0.0;
	FinalEvacuationCost = 0.0;
	IPointCollectionPtr pline = IPointCollectionPtr(CLSID_Polyline);
	long pointCount = -1;
	VARIANT_BOOL keepGoing;
	PathSegmentPtr pathSegment = NULL;
	IGeometryPtr ipGeometry;
	esriGeometryType type;
	IPointCollectionPtr pcollect;
	IPointPtr p;
	VARIANT RouteOID, RouteEdgesOID;

	for (const_iterator psit = begin(); psit != end(); ++psit)
	{
		// Check to see if the user wishes to continue or cancel the solve (i.e., check whether or not the user has hit the ESC key to stop processing)
		if (pTrackCancel)
		{
			if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
			if (keepGoing == VARIANT_FALSE) return E_ABORT;
		}

		// take a path segment from the stack
		pathSegment = *psit;
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

	// now export each path segment into ReouteEdges table
	long seq = 0;
	double segmentCost = RoutedPop * initDelayCostPerPop;
	BSTR dir;
	if (!DoNotExportRouteEdges)
	{
		for (const_iterator psit = begin(); psit != end(); ++psit, ++seq)
		{
			pathSegment = *psit;
			segmentCost += pathSegment->Edge->GetCurrentCost() * abs(pathSegment->GetEdgePortion());
			dir = pathSegment->Edge->Direction == esriNEDAgainstDigitized ? L"Against" : L"Along";

			if (FAILED(hr = ipFeatureBufferE->putref_Shape(pathSegment->pline))) return hr;
			if (FAILED(hr = ipFeatureBufferE->put_Value(ERRouteFieldIndex, RouteOID))) return hr;
			if (FAILED(hr = ipFeatureBufferE->put_Value(EREdgeFieldIndex, ATL::CComVariant(pathSegment->Edge->EID)))) return hr;
			if (FAILED(hr = ipFeatureBufferE->put_Value(EREdgeDirFieldIndex, ATL::CComVariant(dir)))) return hr;
			if (FAILED(hr = ipFeatureBufferE->put_Value(ERSeqFieldIndex, ATL::CComVariant(seq)))) return hr;
			if (FAILED(hr = ipFeatureBufferE->put_Value(ERFromPosFieldIndex, ATL::CComVariant(pathSegment->GetFromRatio())))) return hr;
			if (FAILED(hr = ipFeatureBufferE->put_Value(ERToPosFieldIndex, ATL::CComVariant(pathSegment->GetToRatio())))) return hr;
			if (FAILED(hr = ipFeatureBufferE->put_Value(ERCostFieldIndex, ATL::CComVariant(segmentCost)))) return hr;

			if (FAILED(hr = ipFeatureCursorE->InsertFeature(ipFeatureBufferE, &RouteEdgesOID))) return hr;
		}
	}
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
	for (std::list<EvcPathPtr>::const_iterator it = Paths->begin(); it != Paths->end(); it++) delete (*it);
	for (std::vector<NAVertexPtr>::const_iterator it = Vertices->begin(); it != Vertices->end(); it++) delete (*it);
	Paths->clear();
	Vertices->clear();
	delete Vertices;
	delete Paths;
}

std::vector<EvacueePtr> * NAEvacueeVertexTable::Find(long junctionEID)
{
	std::vector<EvacueePtr> * p = 0;
	NAEvacueeVertexTableItr i = find(junctionEID);
	if (i != end()) p = i->second;
	return p;
}

void NAEvacueeVertexTable::InsertReachable(EvacueeList * list, CARMASort sortDir)
{
	std::vector<EvacueePtr> * p = 0;
	std::vector<EvacueePtr>::const_iterator i;
	std::vector<NAVertexPtr>::const_iterator v;

	for(i = list->begin(); i != list->end(); i++)
	{
		if ((*i)->Status == EvacueeStatus::Unprocessed && (*i)->Population > 0.0)
		{
			if (sortDir == CARMASort::BWCont || sortDir == CARMASort::FWCont) (*i)->PredictedCost = FLT_MAX; // reset evacuation prediction for continues carma sort

			for (v = (*i)->Vertices->begin(); v != (*i)->Vertices->end(); v++)
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
	}
}

void NAEvacueeVertexTable::RemoveDiscoveredEvacuees(NAVertexPtr myVertex, NAEdgePtr myEdge, EvacueeList * SortedEvacuees, NAEdgeContainer * leafs, double pop, EvcSolverMethod method)
{
	NAEvacueeVertexTableItr pair = find(myVertex->EID);
	EvacueePtr evc;
	NAVertexPtr foundVertex;
	bool AtLeastOneEvacueeFound = false;
	double newPredictedCost = 0.0;

	if (pair != end())
	{
		for (std::vector<EvacueePtr>::const_iterator eitr = pair->second->begin(); eitr != pair->second->end(); eitr++)
		{
			evc = (*eitr);
			foundVertex = NULL;
			for (std::vector<NAVertexPtr>::const_iterator v = evc->Vertices->begin(); v != evc->Vertices->end(); v++)
			{
				if ((*v)->EID == myVertex->EID)
				{
					foundVertex = *v;
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
		this->Erase(myVertex->EID);
		if (AtLeastOneEvacueeFound) leafs->Insert(myEdge); // because this edge helped us find a new evacuee, we save it as a leaf for the next carma loop
	}
}

void NAEvacueeVertexTable::LoadSortedEvacuees(EvacueeList * SortedEvacuees) const
{
	for (NAEvacueeVertexTableItr evcItr = begin(); evcItr != end(); evcItr++)
	{
		for (std::vector<EvacueePtr>::const_iterator eit = evcItr->second->begin(); eit != evcItr->second->end(); eit++)
		{
			if ((*eit)->PredictedCost >= FLT_MAX) (*eit)->Status = EvacueeStatus::Unreachable;
			else SortedEvacuees->push_back(*eit);
		}
	}
}

NAEvacueeVertexTable::~NAEvacueeVertexTable()
{
	for (NAEvacueeVertexTableItr i = begin(); i != end(); i++) delete i->second;
	this->clear();
}

void NAEvacueeVertexTable::Erase(long junctionEID)
{
	std::vector<NAVertexPtr>::const_iterator vi;
	NAEvacueeVertexTableItr evcItr1, evcItr2 = this->find(junctionEID);
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

double SafeZone::SafeZoneCost(double population2Route, EvcSolverMethod solverMethod, double costPerDensity, double * globalDeltaCost)
{
	double cost = 0.0;
	double totalPop = population2Route + reservedPop;
	if (capacity == 0.0 && costPerDensity > 0.0) return FLT_MAX;
	if (totalPop > capacity && capacity > 0.0) cost += costPerDensity * ((totalPop / capacity) - 1.0);
	if (behindEdge) cost += behindEdge->GetCost(population2Route, solverMethod, globalDeltaCost) * positionAlong;
	return cost;
}

HRESULT SafeZone::IsRestricted(NAEdgeCache * ecache, NAEdge * leadingEdge, bool & restricted, double costPerDensity)
{
	HRESULT hr = S_OK;
	restricted = capacity == 0.0 && costPerDensity > 0.0;
	NAEdgePtr currentEdge = NULL;
	vector_NAEdgePtr_Ptr adj;

	if (behindEdge && !restricted)
	{
		restricted = true;
		if (FAILED(hr = ecache->QueryAdjacencies(Vertex, leadingEdge, QueryDirection::Forward, adj))) return hr;
		for (std::vector<NAEdgePtr>::const_iterator e = adj->begin(); e != adj->end(); ++e)
		{
			currentEdge = *e;
			if (IsEqual(behindEdge, currentEdge)) restricted = false;
		}
	}
	return hr;
}
