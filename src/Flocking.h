#pragma once

#include "Evacuee.h"
#include "NAGraph.h"
#include "SimpleVehicle.h"

#define FLOCK_OBJ_STAT char
#define FLOCK_OBJ_STAT_INIT		0x0
#define FLOCK_OBJ_STAT_MOVE		0x1
#define FLOCK_OBJ_STAT_END		0x2

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

	// constructors
	FlockingLocation(void)
	{
		MyTime = -1.0;
		GTime = -1.0;
		Traveled = 0.0;
		Velocity = OpenSteer::Vec3::zero;
		MyLocation = 0;
		ID = -1;
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

	// methods

	HRESULT loadNewEdge(void);
	HRESULT buildNeighborList(std::vector<FlockingObject *> * objects);

public:
	// properties

	IPointPtr		FinalPoint;
	long			BindVertex;
	FLOCK_OBJ_STAT	MyStatus;

	// methods
	
	FlockingObject(int id, EvcPathPtr path, double startTime, VARIANT groupName, INetworkQueryPtr ipNetworkQuery, ISpatialReferencePtr MetricProjection);
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

public:
	FlockingEnviroment(double SnapshotInterval, double SimulationInterval);
	virtual ~FlockingEnviroment(void);
	void Init(EvacueeList * evcList, INetworkQueryPtr ipNetworkQuery);
	HRESULT RunSimulation(IStepProgressorPtr ipStepProgressor, ITrackCancelPtr pTrackCancel, double maxCost);
	void GetResult(std::list<FlockingLocationPtr> ** History, std::list<double> ** collisionTimes);
	double static PathLength(EvcPathPtr path);
};
