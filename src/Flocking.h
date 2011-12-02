#pragma once

#include "Evacuee.h"
#include "NAGraph.h"

#define FLOCK_OBJ_STAT char
#define FLOCK_OBJ_STAT_INIT		0x0
#define FLOCK_OBJ_STAT_MOVE		0x1
#define FLOCK_OBJ_STAT_END		0x2

class FlockingObject
{
public:
	// properties

	double SpeedX;
	double SpeedY;
	double X;
	double Y;
	EvcPathPtr MyPath;
	NAEdgePtr MyEdge;
	INetworkJunctionPtr NextVertex;
	IPointPtr FinalPoint;
	FLOCK_OBJ_STAT MyStatus;

	// methods
	
	FlockingObject(EvcPathPtr path);
	virtual ~FlockingObject(void);
};

typedef FlockingObject * FlockingObjectPtr;
typedef std::list<FlockingObjectPtr>::iterator FlockingObjectItr;

class FlockingEnviroment
{
private:
	std::list<FlockingObjectPtr> * objects;
	double snapshotInterval;

public:
	FlockingEnviroment(double SnapshotInterval);
	virtual ~FlockingEnviroment(void);
	void Init(EvacueeList * evcList);
	void RunSimulation(void);
};
