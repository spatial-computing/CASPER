#include "stdafx.h"
#include <cmath>
#include "Flocking.h"

///////////////////////////////////////////////////////////
// Flocking object implementation

FlockingObject::FlockingObject(EvcPathPtr path)
{
	SpeedX = 0.0;
	SpeedY = 0.0;
	MyStatus = FLOCK_OBJ_STAT_INIT;
	MyPath = path;
	NextVertex = 0;

	if (MyPath)
	{
		MyEdge = MyPath->front()->Edge;
		
	}
	else
	{
		MyEdge = 0;
		X = 0.0;
		Y = 0.0;
		FinalPoint = 0;
	}
}

FlockingObject::~FlockingObject(void)
{
}

///////////////////////////////////////////////////////////
// Flocking enviroment implementation

FlockingEnviroment::FlockingEnviroment(double SnapshotInterval)
{
	snapshotInterval = SnapshotInterval;
	objects = new std::list<FlockingObjectPtr>();
}

FlockingEnviroment::~FlockingEnviroment(void)
{
	for (FlockingObjectItr it = objects->begin(); it != objects->end(); it++) delete (*it);
	objects->clear();
	delete objects;	
}

void FlockingEnviroment::Init(EvacueeList * evcList)
{
	EvacueePtr evc = 0;
	EvacueeListItr evcItr;
	int i = 0, size = 0;
	EvcPathPtr path = 0;
	std::list<EvcPathPtr>::iterator pathItr;

	for(evcItr = evcList->begin(); evcItr != evcList->end(); evcItr++)
	{		
		for (pathItr = (*evcItr)->paths->begin(); pathItr != (*evcItr)->paths->end(); pathItr++)
		{
			size = (int)(ceil((*pathItr)->RoutedPop));
			for (i = 0; i < size; i++)
			{
				objects->push_front(new FlockingObject(*pathItr));
			}
		}
	}
}

void FlockingEnviroment::RunSimulation(void)
{
}
