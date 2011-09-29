#include "StdAfx.h"
#include "Evacuee.h"
#include "NAGraph.h"

void NAEvacueeVertexTable::Insert(EvacueeList * list)
{
	std::vector<EvacueePtr> * p = 0;
	std::vector<EvacueePtr>::iterator i;
	std::vector<NAVertexPtr>::iterator v;

	for(i = list->begin(); i != list->end(); i++)
	{
		for (v = (*i)->vertices->begin(); v != (*i)->vertices->end(); v++)
		{
			p = Find((*v)->EID);
			if (!p)
			{
				p = new std::vector<EvacueePtr>();
				insert(_NAEvacueeVertexTablePair((*v)->EID, p));
			}
			p->insert(p->end(), *i);
			s++;
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
}
