

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Mon Apr 16 21:31:50 2012
 */
/* Compiler settings for _EvcSolver.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef ___EvcSolver_h__
#define ___EvcSolver_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IEvcSolver_FWD_DEFINED__
#define __IEvcSolver_FWD_DEFINED__
typedef interface IEvcSolver IEvcSolver;
#endif 	/* __IEvcSolver_FWD_DEFINED__ */


#ifndef __EvcSolver_FWD_DEFINED__
#define __EvcSolver_FWD_DEFINED__

#ifdef __cplusplus
typedef class EvcSolver EvcSolver;
#else
typedef struct EvcSolver EvcSolver;
#endif /* __cplusplus */

#endif 	/* __EvcSolver_FWD_DEFINED__ */


#ifndef __EvcSolverSymbolizer_FWD_DEFINED__
#define __EvcSolverSymbolizer_FWD_DEFINED__

#ifdef __cplusplus
typedef class EvcSolverSymbolizer EvcSolverSymbolizer;
#else
typedef struct EvcSolverSymbolizer EvcSolverSymbolizer;
#endif /* __cplusplus */

#endif 	/* __EvcSolverSymbolizer_FWD_DEFINED__ */


#ifndef __EvcSolverPropPage_FWD_DEFINED__
#define __EvcSolverPropPage_FWD_DEFINED__

#ifdef __cplusplus
typedef class EvcSolverPropPage EvcSolverPropPage;
#else
typedef struct EvcSolverPropPage EvcSolverPropPage;
#endif /* __cplusplus */

#endif 	/* __EvcSolverPropPage_FWD_DEFINED__ */


/* header files for imported files */
#include "prsht.h"
#include "mshtml.h"
#include "mshtmhst.h"
#include "exdisp.h"
#include "objsafe.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IEvcSolver_INTERFACE_DEFINED__
#define __IEvcSolver_INTERFACE_DEFINED__

/* interface IEvcSolver */
/* [unique][helpstring][oleautomation][uuid][object] */ 


EXTERN_C const IID IID_IEvcSolver;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("59da3a5d-109d-41f3-afdc-711b5262c769")
    IEvcSolver : public IUnknown
    {
    public:
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SaturationPerCap( 
            /* [in] */ BSTR value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SaturationPerCap( 
            /* [retval][out] */ BSTR *value) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_CriticalDensPerCap( 
            /* [in] */ BSTR value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CriticalDensPerCap( 
            /* [retval][out] */ BSTR *value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DiscriptiveAttributes( 
            /* [retval][out] */ BSTR **names) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DiscriptiveAttributesCount( 
            /* [retval][out] */ int *count) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_CapacityAttribute( 
            /* [in] */ int index) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CapacityAttribute( 
            /* [retval][out] */ int *index) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SeparableEvacuee( 
            /* [in] */ VARIANT_BOOL value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SeparableEvacuee( 
            /* [retval][out] */ VARIANT_BOOL *value) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ExportEdgeStat( 
            /* [in] */ VARIANT_BOOL value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ExportEdgeStat( 
            /* [retval][out] */ VARIANT_BOOL *value) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SolverMethod( 
            /* [in] */ unsigned char value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SolverMethod( 
            /* [retval][out] */ unsigned char *value) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_TrafficModel( 
            /* [in] */ unsigned char value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_TrafficModel( 
            /* [retval][out] */ unsigned char *value) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_CostPerZoneDensity( 
            /* [in] */ BSTR value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CostPerZoneDensity( 
            /* [retval][out] */ BSTR *value) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_FlockingSimulationInterval( 
            /* [in] */ BSTR value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_FlockingSimulationInterval( 
            /* [retval][out] */ BSTR *value) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_FlockingSnapInterval( 
            /* [in] */ BSTR value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_FlockingSnapInterval( 
            /* [retval][out] */ BSTR *value) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_FlockingEnabled( 
            /* [in] */ VARIANT_BOOL value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_FlockingEnabled( 
            /* [retval][out] */ VARIANT_BOOL *value) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_TwoWayShareCapacity( 
            /* [in] */ VARIANT_BOOL value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_TwoWayShareCapacity( 
            /* [retval][out] */ VARIANT_BOOL *value) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_InitDelayCostPerPop( 
            /* [in] */ BSTR value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_InitDelayCostPerPop( 
            /* [retval][out] */ BSTR *value) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_FlockingProfile( 
            /* [in] */ unsigned char value) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_FlockingProfile( 
            /* [retval][out] */ unsigned char *value) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_CostAttribute( 
            /* [in] */ int index) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CostAttribute( 
            /* [retval][out] */ int *index) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CostAttributes( 
            /* [retval][out] */ BSTR **names) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CostAttributesCount( 
            /* [retval][out] */ int *count) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEvcSolverVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEvcSolver * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEvcSolver * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEvcSolver * This);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SaturationPerCap )( 
            IEvcSolver * This,
            /* [in] */ BSTR value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SaturationPerCap )( 
            IEvcSolver * This,
            /* [retval][out] */ BSTR *value);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CriticalDensPerCap )( 
            IEvcSolver * This,
            /* [in] */ BSTR value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CriticalDensPerCap )( 
            IEvcSolver * This,
            /* [retval][out] */ BSTR *value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscriptiveAttributes )( 
            IEvcSolver * This,
            /* [retval][out] */ BSTR **names);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscriptiveAttributesCount )( 
            IEvcSolver * This,
            /* [retval][out] */ int *count);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CapacityAttribute )( 
            IEvcSolver * This,
            /* [in] */ int index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CapacityAttribute )( 
            IEvcSolver * This,
            /* [retval][out] */ int *index);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SeparableEvacuee )( 
            IEvcSolver * This,
            /* [in] */ VARIANT_BOOL value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SeparableEvacuee )( 
            IEvcSolver * This,
            /* [retval][out] */ VARIANT_BOOL *value);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ExportEdgeStat )( 
            IEvcSolver * This,
            /* [in] */ VARIANT_BOOL value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExportEdgeStat )( 
            IEvcSolver * This,
            /* [retval][out] */ VARIANT_BOOL *value);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SolverMethod )( 
            IEvcSolver * This,
            /* [in] */ unsigned char value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SolverMethod )( 
            IEvcSolver * This,
            /* [retval][out] */ unsigned char *value);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TrafficModel )( 
            IEvcSolver * This,
            /* [in] */ unsigned char value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrafficModel )( 
            IEvcSolver * This,
            /* [retval][out] */ unsigned char *value);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CostPerZoneDensity )( 
            IEvcSolver * This,
            /* [in] */ BSTR value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CostPerZoneDensity )( 
            IEvcSolver * This,
            /* [retval][out] */ BSTR *value);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FlockingSimulationInterval )( 
            IEvcSolver * This,
            /* [in] */ BSTR value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FlockingSimulationInterval )( 
            IEvcSolver * This,
            /* [retval][out] */ BSTR *value);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FlockingSnapInterval )( 
            IEvcSolver * This,
            /* [in] */ BSTR value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FlockingSnapInterval )( 
            IEvcSolver * This,
            /* [retval][out] */ BSTR *value);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FlockingEnabled )( 
            IEvcSolver * This,
            /* [in] */ VARIANT_BOOL value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FlockingEnabled )( 
            IEvcSolver * This,
            /* [retval][out] */ VARIANT_BOOL *value);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TwoWayShareCapacity )( 
            IEvcSolver * This,
            /* [in] */ VARIANT_BOOL value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TwoWayShareCapacity )( 
            IEvcSolver * This,
            /* [retval][out] */ VARIANT_BOOL *value);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_InitDelayCostPerPop )( 
            IEvcSolver * This,
            /* [in] */ BSTR value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InitDelayCostPerPop )( 
            IEvcSolver * This,
            /* [retval][out] */ BSTR *value);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FlockingProfile )( 
            IEvcSolver * This,
            /* [in] */ unsigned char value);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FlockingProfile )( 
            IEvcSolver * This,
            /* [retval][out] */ unsigned char *value);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CostAttribute )( 
            IEvcSolver * This,
            /* [in] */ int index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CostAttribute )( 
            IEvcSolver * This,
            /* [retval][out] */ int *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CostAttributes )( 
            IEvcSolver * This,
            /* [retval][out] */ BSTR **names);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CostAttributesCount )( 
            IEvcSolver * This,
            /* [retval][out] */ int *count);
        
        END_INTERFACE
    } IEvcSolverVtbl;

    interface IEvcSolver
    {
        CONST_VTBL struct IEvcSolverVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEvcSolver_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IEvcSolver_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IEvcSolver_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IEvcSolver_put_SaturationPerCap(This,value)	\
    ( (This)->lpVtbl -> put_SaturationPerCap(This,value) ) 

#define IEvcSolver_get_SaturationPerCap(This,value)	\
    ( (This)->lpVtbl -> get_SaturationPerCap(This,value) ) 

#define IEvcSolver_put_CriticalDensPerCap(This,value)	\
    ( (This)->lpVtbl -> put_CriticalDensPerCap(This,value) ) 

#define IEvcSolver_get_CriticalDensPerCap(This,value)	\
    ( (This)->lpVtbl -> get_CriticalDensPerCap(This,value) ) 

#define IEvcSolver_get_DiscriptiveAttributes(This,names)	\
    ( (This)->lpVtbl -> get_DiscriptiveAttributes(This,names) ) 

#define IEvcSolver_get_DiscriptiveAttributesCount(This,count)	\
    ( (This)->lpVtbl -> get_DiscriptiveAttributesCount(This,count) ) 

#define IEvcSolver_put_CapacityAttribute(This,index)	\
    ( (This)->lpVtbl -> put_CapacityAttribute(This,index) ) 

#define IEvcSolver_get_CapacityAttribute(This,index)	\
    ( (This)->lpVtbl -> get_CapacityAttribute(This,index) ) 

#define IEvcSolver_put_SeparableEvacuee(This,value)	\
    ( (This)->lpVtbl -> put_SeparableEvacuee(This,value) ) 

#define IEvcSolver_get_SeparableEvacuee(This,value)	\
    ( (This)->lpVtbl -> get_SeparableEvacuee(This,value) ) 

#define IEvcSolver_put_ExportEdgeStat(This,value)	\
    ( (This)->lpVtbl -> put_ExportEdgeStat(This,value) ) 

#define IEvcSolver_get_ExportEdgeStat(This,value)	\
    ( (This)->lpVtbl -> get_ExportEdgeStat(This,value) ) 

#define IEvcSolver_put_SolverMethod(This,value)	\
    ( (This)->lpVtbl -> put_SolverMethod(This,value) ) 

#define IEvcSolver_get_SolverMethod(This,value)	\
    ( (This)->lpVtbl -> get_SolverMethod(This,value) ) 

#define IEvcSolver_put_TrafficModel(This,value)	\
    ( (This)->lpVtbl -> put_TrafficModel(This,value) ) 

#define IEvcSolver_get_TrafficModel(This,value)	\
    ( (This)->lpVtbl -> get_TrafficModel(This,value) ) 

#define IEvcSolver_put_CostPerZoneDensity(This,value)	\
    ( (This)->lpVtbl -> put_CostPerZoneDensity(This,value) ) 

#define IEvcSolver_get_CostPerZoneDensity(This,value)	\
    ( (This)->lpVtbl -> get_CostPerZoneDensity(This,value) ) 

#define IEvcSolver_put_FlockingSimulationInterval(This,value)	\
    ( (This)->lpVtbl -> put_FlockingSimulationInterval(This,value) ) 

#define IEvcSolver_get_FlockingSimulationInterval(This,value)	\
    ( (This)->lpVtbl -> get_FlockingSimulationInterval(This,value) ) 

#define IEvcSolver_put_FlockingSnapInterval(This,value)	\
    ( (This)->lpVtbl -> put_FlockingSnapInterval(This,value) ) 

#define IEvcSolver_get_FlockingSnapInterval(This,value)	\
    ( (This)->lpVtbl -> get_FlockingSnapInterval(This,value) ) 

#define IEvcSolver_put_FlockingEnabled(This,value)	\
    ( (This)->lpVtbl -> put_FlockingEnabled(This,value) ) 

#define IEvcSolver_get_FlockingEnabled(This,value)	\
    ( (This)->lpVtbl -> get_FlockingEnabled(This,value) ) 

#define IEvcSolver_put_TwoWayShareCapacity(This,value)	\
    ( (This)->lpVtbl -> put_TwoWayShareCapacity(This,value) ) 

#define IEvcSolver_get_TwoWayShareCapacity(This,value)	\
    ( (This)->lpVtbl -> get_TwoWayShareCapacity(This,value) ) 

#define IEvcSolver_put_InitDelayCostPerPop(This,value)	\
    ( (This)->lpVtbl -> put_InitDelayCostPerPop(This,value) ) 

#define IEvcSolver_get_InitDelayCostPerPop(This,value)	\
    ( (This)->lpVtbl -> get_InitDelayCostPerPop(This,value) ) 

#define IEvcSolver_put_FlockingProfile(This,value)	\
    ( (This)->lpVtbl -> put_FlockingProfile(This,value) ) 

#define IEvcSolver_get_FlockingProfile(This,value)	\
    ( (This)->lpVtbl -> get_FlockingProfile(This,value) ) 

#define IEvcSolver_put_CostAttribute(This,index)	\
    ( (This)->lpVtbl -> put_CostAttribute(This,index) ) 

#define IEvcSolver_get_CostAttribute(This,index)	\
    ( (This)->lpVtbl -> get_CostAttribute(This,index) ) 

#define IEvcSolver_get_CostAttributes(This,names)	\
    ( (This)->lpVtbl -> get_CostAttributes(This,names) ) 

#define IEvcSolver_get_CostAttributesCount(This,count)	\
    ( (This)->lpVtbl -> get_CostAttributesCount(This,count) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IEvcSolver_INTERFACE_DEFINED__ */



#ifndef __CustomSolver_LIBRARY_DEFINED__
#define __CustomSolver_LIBRARY_DEFINED__

/* library CustomSolver */
/* [helpstring][uuid][version] */ 


EXTERN_C const IID LIBID_CustomSolver;

EXTERN_C const CLSID CLSID_EvcSolver;

#ifdef __cplusplus

class DECLSPEC_UUID("7b081f99-b691-46b1-b756-aa72868c8683")
EvcSolver;
#endif

EXTERN_C const CLSID CLSID_EvcSolverSymbolizer;

#ifdef __cplusplus

class DECLSPEC_UUID("6e127557-b452-444d-ac96-3a96a40c4071")
EvcSolverSymbolizer;
#endif

EXTERN_C const CLSID CLSID_EvcSolverPropPage;

#ifdef __cplusplus

class DECLSPEC_UUID("cd267b89-0144-4e7c-929e-bcd5d82f4c4d")
EvcSolverPropPage;
#endif
#endif /* __CustomSolver_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


