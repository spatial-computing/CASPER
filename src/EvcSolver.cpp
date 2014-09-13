// Copyright 2011 ESRI
// 
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
// 
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
// 
// See the use restrictions at http://help.arcgis.com/en/sdk/10.1/usageRestrictions.htm

#include "stdafx.h"
#include "NameConstants.h"
#include "EvcSolver.h"

/////////////////////////////////////////////////////////////////////
// INASolver

STDMETHODIMP EvcSolver::Bind(INAContext* pContext, IDENetworkDataset* pNetwork, IGPMessages* pMessages)
{
	// Bind() is a method used to re-associate the solver with a given network dataset and its schema. Calling Bind()
	// on the solver re-attaches the solver to the NAContext based on the current network dataset settings.
	// This is typically used to update the solver and its context based on changes in the network dataset's available
	// restrictions, hierarchy attributes, cost attributes, etc.	

	// load network attributes for later configuration and usage
	// this will be used to load restriction and also to load proper impedance (cost) value
	
	INetworkAttribute2Ptr networkAttrib = 0;
	long count, i;
	HRESULT hr = S_OK;
	esriNetworkAttributeUsageType utype;
	esriNetworkAttributeDataType dtype;	
	IUnknownPtr unk;

	if (pNetwork)
	{
		if (FAILED(hr = pNetwork->get_Attributes(&allAttribs))) return hr;
		if (FAILED(hr = allAttribs->get_Count(&count))) return hr;

		turnAttribs.clear();
		costAttribs.clear();
		discriptiveAttribs.clear();

		for (i = 0; i < count; i++)
		{
			if (FAILED(hr = allAttribs->get_Element(i, &unk))) return hr;
			networkAttrib = unk;
			if (FAILED(hr = networkAttrib->get_UsageType(&utype))) return hr;
			if (FAILED(hr = networkAttrib->get_DataType(&dtype))) return hr;
			if (utype == esriNAUTRestriction) turnAttribs.insert(turnAttribs.end(), networkAttrib);			
			else if (utype == esriNAUTCost) costAttribs.insert(costAttribs.end(), networkAttrib);
			else if (utype == esriNAUTDescriptor && (dtype == esriNADTDouble || dtype == esriNADTInteger))
				discriptiveAttribs.insert(discriptiveAttribs.end(), networkAttrib);
		}

		if (costAttribs.size() < 1 && pMessages) pMessages->AddError(-1, L"There are no cost attributes in the network dataset.");
		if (discriptiveAttribs.size() < 1 && pMessages) pMessages->AddError(-1, L"There are no descriptive attributes in the network dataset to be used as street capacity.");		
		if (costAttributeID == -1 && costAttribs.size() > 0) costAttribs[0]->get_ID(&costAttributeID);
		if (capAttributeID  == -1 && discriptiveAttribs.size() > 0) discriptiveAttribs[0]->get_ID(&capAttributeID);		

		// Agents setup
		// NOTE: this is an appropriate place to find and attach any agents used by this solver.
		// For example, the route solver would attach the directions agent.
		INamedSetPtr agents;
		IUnknownPtr pStreet;
		INAContextHelperPtr ipContextHelper(pContext);
		if (FAILED(hr = pContext->get_Agents(&agents))) return hr;	
		if (FAILED(hr = agents->get_ItemByName(L"StreetDirectionsAgent", &pStreet))) return hr;
		if (!pStreet)
		{
			pStreetAgent = INAStreetDirectionsAgentPtr(CLSID_NAStreetDirectionsAgent);
			((INAAgentPtr)pStreetAgent)->Initialize(pNetwork, ipContextHelper);	
			if (FAILED(hr = agents->Add(L"StreetDirectionsAgent", pStreetAgent))) return hr;
		}
		else pStreetAgent = pStreet;
	}
	return hr;  
}

STDMETHODIMP EvcSolver::CreateLayer(INAContext * pContext, INALayer ** ppLayer)
{
	if (!ppLayer) return E_POINTER;

	*ppLayer = 0;

	// This is an appropriate place to check if the user is licensed to run
	// your solver and fail with "E_NOT_LICENSED" or similar.

	// Create our custom symbolizer and use it to create the NALayer.
	// NOTE: we are assuming here that there is only one symbolizer to
	// consider.  The Network Analyst framework and ESRI solvers support the notion
	// that there can be many symbolizers in CATID_NetworkAnalystSymbolizer.
	// One can iterate the objects in this category and call the Applies()
	// method to see if the symbolizer should be used for a particular solver/context.
	// There is also a get_Priority() method that is used to determine which
	// to use if many Apply.

	INASymbolizerPtr ipNASymbolizer(CLSID_EvcSolverSymbolizer);

	return ipNASymbolizer->CreateLayer(pContext, ppLayer);
}

STDMETHODIMP EvcSolver::UpdateLayer(INALayer* pLayer, VARIANT_BOOL* pLayerUpdated)
{
	if (!pLayer || !pLayerUpdated) return E_POINTER;

	// This method is called after Solve() and gives us a chance to react to the results of
	// the Solve and change the layer. For example, the Service Area solver updates
	// its layer renderers after Solve has been called to adjust for unique values.

	// We will not need to update our layer, since our renderers will remain the same throughout

	*pLayerUpdated = VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP EvcSolver::CreateContext(IDENetworkDataset* pNetwork, BSTR contextName, INAContext** ppNAContext)
{
	if (!pNetwork || !ppNAContext) return E_POINTER;

	if (!contextName || !::wcslen(contextName)) return E_INVALIDARG;

	HRESULT hr;
	*ppNAContext = 0;

	// CreateContext() is called by the command that creates the solver and
	// initializes it.  Below we'll:
	//
	// - create the class definitions
	// - set up defaults
	//
	// After this method is called, this class should be prepared to have
	// its solve method called.

	// This is an appropriate place to check if the user is licensed to run
	// your solver and fail with "E_NOT_LICENSED" or similar.

	// Get the NDS SpatialRef, that will be the same for the context as well as all spatial NAClasses
	IDEGeoDatasetPtr ipDEGeoDataset(pNetwork);
	if (!ipDEGeoDataset) return E_INVALIDARG;
	ISpatialReferencePtr ipNAContextSR;
	if (FAILED(hr = ipDEGeoDataset->get_SpatialReference(&ipNAContextSR))) return hr;

	IUnknownPtr           ipUnknown;
	INamedSetPtr          ipNAClassDefinitions;
	INAClassDefinitionPtr ipEvacueePointsClassDef, ipBarriersClassDef, ipRoutesClassDef, ipZonesClassDef, ipEdgeStatClassDef, ipFlocksClassDef, ipRouteEdgesClassDef;

	// Build the class definitions
	if (FAILED(hr = BuildClassDefinitions(ipNAContextSR, &ipNAClassDefinitions, pNetwork))) return hr;

	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_EVACUEES_NAME), &ipUnknown);
	ipEvacueePointsClassDef = ipUnknown;

	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_ZONES_NAME), &ipUnknown);
	ipZonesClassDef = ipUnknown;

	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_BARRIERS_NAME), &ipUnknown);
	ipBarriersClassDef = ipUnknown;

	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_ROUTES_NAME), &ipUnknown);
	ipRoutesClassDef = ipUnknown;

	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_EDGES_NAME), &ipUnknown);
	ipEdgeStatClassDef = ipUnknown;
#if defined(_FLOCK)
	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_FLOCKS_NAME), &ipUnknown);
	ipFlocksClassDef = ipUnknown;
#endif
	ipNAClassDefinitions->get_ItemByName(CComBSTR(CS_ROUTEEDGES_NAME), &ipUnknown);
	ipRouteEdgesClassDef = ipUnknown;
	// Create a context and initialize it
	INAContextPtr     ipNAContext(CLSID_NAContext);
	INAContextEditPtr ipNAContextEdit(ipNAContext);
	if (FAILED(hr = ipNAContextEdit->Initialize(contextName, pNetwork))) return hr;
	if (FAILED(hr = ipNAContextEdit->putref_Solver(static_cast<INASolver*>(this)))) return hr;

	// Create NAClasses for each of our class definitions
	INamedSetPtr ipNAClasses;
	INAClassPtr  ipNAClass;

	ipNAContext->get_NAClasses(&ipNAClasses);
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipZonesClassDef, &ipNAClass))) return hr;
	ipNAClasses->Add(CComBSTR(CS_ZONES_NAME), (IUnknownPtr)ipNAClass);
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipEvacueePointsClassDef, &ipNAClass))) return hr;
	ipNAClasses->Add(CComBSTR(CS_EVACUEES_NAME), (IUnknownPtr)ipNAClass);
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipBarriersClassDef, &ipNAClass))) return hr;
	ipNAClasses->Add(CComBSTR(CS_BARRIERS_NAME), (IUnknownPtr)ipNAClass);
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipRoutesClassDef, &ipNAClass))) return hr;
	ipNAClasses->Add(CComBSTR(CS_ROUTES_NAME), (IUnknownPtr)ipNAClass);
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipEdgeStatClassDef, &ipNAClass))) return hr;
	ipNAClasses->Add(CComBSTR(CS_EDGES_NAME), (IUnknownPtr)ipNAClass);
#if defined(_FLOCK)
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipFlocksClassDef, &ipNAClass))) return hr;
	ipNAClasses->Add(CComBSTR(CS_FLOCKS_NAME), (IUnknownPtr)ipNAClass);
#endif
	if (FAILED(hr = ipNAContextEdit->CreateAnalysisClass(ipRouteEdgesClassDef, &ipNAClass))) return hr;
	if (FAILED(hr = ipNAClasses->Add(CComBSTR(CS_ROUTEEDGES_NAME), (IUnknownPtr)ipNAClass))) return hr;

	// NOTE: this is an appropriate place to set up any hierarchy defaults if your
	// solver supports using a hierarchy attribute (this solver does not). This is
	// also an appropriate place to set the default impedance attribute if you
	// had one.

	// Initialize the default field mappings
	// NOTE: this is an appropriate place to set up any default field mappings to be used

	// Return the context once it has been fully created and initialized
	*ppNAContext = ipNAContext;
	ipNAContext->AddRef();

	// Set up our solver defaults
	m_outputLineType = esriNAOutputLineTrueShapeWithMeasure;
	costAttributeID = -1;
	capAttributeID = -1;
	SaturationPerCap = 500.0;
	CriticalDensPerCap = 20.0;
	solverMethod = CASPERSolver;
	trafficModel = POWERModel;
	flockingProfile = FLOCK_PROFILE_CAR;
	
	m_CreateTraversalResult = VARIANT_TRUE;
	m_FindBestSequence = VARIANT_FALSE;
	m_PreserveFirstStop = VARIANT_FALSE;
	m_PreserveLastStop = VARIANT_FALSE;
	m_UseTimeWindows = VARIANT_FALSE;
	separable = VARIANT_FALSE;
	exportEdgeStat = VARIANT_TRUE;
	costPerDensity = 0.0f;
	flockingEnabled = VARIANT_FALSE;
	twoWayShareCapacity = VARIANT_TRUE;
	ThreeGenCARMA = VARIANT_FALSE;
	
	flockingSnapInterval = 0.1f;
	flockingSimulationInterval = 0.01f;
	initDelayCostPerPop = 0.01f;
	CARMAPerformanceRatio = 0.1f;
	selfishRatio = 0.0f;

	backtrack = esriNFSBAllowBacktrack;
	carmaSortDirection = BWCont;
	savedVersion = c_version;

	return S_OK;
}

#pragma warning(push)
#pragma warning(disable : 4100) /* Ignore warnings for unreferenced function parameters */

STDMETHODIMP EvcSolver::UpdateContext(INAContext* pNAContext, IDENetworkDataset* pNetwork, IGPMessages* pMessages)
{
	// UpdateContext() is a method used to update the context based on any changes that may have been made to the
	// solver settings. This typically includes changes to the set of accumulation attribute names, etc., which can
	// be set as fields in the context's NAClass schemas
	return S_OK;
}

#pragma warning(pop)

/////////////////////////////////////////////////////////////////////
// IPersistStream

STDMETHODIMP EvcSolver::IsDirty()
{
	return (m_bPersistDirty ? S_OK : S_FALSE);
}

STDMETHODIMP EvcSolver::Load(IStream* pStm)
{
	if (!pStm) return E_POINTER;

	ULONG     numBytes;
	HRESULT   hr;

	// We need to check the saved version number
	if (FAILED(hr = pStm->Read(&savedVersion, sizeof(savedVersion), &numBytes))) return hr;

	// We only support versions less than or equal to the current c_version
	if (savedVersion > c_version || savedVersion <= 0) return E_FAIL;
	
	// We need to read our persisted solver settings
	// version 1
	if (FAILED(hr = pStm->Read(&m_outputLineType, sizeof(m_outputLineType), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&costAttributeID, sizeof(costAttributeID), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&capAttributeID, sizeof(capAttributeID), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&trafficModel, sizeof(trafficModel), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&solverMethod, sizeof(solverMethod), &numBytes))) return hr;	
	if (FAILED(hr = pStm->Read(&SaturationPerCap, sizeof(SaturationPerCap), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&CriticalDensPerCap, sizeof(CriticalDensPerCap), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&m_CreateTraversalResult, sizeof(m_CreateTraversalResult), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&m_FindBestSequence, sizeof(m_FindBestSequence), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&m_PreserveFirstStop, sizeof(m_PreserveFirstStop), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&m_PreserveLastStop, sizeof(m_PreserveLastStop), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&m_UseTimeWindows, sizeof(m_UseTimeWindows), &numBytes))) return hr;	
	if (FAILED(hr = pStm->Read(&separable, sizeof(separable), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&exportEdgeStat, sizeof(exportEdgeStat), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&backtrack, sizeof(backtrack), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&costPerDensity, sizeof(costPerDensity), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&flockingEnabled, sizeof(flockingEnabled), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&flockingSnapInterval, sizeof(flockingSnapInterval), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&flockingSimulationInterval, sizeof(flockingSimulationInterval), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&twoWayShareCapacity, sizeof(twoWayShareCapacity), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&initDelayCostPerPop, sizeof(initDelayCostPerPop), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&flockingProfile, sizeof(flockingProfile), &numBytes))) return hr;
	if (FAILED(hr = pStm->Read(&CARMAPerformanceRatio, sizeof(CARMAPerformanceRatio), &numBytes))) return hr;

	//version 2
	if (savedVersion >= 2)
	{
		if (FAILED(hr = pStm->Read(&ThreeGenCARMA, sizeof(ThreeGenCARMA), &numBytes))) return hr;
	}
	else
	{
		ThreeGenCARMA = VARIANT_FALSE;
		savedVersion = 2;
	}
	
	//version 4
	if (savedVersion >= 4)
	{
		if (FAILED(hr = pStm->Read(&selfishRatio, sizeof(selfishRatio), &numBytes))) return hr;
	}
	else
	{
		selfishRatio = 0.0f;
		savedVersion = 4;
	}

	//version 5
	if (savedVersion >= 5)
	{
		if (FAILED(hr = pStm->Read(&carmaSortDirection, sizeof(carmaSortDirection), &numBytes))) return hr;
	}
	else
	{
		carmaSortDirection = BWCont;
		DoNotExportRouteEdges = true;
	}
	
	CARMAPerformanceRatio = min(max(CARMAPerformanceRatio, 0.0f), 1.0f);	
	selfishRatio = min(max(selfishRatio, 0.0f), 1.0f);
	m_bPersistDirty = false;

	return S_OK;
}

STDMETHODIMP EvcSolver::Save(IStream* pStm, BOOL fClearDirty)
{
	if (fClearDirty) m_bPersistDirty = false;

	ULONG     numBytes;
	HRESULT   hr;

	// We need to persist the c_version number
	if (FAILED(hr = pStm->Write(&savedVersion, sizeof(savedVersion), &numBytes))) return hr;

	// We need to persist our solver settings
	if (FAILED(hr = pStm->Write(&m_outputLineType, sizeof(m_outputLineType), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&costAttributeID, sizeof(costAttributeID), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&capAttributeID, sizeof(capAttributeID), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&trafficModel, sizeof(trafficModel), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&solverMethod, sizeof(solverMethod), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&SaturationPerCap, sizeof(SaturationPerCap), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&CriticalDensPerCap, sizeof(CriticalDensPerCap), &numBytes))) return hr;	
	if (FAILED(hr = pStm->Write(&m_CreateTraversalResult, sizeof(m_CreateTraversalResult), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&m_FindBestSequence, sizeof(m_FindBestSequence), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&m_PreserveFirstStop, sizeof(m_PreserveFirstStop), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&m_PreserveLastStop, sizeof(m_PreserveLastStop), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&m_UseTimeWindows, sizeof(m_UseTimeWindows), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&separable, sizeof(separable), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&exportEdgeStat, sizeof(exportEdgeStat), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&backtrack, sizeof(backtrack), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&costPerDensity, sizeof(costPerDensity), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&flockingEnabled, sizeof(flockingEnabled), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&flockingSnapInterval, sizeof(flockingSnapInterval), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&flockingSimulationInterval, sizeof(flockingSimulationInterval), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&twoWayShareCapacity, sizeof(twoWayShareCapacity), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&initDelayCostPerPop, sizeof(initDelayCostPerPop), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&flockingProfile, sizeof(flockingProfile), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&CARMAPerformanceRatio, sizeof(CARMAPerformanceRatio), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&ThreeGenCARMA, sizeof(ThreeGenCARMA), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&selfishRatio, sizeof(selfishRatio), &numBytes))) return hr;
	if (FAILED(hr = pStm->Write(&carmaSortDirection, sizeof(carmaSortDirection), &numBytes))) return hr;
	
	return S_OK;
}

STDMETHODIMP EvcSolver::GetSizeMax(_ULARGE_INTEGER* pCbSize)
{
	if (!pCbSize) return E_POINTER;

	pCbSize->HighPart = 0;
	pCbSize->LowPart = sizeof(short);

	return S_OK;
}

STDMETHODIMP EvcSolver::GetClassID(CLSID *pClassID)
{
	if (!pClassID) return E_POINTER;
	*pClassID = __uuidof(EvcSolver); 

	return S_OK;
}

//////////////////////////////////////////
// private methods

HRESULT EvcSolver::BuildClassDefinitions(ISpatialReference* pSpatialRef, INamedSet** ppDefinitions, IDENetworkDataset* pDENDS)
{
	if (!pSpatialRef || !ppDefinitions) return E_POINTER;

	// This function creates the class definitions for the EvcSolver.
	// Recall, class definitions are in-memory feature classes that store the
	// inputs and outputs for a solver.  This solver's class definitions are:

	// Zones (input)                  Safe zones determine the end locations
	// - OID                          of the search
	// - Shape
	// - Name 
	// - (NALocation fields)

	// Evacuee Points (input)         Evacuee points determine the start locations
	// - OID                          of the search
	// - Shape
	// - Name 
	// - Population					  number of un-seperatable people/cars at this location
	// - (NALocation fields)

	// Barriers (input)               Barriers restrict network edges from being
	// - OID                          traversable
	// - Shape
	// - Name
	// - (NALocation fields)

	// Routes (output)                Evacuation routes
	// - OID                          
	// - Shape                        evacuation route (polyline)
	// - EvcOID						  Evacuee user ID
	// - EvcTime					  Evacuation cost on this route
	// - OrgTime					  Evacuation cost assuming unlimited capacity
	// - RoutedPop					  The population who would use this route

	// Edges (output)                 Edge/street lines with their population reservations
	// - OID                          
	// - Shape                        edge/street shape (polyline)
	// - EdgeID						  Edge ID from NetworkDataset. there could be at most two edges (different directions) with one edgeID.
	// - Direction					  Direction of travel on this edge
	// - SourceID					  Refers to source street file ID
	// - SourceOID					  Refers to oid of the shape in the street file
	// - ReservPop					  The total population who would use this edge at some time
	// - TravCost					  Traversal cost based on the selected evacuation method on this edge
	// - OrgCost					  Original traversal cost of the edge without any population

	HRESULT hr = S_OK;

	// Create the class definitions named set and the variables needed to properly instantiate them
	INamedSetPtr              ipClassDefinitions(CLSID_NamedSet);
	INAClassDefinitionPtr     ipClassDef;
	INAClassDefinitionEditPtr ipClassDefEdit;
	IUIDPtr                   ipIUID;
	IFieldsPtr                ipFields;
	IFieldsEditPtr            ipFieldsEdit;
	IFieldPtr                 ipField;
	IFieldEditPtr             ipFieldEdit;

	//////////////////////////////////////////////////////////
	// Zones class definition

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriNetworkAnalyst.NALocationFeature")))) return hr;
	ipClassDefEdit->putref_ClassCLSID(ipIUID);

	// Create the fields for the class definition
	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	// Create and add an OID field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	// Create and add a Shape field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPoint);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	// Create and add a Name field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_NAME));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	// Create and add a capacity field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_CAP));
	ipFieldEdit->put_Type(esriFieldTypeDouble); // it use to be String. Have to be careful when I'm reading numbers from this field
	ipFieldEdit->put_DefaultValue(CComVariant(-1.0));
	ipFieldEdit->put_IsNullable(VARIANT_FALSE);	
	ipFieldsEdit->AddField(ipFieldEdit);

	// Add the NALocation fields
	AddLocationFields(ipFieldsEdit, pDENDS);

	// Add the new fields to the class definition (these must be set before setting field types)
	ipClassDefEdit->putref_Fields(ipFields);

	// Setup the field types (i.e., input/output fields, editable/non-editable fields, visible/non-visible fields, or a combination of these)
	AddLocationFieldTypes(ipClassDefEdit);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeInput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeInput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_NAME), esriNAFieldTypeInput);

	// Setup whether the NAClass is considered as input/output
	ipClassDefEdit->put_IsInput(VARIANT_TRUE);
	ipClassDefEdit->put_IsOutput(VARIANT_FALSE);

	// Setup necessary cardinality for allowing a Solve to be run 
	// NOTE: the LowerBound property is used to define the minimum number of required NALocationObjects that are required by the solver
	// to perform analysis. The UpperBound property is used to define the maximum number of NALocationObjects that are allowed by the solver
	// to perform analysis.
	// In our case, we must have at least one zone point stored in the class before allowing Solve to enabled in ArcMap, and we have no UpperBound
	ipClassDefEdit->put_LowerBound(1);

	// Give the class definition a name...
	ipClassDefEdit->put_Name(CComBSTR(CS_ZONES_NAME));

	// ...and add it to the named set
	ipClassDefinitions->Add(CComBSTR(CS_ZONES_NAME), (IUnknownPtr)ipClassDef);

	//////////////////////////////////////////////////////////
	// Evacuee Points class definition

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriNetworkAnalyst.NALocationFeature")))) return hr;
	ipClassDefEdit->putref_ClassCLSID(ipIUID);

	// Create the fields for the class definition
	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	// Create and add an OID field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	// Create and add an population field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_EVC_POP));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldEdit->put_DefaultValue(CComVariant(1.0));
	ipFieldEdit->put_IsNullable(VARIANT_FALSE);
	ipFieldsEdit->AddField(ipFieldEdit);

	// Create and add a Shape field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPoint);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	// Create and add a Name field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_NAME));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldEdit->put_Length(128);
	ipFieldsEdit->AddField(ipFieldEdit);

	// Add the NALocation fields
	AddLocationFields(ipFieldsEdit, pDENDS);

	// Add the new fields to the class definition (these must be set before setting field types)
	ipClassDefEdit->putref_Fields(ipFields);

	// Setup the field types (i.e., input/output fields, editable/non-editable fields, visible/non-visible fields, or a combination of these)
	AddLocationFieldTypes(ipClassDefEdit);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeInput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeInput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_NAME), esriNAFieldTypeInput);

	// Setup whether the NAClass is considered as input/output
	ipClassDefEdit->put_IsInput(VARIANT_TRUE);
	ipClassDefEdit->put_IsOutput(VARIANT_FALSE);

	// Setup necessary cardinality for allowing a Solve to be run 
	// NOTE: the LowerBound property is used to define the minimum number of required NALocationObjects that are required by the solver
	// to perform analysis. The UpperBound property is used to define the maximum number of NALocationObjects that are allowed by the solver
	// to perform analysis.
	// In our case, we must have at least one Evacuee point stored in the class before allowing Solve to enabled in ArcMap, and we have no UpperBound
	ipClassDefEdit->put_LowerBound(1);

	// Give the class definition a name...
	ipClassDefEdit->put_Name(CComBSTR(CS_EVACUEES_NAME));

	// ...and add it to the named set
	ipClassDefinitions->Add(CComBSTR(CS_EVACUEES_NAME), (IUnknownPtr)ipClassDef);

	//////////////////////////////////////////////////////////
	// Barriers class definition

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriNetworkAnalyst.NALocationFeature")))) return hr;
	ipClassDefEdit->putref_ClassCLSID(ipIUID);

	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPoint);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_NAME));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldEdit->put_Length(128);
	ipFieldsEdit->AddField(ipFieldEdit);

	AddLocationFields(ipFieldsEdit, pDENDS);

	ipClassDefEdit->putref_Fields(ipFields);  

	AddLocationFieldTypes(ipClassDefEdit);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeInput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeInput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_NAME), esriNAFieldTypeInput);

	ipClassDefEdit->put_IsInput(VARIANT_TRUE);
	ipClassDefEdit->put_IsOutput(VARIANT_FALSE);

	ipClassDefEdit->put_Name(CComBSTR(CS_BARRIERS_NAME));
	ipClassDefinitions->Add(CComBSTR(CS_BARRIERS_NAME), (IUnknownPtr)ipClassDef);

	//////////////////////////////////////////////////////////
	// Flocks class definition 

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriGeoDatabase.Feature")))) return hr;
	ipClassDefEdit->putref_ClassCLSID(ipIUID);

	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPoint);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_NAME));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_ID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_COST));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_VelocityX));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_VelocityY));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SPEED));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_TRAVELED));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_TIME));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_PTIME));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_STATUS));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldEdit->put_Length(1);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipClassDefEdit->putref_Fields(ipFields);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_ID), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_NAME), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_COST), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_VelocityX), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_VelocityY), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SPEED), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_TRAVELED), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_TIME), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_PTIME), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_STATUS), esriNAFieldTypeOutput);

	ipClassDefEdit->put_IsInput(VARIANT_FALSE);
	ipClassDefEdit->put_IsOutput(VARIANT_TRUE);

	ipClassDefEdit->put_Name(CComBSTR(CS_FLOCKS_NAME));
#if defined(_FLOCK)
	ipClassDefinitions->Add(CComBSTR(CS_FLOCKS_NAME), (IUnknownPtr)ipClassDef);
#endif

	//////////////////////////////////////////////////////////
	// Routes class definition 

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriGeoDatabase.Feature")))) return hr;
	ipClassDefEdit->putref_ClassCLSID(ipIUID);

	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPolyline);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_EVC_NAME));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_E_TIME));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_E_ORG));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_E_POP));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipClassDefEdit->putref_Fields(ipFields);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_EVC_NAME), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_E_TIME), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_E_POP), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_E_ORG), esriNAFieldTypeOutput);

	ipClassDefEdit->put_IsInput(VARIANT_FALSE);
	ipClassDefEdit->put_IsOutput(VARIANT_TRUE);

	ipClassDefEdit->put_Name(CComBSTR(CS_ROUTES_NAME));
	ipClassDefinitions->Add(CComBSTR(CS_ROUTES_NAME), (IUnknownPtr)ipClassDef);

	//////////////////////////////////////////////////////////
	// EdgeStat class definition 

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriGeoDatabase.Feature")))) return hr;
	ipClassDefEdit->putref_ClassCLSID(ipIUID);

	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPolyline);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_EID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_DIR));
	ipFieldEdit->put_Type(esriFieldTypeString);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SOURCE_ID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SOURCE_OID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_ReservPop));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_TravCost));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OrgCost));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_Congestion));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipClassDefEdit->putref_Fields(ipFields);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SOURCE_ID), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SOURCE_OID), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_ReservPop), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_TravCost), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OrgCost), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_Congestion), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_DIR), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_EID), esriNAFieldTypeOutput);

	ipClassDefEdit->put_IsInput(VARIANT_FALSE);
	ipClassDefEdit->put_IsOutput(VARIANT_TRUE);

	ipClassDefEdit->put_Name(CComBSTR(CS_EDGES_NAME));
	ipClassDefinitions->Add(CComBSTR(CS_EDGES_NAME), (IUnknownPtr)ipClassDef);

	//////////////////////////////////////////////////////////
	// RouteEdges class definition 

	ipClassDef.CreateInstance(CLSID_NAClassDefinition);
	ipClassDefEdit = ipClassDef;
	ipIUID.CreateInstance(CLSID_UID);
	if (FAILED(hr = ipIUID->put_Value(CComVariant(L"esriGeoDatabase.Feature")))) return hr;
	if (FAILED(hr = ipClassDefEdit->putref_ClassCLSID(ipIUID))) return hr;

	ipFields.CreateInstance(CLSID_Fields);
	ipFieldsEdit = ipFields;

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_OID));
	ipFieldEdit->put_Type(esriFieldTypeOID);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	{
		IGeometryDefEditPtr  ipGeoDef(CLSID_GeometryDef);

		ipGeoDef->put_GeometryType(esriGeometryPolyline);
		ipGeoDef->put_HasM(VARIANT_FALSE);
		ipGeoDef->put_HasZ(VARIANT_FALSE);
		ipGeoDef->putref_SpatialReference(pSpatialRef);
		ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SHAPE));
		ipFieldEdit->put_IsNullable(VARIANT_TRUE);
		ipFieldEdit->put_Type(esriFieldTypeGeometry);
		ipFieldEdit->putref_GeometryDef(ipGeoDef);
	}
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_RID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_EID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SEQ));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_FromP));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_ToP));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_COST));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldsEdit->AddField(ipFieldEdit);

	ipClassDefEdit->putref_Fields(ipFields);

	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_OID), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SHAPE), esriNAFieldTypeOutput | esriNAFieldTypeNotEditable | esriNAFieldTypeNotVisible);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_RID), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_EID), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_SEQ), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_FromP), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_ToP), esriNAFieldTypeOutput);
	ipClassDefEdit->put_FieldType(CComBSTR(CS_FIELD_COST), esriNAFieldTypeOutput);

	ipClassDefEdit->put_IsInput(VARIANT_FALSE);
	ipClassDefEdit->put_IsOutput(VARIANT_TRUE);

	ipClassDefEdit->put_Name(CComBSTR(CS_ROUTEEDGES_NAME));
	ipClassDefinitions->Add(CComBSTR(CS_ROUTEEDGES_NAME), (IUnknownPtr)ipClassDef);
	
	///////////////////////////////////////////////////////////
	// Return the class definitions once we have finished
	ipClassDefinitions->AddRef();
	*ppDefinitions = ipClassDefinitions;

	return hr;
}

HRESULT EvcSolver::GetNAClassTable(INAContext* pContext, BSTR className, ITable** ppTable)
{
	if (!pContext || !ppTable) return E_POINTER;

	HRESULT hr;
	INamedSetPtr ipNamedSet;

	if (FAILED(hr = pContext->get_NAClasses(&ipNamedSet))) return hr;

	IUnknownPtr ipUnk;
	if (FAILED(hr = ipNamedSet->get_ItemByName(className, &ipUnk))) return hr;

	ITablePtr ipTable(ipUnk);

	if (!ipTable) return AtlReportError(GetObjectCLSID(), _T("Context has an invalid NAClass."), IID_INASolver);

	ipTable->AddRef();
	*ppTable = ipTable;

	return S_OK;
}

HRESULT EvcSolver::LoadBarriers(ITable* pTable, INetworkQuery* pNetworkQuery, INetworkForwardStarEx* pNetworkForwardStarEx)
{
	if (!pTable || !pNetworkForwardStarEx) return E_POINTER;

	HRESULT hr;

	// Initialize all network elements as traversable
	if (FAILED(hr = pNetworkForwardStarEx->RemoveElementRestrictions())) return hr;

	// Get a cursor on the table to loop through each row
	ICursorPtr ipCursor;
	if (FAILED(hr = pTable->Search(0, VARIANT_TRUE, &ipCursor))) return hr;

	// Create variables for looping through the cursor and setting up barriers
	IRowESRI * ipRow;
	INALocationObjectPtr ipNALocationObject;
	INALocationPtr ipNALocation(CLSID_NALocation);
	IEnumNetworkElementPtr ipEnumNetworkElement;
	long sourceID = -1, sourceOID = -1;
	double posAlong, fromPosition, toPosition;

	INetworkEdgePtr ipReverseEdge;  // we need one edge element for querying the opposing direction
	INetworkElementPtr ipElement;
	pNetworkQuery->CreateNetworkElement(esriNETEdge, &ipElement);
	ipReverseEdge = ipElement;
	esriNetworkElementType elementType;

	// Loop through the cursor getting the NALocation of each NALocationObject,  
	// then query the network elements associated with each NALocation,
	// and set their traversability to false
	while (ipCursor->NextRow(&ipRow) == S_OK)
	{
		ipNALocationObject = ipRow;
		if (!ipNALocationObject) continue;// we only want valid NALocationObjects			
		if (FAILED(hr = ipNALocationObject->QueryNALocation(ipNALocation))) return hr;

		// Once we have the NALocation, we need to check if it is actually located within the network dataset
		VARIANT_BOOL isLocated = VARIANT_FALSE;
		if (ipNALocation)
		{
			if (FAILED(hr = ipNALocation->get_IsLocated(&isLocated))) return hr;
		}

		// We are only concerned with located NALocations
		if (isLocated)
		{
			// Get the SourceID for the NALocation
			if (FAILED(hr = ipNALocation->get_SourceID(&sourceID))) return hr;

			// Get the SourceOID for the NALocation
			if (FAILED(hr = ipNALocation->get_SourceOID(&sourceOID))) return hr;

			// Get the PosAlong for the NALocation
			if (FAILED(hr = ipNALocation->get_SourcePosition(&posAlong))) return hr;

			// Once we have a located NALocation, we query the network to obtain its associated network elements
			if (FAILED(hr = pNetworkQuery->get_ElementsByOID(sourceID, sourceOID, &ipEnumNetworkElement))) return hr;

			// Set the traversability for all appropriate network elements to false
			ipEnumNetworkElement->Reset();
			while (ipEnumNetworkElement->Next(&ipElement) == S_OK)
			{
				if (FAILED(hr = ipElement->get_ElementType(&elementType))) return hr;

				switch (elementType)
				{
				case esriNETJunction:
					{
						INetworkJunctionPtr ipJunction(ipElement);
						// If it is a junction, we can just set the element to non-traversable
						if (FAILED(hr = pNetworkForwardStarEx->AddJunctionRestriction(ipJunction))) return hr;

						break;
					}
				case esriNETEdge:
					{
						// If it is an edge, we must make sure it is an appropriate edge to set as non-traversable (by checking its fromPosition and toPosition)
						INetworkEdgePtr ipEdge(ipElement);
						if (FAILED(hr = ipEdge->QueryPositions(&fromPosition, &toPosition))) return hr;

						if (fromPosition <= posAlong && posAlong <= toPosition)
						{
							// Our NALocation lies along this edge element
							// Set it as non-traversable in both directions  

							// First, set this edge element as non-traversable in the AlongDigitized direction (this is the default direction of edge elements returned from the get_ElementsByOID method above)
							INetworkEdgePtr ipEdge(ipElement);
							if (FAILED(hr = pNetworkForwardStarEx->AddEdgeRestriction(ipEdge, 0.0, 1.0))) return hr;

							// Then, query the edge element to get its opposite direction
							if (FAILED(hr = ipEdge->QueryEdgeInOtherDirection(ipReverseEdge))) return hr;

							// Finally, set this edge as non-traversable in the AgainstDigitized direction
							if (FAILED(hr = pNetworkForwardStarEx->AddEdgeRestriction(ipReverseEdge, 0.0, 1.0))) return hr;

							break;
						}
					}
				}
			}
		}
	}

	return S_OK;
}

HRESULT EvcSolver::AddLocationFields(IFieldsEdit* pFieldsEdit, IDENetworkDataset* pDENDS)
{
	if (!pFieldsEdit) return E_POINTER;

	HRESULT                   hr;
	IFieldPtr                 ipField;
	IFieldEditPtr             ipFieldEdit;
	IRangeDomainPtr           ipRangeDomain;
	CComVariant                longDefaultValue(static_cast<long>(-1));
	CComVariant                doubleDefaultValue(static_cast<double>(-1));

	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SOURCE_ID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldEdit->put_DefaultValue(longDefaultValue);
	ipFieldEdit->put_Precision(0);
	ipFieldEdit->put_Required(VARIANT_TRUE);
	ipFieldEdit->put_Length(4);

	// if we can get source names, then add them to a coded value domain
	if (pDENDS)
	{
		IArrayPtr               ipSourceArray;
		if (FAILED(hr = pDENDS->get_Sources(&ipSourceArray))) return hr;

		ICodedValueDomainPtr    ipCodedValueDomain(CLSID_CodedValueDomain);
		IDomainPtr(ipCodedValueDomain)->put_Name(CS_FIELD_SOURCE_ID);
		IDomainPtr(ipCodedValueDomain)->put_FieldType(esriFieldTypeInteger);

		IUnknownPtr             ipUnk;
		INetworkSourcePtr       ipSource;
		long                    sourceCount;
		ipSourceArray->get_Count(&sourceCount);
		for (long i = 0; i < sourceCount; i++)
		{
			ipSourceArray->get_Element(i, &ipUnk);
			ipSource = ipUnk;

			long        sourceID;
			ipSource->get_ID(&sourceID);
			CComBSTR     name;
			ipSource->get_Name(&name);
			CComVariant     value(sourceID);

			ipCodedValueDomain->AddCode(value, name);
		}
		ipFieldEdit->putref_Domain((IDomainPtr)ipCodedValueDomain);
	}

	pFieldsEdit->AddField(ipFieldEdit);

	// Add the source OID field
	ipField.CreateInstance(CLSID_Field);
	ipRangeDomain.CreateInstance(CLSID_RangeDomain);
	IDomainPtr(ipRangeDomain)->put_Name(CS_FIELD_SOURCE_OID);
	IDomainPtr(ipRangeDomain)->put_FieldType(esriFieldTypeInteger);
	ipRangeDomain->put_MinValue(CComVariant((long)-1));
	ipRangeDomain->put_MaxValue(CComVariant((long)LONG_MAX));

	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SOURCE_OID));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldEdit->put_DefaultValue(longDefaultValue);
	ipFieldEdit->put_Precision(0);
	ipFieldEdit->put_Length(4);
	ipFieldEdit->put_Required(VARIANT_TRUE);
	ipFieldEdit->putref_Domain((IDomainPtr)ipRangeDomain);
	pFieldsEdit->AddField(ipFieldEdit);

	// Add the position field
	ipField.CreateInstance(CLSID_Field);
	ipRangeDomain.CreateInstance(CLSID_RangeDomain);
	IDomainPtr(ipRangeDomain)->put_Name(CS_FIELD_POSITION);
	IDomainPtr(ipRangeDomain)->put_FieldType(esriFieldTypeDouble);
	ipRangeDomain->put_MinValue(CComVariant((double)0.0));
	ipRangeDomain->put_MaxValue(CComVariant((double)1.0));
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_POSITION));
	ipFieldEdit->put_Type(esriFieldTypeDouble);
	ipFieldEdit->put_DefaultValue(doubleDefaultValue);
	ipFieldEdit->put_Precision(0);
	ipFieldEdit->put_Length(8);
	ipFieldEdit->put_Required(VARIANT_TRUE);
	ipFieldEdit->putref_Domain((IDomainPtr)ipRangeDomain);
	pFieldsEdit->AddField(ipFieldEdit);

	// Add the side of Edge domain
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_SIDE_OF_EDGE));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldEdit->put_DefaultValue(longDefaultValue);
	ipFieldEdit->put_Precision(0);
	ipFieldEdit->put_Length(4);
	ipFieldEdit->put_Required(VARIANT_TRUE);

	IDomainPtr    ipSideDomain;
	CreateSideOfEdgeDomain(&ipSideDomain);
	ipFieldEdit->putref_Domain(ipSideDomain);
	pFieldsEdit->AddField(ipFieldEdit);

	// Add the curb approach domain
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_CURBAPPROACH));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldEdit->put_DefaultValue(CComVariant((long)esriNAEitherSideOfVehicle));
	ipFieldEdit->put_Precision(0);
	ipFieldEdit->put_Length(4);
	ipFieldEdit->put_Required(VARIANT_TRUE);

	IDomainPtr    ipCurbDomain;
	CreateCurbApproachDomain(&ipCurbDomain);
	ipFieldEdit->putref_Domain(ipCurbDomain);
	pFieldsEdit->AddField(ipFieldEdit);

	// Add the status field
	ipField.CreateInstance(CLSID_Field);
	ipFieldEdit = ipField;
	ipFieldEdit->put_Name(CComBSTR(CS_FIELD_STATUS));
	ipFieldEdit->put_Type(esriFieldTypeInteger);
	ipFieldEdit->put_DefaultValue(CComVariant((long)0));
	ipFieldEdit->put_Precision(0);
	ipFieldEdit->put_Length(4);
	ipFieldEdit->put_Required(VARIANT_TRUE);

	// set up coded value domains for the the status values
	ICodedValueDomainPtr    ipCodedValueDomain(CLSID_CodedValueDomain);
	CreateStatusCodedValueDomain(ipCodedValueDomain);
	ipFieldEdit->putref_Domain((IDomainPtr)ipCodedValueDomain);
	pFieldsEdit->AddField(ipFieldEdit);

	return S_OK;
}

HRESULT EvcSolver::CreateSideOfEdgeDomain(IDomain** ppDomain)
{
	if (!ppDomain) return E_POINTER;

	HRESULT hr;
	ICodedValueDomainPtr ipCodedValueDomain(CLSID_CodedValueDomain);

	// Domains need names in order to serialize them
	IDomainPtr(ipCodedValueDomain)->put_Name(CComBSTR(CS_SIDEOFEDGE_DOMAINNAME));
	IDomainPtr(ipCodedValueDomain)->put_FieldType(esriFieldTypeInteger);

	CComVariant value(static_cast<long>(esriNAEdgeSideLeft));
	if (FAILED(hr = ipCodedValueDomain->AddCode(value, CComBSTR(L"Left Side")))) return hr;

	value.lVal = esriNAEdgeSideRight;
	if (FAILED(hr = ipCodedValueDomain->AddCode(value, CComBSTR(L"Right Side")))) return hr;

	*ppDomain = (IDomainPtr)ipCodedValueDomain;
	ipCodedValueDomain->AddRef();
	return S_OK;
}

HRESULT EvcSolver::CreateCurbApproachDomain(IDomain** ppDomain)
{
	if (!ppDomain) return E_POINTER;

	HRESULT       hr;
	ICodedValueDomainPtr    ipCodedValueDomain(CLSID_CodedValueDomain);

	IDomainPtr(ipCodedValueDomain)->put_Name(CComBSTR(CS_CURBAPPROACH_DOMAINNAME));
	IDomainPtr(ipCodedValueDomain)->put_FieldType(esriFieldTypeInteger);

	CComVariant value(static_cast<long>(esriNAEitherSideOfVehicle));
	if (FAILED(hr = ipCodedValueDomain->AddCode(value, CComBSTR(L"Either side of vehicle")))) return hr;

	value.lVal = esriNARightSideOfVehicle;
	if (FAILED(hr = ipCodedValueDomain->AddCode(value, CComBSTR(L"Right side of vehicle")))) return hr;

	value.lVal = esriNALeftSideOfVehicle;
	if (FAILED(hr = ipCodedValueDomain->AddCode(value, CComBSTR(L"Left side of vehicle")))) return hr;

	*ppDomain = (IDomainPtr)ipCodedValueDomain;
	ipCodedValueDomain->AddRef();
	return S_OK;
}

HRESULT EvcSolver::CreateStatusCodedValueDomain(ICodedValueDomain* pCodedValueDomain)
{
	if (!pCodedValueDomain) return E_POINTER;

	IDomainPtr(pCodedValueDomain)->put_Name(CComBSTR(CS_LOCATIONSTATUS_DOMAINNAME));
	IDomainPtr(pCodedValueDomain)->put_FieldType(esriFieldTypeInteger);

	CComVariant value(static_cast<long>(esriNAObjectStatusOK)); // this code is exclusive with the other codes
	pCodedValueDomain->AddCode(value, CComBSTR(L"OK"));

	value.lVal = esriNAObjectStatusNotLocated;
	pCodedValueDomain->AddCode(value, CComBSTR(L"Not located"));

	value.lVal = esriNAObjectStatusElementNotLocated;
	pCodedValueDomain->AddCode(value, CComBSTR(L"Network element not located"));

	value.lVal = esriNAObjectStatusElementNotTraversable;
	pCodedValueDomain->AddCode(value, CComBSTR(L"Element not traversible"));

	value.lVal = esriNAObjectStatusInvalidFieldValues;
	pCodedValueDomain->AddCode(value, CComBSTR(L"Invalid field values"));

	value.lVal = esriNAObjectStatusNotReached;
	pCodedValueDomain->AddCode(value, CComBSTR(L"Not reached"));

	value.lVal = esriNAObjectStatusTimeWindowViolation;
	pCodedValueDomain->AddCode(value, CComBSTR(L"Time window violation"));

	return S_OK;
}

HRESULT EvcSolver::AddLocationFieldTypes(INAClassDefinitionEdit* pClassDef)
{
	if (!pClassDef) return E_INVALIDARG;

	pClassDef->put_FieldType(CComBSTR(CS_FIELD_SOURCE_ID), esriNAFieldTypeInput);
	pClassDef->put_FieldType(CComBSTR(CS_FIELD_SOURCE_OID), esriNAFieldTypeInput);
	pClassDef->put_FieldType(CComBSTR(CS_FIELD_POSITION), esriNAFieldTypeInput);
	pClassDef->put_FieldType(CComBSTR(CS_FIELD_SIDE_OF_EDGE), esriNAFieldTypeInput);
	pClassDef->put_FieldType(CComBSTR(CS_FIELD_CURBAPPROACH), esriNAFieldTypeInput);
	pClassDef->put_FieldType(CComBSTR(CS_FIELD_STATUS), esriNAFieldTypeInput | esriNAFieldTypeNotEditable);

	return S_OK;
}