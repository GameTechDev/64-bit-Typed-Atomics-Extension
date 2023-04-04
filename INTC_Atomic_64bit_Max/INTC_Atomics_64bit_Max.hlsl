#define INTEL_SHADER_EXT_UAV_SLOT u7
#include "IntelExtensions12.hlsl"

#define TEX_WIDTH 640
#define TEX_HEIGHT 480

RWTexture2D<uint2> outputbuff : register(u0);

[numthreads(32, 32, 1)]
void CSMain(uint2 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
	uint2 address = uint2(DTid.x, DTid.y);

	uint2 value1 = uint2(TEX_HEIGHT / 2, 0);
	uint2 value2 = uint2(DTid.y, 0);

	outputbuff[address] = value2;

	IntelExt_Init();
	IntelExt_InterlockedMaxUint64(outputbuff, address, value1);

	if (outputbuff[address].x >= value1.x)
	{
		outputbuff[address] = uint2(0xffffffff, 0xffffffff);
	}
	else
	{
		outputbuff[address] = uint2(0, 0);
	}
}
