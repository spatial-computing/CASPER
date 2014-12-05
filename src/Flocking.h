// ===============================================================================================
// Evacuation Solver: Flocking module definition
// Description: definition of the evacuation simulation module. Uses OpenSteer headers
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#pragma once

#include "Evacuee.h"
#include "NAVertex.h"
#include "NAedge.h"
#include "SimpleVehicle.h"
#include "utils.h"

double PointToLineDistance(OpenSteer::Vec3 point, OpenSteer::Vec3 line[2], bool shouldRotateLine, bool DirAsSign);

class FlockProfile
{
public:
	double			Mass;
	double			Radius;
	double			CloseNeighborDistance;
	double			NeighborDistance;
	double			UsualSpeed;
	double			IntersectionRadius;
	double			ZoneRadius;
	double			MaxForce;

	virtual ~FlockProfile(void) { }

	FlockProfile(FLOCK_PROFILE profile)
	{
		switch (profile)
		{
		case FLOCK_PROFILE_CAR:
			IntersectionRadius = 20.0;
			Mass = 1.0;
			ZoneRadius = 75.0;
			Radius = 1.0;
			CloseNeighborDistance = 15.0;
			NeighborDistance = 30.0;
			UsualSpeed = 15.0;
			MaxForce = 150000.0;
			break;
		case FLOCK_PROFILE_PERSON:
			IntersectionRadius = 10.0;
			Mass = 0.1;
			ZoneRadius = 25.0;
			Radius = 0.2;
			CloseNeighborDistance = 2.0;
			NeighborDistance = 4.0;
			UsualSpeed = 2.0;
			MaxForce = 300000.0;
			break;
		case FLOCK_PROFILE_BIKE:
			IntersectionRadius = 20.0;
			Mass = 0.2;
			ZoneRadius = 50.0;
			Radius = 0.5;
			CloseNeighborDistance = 5.0;
			NeighborDistance = 10.0;
			UsualSpeed = 5.0;
			MaxForce = 200000.0;
			break;
		}
	}
};

class FlockingLocation
{
public:
	VARIANT			GroupName;
	double			MyTime;
	double			GTime;
	double			Traveled;
	IPointPtr		MyLocation;
	OpenSteer::Vec3	Velocity;
	int				ID;
	FLOCK_OBJ_STAT	MyStatus;

	// constructors
	FlockingLocation(void) throw(...)
	{
		MyTime = -1.0;
		GTime = -1.0;
		Traveled = 0.0;
		Velocity = OpenSteer::Vec3::zero;
		MyLocation = nullptr;
		ID = -1;
		MyStatus = FLOCK_OBJ_STAT_INIT;
	}

	FlockingLocation(const FlockingLocation &copy)
	{
		GroupName = VARIANT(copy.GroupName);
		MyTime = copy.MyTime;
		GTime = copy.GTime;
		Traveled = copy.Traveled;
		Velocity = OpenSteer::Vec3(copy.Velocity.x, copy.Velocity.y, copy.Velocity.z);
		IClonePtr pointClone;
		((IClonePtr)copy.MyLocation)->Clone(&pointClone);
		MyLocation = pointClone;
		ID = copy.ID;
		MyStatus = copy.MyStatus;
	}

	FlockingLocation & operator=(const FlockingLocation &) = delete;
	virtual ~FlockingLocation(void) { }
};

class FlockingObject : public FlockingLocation
{
private:
	// properties

	INetworkJunctionPtr			nextVertex;
	OpenSteer::Vec3				finishPoint;
	OpenSteer::SimpleVehicle	* myVehicle;
	OpenSteer::PolylinePathway	myVehiclePath;
	OpenSteer::AVGroup			myNeighborVehicles;
	OpenSteer::Vec3				* libpoints;
	bool						newEdgeRequestFlag;
	EvcPath::const_iterator		pathSegIt;
	EvcPathPtr					myPath;
	double						speedLimit;
	bool						initPathIterator;
	FlockProfile				* myProfile;
	bool						twoWayRoadsShareCap;

	// methods

	HRESULT loadNewEdge(void);
	HRESULT buildNeighborList(std::vector<FlockingObject *> * objects);
	bool DetectMyCollision();
	void GetMyInitLocation(std::vector<FlockingObject *> * neighbors, double x, double y, double & dx, double & dy);

public:
	// properties

	long			BindVertex;
	double          PathLen;

	// methods

	FlockingObject(int id, EvcPathPtr, double startTime, VARIANT groupName, INetworkQueryPtr, FlockProfile *, bool TwoWayRoadsShareCap, std::vector<FlockingObject *> * neighbors, double pathLen);
	HRESULT Move(std::vector<FlockingObject *> * objects, double deltatime);
	static bool DetectCollisions(std::vector<FlockingObject *> * objects);

	FlockingObject(const FlockingObject & that) = delete;
	FlockingObject & operator=(const FlockingObject &) = delete;
	virtual ~FlockingObject(void)
	{
		delete [] libpoints;
		delete myVehicle;
	}
};

typedef FlockingObject * FlockingObjectPtr;
typedef FlockingLocation * FlockingLocationPtr;
typedef std::vector<FlockingObjectPtr>::const_iterator FlockingObjectItr;
typedef std::vector<FlockingLocationPtr>::const_iterator FlockingLocationItr;

class FlockingEnviroment
{
private:
	std::vector<FlockingObjectPtr>	 * objects;
	std::vector<FlockingLocationPtr> * history;
	std::list<double>			 	 * collisions;
	double						 	 snapshotInterval;
	double						 	 simulationInterval;
	double						 	 maxPathLen;
	double						 	 minPathLen;
	double						 	 initDelayCostPerPop;
	bool							 movingObjectLeft;

public:
	FlockingEnviroment(double SnapshotInterval, double SimulationInterval, double InitDelayCostPerPop);
	virtual ~FlockingEnviroment(void);
	FlockingEnviroment(const FlockingEnviroment & that) = delete;
	FlockingEnviroment & operator=(const FlockingEnviroment &) = delete;

	void Init(std::shared_ptr<EvacueeList>, INetworkQueryPtr, FlockProfile *, bool TwoWayRoadsShareCap);
	HRESULT RunSimulation(IStepProgressorPtr, ITrackCancelPtr, double predictedCost);
	void GetResult(std::vector<FlockingLocationPtr> ** History, std::list<double> ** collisionTimes, bool * MovingObjectLeft);
	double static PathLength(EvcPathPtr path);
};
