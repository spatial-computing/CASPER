#pragma once

#include "Evacuee.h"
#include "NAGraph.h"
#include "SimpleVehicle.h"

#define FLOCK_OBJ_STAT char
#define FLOCK_OBJ_STAT_INIT		0x0
#define FLOCK_OBJ_STAT_MOVE		0x1
#define FLOCK_OBJ_STAT_STOP		0x2
#define FLOCK_OBJ_STAT_END		0x3

#define FLOCK_PROFILE char
#define FLOCK_PROFILE_CAR		0x0
#define FLOCK_PROFILE_PERSON	0x1
#define FLOCK_PROFILE_BIKE		0x2

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

	~FlockProfile(void) { }

	FlockProfile(FLOCK_PROFILE profile)
	{
		IntersectionRadius = 30.0;
		ZoneRadius = 50.0;
		MaxForce = 150000.0;
		switch (profile)
		{
		case FLOCK_PROFILE_CAR:
			Mass = 1.0;
			Radius = 1.0;
			CloseNeighborDistance = 5.0;
			NeighborDistance = 20.0;
			UsualSpeed = 5.0;
			break;
		case FLOCK_PROFILE_PERSON:
			Mass = 1.0;
			Radius = 0.2;
			CloseNeighborDistance = 1.0;
			NeighborDistance = 2.0;
			UsualSpeed = 1.0;
			break;
		case FLOCK_PROFILE_BIKE:
			Mass = 2.0;
			Radius = 0.5;
			CloseNeighborDistance = 2.0;
			NeighborDistance = 5.0;
			UsualSpeed = 2.0;
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
	FlockingLocation(void)
	{
		MyTime = -1.0;
		GTime = -1.0;
		Traveled = 0.0;
		Velocity = OpenSteer::Vec3::zero;
		MyLocation = 0;
		ID = -1;
		MyStatus = FLOCK_OBJ_STAT_INIT;
	}

	FlockingLocation(const FlockingLocation &copy)
	{
		GroupName = copy.GroupName;
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

	virtual ~FlockingLocation(void) { }
};

class FlockingObject : public FlockingLocation
{
private:
	// properties

	INetworkJunctionPtr			nextVertex;
	IPointPtr					nextVertexPoint;
	OpenSteer::SimpleVehicle	* myVehicle;
	OpenSteer::PolylinePathway	myVehiclePath;
	OpenSteer::AVGroup			myNeighborVehicles;
	OpenSteer::Vec3				* libpoints;
	bool						newEdgeRequestFlag;
	EvcPath::iterator			pathSegIt;
	EvcPathPtr					myPath;
	double						speedLimit;
	ISpatialReferencePtr		metricProjection;
	bool						initPathIterator;
	FlockProfile				* myProfile;

	// methods

	HRESULT loadNewEdge(void);
	HRESULT buildNeighborList(std::vector<FlockingObject *> * objects);
	bool DetectMyCollision();

public:
	// properties

	IPointPtr		FinalPoint;
	long			BindVertex;

	// methods
	
	FlockingObject(int id, EvcPathPtr path, double startTime, VARIANT groupName, INetworkQueryPtr ipNetworkQuery, ISpatialReferencePtr MetricProjection, FlockProfile * profile);
	HRESULT Move(std::vector<FlockingObject *> * objects, double deltatime);
	static HRESULT DetectCollision(std::vector<FlockingObject *> * objects, bool * collided);

	virtual ~FlockingObject(void)
	{
		delete [] libpoints;
		delete myVehicle;
	}
};

typedef FlockingObject * FlockingObjectPtr;
typedef FlockingLocation * FlockingLocationPtr;
typedef std::vector<FlockingObjectPtr>::iterator FlockingObjectItr;
typedef std::list<FlockingLocationPtr>::iterator FlockingLocationItr;

class FlockingEnviroment
{
private:
	std::vector<FlockingObjectPtr>	* objects;
	std::list<FlockingLocationPtr>	* history;
	std::list<double>				* collisions;
	double							snapshotInterval;
	double							simulationInterval;
	double							maxPathLen;
	double							initDelayCostPerPop;
	bool							movingObjectLeft;
	bool							twoWayRoadsShareCap;

public:
	FlockingEnviroment(double SnapshotInterval, double SimulationInterval, bool TwoWayRoadsShareCap, double InitDelayCostPerPop);
	virtual ~FlockingEnviroment(void);
	void Init(EvacueeList * evcList, INetworkQueryPtr ipNetworkQuery, double costPerSec, FlockProfile * profile);
	HRESULT RunSimulation(IStepProgressorPtr ipStepProgressor, ITrackCancelPtr pTrackCancel, double predictedCost);
	void GetResult(std::list<FlockingLocationPtr> ** History, std::list<double> ** collisionTimes, bool * MovingObjectLeft);
	double static PathLength(EvcPathPtr path);
};
