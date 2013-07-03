#include "StdAfx.h"
#include "NAVertex.h"
#include "NAEdge.h"
#include "Evacuee.h"

///////////////////////////////////////////////
// NAVertex methods

NAVertex::NAVertex(const NAVertex& cpy)
{
	g = cpy.g;
	//h = new DEBUG_NEW_PLACEMENT std::vector<HValue>(*(cpy.h));
	h = cpy.h;
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
				_ASSERT(hur < i->Value);
				unnesecery = false;
				i->Value = hur;
			}
			else // carmaLoop > i->CarmaLoop
			{
				_ASSERT(i->Value - hur <= FLT_EPSILON);
				unnesecery = i->Value == hur;
				i->Value = hur;
				i->CarmaLoop = carmaLoop;
			}
			if (!unnesecery) std::sort(h->begin(), h->end(), HValue::LessThan);
			return unnesecery;
		}
	}
	h->push_back(HValue(edgeid, hur, carmaLoop));
	std::sort(h->begin(), h->end(), HValue::LessThan);
	return unnesecery;	
}

// return true if update was unnecessary
bool NAVertexCache::UpdateHeuristic(long edgeid, NAVertex * n, unsigned short carmaLoop)
{
	NAVertexPtr a = Get(n->EID);
	return a->UpdateHeuristic(edgeid, n->g, carmaLoop);
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

NAVertexPtr NAVertexCache::New(INetworkJunctionPtr junction)
{
	NAVertexPtr n = 0;
	long JunctionEID;
	if (FAILED(junction->get_EID(&JunctionEID))) return 0;
	NAVertexTableItr it = cache->find(JunctionEID);

	if (it == cache->end())
	{
		n = new DEBUG_NEW_PLACEMENT NAVertex(junction, 0);
		n->UpdateHeuristic(-1, heuristicForOutsideVertices, 0);
		cache->insert(NAVertexTablePair(n));
	}
	else
	{
		n = new DEBUG_NEW_PLACEMENT NAVertex(*(it->second));
		sideCache->push_back(n);
	}
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
	for(NAVertexTableItr cit = cache->begin(); cit != cache->end(); cit++)
	{
		cit->second->ReleaseH();
		delete cit->second;
	}
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
	for(std::vector<NAVertexPtr>::iterator i = sideCache->begin(); i != sideCache->end(); i++)
	{
		(*i)->SetBehindEdge(0);
		delete (*i);
		count++; 
	}
	sideCache->clear();
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
