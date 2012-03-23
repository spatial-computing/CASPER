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
	GTime = 0.0;
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

HRESULT FlockingObject::buildNeighborList(std::vector<FlockingObjectPtr> * objects)
{
	myNeighborVehicles.clear();
	double dist = 0.0;
	HRESULT hr = S_OK;

	if (MyStatus == FLOCK_OBJ_STAT_END)
	{
		for (FlockingObjectItr it = objects->begin(); it != objects->end(); it++)
		{
			// self avoid check
			if ((*it)->ID != ID) myNeighborVehicles.push_back((*it)->myVehicle);
		}
	}
	else
	{
		if (FAILED(hr = ((IProximityOperatorPtr)(nextVertexPoint))->ReturnDistance(MyLocation, &dist))) return hr;
		if (dist <= 30.0)
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

		for (FlockingObjectItr it = objects->begin(); it != objects->end(); it++)
		{
			// self avoid check
			if ((*it)->ID == ID) continue;

			// moving object check
			if ((*it)->MyStatus != FLOCK_OBJ_STAT_MOVE && (*it)->MyStatus != FLOCK_OBJ_STAT_STOP) continue;

			// check if they share an edge or if they are both crossing an intersection
			if (((*(*it)->pathSegIt)->Edge->EID != (*pathSegIt)->Edge->EID || (*(*it)->pathSegIt)->Edge->Direction != (*pathSegIt)->Edge->Direction)
				&& (BindVertex == -1l || (*it)->BindVertex != BindVertex)) continue;

			myNeighborVehicles.push_back((*it)->myVehicle);
		}
	}
	return hr;
}

HRESULT FlockingObject::Move(std::vector<FlockingObjectPtr> * objects, double dt)
{	
	// check destination arriaval
	HRESULT hr = S_OK;
	OpenSteer::Vec3 steer = OpenSteer::Vec3::zero, pos = OpenSteer::Vec3::zero;
	double dist = 0.0;
	myVehicle->setMaxForce(150000.0/* / (dt * dt)*/);

	// check distance to safe zone
	if (FAILED(hr = ((IProximityOperatorPtr)(MyLocation))->ReturnDistance(FinalPoint, &dist))) return hr;

	if (MyStatus == FLOCK_OBJ_STAT_END)
	{
		if (FAILED(hr = buildNeighborList(objects))) return hr;

		// generate a steer based on current situation
		myVehicle->setMaxSpeed(speedLimit / 2.0);
		steer += myVehicle->steerToAvoidCloseNeighbors (10.0, myNeighborVehicles);
		if (dist < 50.0) steer += myVehicle->steerForWander(dt);
		else steer += myVehicle->steerForSeek(myVehiclePath.points[myVehiclePath.pointCount - 1]);
		myVehicle->applySteeringForce(steer / dt, dt);	
	}
	else
	{
		if (dist < 50.0) MyStatus = FLOCK_OBJ_STAT_END;
		
		MyTime += dt;
		dt = min(dt, MyTime);

		// check time
		if (MyTime > 0 && dt > 0)
		{
			if (FAILED(hr = loadNewEdge())) return hr;
			if (MyStatus == FLOCK_OBJ_STAT_END) return S_OK;
			if (FAILED(hr = buildNeighborList(objects))) return hr;

			// generate a steer based on current situation
			myVehicle->setMaxSpeed(speedLimit);
			myVehicle->setSpeed(speedLimit);

			steer  = 2.0 * myVehicle->steerToAvoidCloseNeighbors (5.0, myNeighborVehicles);
			steer += myVehicle->steerForSeparation(20.0, 60.0, myNeighborVehicles);
			steer += myVehicle->steerToFollowPath(+1, dt, myVehiclePath);
			
			// backup the position in case we needed to back off from a collision
			pos = myVehicle->position();
			myVehicle->applySteeringForce(steer / dt, dt);
			if (DetectMyCollision())
			{
				myVehicle->setPosition(pos);
				MyStatus = FLOCK_OBJ_STAT_STOP;
			}
			else
			{
				Traveled += myVehicle->speed() * dt;
				MyStatus = FLOCK_OBJ_STAT_MOVE;
			}
		}
	}
	// update coordinate and velocity
	pos = myVehicle->position();
	if (FAILED(hr = MyLocation->PutCoords(pos.x, pos.y))) return hr;
	Velocity = myVehicle->velocity();

	return hr;
}

bool FlockingObject::DetectMyCollision()
{
	OpenSteer::AbstractVehicle * n;
	OpenSteer::AVGroup::iterator git;
	OpenSteer::Vec3 offset;
	bool collide = false;

	for (git = myNeighborVehicles.begin(); git != myNeighborVehicles.end(); git++)
	{
		n = *git;
		offset = myVehicle->position() - n->position();
		if (offset.length() < myVehicle->radius() + n->radius()) collide = true;		
	}
	return collide;
}

HRESULT FlockingObject::DetectCollision(std::vector<FlockingObjectPtr> * objects, bool * collided)
{	
	HRESULT hr = S_OK;

	// collision detection
	FlockingObjectPtr n;
	size_t i, k = objects->size();
	
	for (i = 0; i < k; i++)
	{
		n = objects->at(i);
		if (n->MyStatus != FLOCK_OBJ_STAT_MOVE && n->MyStatus != FLOCK_OBJ_STAT_STOP) continue;
		if (n->DetectMyCollision()) *collided = true;
	}
	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Flocking enviroment implementation

FlockingEnviroment::FlockingEnviroment(double SnapshotInterval, double SimulationInterval, bool TwoWayRoadsShareCap, double InitDelayCostPerPop)
{
	snapshotInterval = abs(SnapshotInterval);
	simulationInterval = abs(SimulationInterval);
	objects = new std::vector<FlockingObjectPtr>();
	history = new std::list<FlockingLocationPtr>();
	collisions = new std::list<double>();
	maxPathLen = 0.0;
	initDelayCostPerPop = InitDelayCostPerPop;
	twoWayRoadsShareCap = TwoWayRoadsShareCap;
}

FlockingEnviroment::~FlockingEnviroment(void)
{
	for (FlockingObjectItr it1 = objects->begin(); it1 != objects->end(); it1++) delete (*it1);
	for (FlockingLocationItr it2 = history->begin(); it2 != history->end(); it2++) delete (*it2);
	objects->clear();
	history->clear();
	collisions->clear();
	delete objects;	
	delete history;
	delete collisions;
}

void FlockingEnviroment::Init(EvacueeList * evcList, INetworkQueryPtr ipNetworkQuery, double costPerSec)
{
	EvacueePtr evc = 0;
	EvacueeListItr evcItr;
	int i = 0, size = 0, id = 0;
	EvcPathPtr path = 0;
	std::list<EvcPathPtr>::iterator pathItr;
	maxPathLen = 0.0;
	srand((unsigned int)time(NULL));
	double flockInitGap = ceil(initDelayCostPerPop / costPerSec) * simulationInterval;
	
	// metric projection
	IProjectedCoordinateSystemPtr ipNAContextPC;
	ISpatialReferenceFactoryPtr pSpatRefFact = ISpatialReferenceFactoryPtr(CLSID_SpatialReferenceEnvironment);
	pSpatRefFact->CreateProjectedCoordinateSystem(esriSRProjCS_WGS1984WorldMercator, &ipNAContextPC);
	ISpatialReferencePtr metricProjection = ipNAContextPC;

	// pre-init clean up just in case the enviroment is being re-used
	for (FlockingObjectItr it1 = objects->begin(); it1 != objects->end(); it1++) delete (*it1);
	for (FlockingLocationItr it2 = history->begin(); it2 != history->end(); it2++) delete (*it2);
	objects->clear();
	history->clear();
	collisions->clear();

	for(evcItr = evcList->begin(); evcItr != evcList->end(); evcItr++)
	{		
		for (pathItr = (*evcItr)->paths->begin(); pathItr != (*evcItr)->paths->end(); pathItr++)
		{
			maxPathLen = max(maxPathLen, PathLength(*pathItr));
			size = (int)(ceil((*pathItr)->RoutedPop));
			for (i = 0; i < size; i++)
			{
				objects->push_back(new FlockingObject(id++, *pathItr, flockInitGap * -i, (*evcItr)->Name, ipNetworkQuery, metricProjection));
			}
		}
	}
}

HRESULT FlockingEnviroment::RunSimulation(IStepProgressorPtr ipStepProgressor, ITrackCancelPtr pTrackCancel, double predictedCost)
{
	movingObjectLeft = true;
	FLOCK_OBJ_STAT newStat, oldStat;
	double nextSnapshot = 0.0;
	bool snapshotTaken = false;
	HRESULT hr = S_OK;
	VARIANT_BOOL keepGoing;

	if (ipStepProgressor)
	{
		if (FAILED(hr = ipStepProgressor->put_MinRange(0))) return hr;
		if (FAILED(hr = ipStepProgressor->put_MaxRange((long)(ceil(predictedCost))))) return hr;
		if (FAILED(hr = ipStepProgressor->put_StepValue(1))) return hr;
		if (FAILED(hr = ipStepProgressor->put_Position(0))) return hr;
	}

	// just to make sure we do our best to finish the simulation with no moving object event after the predicted cost
	predictedCost *= 5.0;

	for (double time = 0.0; movingObjectLeft && time <= predictedCost; time += simulationInterval)
	{
		movingObjectLeft = false;
		for (FlockingObjectItr it = objects->begin(); it != objects->end(); it++)
		{
			if (pTrackCancel)
			{
				if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
				if (keepGoing == VARIANT_FALSE) return E_ABORT;			
			}

			oldStat = (*it)->MyStatus;
			(*it)->GTime = time;

			// pre-movement snapshot: check if we have to take a snapshot of this object
			if (oldStat == FLOCK_OBJ_STAT_INIT && (*it)->MyTime >= 0.0) history->push_front(new FlockingLocation(**it));

			if (FAILED(hr = (*it)->Move(objects, simulationInterval))) return hr;
			newStat = (*it)->MyStatus;
			movingObjectLeft |= newStat != FLOCK_OBJ_STAT_END;

			// post-movement snapshot
			if ((oldStat != FLOCK_OBJ_STAT_END && newStat == FLOCK_OBJ_STAT_END) ||
				(newStat != FLOCK_OBJ_STAT_INIT && nextSnapshot <= time))
			{
				history->push_front(new FlockingLocation(**it));
				snapshotTaken = true;
			}
		}
		if (ipStepProgressor)
		{
			if (FAILED(hr = ipStepProgressor->put_Position((long)(ceil(time))))) return hr;
		}
		
		if (snapshotTaken)
		{
			nextSnapshot = time + snapshotInterval;
			snapshotTaken = false;
		}
		bool collided = false;
		FlockingObject::DetectCollision(objects, &collided);
		if (collided) collisions->push_back(time);
	}
	return hr;
}

void FlockingEnviroment::GetResult(std::list<FlockingLocationPtr> ** History, std::list<double> ** collisionTimes, bool * MovingObjectLeft)
{
	*History = history;
	*collisionTimes = collisions;
	*MovingObjectLeft = movingObjectLeft;
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
