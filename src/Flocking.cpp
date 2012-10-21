#include "stdafx.h"
#include <cmath>
#include "Flocking.h"
#include <ctime>

////////////////////////////////////////////////////////////////////////////////////////////////
// Flocking object implementation

FlockingObject::FlockingObject(int id, EvcPathPtr path, double startTime, VARIANT groupName, INetworkQueryPtr ipNetworkQuery,
							   FlockProfile * flockProfile, bool TwoWayRoadsShareCap, std::vector<FlockingObject *> * neighbors, double pathLen)
{
	// construct FlockingLocation	
	HRESULT hr = S_OK;
	MyTime = startTime;
	GTime = 0.0;
	Traveled = 0.0;
	ID = id;
	myProfile = flockProfile;
	twoWayRoadsShareCap = TwoWayRoadsShareCap;
	PathLen = pathLen;

	// init object
	MyStatus = FLOCK_OBJ_STAT_INIT;
	myPath = path;
	GroupName = groupName;
	BindVertex = -1l;
	INetworkElementPtr element;
	newEdgeRequestFlag = true;
	speedLimit = 0.0;

	// build the path itterator and upcoming vertices
	if (FAILED(hr = myPath->front()->pline->get_FromPoint(&MyLocation)))
	{
		OutputDebugString(L"FlockingObject - get_FromPoint: failed to get start point.");
	}

	// network element to keep intersection vertex
	if (FAILED(hr = ipNetworkQuery->CreateNetworkElement(esriNETJunction, &element)))
	{
		OutputDebugString(L"FlockingObject - CreateNetworkElement: failed to create a junction element.");		
	}
	nextVertex = element;
	initPathIterator = true;

	// steering lib init
	libpoints = new OpenSteer::Vec3[0];
	myVehicle = new OpenSteer::SimpleVehicle();
	myVehicle->reset();
	myVehicle->setRadius(myProfile->Radius * 4.0);
	
	myVehicle->setForward(Velocity.normalize());
	myVehicle->setSpeed(Velocity.length());
	myVehicle->setMass(myProfile->Mass);

	// init location
	double x, y, dx, dy;
	MyLocation->QueryCoords(&x, &y);
	GetMyInitLocation(neighbors, x, y, dx, dy);
	MyLocation->PutCoords(x + dx, y + dy);
	Velocity = OpenSteer::Vec3(-dx, -dy, 0.0);

	// steering lib modify
	myVehicle->setRadius(myProfile->Radius);
	myVehicle->setPosition(x + dx, y + dy, 0.0);
	myVehicle->setForward(Velocity.normalize());
	myVehicle->setSpeed(Velocity.length());

	// finish line construction
	IPointPtr point;
	IPointCollectionPtr pcollect = myPath->back()->pline;
	long pointCount = 0;

	if (FAILED(hr = pcollect->get_PointCount(&pointCount)))
	{		
		OutputDebugString(L"FlockingObject - get_PointCount: failed to get number of path points.");
		_ASSERT(0);
	}
	if (pointCount > 0)
	{
		if (FAILED(hr = pcollect->get_Point(pointCount - 1, &point)))
		{
			OutputDebugString(L"FlockingObject - get_Point: failed to get end path point.");
			_ASSERT(0);
		}
		if (point)
		{
			if (FAILED(hr = point->QueryCoords(&x, &y)))
			{
				OutputDebugString(L"FlockingObject - QueryCoords: failed to get end path point coordinates.");
				_ASSERT(0);
			}
			finishPoint.set(x, y, 0.0);
		}
	}
}

void FlockingObject::GetMyInitLocation(std::vector<FlockingObject *> * neighbors, double x1, double y1, double & dx, double & dy)
{
	myNeighborVehicles.clear();
	// HRESULT hr = S_OK;
	bool possibleCollision = true;
	IPointPtr p = 0;
	double x2, y2, step = myVehicle->radius() * 4.0;
	((IPointCollectionPtr)(myPath->front()->pline))->get_Point(1, &p);
	p->QueryCoords(&x2, &y2);

	OpenSteer::Vec3 loc(x1, y1, 0.0);
	OpenSteer::Vec3 move(x2 - x1, y2 - y1, 0.0);
	// if (move.length() == 0.0) move = OpenSteer::Vec3::up;
	_ASSERT(move.length() > 0.0);
	move = move.normalize();
	OpenSteer::Vec3 dir;
	dir.cross(move, OpenSteer::Vec3(0.0, 0.0, 1.0));

	for (FlockingObjectItr it = neighbors->begin(); it != neighbors->end(); it++)
	{
		// same group check
		if (wcscmp((*it)->GroupName.bstrVal, GroupName.bstrVal) == 0) myNeighborVehicles.push_back((*it)->myVehicle);
	}

	// create a little bit of randomness within initial location and velocity while avoiding collision
	for (double radius = 0.0; possibleCollision; radius += step)
	{	
		dx = radius + DoubleRangedRand(0.0, step);
		// if (rand() < RAND_MAX / 2) dx *= -1.0;
		dy = DoubleRangedRand(0.0, max(step, radius));
		myVehicle->setPosition(loc + dx * dir - dy * move);
		possibleCollision = DetectMyCollision();
	}
	dx = myVehicle->position().x - x1;
	dy = myVehicle->position().y - y1;
	myNeighborVehicles.clear();
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
		
		for(i = 1; i < pointCount; i++)
		{
			if (FAILED(hr = pcollect->get_Point(i - 1, &p))) return hr;
			if (FAILED(hr = p->QueryCoords(&x, &y))) return hr;
			libpoints[i].set(x, y, 0.0);
		}
		
		// push self location first. this will help the steer library since we're swapping the edge during process.
		// we'll use the current location shadow on road segment as the start point.
		OpenSteer::Vec3 o12 = (libpoints[2] - libpoints[1]).normalize();
		libpoints[0] = libpoints[1] - (max(1.0, o12.dot(libpoints[1] - myVehicle->position())) * o12);
		_ASSERT(libpoints[0] != libpoints[1]);

		// speed limit update
		if (FAILED(hr = (*pathSegIt)->pline->get_Length(&speedLimit))) return hr;
		speedLimit = speedLimit / (*pathSegIt)->Edge->OriginalCost;

		if (FAILED(hr = (*pathSegIt)->Edge->NetEdge->QueryJunctions(0, nextVertex))) return hr;

		// load new edge points into the steer library
		myVehiclePath.initialize(pointCount, libpoints, (*pathSegIt)->Edge->OriginalCapacity() * myProfile->Radius * 1.2, false);
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
		dist = OpenSteer::Vec3::distance(myVehicle->position(), myVehiclePath.points[myVehiclePath.pointCount - 1]);
		if (dist < myProfile->IntersectionRadius)
		{
			newEdgeRequestFlag = true;
			if (FAILED(hr = nextVertex->get_EID(&BindVertex))) return hr;
		}

		for (FlockingObjectItr it = objects->begin(); it != objects->end(); it++)
		{
			// self avoid check
			if ((*it)->ID == ID) continue;

			// moving object check
			if ((*it)->MyStatus == FLOCK_OBJ_STAT_INIT || (*it)->MyStatus == FLOCK_OBJ_STAT_END) continue;

			// check if they share an edge or if they are both crossing an intersection
			/*
			if (twoWayRoadsShareCap)
			{
				if (((*(*it)->pathSegIt)->Edge->EID != (*pathSegIt)->Edge->EID) && (BindVertex == -1l || (*it)->BindVertex != BindVertex)) continue;
			}
			else
			{
				if (((*(*it)->pathSegIt)->Edge->EID != (*pathSegIt)->Edge->EID || (*(*it)->pathSegIt)->Edge->Direction != (*pathSegIt)->Edge->Direction)
					&& (BindVertex == -1l || (*it)->BindVertex != BindVertex)) continue;
			}
			*/
			myNeighborVehicles.push_back((*it)->myVehicle);
		}
	}
	return hr;
}

HRESULT FlockingObject::Move(std::vector<FlockingObjectPtr> * objects, double dt)
{	
	// check destination arriaval
	HRESULT hr = S_OK;
	OpenSteer::Vec3 steer = OpenSteer::Vec3::zero, pos = OpenSteer::Vec3::zero, dir = OpenSteer::Vec3::zero;
	double dist = 0.0;
	myVehicle->setMaxForce(myProfile->MaxForce);
	dist = OpenSteer::Vec3::distance(myVehicle->position(), finishPoint);

	if (MyStatus == FLOCK_OBJ_STAT_END)
	{
		// check distance to safe zone
		if (FAILED(hr = buildNeighborList(objects))) return hr;

		// generate a steer based on current situation
		myVehicle->setMaxSpeed(speedLimit / 2.0);
		steer += myVehicle->steerToAvoidCloseNeighbors (myProfile->CloseNeighborDistance, myNeighborVehicles);
		if (dist < myProfile->ZoneRadius) steer += myVehicle->steerForWander(dt, 20);
		else steer += myVehicle->steerForSeek(myVehiclePath.points[myVehiclePath.pointCount - 1], dt);
			
		// backup the position in case we needed to back off from a collision
		pos = myVehicle->position();
		myVehicle->applySteeringForce(steer / dt, dt);
		if (DetectMyCollision()) myVehicle->setPosition(pos);
	}
	else
	{
		if (dist < myProfile->ZoneRadius) MyStatus = FLOCK_OBJ_STAT_END;
		MyTime += dt;
		dt = min(dt, MyTime);

		// check time
		if (MyTime > 0 && dt > 0)
		{
			if (FAILED(hr = loadNewEdge())) return hr;
			if (MyStatus == FLOCK_OBJ_STAT_END) return S_OK;
			if (FAILED(hr = buildNeighborList(objects))) return hr;

			myVehicle->setMaxSpeed(speedLimit);
			if (MyStatus != FLOCK_OBJ_STAT_STOP) myVehicle->setSpeed(speedLimit);
			else 
			{
				OpenSteer::Vec3 forward = OpenSteer::RandomVectorInUnitRadiusSphere();
				forward.z = 0.0;
				myVehicle->setForward(forward.normalize());
				myVehicle->setSpeed(speedLimit / 2.0);
			}

			// generate a steer based on current situation
			// this would be replaced by steerToAvoidNeighbors
			// steer  = myVehicle->steerToAvoidCloseNeighbors(myProfile->CloseNeighborDistance, myNeighborVehicles);

			// sperates you form boids in front
			steer += myVehicle->steerForSeparation(myProfile->NeighborDistance, 60.0, myNeighborVehicles);
			steer += myVehicle->steerToAvoidNeighbors(dt, myNeighborVehicles);

			// to stay inside the path. if last round we had to stop to avoid collision, this round we only focus on avoid neighbors.
			if (MyStatus != FLOCK_OBJ_STAT_STOP) steer += myVehicle->steerToFollowPath(+1, dt, myVehiclePath);
			
			// backup the position in case we needed to back off from a collision
			pos = myVehicle->position();
			dir = myVehicle->forward();
			myVehicle->applySteeringForce(steer / dt, dt);
			if (DetectMyCollision())
			{
				myVehicle->setPosition(pos);
				myVehicle->setForward(dir);
				myVehicle->setSpeed(0.0);
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
	bool collided = false;

	for (git = myNeighborVehicles.begin(); git != myNeighborVehicles.end(); git++)
	{
		n = *git;
		offset = myVehicle->position() - n->position();
		if (offset.length() <= myVehicle->radius() + n->radius())
		{
			collided = true;
			break;
		}
	}
	return collided;
}

bool FlockingObject::DetectCollision(std::vector<FlockingObjectPtr> * objects)
{	
	bool collided = false;

	// collision detection
	FlockingObjectPtr n;
	size_t i, k = objects->size();
	
	for (i = 0; i < k; i++)
	{
		n = objects->at(i);
		if (n->MyStatus == FLOCK_OBJ_STAT_INIT || n->MyStatus == FLOCK_OBJ_STAT_END) continue;
		if (n->DetectMyCollision())
		{
			collided = true;
			n->MyStatus = FLOCK_OBJ_STAT_COLLID;
		}
	}
	return collided;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Flocking enviroment implementation

FlockingEnviroment::FlockingEnviroment(double SnapshotInterval, double SimulationInterval, double InitDelayCostPerPop)
{
	snapshotInterval = abs(SnapshotInterval);
	simulationInterval = abs(SimulationInterval);
	objects = new std::vector<FlockingObjectPtr>();
	history = new std::vector<FlockingLocationPtr>();
	collisions = new std::list<double>();
	maxPathLen = 0.0;
	minPathLen = 0.0;
	initDelayCostPerPop = InitDelayCostPerPop;
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

void FlockingEnviroment::Init(EvacueeList * evcList, INetworkQueryPtr ipNetworkQuery, double costPerSec, FlockProfile * flockProfile, bool TwoWayRoadsShareCap)
{
	EvacueeListItr evcItr;
	int i = 0, size = 0, id = 0;
	double pathLen = 0.0;
	std::list<EvcPathPtr>::iterator pathItr;
	maxPathLen = 0.0;
	minPathLen = DBL_MAX;
	srand((unsigned int)time(NULL));

	// pre-init clean up just in case the enviroment is being re-used
	for (FlockingObjectItr it1 = objects->begin(); it1 != objects->end(); it1++) delete (*it1);
	for (FlockingLocationItr it2 = history->begin(); it2 != history->end(); it2++) delete (*it2);
	objects->clear();
	history->clear();
	collisions->clear();

	for(evcItr = evcList->begin(); evcItr != evcList->end(); evcItr++)
	{
		if (!((*evcItr)->paths->empty()))
		{
			for (pathItr = (*evcItr)->paths->begin(); pathItr != (*evcItr)->paths->end(); pathItr++)
			{
				pathLen = PathLength(*pathItr);
				maxPathLen = max(maxPathLen, pathLen);
				minPathLen = min(minPathLen, pathLen);
				size = (int)(ceil((*pathItr)->RoutedPop));
				for (i = 0; i < size; i++)
				{
					objects->push_back(new FlockingObject(id++, *pathItr, initDelayCostPerPop * -i, (*evcItr)->Name, ipNetworkQuery, flockProfile, TwoWayRoadsShareCap, objects, pathLen));
				}
			}
		}
	}
}

HRESULT FlockingEnviroment::RunSimulation(IStepProgressorPtr ipStepProgressor, ITrackCancelPtr pTrackCancel, double predictedCost)
{
	movingObjectLeft = true;
	FlockingObjectPtr fo = NULL;
	FLOCK_OBJ_STAT newStat, oldStat;
	double nextSnapshot = 0.0, minDistLeft = maxPathLen + 1.0, maxDistLeft = 0.0, distLeft = 0.0, progressValue = 0.0;
	long lastReportedProgress = 0l;
	bool snapshotTaken = false;
	int objPos = 0, objStart = 0, objEnd = 0, objStep = 1;
	HRESULT hr = S_OK;
	VARIANT_BOOL keepGoing;
	std::vector<FlockingObjectPtr> * snapshotTempList = new std::vector<FlockingObjectPtr>();

	if (ipStepProgressor)
	{
		if (FAILED(hr = ipStepProgressor->put_MinRange(0))) return hr;
		if (FAILED(hr = ipStepProgressor->put_MaxRange(100))) return hr;
		if (FAILED(hr = ipStepProgressor->put_StepValue(1))) return hr;
		if (FAILED(hr = ipStepProgressor->put_Position(0))) return hr;
	}

	// just to make sure we do our best to finish the simulation with no moving object event after the predicted cost
	predictedCost *= 4.0;

	for (double thetime = simulationInterval; movingObjectLeft && thetime <= predictedCost; thetime += simulationInterval)
	{
		movingObjectLeft = false;
		if (objStep == 1)
		{
			objStep = -1;
			objStart = objects->size() - 1;
			objEnd = -1;
		}
		else
		{
			objStep = 1;
			objStart = 0;
			objEnd = objects->size();
		}

		for (objPos = objStart; objPos != objEnd; objPos += objStep)
		{
			if (pTrackCancel)
			{
				if (FAILED(hr = pTrackCancel->Continue(&keepGoing))) return hr;
				if (keepGoing == VARIANT_FALSE) return E_ABORT;			
			}
			fo = objects->at(objPos);

			fo->GTime = thetime;
			oldStat = fo->MyStatus;
			if (FAILED(hr = fo->Move(objects, simulationInterval))) return hr;
			newStat = fo->MyStatus;
			distLeft = max(0.0, fo->PathLen - fo->Traveled);
			minDistLeft = min(minDistLeft, distLeft);
			maxDistLeft = max(maxDistLeft, distLeft);

			// Check if we have to take a snapshot of this object
			if ((oldStat == FLOCK_OBJ_STAT_INIT && newStat != FLOCK_OBJ_STAT_INIT) || // pre-movement snapshot
				(oldStat != FLOCK_OBJ_STAT_END && newStat == FLOCK_OBJ_STAT_END)) // post-movement snapshot
			{
				snapshotTempList->push_back(fo);
			}
			else if (newStat != FLOCK_OBJ_STAT_INIT && nextSnapshot <= thetime)
			{
				snapshotTempList->push_back(fo);
				snapshotTaken = true;
			}

			movingObjectLeft |= newStat != FLOCK_OBJ_STAT_END;
		}

		// see if any collisions happended and update status if nessecery
		if (FlockingObject::DetectCollision(objects)) collisions->push_back(thetime);

		// flush the snapshot objects into history
		for (FlockingObjectItr it = snapshotTempList->begin(); it != snapshotTempList->end(); it++)
		{
			history->push_back(new FlockingLocation(**it));
		}
		snapshotTempList->clear();

		if (snapshotTaken)
		{
			nextSnapshot = thetime + snapshotInterval;
			snapshotTaken = false;
		}

		// progress bar is based on a combination of first evacuee saved and last evacuee saved.
		if (ipStepProgressor)
		{
			progressValue = 50.0 * ((1.0 - (minDistLeft / minPathLen)) + (1.0 - (maxDistLeft / maxPathLen)));
			if (progressValue >= lastReportedProgress + 1l)
			{
				if (FAILED(hr = ipStepProgressor->Step())) return hr;
				lastReportedProgress++;
			}
			maxDistLeft = 0.0;
		}
	}
	delete snapshotTempList;
	return hr;
}

void FlockingEnviroment::GetResult(std::vector<FlockingLocationPtr> ** History, std::list<double> ** collisionTimes, bool * MovingObjectLeft)
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

double PointToLineDistance(OpenSteer::Vec3 point, OpenSteer::Vec3 line[2], bool shouldRotateLine, bool DirAsSign)
{
	double dist = 0.0;
	OpenSteer::Vec3 distV, lineVector = line[1] - line[0];
	if (shouldRotateLine) lineVector.cross(lineVector, OpenSteer::Vec3(0.0, 0.0, 1.0));
	lineVector = lineVector.normalize();
	dist = lineVector.dot(line[1] - point);
	distV = (line[1] - dist * lineVector) - point;
	dist = distV.length();	

	if (DirAsSign && dist > 0.0)
	{
		distV.cross(distV, lineVector);
		if (distV.z <= 0.0) dist = -dist;
	}

	return dist;
}
