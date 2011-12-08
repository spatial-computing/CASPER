#pragma once

#include "Evacuee.h"
#include "NAGraph.h"

#define FLOCK_OBJ_STAT char
#define FLOCK_OBJ_STAT_INIT		0x0
#define FLOCK_OBJ_STAT_MOVE		0x1
#define FLOCK_OBJ_STAT_END		0x2

class FlockingLocation
{
public:
	VARIANT		GroupName;
	double		MyTime;
	double		Traveled;
	double		SpeedX;
	double		SpeedY;
	IPointPtr	MyLocation;
};

class FlockingObject : public FlockingLocation
{
private:
	INetworkJunctionPtr	NextVertex;	

public:
	// properties

	IPointPtr 	StartPoint;
	IPointPtr	FinalPoint;
	EvcPathPtr	MyPath;
	NAEdgePtr	MyEdge;
	double		StartTime;
	int			BindVertex;
	FLOCK_OBJ_STAT		MyStatus;

	// methods
	
	FlockingObject(EvcPathPtr path, double startTime, VARIANT groupName, INetworkQueryPtr ipNetworkQuery);
	virtual ~FlockingObject(void);
	FLOCK_OBJ_STAT Move(std::list<FlockingObject *> * objects, double time);
};

typedef FlockingObject * FlockingObjectPtr;
typedef FlockingLocation * FlockingLocationPtr;
typedef std::list<FlockingObjectPtr>::iterator FlockingObjectItr;

class FlockingEnviroment
{
private:
	std::list<FlockingObjectPtr> * objects;
	double snapshotInterval;
	double simulationInterval;
	double maxPathLen;

public:
	FlockingEnviroment(double SnapshotInterval, double SimulationInterval);
	virtual ~FlockingEnviroment(void);
	void Init(EvacueeList * evcList, INetworkQueryPtr ipNetworkQuery);
	void RunSimulation(IStepProgressorPtr ipStepProgressor);
	void FlushHistory(std::list<FlockingLocationPtr> * history);
	double static PathLength(EvcPathPtr path);
};
