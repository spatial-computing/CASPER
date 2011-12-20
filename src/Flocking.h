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
	double			Traveled;
	IPointPtr		MyLocation;
	OpenSteer::Vec3	Velocity;

	// constructors
	FlockingLocation(void)
	{
		MyTime = -1.0;
		Traveled = 0.0;
		Velocity = OpenSteer::Vec3::zero;
		MyLocation = 0;
	}

	FlockingLocation(const FlockingLocation &copy)
	{
		GroupName = copy.GroupName;
		MyTime = copy.MyTime;
		Traveled = copy.Traveled;
		Velocity = OpenSteer::Vec3(copy.Velocity.x, copy.Velocity.y, copy.Velocity.z);
		IClonePtr pointClone;
		((IClonePtr)copy.MyLocation)->Clone(&pointClone);
		MyLocation = pointClone;
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

	// methods

	HRESULT loadNewEdge(void);
	HRESULT buildNeighborList(std::list<FlockingObject *> * objects);

public:
	// properties

	IPointPtr		FinalPoint;
	long			BindVertex;
	FLOCK_OBJ_STAT	MyStatus;

	// methods
	
	FlockingObject(EvcPathPtr path, double startTime, VARIANT groupName, INetworkQueryPtr ipNetworkQuery, ISpatialReferencePtr MetricProjection);
	HRESULT Move(std::list<FlockingObject *> * objects, double deltatime);

	virtual ~FlockingObject(void)
	{
		delete [] libpoints;
		delete myVehicle;
	}
};

typedef FlockingObject * FlockingObjectPtr;
typedef FlockingLocation * FlockingLocationPtr;
typedef std::list<FlockingObjectPtr>::iterator FlockingObjectItr;
typedef std::list<FlockingLocationPtr>::iterator FlockingLocationItr;

class FlockingEnviroment
{
private:
	std::list<FlockingObjectPtr>	* objects;
	std::list<FlockingLocationPtr>	* history;
	double							snapshotInterval;
	double							simulationInterval;
	double							maxPathLen;

public:
	FlockingEnviroment(double SnapshotInterval, double SimulationInterval);
	virtual ~FlockingEnviroment(void);
	void Init(EvacueeList * evcList, INetworkQueryPtr ipNetworkQuery);
	HRESULT RunSimulation(IStepProgressorPtr ipStepProgressor, ITrackCancelPtr pTrackCancel);
	void GetHistory(std::list<FlockingLocationPtr> ** History);
	double static PathLength(EvcPathPtr path);
};
