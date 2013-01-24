

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Tue Jan 22 18:57:37 2013
 */
/* Compiler settings for _EvcSolver.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_IEvcSolver,0x59da3a5d,0x109d,0x41f3,0xaf,0xdc,0x71,0x1b,0x52,0x62,0xc7,0x69);


MIDL_DEFINE_GUID(IID, LIBID_CustomSolver,0xF25947D1,0x9C81,0x48A6,0x9B,0xFF,0xCF,0x9E,0xB1,0x58,0xFF,0xD7);


MIDL_DEFINE_GUID(CLSID, CLSID_EvcSolver,0x7b081f99,0xb691,0x46b1,0xb7,0x56,0xaa,0x72,0x86,0x8c,0x86,0x83);


MIDL_DEFINE_GUID(CLSID, CLSID_EvcSolverPropPage,0xcd267b89,0x0144,0x4e7c,0x92,0x9e,0xbc,0xd5,0xd8,0x2f,0x4c,0x4d);


MIDL_DEFINE_GUID(CLSID, CLSID_EvcSolverSymbolizer,0x6e127557,0xb452,0x444d,0xac,0x96,0x3a,0x96,0xa4,0x0c,0x40,0x71);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



