#include "stdafx.h"
#include <cmath>
#include "Flocking.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Flocking object implementation

FlockingObject::FlockingObject(EvcPathPtr path, double startTime, VARIANT groupName, INetworkQueryPtr ipNetworkQuery)
{
	SpeedX = 0.0;
	SpeedY = 0.0;
	MyStatus = FLOCK_OBJ_STAT_INIT;
	myPath = path;
	MyTime = startTime;
	GroupName = groupName;
	Traveled = 0.0;
	BindVertex = -1;
	INetworkElementPtr element;
	newEdgeRequestFlag = true;
	speedLimit = 0.0;

	// build the path itterator and upcoming vertices
	myPath->front()->pline->get_FromPoint(&StartPoint);
	myPath->back()->pline->get_ToPoint(&FinalPoint);
	ipNetworkQuery->CreateNetworkElement(esriNETJunction, &element);
	NextVertex = element;
	pathSegIt = myPath->begin();
	(*pathSegIt)->Edge->NetEdge->QueryJunctions(0, NextVertex);

	// steering lib init
	double x, y;
	myVehicle = new OpenSteer::SimpleVehicle();
	myVehicle->reset();
	myVehicle->setRadius(0.1f);
	libpoints = new OpenSteer::Vec3[0];
	MyLocation->QueryCoords(&x, &y);
	myVehicle->setPosition((float)x, (float)y, 0.0f);	
}

bool FlockingObject::LoadNewEdge(void)
{
	if (newEdgeRequestFlag)
	{
		// convert the path to Steer Library format
		IPointCollectionPtr pcollect = 0;
		long pointCount = 0, i = 0;
		IPointPtr p = 0;
		double x, y;

		pathSegIt++;
		if (pathSegIt == myPath->end()) return false;
		
		pcollect = (*pathSegIt)->pline;
		pcollect->get_PointCount(&pointCount);
		delete [] libpoints;
		libpoints = new OpenSteer::Vec3[pointCount];
		for(i = 0; i < pointCount; i++)
		{
			pcollect->get_Point(i, &p);
			p->QueryCoords(&x, &y);
			libpoints[i].set((float)x, (float)y, 0.0f);
		}

		// speed limit update
		(*pathSegIt)->pline->get_Length(&speedLimit);
		speedLimit = speedLimit / (*pathSegIt)->Edge->originalCost;

		// update next vertex based on new edge
		(*pathSegIt)->Edge->NetEdge->QueryJunctions(0, NextVertex);

		// load new path into the steer lib
		myVehiclePath.initialize(pointCount, libpoints, (float)((*pathSegIt)->Edge->OriginalCapacity()), false);
		newEdgeRequestFlag = false;
	}
	return true;
}

bool FlockingObject::BuildNeighborList(std::list<FlockingObjectPtr> * objects)
{
	myNeighborVehicles.clear();
	for (std::list<FlockingObjectPtr>::iterator it = objects->begin(); it != objects->end(); it++)
	{
		// self avoid check
		if ((*it) == this) continue;

		// check if they share an edge or if they are both crossing an intersection
		if ((*(*it)->pathSegIt)->Edge->EID != (*pathSegIt)->Edge->EID || (*(*it)->pathSegIt)->Edge->Direction != (*pathSegIt)->Edge->Direction) continue;

		myNeighborVehicles.push_back((*it)->myVehicle);
	}
	return true;
}

FLOCK_OBJ_STAT FlockingObject::Move(std::list<FlockingObjectPtr> * objects, double dt)
{	
	OpenSteer::Vec3 steer(0,0,0);
	double x, y;

	// check destination arriaval

	// check time
	MyTime += dt;
	dt = min(dt, MyTime);
	if (MyTime >= 0.0) MyStatus = MyStatus = FLOCK_OBJ_STAT_MOVE;
	if (MyTime > 0 && dt > 0)
	{
		LoadNewEdge();
		BuildNeighborList(objects);

		// generate an steer based on current situation
		steer += myVehicle->steerToFollowPath(+1, 1, myVehiclePath);
		steer += myVehicle->steerForSeparation(1.0f,  60.0f, myNeighborVehicles);
		steer += myVehicle->steerForSeparation(0.2f, 270.0f, myNeighborVehicles);

		// use the steer to create speed and finally move
		

		// update coordinate and speed		
		MyLocation->PutCoords(x, y);
		myVehicle->setPosition((float)x, (float)y, 0.0f);
	}
	return MyStatus;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Flocking enviroment implementation

FlockingEnviroment::FlockingEnviroment(double SnapshotInterval, double SimulationInterval)
{
	snapshotInterval = abs(SnapshotInterval);
	simulationInterval = abs(SimulationInterval);
	objects = new std::list<FlockingObjectPtr>();
	history = new std::list<FlockingLocationPtr>();
	maxPathLen = 0.0;
}

FlockingEnviroment::~FlockingEnviroment(void)
{
	for (FlockingObjectItr it1 = objects->begin(); it1 != objects->end(); it1++) delete (*it1);
	for (FlockingLocationItr it2 = history->begin(); it2 != history->end(); it2++) delete (*it2);
	objects->clear();
	history->clear();
	delete objects;	
	delete history;	
}

void FlockingEnviroment::Init(EvacueeList * evcList, INetworkQueryPtr ipNetworkQuery)
{
	EvacueePtr evc = 0;
	EvacueeListItr evcItr;
	int i = 0, size = 0;
	EvcPathPtr path = 0;
	std::list<EvcPathPtr>::iterator pathItr;
	maxPathLen = 0.0;

	// pre-init clean up just in case the object is being re-used
	for (FlockingObjectItr it1 = objects->begin(); it1 != objects->end(); it1++) delete (*it1);
	for (FlockingLocationItr it2 = history->begin(); it2 != history->end(); it2++) delete (*it2);
	objects->clear();
	history->clear();

	for(evcItr = evcList->begin(); evcItr != evcList->end(); evcItr++)
	{		
		for (pathItr = (*evcItr)->paths->begin(); pathItr != (*evcItr)->paths->end(); pathItr++)
		{
			maxPathLen = max(maxPathLen, PathLength(*pathItr));
			size = (int)(ceil((*pathItr)->RoutedPop));
			for (i = 0; i < size; i++)
			{
				objects->push_front(new FlockingObject(*pathItr, simulationInterval * i, (*evcItr)->Name, ipNetworkQuery));
			}
		}
	}
}

HRESULT FlockingEnviroment::RunSimulation(IStepProgressorPtr ipStepProgressor, ITrackCancelPtr pTrackCancel)
{
	bool movingObjectLeft = true;
	FLOCK_OBJ_STAT newStat, oldStat;
	long frontRunnerDistance = 0;
	double lastSnapshot = 0.0;
	bool snapshotTaken = false;
	HRESULT hr = 0;
	VARIANT_BOOL keepGoing;

	if (ipStepProgressor)
	{
		ipStepProgressor->put_MinRange(0);
		ipStepProgressor->put_MaxRange((long)(ceil(maxPathLen)));
		ipStepProgressor->put_StepValue(1);
		ipStepProgressor->put_Position(0);
	}

	for (double time = 0.0; movingObjectLeft; time += simulationInterval)
	{
		for (FlockingObjectItr it = objects->begin(); it != objects->end(); it++)
		{
			if (pTrackCancel)
			{
				if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
				if (keepGoing == VARIANT_FALSE) return E_ABORT;			
			}

			oldStat = (*it)->MyStatus;

			// pre-movement snapshot: check if we have to take a snapshot of this object
			if (oldStat == FLOCK_OBJ_STAT_INIT && (*it)->MyTime >= 0.0) history->push_front(new FlockingLocation(**it));

			newStat = (*it)->Move(objects, simulationInterval);
			movingObjectLeft |= newStat == FLOCK_OBJ_STAT_MOVE;

			// post-movement snapshot
			if ((oldStat != FLOCK_OBJ_STAT_END && newStat == FLOCK_OBJ_STAT_END) ||
				(newStat == FLOCK_OBJ_STAT_MOVE && lastSnapshot + snapshotInterval >= time))
			{
				history->push_front(new FlockingLocation(**it));
				snapshotTaken = true;
			}

			if (ipStepProgressor)
			{
				if ((long)((*it)->Traveled) + 1 >= frontRunnerDistance)
				{
					frontRunnerDistance = (long)((*it)->Traveled);
					ipStepProgressor->Step();
				}
			}
		}
		if (snapshotTaken) lastSnapshot = time;
	}
	return S_OK;
}

void FlockingEnviroment::GetHistory(std::list<FlockingLocationPtr> ** History)
{
	*History = history;
}

double FlockingEnviroment::PathLength(EvcPathPtr path)
{
	double len = 0.0, temp = 0.0;
	for (std::list<PathSegmentPtr>::iterator pathItr = path->begin(); pathItr != path->end(); pathItr++)
	{
		(*pathItr)->pline->get_Length(&temp);
		len += temp;
	}
	return len;
}