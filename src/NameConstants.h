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

#define CS_NAME                                     L"Evacuation Solver"
#define CS_DISPLAY_NAME                             L"Evacuation Routing"

// NAClass SubLayer Names

#define CS_BARRIERS_NAME					        L"Barriers"
#define CS_EVACUEES_NAME							L"Evacuees"
#define CS_ZONES_NAME						        L"Zones"
#define CS_ROUTES_NAME						        L"Routes"
#define CS_EDGES_NAME						        L"EdgeStat"

// Generic Class Definition Field Names

#define CS_FIELD_SHAPE                              L"Shape"
#define CS_FIELD_OID                                L"ObjectID"
#define CS_FIELD_NAME                               L"Name"
#define CS_FIELD_E_TIME                             L"EvacCost"
#define CS_FIELD_E_ORG                              L"OrgCost"
#define CS_FIELD_E_POP	                            L"Pop"
#define CS_FIELD_Capacity                           L"Capacity"

// NALocation Field Names

#define CS_FIELD_SOURCE_ID                          L"SourceID"
#define CS_FIELD_SOURCE_OID                         L"SourceOID"
#define CS_FIELD_EVC_NAME						    L"EvcName"
#define CS_FIELD_EVC_POP						    L"Population"
#define CS_FIELD_POSITION                           L"PosAlong"
#define CS_FIELD_SIDE_OF_EDGE                       L"SideOfEdge"
#define CS_FIELD_CURBAPPROACH                       L"CurbApproach"
#define CS_FIELD_STATUS                             L"Status"

#define CS_FIELD_ReservPop                          L"ReservPop"
#define CS_FIELD_TravCost                           L"TravCost"
#define CS_FIELD_OrgCost                            L"OrgCost"
#define CS_FIELD_DIR                                L"Direction"
#define CS_FIELD_EID                                L"EdgeID"

// Coded Value Domain Names

#define CS_CURBAPPROACH_DOMAINNAME                  L"CurbApproach"
#define CS_LOCATIONSTATUS_DOMAINNAME                L"LocationStatus"
#define CS_SIDEOFEDGE_DOMAINNAME                    L"SideOfEdge"
