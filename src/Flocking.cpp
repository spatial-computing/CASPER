#include "stdafx.h"
#include <cmath>
#include "Flocking.h"
#include <ctime>

////////////////////////////////////////////////////////////////////////////////////////////////
// Flocking object implementation

FlockingObject::FlockingObject(EvcPathPtr path, float startTime, VARIANT groupName, INetworkQueryPtr ipNetworkQuery)
{
	// construct FlockingLocation
	Velocity = OpenSteer::Vec3::zero;
	MyTime = startTime;
	Traveled = 0.0;

	// init object
	MyStatus = FLOCK_OBJ_STAT_INIT;
	myPath = path;
	GroupName = groupName;
	BindVertex = -1l;
	INetworkElementPtr element;
	newEdgeRequestFlag = true;
	speedLimit = 0.0;

	// build the path itterator and upcoming vertices
	myPath->front()->pline->get_FromPoint(&MyLocation);

	myPath->back()->pline->get_ToPoint(&FinalPoint);
	ipNetworkQuery->CreateNetworkElement(esriNETJunction, &element);
	NextVertex = element;
	pathSegIt = myPath->begin();

	// create a little bit of randomness within initial location
	double x, y;
	MyLocation->QueryCoords(&x, &y);
	x += (rand() % 20) - 10;
	y += (rand() % 20) - 10;
	MyLocation->PutCoords(x, y);

	// steering lib init
	myVehicle = new OpenSteer::SimpleVehicle();
	myVehicle->reset();
	myVehicle->setRadius(0.1f);
	libpoints = new OpenSteer::Vec3[0];
	myVehicle->setPosition((float)x, (float)y, 0.0f);
}

HRESULT FlockingObject::loadNewEdge(void)
{
	HRESULT hr = S_OK;
	if (newEdgeRequestFlag)
	{
		// convert the path to Steer Library format
		IPointCollectionPtr pcollect = 0;
		long pointCount = 0, i = 0;
		IPointPtr p = 0;
		double x, y;

		// check if any edge is left
		if (pathSegIt == myPath->end())
		{
			MyStatus = FLOCK_OBJ_STAT_END;
			return S_OK;
		}
		
		pcollect = (*pathSegIt)->pline;
		if (FAILED(hr = pcollect->get_PointCount(&pointCount))) return hr;
		delete [] libpoints;
		libpoints = new OpenSteer::Vec3[pointCount];
		for(i = 0; i < pointCount; i++)
		{
			if (FAILED(hr = pcollect->get_Point(i, &p))) return hr;
			if (FAILED(hr = p->QueryCoords(&x, &y))) return hr;
			libpoints[i].set((float)x, (float)y, 0.0f);
		}

		// speed limit update
		if (FAILED(hr = (*pathSegIt)->pline->get_Length(&speedLimit))) return hr;
		speedLimit = speedLimit / (*pathSegIt)->Edge->originalCost;

		// update next vertex based on the new edge
		if (FAILED(hr = (*pathSegIt)->Edge->NetEdge->QueryJunctions(0, NextVertex))) return hr;

		// load new path into the steer lib
		myVehiclePath.initialize(pointCount, libpoints, (float)((*pathSegIt)->Edge->OriginalCapacity()), false);
		newEdgeRequestFlag = false;
		pathSegIt++;
	}
	return hr;
}

HRESULT FlockingObject::buildNeighborList(std::list<FlockingObjectPtr> * objects)
{
	myNeighborVehicles.clear();
	double dist = 300.0;
	HRESULT hr = S_OK;
	IPointPtr nextVertexPoint(CLSID_Point);

	if (FAILED(hr = NextVertex->QueryPoint(nextVertexPoint))) return hr;	
	// if (FAILED(hr = ((IProximityOperatorPtr)(MyLocation))->ReturnDistance(nextVertexPoint, &dist))) return hr;
	if (dist <= 200.0) NextVertex->get_EID(&BindVertex); else BindVertex = -1l;

	for (std::list<FlockingObjectPtr>::iterator it = objects->begin(); it != objects->end(); it++)
	{
		// self avoid check
		if ((*it) == this) continue;

		// moving object check
		if ((*it)->MyStatus != FLOCK_OBJ_STAT_MOVE) continue;

		// check if they share an edge or if they are both crossing an intersection
		if (((*(*it)->pathSegIt)->Edge->EID != (*pathSegIt)->Edge->EID || (*(*it)->pathSegIt)->Edge->Direction != (*pathSegIt)->Edge->Direction)
			&& (BindVertex != -1l || (*it)->BindVertex != BindVertex)) continue;

		myNeighborVehicles.push_back((*it)->myVehicle);
	}
	return hr;
}

HRESULT FlockingObject::Move(std::list<FlockingObjectPtr> * objects, float dt)
{	
	// check destination arriaval
	HRESULT hr = S_OK;
	if (MyStatus != FLOCK_OBJ_STAT_END)
	{
		double dist = 0.0;
		if (FAILED(hr = ((IProximityOperatorPtr)(MyLocation))->ReturnDistance(FinalPoint, &dist))) return hr;
		if (dist <= 200.0) MyStatus = FLOCK_OBJ_STAT_END;
		else
		{
			MyTime += dt;
			dt = min(dt, MyTime);

			// check time
			if (MyTime >= 0.0) MyStatus = FLOCK_OBJ_STAT_MOVE;
			if (MyTime > 0 && dt > 0)
			{
				if (FAILED(hr = loadNewEdge())) return hr;
				if (MyStatus == FLOCK_OBJ_STAT_END) return S_OK;
				if (FAILED(hr = buildNeighborList(objects))) return hr;

				// generate a steer based on current situation
				Velocity.set(0.0f, 0.0f, 0.0f);
				Velocity += myVehicle->steerForSeparation(1.0f,  60.0f, myNeighborVehicles);		
				Velocity += myVehicle->steerForSeparation(0.2f, 270.0f, myNeighborVehicles);
				Velocity += myVehicle->steerToFollowPath(+1, dt, myVehiclePath);

				// use the steer to create velocity and finally move
				OpenSteer::Vec3 pos = OpenSteer::Vec3::zero;
				pos += myVehicle->position();
				Velocity += myVehicle->velocity();
				Velocity.truncateLength((float)speedLimit);				
				Traveled += Velocity.length() * dt;
				pos += Velocity * dt;

				// update coordinate and velocity
				if (FAILED(hr = MyLocation->PutCoords(pos.x, pos.y))) return hr;
				myVehicle->setPosition(pos.x, pos.y, 0.0f);
				myVehicle->setForward(Velocity.normalize());
				myVehicle->setSpeed(Velocity.length());
			}
		}
	}
	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Flocking enviroment implementation

FlockingEnviroment::FlockingEnviroment(float SnapshotInterval, float SimulationInterval)
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
	srand((unsigned int)time(NULL));

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
				objects->push_front(new FlockingObject(*pathItr, simulationInterval * -i, (*evcItr)->Name, ipNetworkQuery));
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
	HRESULT hr = S_OK;
	VARIANT_BOOL keepGoing;

	if (ipStepProgressor)
	{
		if (FAILED(hr = ipStepProgressor->put_MinRange(0))) return hr;
		if (FAILED(hr = ipStepProgressor->put_MaxRange((long)(ceil(maxPathLen))))) return hr;
		if (FAILED(hr = ipStepProgressor->put_StepValue(1))) return hr;
		if (FAILED(hr = ipStepProgressor->put_Position(0))) return hr;
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

			if (FAILED(hr = (*it)->Move(objects, simulationInterval))) return hr;
			newStat = (*it)->MyStatus;
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
					if (FAILED(hr = ipStepProgressor->Step())) return hr;
				}
			}
		}
		if (snapshotTaken) lastSnapshot = time;
	}
	return hr;
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