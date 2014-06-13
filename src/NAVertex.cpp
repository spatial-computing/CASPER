#include "StdAfx.h"
#include "NAVertex.h"
#include "NAEdge.h"
#include "Evacuee.h"

///////////////////////////////////////////////
// NAVertex methods

NAVertex::NAVertex(const NAVertex& cpy)
{
	GVal = cpy.GVal;
	GlobalPenaltyCost = cpy.GlobalPenaltyCost;
	h = cpy.h;
	Junction = cpy.Junction;
	BehindEdge = cpy.BehindEdge;
	Previous = cpy.Previous;
	EID = cpy.EID;
	isShadowCopy = true;
}

void NAVertex::Clone(NAVertex * cpy)
{
	GVal = cpy->GVal;
	GlobalPenaltyCost = cpy->GlobalPenaltyCost;
	h = cpy->h;
	Junction = cpy->Junction;
	BehindEdge = cpy->BehindEdge;
	Previous = cpy->Previous;
	EID = cpy->EID;
	isShadowCopy = true;
}

NAVertex::NAVertex(void)
{
	EID = -1;
	Junction = 0;
	BehindEdge = 0;
	Previous = 0;
	GVal = 0.0;
	GlobalPenaltyCost = 0.0;
	h = NULL;
	isShadowCopy = true;
}

NAVertex::NAVertex(INetworkJunctionPtr junction, NAEdge * behindEdge)
{
	Previous = 0;
	isShadowCopy = false;
	GVal = 0.0;
	GlobalPenaltyCost = 0.0;
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
	if (behindEdge == NULL && BehindEdge != NULL) BehindEdge->ToVertex = NULL;
	BehindEdge = behindEdge;
	if (BehindEdge != NULL) BehindEdge->ToVertex = this;
}

// return true if update was unnecessary
bool NAVertex::UpdateHeuristic(long edgeid, double hur, unsigned short carmaLoop)
{
	bool unnesecery = false;
	for(std::vector<HValue>::iterator i = h->begin(); i != h->end(); i++)
	{
		if (i->EdgeID == edgeid)
		{
			_ASSERT(carmaLoop >= i->CarmaLoop);
			if (carmaLoop == i->CarmaLoop)
			{
				_ASSERT(hur <= i->Value);
				unnesecery = false;
				i->Value = hur;
			}
			else // carmaLoop > i->CarmaLoop
			{
				_ASSERT(i->Value - hur <= FLT_EPSILON);
				unnesecery = hur - i->Value <= FLT_EPSILON;
				i->Value = hur;
				i->CarmaLoop = carmaLoop;
			}
			if (!unnesecery) std::sort(h->begin(), h->end(), HValue::LessThan);
			return unnesecery;
		}
	}
	h->push_back(HValue(edgeid, hur, carmaLoop));
	std::sort(h->begin(), h->end(), HValue::LessThan);
	_ASSERT(!unnesecery);
	return unnesecery;	
}

// double GetHeapKeyNonHur(const NAEdge * edge) { return AddCostToPenalty(edge->ToVertex->GVal, edge->ToVertex->GlobalPenaltyCost); }
double GetHeapKeyNonHur(const NAEdge * edge) { return edge->ToVertex->GVal; }
double GetHeapKeyHur   (const NAEdge * edge) { return AddCostToPenalty(edge->ToVertex->GVal, edge->ToVertex->GlobalPenaltyCost) + edge->ToVertex->GetMinHOrZero(); }

// return true if update was unnecessary
bool NAVertexCache::UpdateHeuristic(long edgeid, NAVertex * n, unsigned short carmaLoop)
{
	NAVertexPtr a = Get(n->EID);
	return a->UpdateHeuristic(edgeid, n->GVal, carmaLoop);
}

void NAVertexCache::UpdateHeuristicForOutsideVertices(double hur, unsigned short carmaLoop)
{
	bool goDeep = carmaLoop == 1;
	if (heuristicForOutsideVertices < hur)
	{
		heuristicForOutsideVertices = hur;
		if (goDeep) for(NAVertexTableItr it = cache->begin(); it != cache->end(); it++) it->second->UpdateHeuristic(-1, hur, carmaLoop);			
	}
}

NAVertexPtr NAVertexCache::New(INetworkJunctionPtr junction, INetworkQueryPtr ipNetworkQuery)
{
	NAVertexPtr n = 0;
	INetworkElementPtr ipJunctionElement;
	INetworkJunctionPtr junctionClone;
	long JunctionEID;
	if (FAILED(junction->get_EID(&JunctionEID))) return 0;
	NAVertexTableItr it = cache->find(JunctionEID);

	if (it == cache->end())
	{
		if (ipNetworkQuery)
		{
			if (FAILED(ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipJunctionElement))) return 0;
			junctionClone = ipJunctionElement;
			if (FAILED(ipNetworkQuery->QueryJunction(JunctionEID, junctionClone))) return 0;
		}
		else
		{
			junctionClone = junction;
		}
		n = new DEBUG_NEW_PLACEMENT NAVertex(junctionClone, 0);
		n->UpdateHeuristic(-1, heuristicForOutsideVertices, 0);
		cache->insert(NAVertexTablePair(n));
	}
	else
	{
		n = NewFromBucket(it->second);
	}
	return n;
}

NAVertexPtr NAVertexCache::NewFromBucket(NAVertexPtr clone)
{
	NAVertex * n = NULL;
	if (currentBucketIndex >= NAVertexCache_BucketSize) currentBucket = NULL;
	if (currentBucket == NULL)
	{
		currentBucket = new DEBUG_NEW_PLACEMENT NAVertex[NAVertexCache_BucketSize];
		currentBucketIndex = 0;
		bucketCache->push_back(currentBucket);
	}
	
	n = &(currentBucket[currentBucketIndex]);
	++currentBucketIndex;
	n->Clone(clone);

	return n;
}
	
NAVertexPtr NAVertexCache::Get(INetworkJunctionPtr junction)
{
	long JunctionEID;
	if (FAILED(junction->get_EID(&JunctionEID))) return 0;
	return Get(JunctionEID);
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
	CollectAndRelease();
	for(NAVertexTableItr cit = cache->begin(); cit != cache->end(); cit++) delete cit->second;
	cache->clear();
}

void NAVertexCache::PrintVertexHeuristicFeq()
{
	std::wostringstream os_;
	size_t freq[20] = {0};
	
	for(NAVertexTableItr cit = cache->begin(); cit != cache->end(); cit++) freq[cit->second->HCount()]++;
	os_ << "PrintVertexHeuristicFeq:" << std::endl;
	for(size_t i = 0; i < 20; i++)	if (freq[i] > 0) os_ << i << '=' << freq[i] << std::endl;	
	OutputDebugStringW( os_.str().c_str() );
}

void NAVertexCache::CollectAndRelease()
{	
	int count = 0;
	NAVertexPtr temp = NULL;
	size_t j = 0;
	for(std::vector<NAVertexPtr>::iterator i = bucketCache->begin(); i != bucketCache->end(); i++)
	{
		temp = (*i);
		for(j = 0; j < NAVertexCache_BucketSize; ++j) temp[j].SetBehindEdge(0);
		delete [] temp;
		count++; 
	}
	currentBucket = NULL;
	bucketCache->clear();
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
