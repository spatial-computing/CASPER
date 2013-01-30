

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Tue Jan 29 19:59:00 2013
 */
/* Compiler settings for _EvcSolver.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif /* __RPCPROXY_H_VERSION__ */


#include "_EvcSolver.h"

#define TYPE_FORMAT_STRING_SIZE   77                                
#define PROC_FORMAT_STRING_SIZE   1369                              
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   1            

typedef struct __EvcSolver_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } _EvcSolver_MIDL_TYPE_FORMAT_STRING;

typedef struct __EvcSolver_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } _EvcSolver_MIDL_PROC_FORMAT_STRING;

typedef struct __EvcSolver_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } _EvcSolver_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const _EvcSolver_MIDL_TYPE_FORMAT_STRING _EvcSolver__MIDL_TypeFormatString;
extern const _EvcSolver_MIDL_PROC_FORMAT_STRING _EvcSolver__MIDL_ProcFormatString;
extern const _EvcSolver_MIDL_EXPR_FORMAT_STRING _EvcSolver__MIDL_ExprFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IEvcSolver_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IEvcSolver_ProxyInfo;


extern const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ];

#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const _EvcSolver_MIDL_PROC_FORMAT_STRING _EvcSolver__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure put_SaturationPerCap */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 16 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x1 ),	/* 1 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 26 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 28 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 30 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Return value */

/* 32 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 34 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 36 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_SaturationPerCap */

/* 38 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 40 */	NdrFcLong( 0x0 ),	/* 0 */
/* 44 */	NdrFcShort( 0x4 ),	/* 4 */
/* 46 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 48 */	NdrFcShort( 0x0 ),	/* 0 */
/* 50 */	NdrFcShort( 0x8 ),	/* 8 */
/* 52 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 54 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 56 */	NdrFcShort( 0x1 ),	/* 1 */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */
/* 60 */	NdrFcShort( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 64 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 66 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 68 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Return value */

/* 70 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 72 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 74 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_CriticalDensPerCap */

/* 76 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 78 */	NdrFcLong( 0x0 ),	/* 0 */
/* 82 */	NdrFcShort( 0x5 ),	/* 5 */
/* 84 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 86 */	NdrFcShort( 0x0 ),	/* 0 */
/* 88 */	NdrFcShort( 0x8 ),	/* 8 */
/* 90 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 92 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 94 */	NdrFcShort( 0x0 ),	/* 0 */
/* 96 */	NdrFcShort( 0x1 ),	/* 1 */
/* 98 */	NdrFcShort( 0x0 ),	/* 0 */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 102 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 104 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 106 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Return value */

/* 108 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 110 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 112 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_CriticalDensPerCap */

/* 114 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 116 */	NdrFcLong( 0x0 ),	/* 0 */
/* 120 */	NdrFcShort( 0x6 ),	/* 6 */
/* 122 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 124 */	NdrFcShort( 0x0 ),	/* 0 */
/* 126 */	NdrFcShort( 0x8 ),	/* 8 */
/* 128 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 130 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 132 */	NdrFcShort( 0x1 ),	/* 1 */
/* 134 */	NdrFcShort( 0x0 ),	/* 0 */
/* 136 */	NdrFcShort( 0x0 ),	/* 0 */
/* 138 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 140 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 142 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 144 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Return value */

/* 146 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 148 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 150 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_DiscriptiveAttributes */

/* 152 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 154 */	NdrFcLong( 0x0 ),	/* 0 */
/* 158 */	NdrFcShort( 0x7 ),	/* 7 */
/* 160 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 162 */	NdrFcShort( 0x0 ),	/* 0 */
/* 164 */	NdrFcShort( 0x8 ),	/* 8 */
/* 166 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 168 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 170 */	NdrFcShort( 0x1 ),	/* 1 */
/* 172 */	NdrFcShort( 0x0 ),	/* 0 */
/* 174 */	NdrFcShort( 0x0 ),	/* 0 */
/* 176 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter names */

/* 178 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 180 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 182 */	NdrFcShort( 0x38 ),	/* Type Offset=56 */

	/* Return value */

/* 184 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 186 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 188 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_DiscriptiveAttributesCount */

/* 190 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 192 */	NdrFcLong( 0x0 ),	/* 0 */
/* 196 */	NdrFcShort( 0x8 ),	/* 8 */
/* 198 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0x24 ),	/* 36 */
/* 204 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 206 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 208 */	NdrFcShort( 0x0 ),	/* 0 */
/* 210 */	NdrFcShort( 0x0 ),	/* 0 */
/* 212 */	NdrFcShort( 0x0 ),	/* 0 */
/* 214 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter count */

/* 216 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 218 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 220 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 222 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 224 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 226 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_CapacityAttribute */

/* 228 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 230 */	NdrFcLong( 0x0 ),	/* 0 */
/* 234 */	NdrFcShort( 0x9 ),	/* 9 */
/* 236 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 238 */	NdrFcShort( 0x8 ),	/* 8 */
/* 240 */	NdrFcShort( 0x8 ),	/* 8 */
/* 242 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 244 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 246 */	NdrFcShort( 0x0 ),	/* 0 */
/* 248 */	NdrFcShort( 0x0 ),	/* 0 */
/* 250 */	NdrFcShort( 0x0 ),	/* 0 */
/* 252 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */

/* 254 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 256 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 258 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 260 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 262 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 264 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_CapacityAttribute */

/* 266 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 268 */	NdrFcLong( 0x0 ),	/* 0 */
/* 272 */	NdrFcShort( 0xa ),	/* 10 */
/* 274 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 276 */	NdrFcShort( 0x0 ),	/* 0 */
/* 278 */	NdrFcShort( 0x24 ),	/* 36 */
/* 280 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 282 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 284 */	NdrFcShort( 0x0 ),	/* 0 */
/* 286 */	NdrFcShort( 0x0 ),	/* 0 */
/* 288 */	NdrFcShort( 0x0 ),	/* 0 */
/* 290 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */

/* 292 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 294 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 296 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 298 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 300 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 302 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_SeparableEvacuee */

/* 304 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 306 */	NdrFcLong( 0x0 ),	/* 0 */
/* 310 */	NdrFcShort( 0xb ),	/* 11 */
/* 312 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 314 */	NdrFcShort( 0x6 ),	/* 6 */
/* 316 */	NdrFcShort( 0x8 ),	/* 8 */
/* 318 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 320 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 324 */	NdrFcShort( 0x0 ),	/* 0 */
/* 326 */	NdrFcShort( 0x0 ),	/* 0 */
/* 328 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 330 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 332 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 334 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 336 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 338 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 340 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_SeparableEvacuee */

/* 342 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 344 */	NdrFcLong( 0x0 ),	/* 0 */
/* 348 */	NdrFcShort( 0xc ),	/* 12 */
/* 350 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 352 */	NdrFcShort( 0x0 ),	/* 0 */
/* 354 */	NdrFcShort( 0x22 ),	/* 34 */
/* 356 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 358 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 360 */	NdrFcShort( 0x0 ),	/* 0 */
/* 362 */	NdrFcShort( 0x0 ),	/* 0 */
/* 364 */	NdrFcShort( 0x0 ),	/* 0 */
/* 366 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 368 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 370 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 372 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 374 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 376 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 378 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_ExportEdgeStat */

/* 380 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 382 */	NdrFcLong( 0x0 ),	/* 0 */
/* 386 */	NdrFcShort( 0xd ),	/* 13 */
/* 388 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 390 */	NdrFcShort( 0x6 ),	/* 6 */
/* 392 */	NdrFcShort( 0x8 ),	/* 8 */
/* 394 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 396 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 398 */	NdrFcShort( 0x0 ),	/* 0 */
/* 400 */	NdrFcShort( 0x0 ),	/* 0 */
/* 402 */	NdrFcShort( 0x0 ),	/* 0 */
/* 404 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 406 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 408 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 410 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 412 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 414 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 416 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_ExportEdgeStat */

/* 418 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 420 */	NdrFcLong( 0x0 ),	/* 0 */
/* 424 */	NdrFcShort( 0xe ),	/* 14 */
/* 426 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 428 */	NdrFcShort( 0x0 ),	/* 0 */
/* 430 */	NdrFcShort( 0x22 ),	/* 34 */
/* 432 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 434 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 436 */	NdrFcShort( 0x0 ),	/* 0 */
/* 438 */	NdrFcShort( 0x0 ),	/* 0 */
/* 440 */	NdrFcShort( 0x0 ),	/* 0 */
/* 442 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 444 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 446 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 448 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 450 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 452 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 454 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_SolverMethod */

/* 456 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 458 */	NdrFcLong( 0x0 ),	/* 0 */
/* 462 */	NdrFcShort( 0xf ),	/* 15 */
/* 464 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 466 */	NdrFcShort( 0x5 ),	/* 5 */
/* 468 */	NdrFcShort( 0x8 ),	/* 8 */
/* 470 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 472 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 474 */	NdrFcShort( 0x0 ),	/* 0 */
/* 476 */	NdrFcShort( 0x0 ),	/* 0 */
/* 478 */	NdrFcShort( 0x0 ),	/* 0 */
/* 480 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 482 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 484 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 486 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Return value */

/* 488 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 490 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 492 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_SolverMethod */

/* 494 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 496 */	NdrFcLong( 0x0 ),	/* 0 */
/* 500 */	NdrFcShort( 0x10 ),	/* 16 */
/* 502 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 504 */	NdrFcShort( 0x0 ),	/* 0 */
/* 506 */	NdrFcShort( 0x21 ),	/* 33 */
/* 508 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 510 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 512 */	NdrFcShort( 0x0 ),	/* 0 */
/* 514 */	NdrFcShort( 0x0 ),	/* 0 */
/* 516 */	NdrFcShort( 0x0 ),	/* 0 */
/* 518 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 520 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 522 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 524 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Return value */

/* 526 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 528 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 530 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_TrafficModel */

/* 532 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 534 */	NdrFcLong( 0x0 ),	/* 0 */
/* 538 */	NdrFcShort( 0x11 ),	/* 17 */
/* 540 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 542 */	NdrFcShort( 0x5 ),	/* 5 */
/* 544 */	NdrFcShort( 0x8 ),	/* 8 */
/* 546 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 548 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 550 */	NdrFcShort( 0x0 ),	/* 0 */
/* 552 */	NdrFcShort( 0x0 ),	/* 0 */
/* 554 */	NdrFcShort( 0x0 ),	/* 0 */
/* 556 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 558 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 560 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 562 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Return value */

/* 564 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 566 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 568 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_TrafficModel */

/* 570 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 572 */	NdrFcLong( 0x0 ),	/* 0 */
/* 576 */	NdrFcShort( 0x12 ),	/* 18 */
/* 578 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 580 */	NdrFcShort( 0x0 ),	/* 0 */
/* 582 */	NdrFcShort( 0x21 ),	/* 33 */
/* 584 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 586 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 588 */	NdrFcShort( 0x0 ),	/* 0 */
/* 590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 592 */	NdrFcShort( 0x0 ),	/* 0 */
/* 594 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 596 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 598 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 600 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Return value */

/* 602 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 604 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 606 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_CostPerZoneDensity */

/* 608 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 610 */	NdrFcLong( 0x0 ),	/* 0 */
/* 614 */	NdrFcShort( 0x13 ),	/* 19 */
/* 616 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 618 */	NdrFcShort( 0x0 ),	/* 0 */
/* 620 */	NdrFcShort( 0x8 ),	/* 8 */
/* 622 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 624 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 626 */	NdrFcShort( 0x0 ),	/* 0 */
/* 628 */	NdrFcShort( 0x1 ),	/* 1 */
/* 630 */	NdrFcShort( 0x0 ),	/* 0 */
/* 632 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 634 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 636 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 638 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Return value */

/* 640 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 642 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 644 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_CostPerZoneDensity */

/* 646 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 648 */	NdrFcLong( 0x0 ),	/* 0 */
/* 652 */	NdrFcShort( 0x14 ),	/* 20 */
/* 654 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 656 */	NdrFcShort( 0x0 ),	/* 0 */
/* 658 */	NdrFcShort( 0x8 ),	/* 8 */
/* 660 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 662 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 664 */	NdrFcShort( 0x1 ),	/* 1 */
/* 666 */	NdrFcShort( 0x0 ),	/* 0 */
/* 668 */	NdrFcShort( 0x0 ),	/* 0 */
/* 670 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 672 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 674 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 676 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Return value */

/* 678 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 680 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 682 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_FlockingSimulationInterval */

/* 684 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 686 */	NdrFcLong( 0x0 ),	/* 0 */
/* 690 */	NdrFcShort( 0x15 ),	/* 21 */
/* 692 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 694 */	NdrFcShort( 0x0 ),	/* 0 */
/* 696 */	NdrFcShort( 0x8 ),	/* 8 */
/* 698 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 700 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 702 */	NdrFcShort( 0x0 ),	/* 0 */
/* 704 */	NdrFcShort( 0x1 ),	/* 1 */
/* 706 */	NdrFcShort( 0x0 ),	/* 0 */
/* 708 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 710 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 712 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 714 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Return value */

/* 716 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 718 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 720 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_FlockingSimulationInterval */

/* 722 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 724 */	NdrFcLong( 0x0 ),	/* 0 */
/* 728 */	NdrFcShort( 0x16 ),	/* 22 */
/* 730 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 732 */	NdrFcShort( 0x0 ),	/* 0 */
/* 734 */	NdrFcShort( 0x8 ),	/* 8 */
/* 736 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 738 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 740 */	NdrFcShort( 0x1 ),	/* 1 */
/* 742 */	NdrFcShort( 0x0 ),	/* 0 */
/* 744 */	NdrFcShort( 0x0 ),	/* 0 */
/* 746 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 748 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 750 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 752 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Return value */

/* 754 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 756 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 758 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_FlockingSnapInterval */

/* 760 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 762 */	NdrFcLong( 0x0 ),	/* 0 */
/* 766 */	NdrFcShort( 0x17 ),	/* 23 */
/* 768 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 770 */	NdrFcShort( 0x0 ),	/* 0 */
/* 772 */	NdrFcShort( 0x8 ),	/* 8 */
/* 774 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 776 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 778 */	NdrFcShort( 0x0 ),	/* 0 */
/* 780 */	NdrFcShort( 0x1 ),	/* 1 */
/* 782 */	NdrFcShort( 0x0 ),	/* 0 */
/* 784 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 786 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 788 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 790 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Return value */

/* 792 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 794 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 796 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_FlockingSnapInterval */

/* 798 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 800 */	NdrFcLong( 0x0 ),	/* 0 */
/* 804 */	NdrFcShort( 0x18 ),	/* 24 */
/* 806 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 808 */	NdrFcShort( 0x0 ),	/* 0 */
/* 810 */	NdrFcShort( 0x8 ),	/* 8 */
/* 812 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 814 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 816 */	NdrFcShort( 0x1 ),	/* 1 */
/* 818 */	NdrFcShort( 0x0 ),	/* 0 */
/* 820 */	NdrFcShort( 0x0 ),	/* 0 */
/* 822 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 824 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 826 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 828 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Return value */

/* 830 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 832 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 834 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_FlockingEnabled */

/* 836 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 838 */	NdrFcLong( 0x0 ),	/* 0 */
/* 842 */	NdrFcShort( 0x19 ),	/* 25 */
/* 844 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 846 */	NdrFcShort( 0x6 ),	/* 6 */
/* 848 */	NdrFcShort( 0x8 ),	/* 8 */
/* 850 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 852 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 854 */	NdrFcShort( 0x0 ),	/* 0 */
/* 856 */	NdrFcShort( 0x0 ),	/* 0 */
/* 858 */	NdrFcShort( 0x0 ),	/* 0 */
/* 860 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 862 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 864 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 866 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 868 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 870 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 872 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_FlockingEnabled */

/* 874 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 876 */	NdrFcLong( 0x0 ),	/* 0 */
/* 880 */	NdrFcShort( 0x1a ),	/* 26 */
/* 882 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 884 */	NdrFcShort( 0x0 ),	/* 0 */
/* 886 */	NdrFcShort( 0x22 ),	/* 34 */
/* 888 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 890 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 892 */	NdrFcShort( 0x0 ),	/* 0 */
/* 894 */	NdrFcShort( 0x0 ),	/* 0 */
/* 896 */	NdrFcShort( 0x0 ),	/* 0 */
/* 898 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 900 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 902 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 904 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 906 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 908 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 910 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_TwoWayShareCapacity */

/* 912 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 914 */	NdrFcLong( 0x0 ),	/* 0 */
/* 918 */	NdrFcShort( 0x1b ),	/* 27 */
/* 920 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 922 */	NdrFcShort( 0x6 ),	/* 6 */
/* 924 */	NdrFcShort( 0x8 ),	/* 8 */
/* 926 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 928 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 930 */	NdrFcShort( 0x0 ),	/* 0 */
/* 932 */	NdrFcShort( 0x0 ),	/* 0 */
/* 934 */	NdrFcShort( 0x0 ),	/* 0 */
/* 936 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 938 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 940 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 942 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 944 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 946 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 948 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_TwoWayShareCapacity */

/* 950 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 952 */	NdrFcLong( 0x0 ),	/* 0 */
/* 956 */	NdrFcShort( 0x1c ),	/* 28 */
/* 958 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 960 */	NdrFcShort( 0x0 ),	/* 0 */
/* 962 */	NdrFcShort( 0x22 ),	/* 34 */
/* 964 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 966 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 968 */	NdrFcShort( 0x0 ),	/* 0 */
/* 970 */	NdrFcShort( 0x0 ),	/* 0 */
/* 972 */	NdrFcShort( 0x0 ),	/* 0 */
/* 974 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 976 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 978 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 980 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 982 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 984 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 986 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_InitDelayCostPerPop */

/* 988 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 990 */	NdrFcLong( 0x0 ),	/* 0 */
/* 994 */	NdrFcShort( 0x1d ),	/* 29 */
/* 996 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 998 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1000 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1002 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1004 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1006 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1008 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1010 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1012 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 1014 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1016 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1018 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Return value */

/* 1020 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1022 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1024 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_InitDelayCostPerPop */

/* 1026 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1028 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1032 */	NdrFcShort( 0x1e ),	/* 30 */
/* 1034 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1036 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1038 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1040 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1042 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1044 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1046 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1048 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1050 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 1052 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1054 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1056 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Return value */

/* 1058 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1060 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1062 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_FlockingProfile */

/* 1064 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1066 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1070 */	NdrFcShort( 0x1f ),	/* 31 */
/* 1072 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1074 */	NdrFcShort( 0x5 ),	/* 5 */
/* 1076 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1078 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 1080 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1082 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1084 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1086 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1088 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 1090 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1092 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1094 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Return value */

/* 1096 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1098 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1100 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_FlockingProfile */

/* 1102 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1104 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1108 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1110 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1112 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1114 */	NdrFcShort( 0x21 ),	/* 33 */
/* 1116 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 1118 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1120 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1124 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1126 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 1128 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1130 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1132 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Return value */

/* 1134 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1136 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1138 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_EvacueeBucketSize */

/* 1140 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1142 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1146 */	NdrFcShort( 0x21 ),	/* 33 */
/* 1148 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1150 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1152 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1154 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1156 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1160 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1162 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1164 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 1166 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1168 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1170 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Return value */

/* 1172 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1174 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1176 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_EvacueeBucketSize */

/* 1178 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1180 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1184 */	NdrFcShort( 0x22 ),	/* 34 */
/* 1186 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1188 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1190 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1192 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1194 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1196 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1202 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 1204 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1206 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1208 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Return value */

/* 1210 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1212 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1214 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_CostAttribute */

/* 1216 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1218 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1222 */	NdrFcShort( 0x23 ),	/* 35 */
/* 1224 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1226 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1228 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1230 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 1232 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1234 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1236 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1238 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1240 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */

/* 1242 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1244 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1246 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1248 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1250 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1252 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_CostAttribute */

/* 1254 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1256 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1260 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1262 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1264 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1266 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1268 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 1270 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1272 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1274 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1276 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1278 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */

/* 1280 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1282 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1284 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1286 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1288 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1290 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_CostAttributes */

/* 1292 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1294 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1298 */	NdrFcShort( 0x25 ),	/* 37 */
/* 1300 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1302 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1304 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1306 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1308 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1310 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1312 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1314 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1316 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter names */

/* 1318 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 1320 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1322 */	NdrFcShort( 0x38 ),	/* Type Offset=56 */

	/* Return value */

/* 1324 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1326 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1328 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_CostAttributesCount */

/* 1330 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1332 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1336 */	NdrFcShort( 0x26 ),	/* 38 */
/* 1338 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1340 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1342 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1344 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 1346 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1348 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1350 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1352 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1354 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter count */

/* 1356 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1358 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1360 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1362 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1364 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1366 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const _EvcSolver_MIDL_TYPE_FORMAT_STRING _EvcSolver__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x12, 0x0,	/* FC_UP */
/*  4 */	NdrFcShort( 0xe ),	/* Offset= 14 (18) */
/*  6 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/*  8 */	NdrFcShort( 0x2 ),	/* 2 */
/* 10 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 12 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 14 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 16 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 18 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 20 */	NdrFcShort( 0x8 ),	/* 8 */
/* 22 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (6) */
/* 24 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 26 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 28 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 30 */	NdrFcShort( 0x0 ),	/* 0 */
/* 32 */	NdrFcShort( 0x8 ),	/* 8 */
/* 34 */	NdrFcShort( 0x0 ),	/* 0 */
/* 36 */	NdrFcShort( 0xffde ),	/* Offset= -34 (2) */
/* 38 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 40 */	NdrFcShort( 0x6 ),	/* Offset= 6 (46) */
/* 42 */	
			0x13, 0x0,	/* FC_OP */
/* 44 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (18) */
/* 46 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 48 */	NdrFcShort( 0x0 ),	/* 0 */
/* 50 */	NdrFcShort( 0x8 ),	/* 8 */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */
/* 54 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (42) */
/* 56 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 58 */	NdrFcShort( 0x2 ),	/* Offset= 2 (60) */
/* 60 */	
			0x13, 0x0,	/* FC_OP */
/* 62 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (46) */
/* 64 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 66 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 68 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 70 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 72 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 74 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            BSTR_UserSize
            ,BSTR_UserMarshal
            ,BSTR_UserUnmarshal
            ,BSTR_UserFree
            }

        };



/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IEvcSolver, ver. 0.0,
   GUID={0x59da3a5d,0x109d,0x41f3,{0xaf,0xdc,0x71,0x1b,0x52,0x62,0xc7,0x69}} */

#pragma code_seg(".orpc")
static const unsigned short IEvcSolver_FormatStringOffsetTable[] =
    {
    0,
    38,
    76,
    114,
    152,
    190,
    228,
    266,
    304,
    342,
    380,
    418,
    456,
    494,
    532,
    570,
    608,
    646,
    684,
    722,
    760,
    798,
    836,
    874,
    912,
    950,
    988,
    1026,
    1064,
    1102,
    1140,
    1178,
    1216,
    1254,
    1292,
    1330
    };

static const MIDL_STUBLESS_PROXY_INFO IEvcSolver_ProxyInfo =
    {
    &Object_StubDesc,
    _EvcSolver__MIDL_ProcFormatString.Format,
    &IEvcSolver_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IEvcSolver_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    _EvcSolver__MIDL_ProcFormatString.Format,
    &IEvcSolver_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(39) _IEvcSolverProxyVtbl = 
{
    &IEvcSolver_ProxyInfo,
    &IID_IEvcSolver,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_SaturationPerCap */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_SaturationPerCap */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_CriticalDensPerCap */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_CriticalDensPerCap */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_DiscriptiveAttributes */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_DiscriptiveAttributesCount */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_CapacityAttribute */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_CapacityAttribute */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_SeparableEvacuee */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_SeparableEvacuee */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_ExportEdgeStat */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_ExportEdgeStat */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_SolverMethod */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_SolverMethod */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_TrafficModel */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_TrafficModel */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_CostPerZoneDensity */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_CostPerZoneDensity */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_FlockingSimulationInterval */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_FlockingSimulationInterval */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_FlockingSnapInterval */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_FlockingSnapInterval */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_FlockingEnabled */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_FlockingEnabled */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_TwoWayShareCapacity */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_TwoWayShareCapacity */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_InitDelayCostPerPop */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_InitDelayCostPerPop */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_FlockingProfile */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_FlockingProfile */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_EvacueeBucketSize */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_EvacueeBucketSize */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::put_CostAttribute */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_CostAttribute */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_CostAttributes */ ,
    (void *) (INT_PTR) -1 /* IEvcSolver::get_CostAttributesCount */
};

const CInterfaceStubVtbl _IEvcSolverStubVtbl =
{
    &IID_IEvcSolver,
    &IEvcSolver_ServerInfo,
    39,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    _EvcSolver__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x700022b, /* MIDL Version 7.0.555 */
    0,
    UserMarshalRoutines,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };

const CInterfaceProxyVtbl * const __EvcSolver_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IEvcSolverProxyVtbl,
    0
};

const CInterfaceStubVtbl * const __EvcSolver_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IEvcSolverStubVtbl,
    0
};

PCInterfaceName const __EvcSolver_InterfaceNamesList[] = 
{
    "IEvcSolver",
    0
};


#define __EvcSolver_CHECK_IID(n)	IID_GENERIC_CHECK_IID( __EvcSolver, pIID, n)

int __stdcall __EvcSolver_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!__EvcSolver_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

const ExtendedProxyFileInfo _EvcSolver_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & __EvcSolver_ProxyVtblList,
    (PCInterfaceStubVtblList *) & __EvcSolver_StubVtblList,
    (const PCInterfaceName * ) & __EvcSolver_InterfaceNamesList,
    0, /* no delegation */
    & __EvcSolver_IID_Lookup, 
    1,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/

