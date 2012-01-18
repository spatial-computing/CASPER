#include "stdafx.h"
#include <cmath>
#include "Flocking.h"
#include <ctime>

////////////////////////////////////////////////////////////////////////////////////////////////
// Flocking object implementation

FlockingObject::FlockingObject(int id, EvcPathPtr path, double startTime, VARIANT groupName, INetworkQueryPtr ipNetworkQuery, ISpatialReferencePtr MetricProjection)
{
	// construct FlockingLocation	
	MyTime = startTime;
	Traveled = 0.0;
	metricProjection = MetricProjection;
	ID = id;

	// init object
	MyStatus = FLOCK_OBJ_STAT_INIT;
	myPath = path;
	GroupName = groupName;
	BindVertex = -1l;
	INetworkElementPtr element;
	newEdgeRequestFlag = true;
	speedLimit = 0.0;
	nextVertexPoint = 0;

	// build the path itterator and upcoming vertices
	myPath->front()->pline->get_FromPoint(&MyLocation);

	myPath->back()->pline->get_ToPoint(&FinalPoint);
	ipNetworkQuery->CreateNetworkElement(esriNETJunction, &element);
	nextVertex = element;
	initPathIterator = true;

	// create a little bit of randomness within initial location and velocity
	double x, y, dx, dy;
	MyLocation->QueryCoords(&x, &y);
	dx = (rand() % 50) - 25;
	dy = (rand() % 50) - 25;
	MyLocation->PutCoords(x + dx, y + dy);
	Velocity = OpenSteer::Vec3(-dx, -dy, 0.0);

	// steering lib init
	libpoints = new OpenSteer::Vec3[0];
	myVehicle = new OpenSteer::SimpleVehicle();
	myVehicle->reset();
	myVehicle->setRadius(0.1);
	myVehicle->setPosition(x + dx, y + dy, 0.0);
	myVehicle->setForward(Velocity.normalize());
	myVehicle->setSpeed(Velocity.length());
	myVehicle->setMaxForce(85000.0);
	myVehicle->setMass(1.0);
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

		if (initPathIterator)
		{
			pathSegIt = myPath->begin();
			initPathIterator = false;
			MyStatus = FLOCK_OBJ_STAT_MOVE;
		}
		else pathSegIt++;

		// check if any edge is left
		if (pathSegIt == myPath->end())
		{
			MyStatus = FLOCK_OBJ_STAT_END;
			return S_OK;
		}
		
		pcollect = (*pathSegIt)->pline;
		if (FAILED(hr = pcollect->get_PointCount(&pointCount))) return hr;
		delete [] libpoints;
		pointCount++;
		libpoints = new OpenSteer::Vec3[pointCount];
		
		// push self location first. this will help the steer library since we're swapping the edge during process
		if (FAILED(hr = MyLocation->QueryCoords(&x, &y))) return hr;
		libpoints[0].set(x, y, 0.0);
		
		for(i = 1; i < pointCount; i++)
		{
			if (FAILED(hr = pcollect->get_Point(i - 1, &p))) return hr;
			if (FAILED(hr = p->QueryCoords(&x, &y))) return hr;
			libpoints[i].set(x, y, 0.0);
		}

		// speed limit update
		if (FAILED(hr = (*pathSegIt)->pline->get_Length(&speedLimit))) return hr;
		speedLimit = speedLimit / (*pathSegIt)->Edge->originalCost;

		if (nextVertexPoint == 0)
		{
			// update next vertex based on the new edge and direction
			nextVertexPoint = IPointPtr(CLSID_Point);
			if (FAILED(hr = (*pathSegIt)->Edge->NetEdge->QueryJunctions(0, nextVertex))) return hr;			
			if (FAILED(hr = nextVertex->QueryPoint(nextVertexPoint))) return hr;
			if (FAILED(hr = nextVertexPoint->Project(metricProjection))) return hr;
		}

		// load new edge points into the steer library
		myVehiclePath.initialize(pointCount, libpoints, (*pathSegIt)->Edge->OriginalCapacity() * 5.0, false);
		newEdgeRequestFlag = false;
	}
	return hr;
}

HRESULT FlockingObject::buildNeighborList(std::list<FlockingObjectPtr> * objects)
{
	myNeighborVehicles.clear();
	double dist = 0.0;
	HRESULT hr = S_OK;

	if (MyStatus == FLOCK_OBJ_STAT_END)
	{
		for (std::list<FlockingObjectPtr>::iterator it = objects->begin(); it != objects->end(); it++)
		{
			// self avoid check
			if ((*it)->ID != ID) myNeighborVehicles.push_back((*it)->myVehicle);
		}
	}
	else
	{
		if (FAILED(hr = ((IProximityOperatorPtr)(nextVertexPoint))->ReturnDistance(MyLocation, &dist))) return hr;
		if (dist <= 20.0)
		{
			if (BindVertex == -1l)
			{
				newEdgeRequestFlag = true;
				if (FAILED(hr = nextVertex->get_EID(&BindVertex))) return hr;
			}
		}
		else
		{
			if (BindVertex != -1l)
			{
				// update next vertex based on the new edge and direction
				if (FAILED(hr = (*pathSegIt)->Edge->NetEdge->QueryJunctions(0, nextVertex))) return hr;			
				if (FAILED(hr = nextVertex->QueryPoint(nextVertexPoint))) return hr;
				if (FAILED(hr = nextVertexPoint->Project(metricProjection))) return hr;
				BindVertex = -1l;
			}
		}

		for (std::list<FlockingObjectPtr>::iterator it = objects->begin(); it != objects->end(); it++)
		{
			// self avoid check
			if ((*it)->ID == ID) continue;

			// moving object check
			if ((*it)->MyStatus != FLOCK_OBJ_STAT_MOVE) continue;

			// check if they share an edge or if they are both crossing an intersection
			if (((*(*it)->pathSegIt)->Edge->EID != (*pathSegIt)->Edge->EID || (*(*it)->pathSegIt)->Edge->Direction != (*pathSegIt)->Edge->Direction)
				&& (BindVertex == -1l || (*it)->BindVertex != BindVertex)) continue;

			myNeighborVehicles.push_back((*it)->myVehicle);
		}
	}
	return hr;
}

HRESULT FlockingObject::Move(std::list<FlockingObjectPtr> * objects, double dt)
{	
	// check destination arriaval
	HRESULT hr = S_OK;
	if (MyStatus == FLOCK_OBJ_STAT_END)
	{
		if (FAILED(hr = buildNeighborList(objects))) return hr;

		// generate a steer based on current situation
		//Velocity.set(0.0, 0.0, 0.0);
		//myVehicle->setSpeed(speedLimit);
		myVehicle->setMaxSpeed(speedLimit / 2.0);
		//Velocity += myVehicle->steerForSeek(myVehiclePath.points[myVehiclePath.pointCount - 1]);
		//Velocity += 2.0 * myVehicle->steerForSeparation(10.0, 360.0, myNeighborVehicles);
		myVehicle->applySteeringForce(myVehicle->steerForSeek(myVehiclePath.points[myVehiclePath.pointCount - 1]) / dt, dt);

		// use the steer to create velocity and finally move
		//Velocity += myVehicle->velocity();
		//Velocity.truncateLength(speedLimit / 2.0);

		// update coordinate and velocity
		OpenSteer::Vec3 pos = myVehicle->position();
		//pos += Velocity * dt;
		if (FAILED(hr = MyLocation->PutCoords(pos.x, pos.y))) return hr;
		//myVehicle->setPosition(pos.x, pos.y, 0.0);
		//myVehicle->setForward(Velocity.normalize());
		//myVehicle->setSpeed(Velocity.length());	
	}
	else
	{
		double dist = 0.0;
		if (FAILED(hr = ((IProximityOperatorPtr)(MyLocation))->ReturnDistance(FinalPoint, &dist))) return hr;
		if (dist <= 50.0) MyStatus = FLOCK_OBJ_STAT_END;
		
		MyTime += dt;
		dt = min(dt, MyTime);

		// check time
		if (MyTime > 0 && dt > 0)
		{
			if (FAILED(hr = loadNewEdge())) return hr;
			if (MyStatus == FLOCK_OBJ_STAT_END) return S_OK;          //////////// ???????
			if (FAILED(hr = buildNeighborList(objects))) return hr;

			// generate a steer based on current situation
			//Velocity.set(0.0, 0.0, 0.0);
			myVehicle->setSpeed(speedLimit);
			myVehicle->setMaxSpeed(speedLimit);

			//// Apply steer for seperation in front
			//Velocity = myVehicle->velocity() + myVehicle->steerForSeparation(10.0, 60.0, myNeighborVehicles);
			//myVehicle->setForward(Velocity.normalize());
			//myVehicle->setSpeed(Velocity.length());

			//// Apply steer for seperation for closeby neighbors
			//Velocity = myVehicle->velocity() + myVehicle->steerForSeparation(1.0, 270.0, myNeighborVehicles);
			//myVehicle->setForward(Velocity.normalize());
			//myVehicle->setSpeed(Velocity.length());

			// Apply steer to follow the path
			//Velocity = myVehicle->velocity() + myVehicle->steerToFollowPath(+1, dt, myVehiclePath);
			myVehicle->applySteeringForce(myVehicle->steerToFollowPath(+1, dt, myVehiclePath) / dt, dt);

			// Finilize velocity and finally move
			//Velocity.truncateLength(speedLimit);
			//myVehicle->setForward(Velocity.normalize());
			//myVehicle->setSpeed(Velocity.length());
			Traveled += myVehicle->speed() * dt;

			// update coordinate and velocity
			OpenSteer::Vec3 pos = myVehicle->position();
			//pos += Velocity * dt;
			if (FAILED(hr = MyLocation->PutCoords(pos.x, pos.y))) return hr;
			//myVehicle->setPosition(pos.x, pos.y, 0.0);
			//myVehicle->setForward(Velocity.normalize());
			//myVehicle->setSpeed(Velocity.length());	
		}				
	}
	Velocity = myVehicle->velocity();
	return hr;
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
	int i = 0, size = 0, id = 0;
	EvcPathPtr path = 0;
	std::list<EvcPathPtr>::iterator pathItr;
	maxPathLen = 0.0;
	srand((unsigned int)time(NULL));
	
	// metric projection
	IProjectedCoordinateSystemPtr ipNAContextPC;
	ISpatialReferenceFactoryPtr pSpatRefFact = ISpatialReferenceFactoryPtr(CLSID_SpatialReferenceEnvironment);
	pSpatRefFact->CreateProjectedCoordinateSystem(esriSRProjCS_WGS1984WorldMercator, &ipNAContextPC);
	ISpatialReferencePtr metricProjection = ipNAContextPC;

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
				objects->push_front(new FlockingObject(id++, *pathItr, simulationInterval * -i, (*evcItr)->Name, ipNetworkQuery, metricProjection));
			}
		}
	}
}

HRESULT FlockingEnviroment::RunSimulation(IStepProgressorPtr ipStepProgressor, ITrackCancelPtr pTrackCancel, double maxCost)
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

	for (double time = 0.0; movingObjectLeft && time <= maxCost; time += simulationInterval)
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
				(newStat != FLOCK_OBJ_STAT_INIT && lastSnapshot + snapshotInterval >= time))
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
