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

// ===============================================================================================
// Evacuation Solver: Symbolizer
// Description: creates symbology for evacuation routing layer
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#include "stdafx.h"
#include "NameConstants.h"
#include "EvcSolver.h"
#include "EvcSolverSymbolizer.h"

// EvcSolverSymbolizer

//******************************************************************************************/
// INASymbolizer

STDMETHODIMP EvcSolverSymbolizer::Applies(INAContext* pNAContext, VARIANT_BOOL* pFlag)
{
	if (!pFlag) return E_POINTER;

	// This symbolizer is only intended to be used with the sample connectivity solver.
	// Get the solver from the context and QI for IEvcSolver to see if it
	// applies.

	*pFlag = VARIANT_FALSE;

	if (!pNAContext) return S_OK;

	INASolverPtr ipNASolver;
	pNAContext->get_Solver(&ipNASolver);

	IEvcSolverPtr ipEvcSolver(ipNASolver);

	if (ipEvcSolver) *pFlag = VARIANT_TRUE;

	return S_OK;
}

STDMETHODIMP EvcSolverSymbolizer::get_Priority(long* pPriority)
{
	if (!pPriority) return E_POINTER;

	// In the event that there are several symbolizers that Applies() to a solver,
	// priority is used to decide which one to use.  Higher number => higher priority.
	// This symbolizer only applies to this solver and is the only one, so we could
	// return anything here.  Network Analyst solver symbolizers typically
	// return 100 by default.

	*pPriority = 100;

	return S_OK;
}

STDMETHODIMP EvcSolverSymbolizer::CreateLayer(INAContext* pNAContext, INALayer** ppNALayer)
{
	if (!pNAContext || !ppNALayer) return E_POINTER;

	*ppNALayer = nullptr;

	// Create the main analysis layer
	HRESULT             hr = S_OK;
	INamedSetPtr        ipNAClasses;
	ATL::CComBSTR       layerName;
	INALayerPtr         ipNALayer(CLSID_NALayer);
	IUnknownPtr         ipUnknown;
	ATL::CString        szSubLayerName;
	IFeatureRendererPtr ipFeatureRenderer;
	IFeatureLayerPtr    ipSubFeatureLayer;

	pNAContext->get_NAClasses(&ipNAClasses);

	// Give the new layer a context and a name
	ipNALayer->putref_Context(pNAContext);
	pNAContext->get_Name(&layerName);
	((ILayerPtr)ipNALayer)->put_Name(layerName);

	// Build solver colors out of a single random base color.
	IColorPtr ipSolverColor;
	CreateRandomColor(&ipSolverColor);

	// Create a standard selection color
	IColorPtr ipSelectionColor(CLSID_RgbColor);
	ipSelectionColor->put_RGBESRI(RGB(0, 255, 255));

	//******************************************************************************************/
	// Zones layer

	// Get the Zone Points NAClass/FeatureClass
	if (FAILED(hr = ipNAClasses->get_ItemByName(ATL::CComBSTR(CS_ZONES_NAME), &ipUnknown))) return hr;

	IFeatureClassPtr ipZonesFC(ipUnknown);
	if (!ipZonesFC) return E_UNEXPECTED;

	// Create a new feature layer for the Zones feature class
	IFeatureLayerPtr ipZonesFeatureLayer(CLSID_FeatureLayer);
	ipZonesFeatureLayer->putref_FeatureClass(ipZonesFC);
	ipZonesFeatureLayer->put_Name(ATL::CComBSTR(CS_ZONES_NAME));

	// Give the Zone Points layer a renderer and a unique value property page
	CreatePointRenderer(ipSolverColor, &ipFeatureRenderer);

	IGeoFeatureLayerPtr ipGeoFeatureLayer(ipZonesFeatureLayer);
	if (!ipGeoFeatureLayer) return S_OK;

	if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;

	IUIDPtr ipUniqueValuePropertyPageUID(CLSID_UID);

	if (FAILED(ipUniqueValuePropertyPageUID->put_Value(ATL::CComVariant(L"esriCartoUI.UniqueValuePropertyPage"))))
	{
		// Renderer Property Pages are not installed with Engine. In this
		// case getting the property page by PROGID is an expected failure.
		ipUniqueValuePropertyPageUID = nullptr;
	}

	if (ipUniqueValuePropertyPageUID)
	{
		if (FAILED(hr = ipGeoFeatureLayer->put_RendererPropertyPageClassID(ipUniqueValuePropertyPageUID))) return hr;
	}

	((IFeatureSelectionPtr)ipZonesFeatureLayer)->putref_SelectionColor(ipSelectionColor);

	// Add the new Zone points layer as a sub-layer in the new NALayer
	ipNALayer->Add(ipZonesFeatureLayer);

	//******************************************************************************************/
	// Evacuees layer

	// Get the Evacuee Points NAClass/FeatureClass
	if (FAILED(hr = ipNAClasses->get_ItemByName(ATL::CComBSTR(CS_EVACUEES_NAME), &ipUnknown))) return hr;

	IFeatureClassPtr ipEvacueesFC(ipUnknown);
	if (!ipEvacueesFC) return E_UNEXPECTED;

	// Create a new feature layer for the Evacuee Points feature class
	IFeatureLayerPtr ipEvacueesFeatureLayer(CLSID_FeatureLayer);
	ipEvacueesFeatureLayer->putref_FeatureClass(ipEvacueesFC);
	ipEvacueesFeatureLayer->put_Name(ATL::CComBSTR(CS_EVACUEES_NAME));

	// Give the Evacuee Points layer a renderer and a unique value property page
	CreatePointRenderer(ipSolverColor, &ipFeatureRenderer);

	ipGeoFeatureLayer = ipEvacueesFeatureLayer;
	if (!ipGeoFeatureLayer) return S_OK;

	if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;

	if (ipUniqueValuePropertyPageUID)
	{
		if (FAILED(hr = ipGeoFeatureLayer->put_RendererPropertyPageClassID(ipUniqueValuePropertyPageUID))) return hr;
	}

	((IFeatureSelectionPtr)ipEvacueesFeatureLayer)->putref_SelectionColor(ipSelectionColor);

	// Add the new Evacuee points layer as a sub-layer in the new NALayer
	ipNALayer->Add(ipEvacueesFeatureLayer);

	//******************************************************************************************/
	// Barriers layer

	// Get the Barriers NAClass/FeatureClass
	if (FAILED(hr = ipNAClasses->get_ItemByName(ATL::CComBSTR(CS_BARRIERS_NAME), &ipUnknown))) return hr;

	IFeatureClassPtr ipBarriersFC(ipUnknown);
	if (!ipBarriersFC) return E_UNEXPECTED;

	// Create a new feature layer for the Barriers feature class
	IFeatureLayerPtr ipBarrierFeatureLayer(CLSID_FeatureLayer);
	ipBarrierFeatureLayer->putref_FeatureClass(ipBarriersFC);
	ipBarrierFeatureLayer->put_Name(ATL::CComBSTR(CS_BARRIERS_NAME));

	// Give the Barriers layer a unique value renderer and a unique value property page
	CreateBarrierRenderer(ipSolverColor, &ipFeatureRenderer);

	ipGeoFeatureLayer = ipBarrierFeatureLayer;
	if (!ipGeoFeatureLayer) return S_OK;

	if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;

	if (ipUniqueValuePropertyPageUID)
	{
		if (FAILED(hr = ipGeoFeatureLayer->put_RendererPropertyPageClassID(ipUniqueValuePropertyPageUID))) return hr;
	}

	((IFeatureSelectionPtr)ipBarrierFeatureLayer)->putref_SelectionColor(ipSelectionColor);

	// Add the new barriers layer as a sub-layer in the new NALayer
	ipNALayer->Add(ipBarrierFeatureLayer);

	//******************************************************************************************/
	// dynamic changes layer

	// Get the Routes NAClass/FeatureClass
	if (FAILED(hr = ipNAClasses->get_ItemByName(ATL::CComBSTR(CS_DYNCHANGES_NAME), &ipUnknown))) return hr;

	IFeatureClassPtr ipDynamicFC(ipUnknown);
	if (!ipDynamicFC) return E_UNEXPECTED;

	// Create a new feature layer for the Routes feature class
	IFeatureLayerPtr ipDynamicFeatureLayer(CLSID_FeatureLayer);
	ipDynamicFeatureLayer->putref_FeatureClass(ipDynamicFC);
	ipDynamicFeatureLayer->put_Name(ATL::CComBSTR(CS_DYNCHANGES_NAME));

	// Give the Routes layer a simple renderer and a single symbol property page
	CreatePolygonRenderer(ipSolverColor, &ipFeatureRenderer);

	ipGeoFeatureLayer = ipDynamicFeatureLayer;
	if (!ipGeoFeatureLayer) return S_OK;

	if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;

	IUIDPtr ipSingleSymbolPropertyPageUIDC(CLSID_UID);

	if (FAILED(ipSingleSymbolPropertyPageUIDC->put_Value(ATL::CComVariant(L"esriCartoUI.SingleSymbolPropertyPage"))))
	{
		// Renderer Property Pages are not installed with Engine. In this
		// case getting the property page by PROGID is an expected failure.
		ipSingleSymbolPropertyPageUIDC = nullptr;
	}

	if (ipSingleSymbolPropertyPageUIDC)
	{
		if (FAILED(hr = ipGeoFeatureLayer->put_RendererPropertyPageClassID(ipSingleSymbolPropertyPageUIDC))) return hr;
	}

	((IFeatureSelectionPtr)ipDynamicFeatureLayer)->putref_SelectionColor(ipSelectionColor);

	// Add the new routes layer as a sub-layer in the new NALayer
	ipNALayer->Add(ipDynamicFeatureLayer);

	//******************************************************************************************/
	// routes layer

	// Get the Routes NAClass/FeatureClass
	if (FAILED(hr = ipNAClasses->get_ItemByName(ATL::CComBSTR(CS_ROUTES_NAME), &ipUnknown))) return hr;

	IFeatureClassPtr ipRoutesFC(ipUnknown);
	if (!ipRoutesFC) return E_UNEXPECTED;

	// Create a new feature layer for the Routes feature class
	IFeatureLayerPtr ipRoutesFeatureLayer(CLSID_FeatureLayer);
	ipRoutesFeatureLayer->putref_FeatureClass(ipRoutesFC);
	ipRoutesFeatureLayer->put_Name(ATL::CComBSTR(CS_ROUTES_NAME));

	// Give the Routes layer a simple renderer and a single symbol property page
	CreateLineRenderer(ipSolverColor, &ipFeatureRenderer);

	ipGeoFeatureLayer = ipRoutesFeatureLayer;
	if (!ipGeoFeatureLayer) return S_OK;

	if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;

	IUIDPtr ipSingleSymbolPropertyPageUIDR(CLSID_UID);

	if (FAILED(ipSingleSymbolPropertyPageUIDR->put_Value(ATL::CComVariant(L"esriCartoUI.SingleSymbolPropertyPage"))))
	{
		// Renderer Property Pages are not installed with Engine. In this
		// case getting the property page by PROGID is an expected failure.
		ipSingleSymbolPropertyPageUIDR = nullptr;
	}

	if (ipSingleSymbolPropertyPageUIDR)
	{
		if (FAILED(hr = ipGeoFeatureLayer->put_RendererPropertyPageClassID(ipSingleSymbolPropertyPageUIDR))) return hr;
	}

	((IFeatureSelectionPtr)ipRoutesFeatureLayer)->putref_SelectionColor(ipSelectionColor);

	// Add the new routes layer as a sub-layer in the new NALayer
	ipNALayer->Add(ipRoutesFeatureLayer);

	//******************************************************************************************/
	// EdgeStat layer

	// Get the EdgeStat NAClass/FeatureClass
	if (FAILED(hr = ipNAClasses->get_ItemByName(ATL::CComBSTR(CS_EDGES_NAME), &ipUnknown))) return hr;

	IFeatureClassPtr ipEdgesFC(ipUnknown);
	if (!ipEdgesFC) return E_UNEXPECTED;

	// Create a new feature layer for the EdgeStat feature class
	IFeatureLayerPtr ipEdgesFeatureLayer(CLSID_FeatureLayer);
	ipEdgesFeatureLayer->putref_FeatureClass(ipEdgesFC);
	ipEdgesFeatureLayer->put_Name(ATL::CComBSTR(CS_EDGES_NAME));

	// Give the EdgeStat layer a simple renderer and a single symbol property page
	CreateLineRenderer(ipSolverColor, &ipFeatureRenderer);

	ipGeoFeatureLayer = ipEdgesFeatureLayer;
	if (!ipGeoFeatureLayer) return S_OK;

	if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;

	IUIDPtr ipSingleSymbolPropertyPageUIDE(CLSID_UID);

	if (FAILED(ipSingleSymbolPropertyPageUIDE->put_Value(ATL::CComVariant(L"esriCartoUI.SingleSymbolPropertyPage"))))
	{
		// Renderer Property Pages are not installed with Engine. In this
		// case getting the property page by PROGID is an expected failure.
		ipSingleSymbolPropertyPageUIDE = nullptr;
	}

	if (ipSingleSymbolPropertyPageUIDE)
	{
		if (FAILED(hr = ipGeoFeatureLayer->put_RendererPropertyPageClassID(ipSingleSymbolPropertyPageUIDE))) return hr;
	}

	((IFeatureSelectionPtr)ipEdgesFeatureLayer)->putref_SelectionColor(ipSelectionColor);

	// Add the new routes layer as a sub-layer in the new NALayer
	ipNALayer->Add(ipEdgesFeatureLayer);

	//******************************************************************************************/
	// RouteEdges layer

	// Get the EdgeStat NAClass/FeatureClass
	if (FAILED(hr = ipNAClasses->get_ItemByName(ATL::CComBSTR(CS_ROUTEEDGES_NAME), &ipUnknown))) return hr;

	IFeatureClassPtr ipRouteEdgesFC(ipUnknown);
	if (!ipRouteEdgesFC) return E_UNEXPECTED;

	// Create a new feature layer for the EdgeStat feature class
	IFeatureLayerPtr ipRouteEdgesFeatureLayer(CLSID_FeatureLayer);
	ipRouteEdgesFeatureLayer->putref_FeatureClass(ipRouteEdgesFC);
	ipRouteEdgesFeatureLayer->put_Name(ATL::CComBSTR(CS_ROUTEEDGES_NAME));

	// Give the EdgeStat layer a simple renderer and a single symbol property page
	CreateLineRenderer(ipSolverColor, &ipFeatureRenderer);

	ipGeoFeatureLayer = ipRouteEdgesFeatureLayer;
	if (!ipGeoFeatureLayer) return S_OK;

	if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;

	IUIDPtr ipSingleSymbolPropertyPageUIDRE(CLSID_UID);

	if (FAILED(ipSingleSymbolPropertyPageUIDRE->put_Value(ATL::CComVariant(L"esriCartoUI.SingleSymbolPropertyPage"))))
	{
		// Renderer Property Pages are not installed with Engine. In this
		// case getting the property page by PROGID is an expected failure.
		ipSingleSymbolPropertyPageUIDRE = nullptr;
	}

	if (ipSingleSymbolPropertyPageUIDRE)
	{
		if (FAILED(hr = ipGeoFeatureLayer->put_RendererPropertyPageClassID(ipSingleSymbolPropertyPageUIDRE))) return hr;
	}

	((IFeatureSelectionPtr)ipRouteEdgesFeatureLayer)->putref_SelectionColor(ipSelectionColor);

	// Add the new RouteEdges layer as a sub-layer in the new NALayer
	ipNALayer->Add(ipRouteEdgesFeatureLayer);

	//******************************************************************************************/
	// flocks layer
	// Get the flocks NAClass/FeatureClass
	if (FAILED(hr = ipNAClasses->get_ItemByName(ATL::CComBSTR(CS_FLOCKS_NAME), &ipUnknown))) return hr;

	IFeatureClassPtr ipFlocksFC(ipUnknown);
	if (!ipRoutesFC) return E_UNEXPECTED;

	// Create a new feature layer for the flocks feature class
	IFeatureLayerPtr ipFlocksFeatureLayer(CLSID_FeatureLayer);
	ipFlocksFeatureLayer->putref_FeatureClass(ipFlocksFC);
	ipFlocksFeatureLayer->put_Name(ATL::CComBSTR(CS_FLOCKS_NAME));

	// Give the Routes layer a simple renderer and a single symbol property page
	CreateSimplePointRenderer(ipSolverColor, &ipFeatureRenderer);

	ipGeoFeatureLayer = ipFlocksFeatureLayer;
	if (!ipGeoFeatureLayer) return S_OK;

	if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;

	IUIDPtr ipSingleSymbolPropertyPageUID3(CLSID_UID);

	if (FAILED(ipSingleSymbolPropertyPageUID3->put_Value(ATL::CComVariant(L"esriCartoUI.SingleSymbolPropertyPage"))))
	{
		// Renderer Property Pages are not installed with Engine. In this
		// case getting the property page by PROGID is an expected failure.
		ipSingleSymbolPropertyPageUID3 = nullptr;
	}

	if (ipSingleSymbolPropertyPageUID3)
	{
		if (FAILED(hr = ipGeoFeatureLayer->put_RendererPropertyPageClassID(ipSingleSymbolPropertyPageUID3))) return hr;
	}

	((IFeatureSelectionPtr)ipFlocksFeatureLayer)->putref_SelectionColor(ipSelectionColor);

	// Add the new flocks layer as a sub-layer in the new NALayer
	ipNALayer->Add(ipFlocksFeatureLayer);

	// Return the newly created NALayer
	(*ppNALayer) = ipNALayer;

	if (*ppNALayer) (*ppNALayer)->AddRef();

	return S_OK;
}

#pragma warning(push)
#pragma warning(disable : 4100) /* Ignore warnings for unreferenced function parameters */
STDMETHODIMP EvcSolverSymbolizer::UpdateLayer(INALayer* pNALayer, VARIANT_BOOL* pUpdated)
{
	return E_NOTIMPL;
}
#pragma warning(pop)

STDMETHODIMP EvcSolverSymbolizer::ResetRenderers(IColor *pSolverColor, INALayer *pNALayer)
{
	if (!pNALayer) return E_POINTER;

	HRESULT hr;

	// ResetRenderers() provides a method for updating the entire NALayer color scheme
	// based on the color passed in to this method. We must get each feature layer in the
	// composite NALayer and update its renderer's colors appropriately.

	// Get the NALayer's context and make sure that this symbolizer applies
	INAContextPtr ipContext;
	pNALayer->get_Context(&ipContext);
	if (!ipContext) return S_OK;

	VARIANT_BOOL doesApply = VARIANT_FALSE;
	Applies(ipContext, &doesApply);
	if (!doesApply) return S_OK;

	// Check to make sure a valid color was passed in; if not, we create a random one
	if (!pSolverColor) CreateRandomColor(&pSolverColor);

	IFeatureRendererPtr ipFeatureRenderer;
	ILayerPtr ipSubLayer;
	IGeoFeatureLayerPtr ipGeoFeatureLayer;

	//******************************************************************************************/
	// Dynamic Changes
	pNALayer->get_LayerByNAClassName(ATL::CComBSTR(CS_DYNCHANGES_NAME), &ipSubLayer);
	if (ipSubLayer)
	{
		ipGeoFeatureLayer = ipSubLayer;
		if (FAILED(hr = CreatePolygonRenderer(pSolverColor, &ipFeatureRenderer))) return hr;
		if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;
	}

	//******************************************************************************************/
	// Flocks
	pNALayer->get_LayerByNAClassName(ATL::CComBSTR(CS_FLOCKS_NAME), &ipSubLayer);
	if (ipSubLayer)
	{
		ipGeoFeatureLayer = ipSubLayer;
		if (FAILED(hr = CreateSimplePointRenderer(pSolverColor, &ipFeatureRenderer))) return hr;
		if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;
	}

	//******************************************************************************************/
	// Zones
	pNALayer->get_LayerByNAClassName(ATL::CComBSTR(CS_ZONES_NAME), &ipSubLayer);
	if (ipSubLayer)
	{
		ipGeoFeatureLayer = ipSubLayer;
		if (FAILED(hr = CreatePointRenderer(pSolverColor, &ipFeatureRenderer))) return hr;
		if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;
	}

	//******************************************************************************************/
	// Evacuee Points
	pNALayer->get_LayerByNAClassName(ATL::CComBSTR(CS_EVACUEES_NAME), &ipSubLayer);
	if (ipSubLayer)
	{
		ipGeoFeatureLayer = ipSubLayer;
		if (FAILED(hr = CreatePointRenderer(pSolverColor, &ipFeatureRenderer))) return hr;
		if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;
	}

	//******************************************************************************************/
	// Barriers
	pNALayer->get_LayerByNAClassName(ATL::CComBSTR(CS_BARRIERS_NAME), &ipSubLayer);
	if (ipSubLayer)
	{
		ipGeoFeatureLayer = ipSubLayer;
		if (FAILED(hr = CreateBarrierRenderer(pSolverColor, &ipFeatureRenderer))) return hr;
		if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;
	}

	//******************************************************************************************/
	// Routes
	pNALayer->get_LayerByNAClassName(ATL::CComBSTR(CS_ROUTES_NAME), &ipSubLayer);
	if (ipSubLayer)
	{
		ipGeoFeatureLayer = ipSubLayer;
		if (FAILED(hr = CreateLineRenderer(pSolverColor, &ipFeatureRenderer))) return hr;
		if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;
	}

	//******************************************************************************************/
	// EdgeStat
	pNALayer->get_LayerByNAClassName(ATL::CComBSTR(CS_EDGES_NAME), &ipSubLayer);
	if (ipSubLayer)
	{
		ipGeoFeatureLayer = ipSubLayer;
		if (FAILED(hr = CreateLineRenderer(pSolverColor, &ipFeatureRenderer))) return hr;
		if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;
	}

	//******************************************************************************************/
	// RouteEdges
	pNALayer->get_LayerByNAClassName(ATL::CComBSTR(CS_ROUTEEDGES_NAME), &ipSubLayer);
	if (ipSubLayer)
	{
		ipGeoFeatureLayer = ipSubLayer;
		if (FAILED(hr = CreateLineRenderer(pSolverColor, &ipFeatureRenderer))) return hr;
		if (FAILED(hr = ipGeoFeatureLayer->putref_Renderer(ipFeatureRenderer))) return hr;
	}

	return S_OK;
}

//******************************************************************************************/
// private methods

HRESULT EvcSolverSymbolizer::CreateRandomColor(IColor** ppColor)
{
	if (!ppColor) return E_POINTER;

	*ppColor = nullptr;

	IHsvColorPtr ipHsvColor(CLSID_HsvColor);
#pragma warning(suppress: 28159)
	DWORD tik = ::GetTickCount();
	ipHsvColor->put_Hue(tik % 360L);
	ipHsvColor->put_Saturation(c_randomColorHSVSaturation);
	ipHsvColor->put_Value(c_baseRandomColorHSVValue + (tik % c_maxAboveBaseRandomColorHSVValue));

	*ppColor = static_cast<IColor*>(ipHsvColor);

	if (*ppColor) (*ppColor)->AddRef();

	return S_OK;
}

HRESULT EvcSolverSymbolizer::CreateBarrierRenderer(IColor* pBarrierColor, IFeatureRenderer** ppFRenderer)
{
	if (!ppFRenderer || !pBarrierColor) return E_POINTER;

	IUniqueValueRendererPtr ipRenderer(CLSID_UniqueValueRenderer);

	// Three symbols: Located, Unlocated and Error (default) are created for the renderer
	HRESULT     hr;
	IColorPtr   ipBackgroundColor(CLSID_RgbColor),
		ipUnlocatedColor(CLSID_RgbColor),
		ipErrorColor(CLSID_RgbColor);
	ISymbolPtr  ipBarrierSymbol,
		ipUnlocatedBarrierSymbol,
		ipErrorSymbol;
	COLORREF    colorRef;

	ipErrorColor->put_RGBESRI(RGB(255, 0, 0));
	ipBackgroundColor->put_RGBESRI(RGB(255, 255, 255));
	pBarrierColor->get_RGBESRI(&colorRef);

	IHsvColorPtr ipHsvColor(CLSID_HsvColor);
	ipHsvColor->put_RGBESRI(colorRef);
	long s;
	ipHsvColor->get_Saturation(&s);
	if (s > c_maxFadedColorHSVSaturation)
	{
		ipHsvColor->put_Saturation(c_maxFadedColorHSVSaturation);
		ipHsvColor->get_RGBESRI(&colorRef);
	}
	ipUnlocatedColor->put_RGBESRI(colorRef);

	if (FAILED(hr = CreateCharacterMarkerSymbol(ATL::CString(_T("ESRI Default Marker")), pBarrierColor, ipBackgroundColor, 67, 33, 18, 10, 45.0, &ipBarrierSymbol))) return hr;
	if (FAILED(hr = CreateUnlocatedSymbol(ipBarrierSymbol, &ipUnlocatedBarrierSymbol))) return hr;
	if (FAILED(hr = CreateCharacterMarkerSymbol(ATL::CString(_T("ESRI Default Marker")), ipErrorColor, ipBackgroundColor, 67, 33, 18, 10, 45.0, &ipErrorSymbol))) return hr;

	ipRenderer->put_FieldCount(1);
	ipRenderer->put_Field(0, ATL::CComBSTR(CS_FIELD_STATUS));

	ipRenderer->put_DefaultSymbol(ipErrorSymbol);
	ipRenderer->put_DefaultLabel(ATL::CComBSTR(L"Error"));
	ipRenderer->put_UseDefaultSymbol(VARIANT_TRUE);

	ipRenderer->AddValue(ATL::CComBSTR(L"0"), ATL::CComBSTR(L""), ipBarrierSymbol);
	ipRenderer->put_Label(ATL::CComBSTR(L"0"), ATL::CComBSTR(L"Located"));

	ipRenderer->AddValue(ATL::CComBSTR(L"1"), ATL::CComBSTR(L""), ipUnlocatedBarrierSymbol);
	ipRenderer->put_Label(ATL::CComBSTR(L"1"), ATL::CComBSTR(L"Unlocated"));

	*ppFRenderer = (IFeatureRendererPtr)ipRenderer;
	if (*ppFRenderer) (*ppFRenderer)->AddRef();

	return S_OK;
}

HRESULT EvcSolverSymbolizer::CreateCharacterMarkerSymbol(ATL::CString   fontName,
	IColor*   pMarkerColor,
	IColor*   pMarkerBackgroundColor,
	long      characterIndex,
	long      backgoundCharacterIndex,
	double    markerSize,
	double    makerBackgroundSize,
	double    markerAngle,
	ISymbol** ppMarkerSymbol)
{
	if (!ppMarkerSymbol || !pMarkerColor || !pMarkerBackgroundColor) return E_POINTER;

	IMultiLayerMarkerSymbolPtr ipMultiLayerSymbol(CLSID_MultiLayerMarkerSymbol);

	*ppMarkerSymbol = (ISymbolPtr)ipMultiLayerSymbol;
	if (*ppMarkerSymbol) (*ppMarkerSymbol)->AddRef();

	// Check font is available
	IFontDispPtr ipFontDisp;
	FONTDESC     fontDesc;

	::memset(&fontDesc, 0, sizeof(FONTDESC));
	fontDesc.cbSizeofstruct = sizeof(FONTDESC);
	fontDesc.lpstrName = ATL::CComBSTR(fontName);
	::OleCreateFontIndirect(&fontDesc, IID_IFontDisp, (void**)&ipFontDisp);
	if (!ipFontDisp) return E_INVALIDARG;

	ICharacterMarkerSymbolPtr ipCharacterMarkerSymbol;

	// Background symbol
	if (makerBackgroundSize)
	{
		ipCharacterMarkerSymbol.CreateInstance(CLSID_CharacterMarkerSymbol);

		ipCharacterMarkerSymbol->put_Font(ipFontDisp);
		ipCharacterMarkerSymbol->put_CharacterIndex(backgoundCharacterIndex);
		ipCharacterMarkerSymbol->put_Color(pMarkerBackgroundColor);
		ipCharacterMarkerSymbol->put_Size(makerBackgroundSize);

		ipMultiLayerSymbol->AddLayer((IMarkerSymbolPtr)ipCharacterMarkerSymbol);
		((ILayerColorLockPtr)ipMultiLayerSymbol)->put_LayerColorLock(0, VARIANT_TRUE);
	}

	// Foreground symbol
	ipCharacterMarkerSymbol.CreateInstance(CLSID_CharacterMarkerSymbol);

	ipCharacterMarkerSymbol->put_Font(ipFontDisp);
	ipCharacterMarkerSymbol->put_CharacterIndex(characterIndex);
	ipCharacterMarkerSymbol->put_Color(pMarkerColor);
	ipCharacterMarkerSymbol->put_Size(markerSize);
	ipCharacterMarkerSymbol->put_Angle(markerAngle);

	ipMultiLayerSymbol->AddLayer((IMarkerSymbolPtr)ipCharacterMarkerSymbol);
	((ILayerColorLockPtr)ipMultiLayerSymbol)->put_LayerColorLock(0, VARIANT_FALSE);


	return S_OK;
}

HRESULT EvcSolverSymbolizer::CreateUnlocatedSymbol(ISymbol* pLocatedMarkerSymbol, ISymbol** ppUnlocatedMarkerSymbol)
{
	if (!ppUnlocatedMarkerSymbol || !pLocatedMarkerSymbol) return E_POINTER;

	if (!IMarkerSymbolPtr(pLocatedMarkerSymbol)) return E_INVALIDARG;

	IMultiLayerMarkerSymbolPtr ipMultiLayerSymbol;

	if (!IMultiLayerMarkerSymbolPtr(pLocatedMarkerSymbol))
	{
		ipMultiLayerSymbol.CreateInstance(CLSID_MultiLayerMarkerSymbol);

		IClonePtr ipCloned;

		((IClonePtr)pLocatedMarkerSymbol)->Clone(&ipCloned);
		ipMultiLayerSymbol->AddLayer((IMarkerSymbolPtr)ipCloned);
		((ILayerColorLockPtr)ipMultiLayerSymbol)->put_LayerColorLock(0, VARIANT_FALSE);
	}
	else
	{
		IClonePtr ipCloned;

		((IClonePtr)pLocatedMarkerSymbol)->Clone(&ipCloned);
		ipMultiLayerSymbol = ipCloned;
	}

	*ppUnlocatedMarkerSymbol = (ISymbolPtr)ipMultiLayerSymbol;
	if (*ppUnlocatedMarkerSymbol) (*ppUnlocatedMarkerSymbol)->AddRef();

	// Wash the symbol color
	double    symbolSize;
	IColorPtr ipLocatedColor;
	IColorPtr ipUnlocatedColor(CLSID_RgbColor);
	COLORREF  colorRef;

	((IMarkerSymbolPtr)ipMultiLayerSymbol)->get_Size(&symbolSize);
	((IMarkerSymbolPtr)ipMultiLayerSymbol)->get_Color(&ipLocatedColor);
	ipLocatedColor->get_RGBESRI(&colorRef);

	IHsvColorPtr ipHsvColor(CLSID_HsvColor);
	ipHsvColor->put_RGBESRI(colorRef);
	long s;
	ipHsvColor->get_Saturation(&s);
	if (s > c_maxFadedColorHSVSaturation)
	{
		ipHsvColor->put_Saturation(c_maxFadedColorHSVSaturation);
		ipHsvColor->get_RGBESRI(&colorRef);
	}

	ipUnlocatedColor->put_RGBESRI(colorRef);

	((IMarkerSymbolPtr)ipMultiLayerSymbol)->put_Color(ipUnlocatedColor);

	// Add a foreground locked red exclamation mark
	IFontDispPtr ipFontDisp;
	FONTDESC     fontDesc;

	::memset(&fontDesc, 0, sizeof(FONTDESC));
	fontDesc.cbSizeofstruct = sizeof(FONTDESC);
	fontDesc.lpstrName = ATL::CComBSTR(L"Arial Black");
	::OleCreateFontIndirect(&fontDesc, IID_IFontDisp, (void**)&ipFontDisp);
	if (!ipFontDisp) return E_INVALIDARG;

	IColorPtr ipRedColor(CLSID_RgbColor);

	ipRedColor->put_RGBESRI(RGB(255, 0, 0));

	ICharacterMarkerSymbolPtr ipCharacterMarkerSymbol;
	ipCharacterMarkerSymbol.CreateInstance(CLSID_CharacterMarkerSymbol);

	ipCharacterMarkerSymbol->put_Font(ipFontDisp);
	ipCharacterMarkerSymbol->put_CharacterIndex(63);
	ipCharacterMarkerSymbol->put_Color(ipRedColor);
	ipCharacterMarkerSymbol->put_Size(10);
	ipCharacterMarkerSymbol->put_XOffset(-9);
	ipCharacterMarkerSymbol->put_YOffset(1);

	ipMultiLayerSymbol->AddLayer((IMarkerSymbolPtr)ipCharacterMarkerSymbol);
	((ILayerColorLockPtr)ipMultiLayerSymbol)->put_LayerColorLock(0, VARIANT_TRUE);

	return S_OK;
}

HRESULT EvcSolverSymbolizer::CreateSimplePointRenderer(IColor* pPointColor, IFeatureRenderer** ppFRenderer)
{
	if (!pPointColor || !ppFRenderer) return E_POINTER;

	ISimpleRendererPtr ipRenderer(CLSID_SimpleRenderer);
	IColorPtr   ipErrorColor(CLSID_RgbColor);
	ipErrorColor->put_RGBESRI(RGB(255, 0, 0));

	ISymbolPtr ipPointSymbol(CLSID_SimpleMarkerSymbol);
	ISimpleMarkerSymbolPtr ipSimpleMarkerPointSymbol(ipPointSymbol);
	ipSimpleMarkerPointSymbol->put_Style(esriSMSCircle);
	ipSimpleMarkerPointSymbol->put_Color(pPointColor);
	ipSimpleMarkerPointSymbol->put_Size(3);

	ipRenderer->putref_Symbol(ipPointSymbol);

	*ppFRenderer = (IFeatureRendererPtr)ipRenderer;
	if (*ppFRenderer) (*ppFRenderer)->AddRef();

	return S_OK;
}

HRESULT EvcSolverSymbolizer::CreatePointRenderer(IColor* pPointColor, IFeatureRenderer** ppFRenderer)
{
	if (!pPointColor || !ppFRenderer) return E_POINTER;

	IUniqueValueRendererPtr ipRenderer(CLSID_UniqueValueRenderer);

	// Three symbols: Located, Unlocated and Error (default) are created for the renderer
	HRESULT     hr;
	IColorPtr   ipErrorColor(CLSID_RgbColor);
	ipErrorColor->put_RGBESRI(RGB(255, 0, 0));

	ISymbolPtr ipPointSymbol(CLSID_SimpleMarkerSymbol);
	ISimpleMarkerSymbolPtr ipSimpleMarkerPointSymbol(ipPointSymbol);
	ipSimpleMarkerPointSymbol->put_Style(esriSMSDiamond);
	ipSimpleMarkerPointSymbol->put_Color(pPointColor);
	ipSimpleMarkerPointSymbol->put_Size(12);

	ISymbolPtr ipErrorPointSymbol(CLSID_SimpleMarkerSymbol);
	ISimpleMarkerSymbolPtr ipSimpleMarkerErrorPointSymbol(ipErrorPointSymbol);
	ipSimpleMarkerErrorPointSymbol->put_Style(esriSMSDiamond);
	ipSimpleMarkerErrorPointSymbol->put_Color(ipErrorColor);
	ipSimpleMarkerErrorPointSymbol->put_Size(12);

	ISymbolPtr ipUnlocatedPointSymbol;
	if (FAILED(hr = CreateUnlocatedSymbol(ipPointSymbol, &ipUnlocatedPointSymbol))) return hr;

	ipRenderer->put_FieldCount(1);
	ipRenderer->put_Field(0, ATL::CComBSTR(CS_FIELD_STATUS));

	ipRenderer->put_DefaultSymbol(ipErrorPointSymbol);
	ipRenderer->put_DefaultLabel(ATL::CComBSTR(L"Error"));
	ipRenderer->put_UseDefaultSymbol(VARIANT_TRUE);

	ipRenderer->AddValue(ATL::CComBSTR(L"0"), ATL::CComBSTR(L""), ipPointSymbol);
	ipRenderer->put_Label(ATL::CComBSTR(L"0"), ATL::CComBSTR(L"Located"));

	ipRenderer->AddValue(ATL::CComBSTR(L"1"), ATL::CComBSTR(L""), ipUnlocatedPointSymbol);
	ipRenderer->put_Label(ATL::CComBSTR(L"1"), ATL::CComBSTR(L"Unlocated"));

	*ppFRenderer = (IFeatureRendererPtr)ipRenderer;
	if (*ppFRenderer) (*ppFRenderer)->AddRef();

	return S_OK;
}

HRESULT EvcSolverSymbolizer::CreateLineRenderer(IColor* pLineColor, IFeatureRenderer** ppFeatureRenderer)
{
	if (!pLineColor || !ppFeatureRenderer) return E_POINTER;

	ISimpleRendererPtr ipRenderer(CLSID_SimpleRenderer);
	ISimpleLineSymbolPtr ipLineSymbol(CLSID_SimpleLineSymbol);

	ipLineSymbol->put_Style(esriSLSSolid);
	ipLineSymbol->put_Color(pLineColor);
	ipLineSymbol->put_Width(2);

	ipRenderer->putref_Symbol((ISymbolPtr)ipLineSymbol);

	*ppFeatureRenderer = (IFeatureRendererPtr)ipRenderer;
	if (*ppFeatureRenderer) (*ppFeatureRenderer)->AddRef();

	return S_OK;
}

HRESULT EvcSolverSymbolizer::CreatePolygonRenderer(IColor* pLineColor, IFeatureRenderer** ppFeatureRenderer)
{
	if (!pLineColor || !ppFeatureRenderer) return E_POINTER;

	ISimpleRendererPtr ipRenderer(CLSID_SimpleRenderer);
	ISimpleFillSymbolPtr SymbolPtr(CLSID_SimpleFillSymbol);

	SymbolPtr->put_Style(esriSimpleFillStyle::esriSFSCross);
	SymbolPtr->put_Color(pLineColor);
	// SymbolPtr->put_Outline(2);

	ipRenderer->putref_Symbol((ISymbolPtr)SymbolPtr);

	*ppFeatureRenderer = (IFeatureRendererPtr)ipRenderer;
	if (*ppFeatureRenderer) (*ppFeatureRenderer)->AddRef();

	return S_OK;
}
