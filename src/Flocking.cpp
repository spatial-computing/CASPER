#include "stdafx.h"
#include "Flocking.h"

FlockingObject::FlockingObject(void)
{
}

FlockingObject::~FlockingObject(void)
{
}

FlockingEnviroment::FlockingEnviroment(double SnapshotInterval)
{
	snapshotInterval = SnapshotInterval;
	objects = 0;
}

FlockingEnviroment::~FlockingEnviroment(void)
{
}

void FlockingEnviroment::Init(EvacueeList * evcList)
{
}
