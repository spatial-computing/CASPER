#include "stdafx.h"
#include <cmath>
#include "Flocking.h"

///////////////////////////////////////////////////////////
// Flocking object implementation

FlockingObject::FlockingObject(EvcPathPtr path, double startTime)
{
	SpeedX = 0.0;
	SpeedY = 0.0;
	MyStatus = FLOCK_OBJ_STAT_INIT;
	MyPath = path;
	NextVertex = 0;
	MyTime = 0;
	StartTime = startTime;

	if (MyPath)
	{
		MyEdge = MyPath->front()->Edge;
		MyPath->front()->pline->get_FromPoint(&StartPoint);
		MyPath->back()->pline->get_ToPoint(&FinalPoint);
	}
	else
	{
		MyEdge = 0;
		MyLocation = 0;
		FinalPoint = 0;
		StartPoint = 0;
	}
}

FlockingObject::~FlockingObject(void)
{
}

///////////////////////////////////////////////////////////
// Flocking enviroment implementation

FlockingEnviroment::FlockingEnviroment(double SnapshotInterval, double SimulationInterval)
{
	snapshotInterval = SnapshotInterval;
	simulationInterval = SimulationInterval;
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
				objects->push_front(new FlockingObject(*pathItr, simulationInterval * i));
			}
		}
	}
}

void FlockingEnviroment::RunSimulation(void)
{
}
