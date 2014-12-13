// Copyright 2010 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the use restrictions at http://help.arcgis.com/en/sdk/10.0/usageRestrictions.htm

#pragma once

// EvcSolver Names

#ifdef _WIN64
#define PROJ_ARCH                                   L"x86_64"
#else
#define PROJ_ARCH                                   L"x86"
#endif

#define PROJ_NAME                                   L"CASPER for ArcGIS"
#define CS_NAME                                     L"Evacuation Solver"
#define CS_DISPLAY_NAME                             L"Evacuation Routing"

// NAClass SubLayer Names

#define CS_BARRIERS_NAME					        L"Barriers"
#define CS_EVACUEES_NAME							L"Evacuees"
#define CS_ZONES_NAME						        L"Zones"
#define CS_ROUTES_NAME						        L"Routes"
#define CS_EDGES_NAME						        L"EdgeStat"
#define CS_ROUTEEDGES_NAME  				        L"RouteEdges"
#define CS_FLOCKS_NAME						        L"Flocks"

// Generic Class Definition Field Names

#define CS_FIELD_SHAPE                              L"Shape"
#define CS_FIELD_OID                                L"ObjectID"
#define CS_FIELD_NAME                               L"Name"
#define CS_FIELD_ID                                 L"ID"
#define CS_FIELD_E_TIME                             L"EvacCost"
#define CS_FIELD_E_ORG                              L"OrgCost"
#define CS_FIELD_E_POP	                            L"Pop"
#define CS_FIELD_CAP                                L"Capacity"

// NALocation Field Names

#define CS_FIELD_SOURCE_ID                          L"SourceID"
#define CS_FIELD_SOURCE_OID                         L"SourceOID"
#define CS_FIELD_EVC_NAME						    L"EvcName"
#define CS_FIELD_EVC_POP1						    L"Population"
#define CS_FIELD_EVC_POP2						    L"VehicleCount"
#define CS_FIELD_POSITION                           L"PosAlong"
#define CS_FIELD_SIDE_OF_EDGE                       L"SideOfEdge"
#define CS_FIELD_CURBAPPROACH                       L"CurbApproach"
#define CS_FIELD_STATUS                             L"Status"

// Flocking

#define CS_FIELD_TRAVELED                           L"Traveled"
#define CS_FIELD_TIME                               L"MyTime"
#define CS_FIELD_PTIME                              L"PassedMin"
#define CS_FIELD_VelocityX                          L"VelocX"
#define CS_FIELD_VelocityY                          L"VelocY"
#define CS_FIELD_SPEED                              L"Speed"
#define CS_FIELD_COST                               L"Cost"
#define CS_FIELD_FLOCKING_STATUS                    L"FlockingStatus"

// EdgeStat

#define CS_FIELD_ReservPop1                         L"ReservPop"
#define CS_FIELD_ReservPop2                         L"ReservVehicle"
#define CS_FIELD_Congestion                         L"Congestion"
#define CS_FIELD_TravCost                           L"TravCost"
#define CS_FIELD_OrgCost                            L"OrgCost"
#define CS_FIELD_DIR                                L"Direction"
#define CS_FIELD_EID                                L"EdgeID"

// RouteSegments

#define CS_FIELD_RID                                L"RouteID"
#define CS_FIELD_SEQ                                L"Sequence"
#define CS_FIELD_FromP                              L"FromPos"
#define CS_FIELD_ToP                                L"ToPos"


// Coded Value Domain Names

#define CS_CURBAPPROACH_DOMAINNAME                  L"CurbApproach"
#define CS_LOCATIONSTATUS_DOMAINNAME                L"LocationStatus"
#define CS_SIDEOFEDGE_DOMAINNAME                    L"SideOfEdge"

// Special constants for algorithms and traffic models

#define OPTIMIZATION_CASPER                         L"CASPER"
#define OPTIMIZATION_CCRP                           L"CCRP"
#define OPTIMIZATION_SP                             L"SP"
#define TRAFFIC_MODEL_LINEAR                        L"LINEAR"
#define TRAFFIC_MODEL_STEP                          L"STEP"
#define TRAFFIC_MODEL_POWER                         L"POWER"
#define TRAFFIC_MODEL_EXP                           L"EXP"
#define TRAFFIC_MODEL_FLAT                          L"FLAT"
