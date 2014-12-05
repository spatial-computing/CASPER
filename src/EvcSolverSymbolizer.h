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
// Evacuation Solver: Symbolizer definition
// Description: 
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#pragma once

#include "resource.h"             // main symbols
#include "CatIDs\ArcCATIDs.h"     // component category IDs

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

// EvcSolverSymbolizer
[
  coclass,
  default(INASymbolizer2),
  threading(apartment),
  vi_progid("CustomSolver.EvcSolverSymbolizer"),
  progid("CustomSolver.EvcSolverSymbolizer.1"),
  version(1.0),
  uuid("6e127557-b452-444d-ac96-3a96a40c4071"),
  helpstring("EvcSolverSymbolizer Class")
]
class ATL_NO_VTABLE EvcSolverSymbolizer : public INASymbolizer2
{
public:
  EvcSolverSymbolizer() :
    c_randomColorHSVSaturation(100),
    c_baseRandomColorHSVValue(75),
    c_maxAboveBaseRandomColorHSVValue(25),
    c_maxFadedColorHSVSaturation(20)
  {
  }

  DECLARE_PROTECT_FINAL_CONSTRUCT()

  // Register the symbolizer in the Network Analyst symbolizers component category so that it can be dynamically applied
  // to a EvcSolver where appropriate.  For example, on the Reset Symbology" context menu item of the NALayer.
  BEGIN_CATEGORY_MAP(EvcSolverSymbolizer)
    IMPLEMENTED_CATEGORY(__uuidof(CATID_NetworkAnalystSymbolizer))
  END_CATEGORY_MAP()

  HRESULT FinalConstruct()
  {
    return S_OK;
  }

  void FinalRelease()
  {
  }

public:

  // INASymbolizer
  STDMETHOD(Applies)(INAContext* pNAContext, VARIANT_BOOL* pFlag);
  STDMETHOD(get_Priority)(long* pPriority);
  STDMETHOD(CreateLayer)(INAContext* pNAContext, INALayer** ppNALayer);
  STDMETHOD(UpdateLayer)(INALayer* pNALayer, VARIANT_BOOL* pUpdated);

  // INASymbolizer2 methods
  STDMETHOD(ResetRenderers)(IColor* pSolverColor, INALayer* pNALayer);

private:

  HRESULT CreateRandomColor(IColor** ppColor);
  HRESULT CreatePointRenderer(IColor* pPointColor, IFeatureRenderer** ppFRenderer);
  HRESULT CreateSimplePointRenderer(IColor* pPointColor, IFeatureRenderer** ppFRenderer);
  HRESULT CreateBarrierRenderer(IColor* pBarrierColor, IFeatureRenderer** ppFRenderer);
  HRESULT CreateLineRenderer(IColor* pLineColor, IFeatureRenderer** ppFeatureRenderer);
  HRESULT CreateUnlocatedSymbol(ISymbol* pLocatedMarkerSymbol, ISymbol** ppUnlocatedMarkerSymbol);
  HRESULT CreateCharacterMarkerSymbol(ATL::CString   fontName,
                                      IColor*   pMarkerColor,
                                      IColor*   pMarkerBackgroundColor,
                                      long      characterIndex,
                                      long      backgoundCharacterIndex,
                                      double    markerSize,
                                      double    makerBackgroundSize,
                                      double    markerAngle,
                                      ISymbol** ppMarkerSymbol);

  const long c_randomColorHSVSaturation;
  const long c_baseRandomColorHSVValue;
  const long c_maxAboveBaseRandomColorHSVValue;
  const long c_maxFadedColorHSVSaturation;
};
