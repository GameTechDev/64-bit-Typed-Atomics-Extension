//-----------------------------------------------------------------------------
// File: IntelExtensions_InternalUseOnly12.hlsl
//
// Desc: HLSL extensions available on Intel processor graphic platforms
//       Extensions defined in this file should be used only internally.
//
// Copyright (c) Intel Corporation 2020. All rights reserved.
//-----------------------------------------------------------------------------

#include "IntelExtensions12.hlsl"

//
// Define extension opcodes for internal usage (no enums in HLSL)
//

#define INTEL_EXT_GRADIENT_AND_BROADCAST    23

#define INTEL_EXT_UINT64_ATOMIC           24
#define INTEL_EXT_INT64_ATOMIC            25
#define INTEL_EXT_FLOAT_ATOMIC			  14

#define INTEL_EXT_FLOAT_ATOMIC_MIN		0
#define INTEL_EXT_FLOAT_ATOMIC_MAX		1
#define INTEL_EXT_FLOAT_ATOMIC_CMPSTORE	2

#define INTEL_EXT_ATOMIC_ADD		  0
#define INTEL_EXT_ATOMIC_MIN		  1
#define INTEL_EXT_ATOMIC_MAX	      2
#define INTEL_EXT_ATOMIC_CMPXCHG      3
#define INTEL_EXT_ATOMIC_XCHG	      4
#define INTEL_EXT_ATOMIC_AND	      5
#define INTEL_EXT_ATOMIC_OR		      6
#define INTEL_EXT_ATOMIC_XOR	      7

float IntelExt_GradientAndBroadcastScalar(float color)
{
    uint opcode = g_IntelExt.IncrementCounter();
    g_IntelExt[opcode].opcode = INTEL_EXT_GRADIENT_AND_BROADCAST;
    g_IntelExt[opcode].src0f.x = color.x;

	return g_IntelExt[opcode].dst0f.x;
}

float4 IntelExt_GradientAndBroadcastVec4(float4 color)
{
    return float4(
        IntelExt_GradientAndBroadcastScalar(color.x),
        IntelExt_GradientAndBroadcastScalar(color.y),
        IntelExt_GradientAndBroadcastScalar(color.z),
        IntelExt_GradientAndBroadcastScalar(color.w)
    );
}

float3 IntelExt_GradientAndBroadcastVec3(float3 color)
{
    return float3(
        IntelExt_GradientAndBroadcastScalar(color.x),
        IntelExt_GradientAndBroadcastScalar(color.y),
        IntelExt_GradientAndBroadcastScalar(color.z)
    );
}

float2 IntelExt_GradientAndBroadcastVec2(float2 color)
{
    return float2(
        IntelExt_GradientAndBroadcastScalar(color.x),
        IntelExt_GradientAndBroadcastScalar(color.y)
    );
}

// int64 atomics
uint2 IntelExt_InterlockedMaxUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    uav[uint2(opcode, opcode)] = uint2(0, 0); //dummy instruction to get the resource handle
    g_IntelExt[opcode].opcode = INTEL_EXT_UINT64_ATOMIC;
    g_IntelExt[opcode].src0u.xy = address;
    g_IntelExt[opcode].src1u.xy = value;
    g_IntelExt[opcode].src2u.x = INTEL_EXT_ATOMIC_MAX;

    return g_IntelExt[opcode].dst0u.xy;
}
