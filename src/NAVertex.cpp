#include "StdAfx.h"
#include "NAVertex.h"
#include "NAEdge.h"
#include "Evacuee.h"

///////////////////////////////////////////////
// NAVertex methods

void NAVertex::Clone(NAVertex * cpy)
{
	GVal = cpy->GVal;
	GlobalPenaltyCost = cpy->GlobalPenaltyCost;
	h = cpy->h;
	Junction = cpy->Junction;
	BehindEdge = nullptr; // cpy->BehindEdge;
	Previous = cpy->Previous;
	EID = cpy->EID;
	isShadowCopy = true;
	ParentCostIsDecreased = cpy->ParentCostIsDecreased;
}

NAVertex::NAVertex(void)
{
	EID = -1;
	Junction = nullptr;
	BehindEdge = nullptr;
	Previous = nullptr;
	GVal = 0.0;
	GlobalPenaltyCost = 0.0;
	h = nullptr;
	isShadowCopy = true;
	ParentCostIsDecreased = false;
}

NAVertex::NAVertex(INetworkJunctionPtr junction, NAEdge * behindEdge)
{
	Previous = nullptr;
	ParentCostIsDecreased = false;
	isShadowCopy = false;
	GVal = 0.0;
	GlobalPenaltyCost = 0.0;
	h = new DEBUG_NEW_PLACEMENT MinimumArrayList<long, double>();
	BehindEdge = behindEdge;

	if (!FAILED(junction->get_EID(&EID)))
	{
		Junction = junction;
	}
	else
	{
		EID = -1;
		Junction = nullptr;
	}
}

inline void NAVertex::SetBehindEdge(NAEdge * behindEdge)
{
	if (!behindEdge && BehindEdge) BehindEdge->ToVertex = nullptr;
	BehindEdge = behindEdge;
	if (BehindEdge) BehindEdge->ToVertex = this;
}

void NAVertex::UpdateYourHeuristic() { UpdateHeuristic(BehindEdge ? BehindEdge->EID : -1, GVal); }
void NAVertex::UpdateHeuristic(long edgeid, double hur) { h->InsertOrUpdate(edgeid, hur); }

void NAVertexCache::UpdateHeuristicForOutsideVertices(double hur, bool goDeep)
{
	if (heuristicForOutsideVertices < hur)
	{
		heuristicForOutsideVertices = hur;
		if (goDeep) for(NAVertexTableItr it = cache->begin(); it != cache->end(); it++) it->second->UpdateHeuristic(-1, hur);
	}
}

NAVertexPtr NAVertexCache::New(INetworkJunctionPtr junction, INetworkQueryPtr ipNetworkQuery)
{
	NAVertexPtr n = nullptr;
	INetworkElementPtr ipJunctionElement;
	INetworkJunctionPtr junctionClone;
	long JunctionEID;
	if (FAILED(junction->get_EID(&JunctionEID))) return nullptr;
	NAVertexTableItr it = cache->find(JunctionEID);

	if (it == cache->end())
	{
		if (ipNetworkQuery)
		{
			if (FAILED(ipNetworkQuery->CreateNetworkElement(esriNETJunction, &ipJunctionElement))) return nullptr;
			junctionClone = ipJunctionElement;
			if (FAILED(ipNetworkQuery->QueryJunction(JunctionEID, junctionClone))) return nullptr;
		}
		else
		{
			junctionClone = junction;
		}
		n = new DEBUG_NEW_PLACEMENT NAVertex(junctionClone, nullptr);
		n->UpdateHeuristic(-1, heuristicForOutsideVertices);
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
	NAVertex * n = nullptr;
	if (currentBucketIndex >= NAVertexCache_BucketSize) currentBucket = nullptr;
	if (!currentBucket)
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
	if (FAILED(junction->get_EID(&JunctionEID))) return nullptr;
	return Get(JunctionEID);
}

NAVertexPtr NAVertexCache::Get(long eid)
{
	NAVertexPtr n = nullptr;
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
	NAVertexPtr temp = nullptr;
	size_t j = 0;
	for(std::vector<NAVertexPtr>::const_iterator i = bucketCache->begin(); i != bucketCache->end(); i++)
	{
		temp = (*i);
		for (j = 0; j < NAVertexCache_BucketSize; ++j) temp[j].SetBehindEdge(nullptr);
		delete [] temp;
		count++;
	}
	currentBucket = nullptr;
	bucketCache->clear();
}

NAVertexPtr NAVertexCollector::New(INetworkJunctionPtr junction)
{
	NAVertexPtr n = new DEBUG_NEW_PLACEMENT NAVertex(junction, nullptr);
	cache->insert(cache->end(), n);
	return n;
}

void NAVertexCollector::Clear()
{
	for(std::vector<NAVertexPtr>::const_iterator cit = cache->begin(); cit != cache->end(); cit++) delete (*cit);
	cache->clear();
}
