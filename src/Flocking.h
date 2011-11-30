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
	INetworkJunctionPtr FinalVertex;
	FLOCK_OBJ_STAT MyStatus;

	// methods
	
	FlockingObject(void);
	virtual ~FlockingObject(void);
};

class FlockingEnviroment
{
private:
	FlockingObject * objects;
	double snapshotInterval;

public:
	FlockingEnviroment(double SnapshotInterval);
	virtual ~FlockingEnviroment(void);
	void Init(EvacueeList * evcList);
};
