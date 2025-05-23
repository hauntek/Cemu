#include "Cafe/HW/Latte/Core/LatteConst.h"
#include "Cafe/HW/Latte/Core/LatteShaderAssembly.h"
#include "Cafe/HW/Latte/ISA/RegDefines.h"
#include "Cafe/HW/Latte/Core/Latte.h"
#include "Cafe/HW/Latte/Core/LatteDraw.h"
#include "Cafe/HW/Latte/LegacyShaderDecompiler/LatteDecompiler.h"
#include "Cafe/HW/Latte/Core/FetchShader.h"
#include "Cafe/HW/Latte/Renderer/Renderer.h"
#include "util/helpers/StringBuf.h"

#define _CRLF	"\r\n"

static void _readLittleEndianAttributeU32x4(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder = in.attrDataSem{};" _CRLF, attributeInputIndex);
}

static void _readLittleEndianAttributeU32x3(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder = uint4(in.attrDataSem{}.xyz,0);" _CRLF, attributeInputIndex);
}

static void _readLittleEndianAttributeU32x2(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder = uint4(in.attrDataSem{}.xy,0,0);" _CRLF, attributeInputIndex);
}

static void _readLittleEndianAttributeU32x1(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder = uint4(in.attrDataSem{}.x,0,0,0);" _CRLF, attributeInputIndex);
}

static void _readLittleEndianAttributeU16x2(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder = uint4(in.attrDataSem{}.xy,0,0);" _CRLF, attributeInputIndex);
}

static void _readLittleEndianAttributeU16x4(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder = in.attrDataSem{};" _CRLF, attributeInputIndex);
}

static void _readBigEndianAttributeU32x4(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder = in.attrDataSem{};" _CRLF, attributeInputIndex);
	src->add("attrDecoder = (attrDecoder>>24)|((attrDecoder>>8)&0xFF00)|((attrDecoder<<8)&0xFF0000)|((attrDecoder<<24));" _CRLF);
}

static void _readBigEndianAttributeU32x3(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder.xyz = in.attrDataSem{}.xyz;" _CRLF, attributeInputIndex);
	src->add("attrDecoder.xyz = (attrDecoder.xyz>>24)|((attrDecoder.xyz>>8)&0xFF00)|((attrDecoder.xyz<<8)&0xFF0000)|((attrDecoder.xyz<<24));" _CRLF);
	src->add("attrDecoder.w = 0;" _CRLF);
}

static void _readBigEndianAttributeU32x2(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder.xy = in.attrDataSem{}.xy;" _CRLF, attributeInputIndex);
	src->add("attrDecoder.xy = (attrDecoder.xy>>24)|((attrDecoder.xy>>8)&0xFF00)|((attrDecoder.xy<<8)&0xFF0000)|((attrDecoder.xy<<24));" _CRLF);
	src->add("attrDecoder.z = 0;" _CRLF);
	src->add("attrDecoder.w = 0;" _CRLF);
}

static void _readBigEndianAttributeU32x1(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder.x = in.attrDataSem{}.x;" _CRLF, attributeInputIndex);
	src->add("attrDecoder.x = (attrDecoder.x>>24)|((attrDecoder.x>>8)&0xFF00)|((attrDecoder.x<<8)&0xFF0000)|((attrDecoder.x<<24));" _CRLF);
	src->add("attrDecoder.y = 0;" _CRLF);
	src->add("attrDecoder.z = 0;" _CRLF);
	src->add("attrDecoder.w = 0;" _CRLF);
}

static void _readBigEndianAttributeU16x1(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder.xy = in.attrDataSem{}.xy;" _CRLF, attributeInputIndex);
	src->add("attrDecoder.x = ((attrDecoder.x>>8)&0xFF)|((attrDecoder.x<<8)&0xFF00);" _CRLF);
	src->add("attrDecoder.y = 0;" _CRLF);
	src->add("attrDecoder.z = 0;" _CRLF);
	src->add("attrDecoder.w = 0;" _CRLF);
}

static void _readBigEndianAttributeU16x2(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder.xy = in.attrDataSem{}.xy;" _CRLF, attributeInputIndex);
	src->add("attrDecoder.xy = ((attrDecoder.xy>>8)&0xFF)|((attrDecoder.xy<<8)&0xFF00);" _CRLF);
	src->add("attrDecoder.z = 0;" _CRLF);
	src->add("attrDecoder.w = 0;" _CRLF);
}

static void _readBigEndianAttributeU16x4(LatteDecompilerShader* shaderContext, StringBuf* src, uint32 attributeInputIndex)
{
	src->addFmt("attrDecoder.xyzw = in.attrDataSem{}.xyzw;" _CRLF, attributeInputIndex);
	src->add("attrDecoder = ((attrDecoder>>8)&0xFF)|((attrDecoder<<8)&0xFF00);" _CRLF);
}

void LatteDecompiler_emitAttributeDecodeMSL(LatteDecompilerShader* shaderContext, StringBuf* src, LatteParsedFetchShaderAttribute_t* attrib)
{
	if (attrib->attributeBufferIndex >= Latte::GPU_LIMITS::NUM_VERTEX_BUFFERS)
	{
		src->add("attrDecoder = int4(0);" _CRLF);
		return;
	}

	uint32 attributeInputIndex = attrib->semanticId;
	if( attrib->endianSwap == LatteConst::VertexFetchEndianMode::SWAP_U32 )
	{
		if( attrib->format == FMT_32_32_32_32_FLOAT && attrib->nfa == 2 )
		{
			_readBigEndianAttributeU32x4(shaderContext, src, attributeInputIndex);
		}
		else if( attrib->format == FMT_32_32_32_FLOAT && attrib->nfa == 2 )
		{
			_readBigEndianAttributeU32x3(shaderContext, src, attributeInputIndex);
		}
		else if( attrib->format == FMT_32_32_FLOAT && attrib->nfa == 2 )
		{
			_readBigEndianAttributeU32x2(shaderContext, src, attributeInputIndex);
		}
		else if( attrib->format == FMT_32_FLOAT && attrib->nfa == 2 )
		{
			_readBigEndianAttributeU32x1(shaderContext, src, attributeInputIndex);
		}
		else if( attrib->format == FMT_2_10_10_10 && attrib->nfa == 0 )
		{
			_readBigEndianAttributeU32x1(shaderContext, src, attributeInputIndex);
			// Bayonetta 2 uses this format to store normals
			src->add("attrDecoder.xyzw = uint4((attrDecoder.x>>0)&0x3FF,(attrDecoder.x>>10)&0x3FF,(attrDecoder.x>>20)&0x3FF,(attrDecoder.x>>30)&0x3);" _CRLF);
			if (attrib->isSigned != 0)
			{
				src->add("if( (attrDecoder.x&0x200) != 0 ) attrDecoder.x |= 0xFFFFFC00;" _CRLF);
				src->add("if( (attrDecoder.y&0x200) != 0 ) attrDecoder.y |= 0xFFFFFC00;" _CRLF);
				src->add("if( (attrDecoder.z&0x200) != 0 ) attrDecoder.z |= 0xFFFFFC00;" _CRLF);
				src->add("attrDecoder.x = as_type<uint>(max(float(int(attrDecoder.x))/511.0,-1.0));" _CRLF);
				src->add("attrDecoder.y = as_type<uint>(max(float(int(attrDecoder.y))/511.0,-1.0));" _CRLF);
				src->add("attrDecoder.z = as_type<uint>(max(float(int(attrDecoder.z))/511.0,-1.0));" _CRLF);
			}
			else
			{
				src->add("attrDecoder.x = as_type<uint>(max(float(int(attrDecoder.x))/1023.0,-1.0));" _CRLF);
				src->add("attrDecoder.y = as_type<uint>(max(float(int(attrDecoder.y))/1023.0,-1.0));" _CRLF);
				src->add("attrDecoder.z = as_type<uint>(max(float(int(attrDecoder.z))/1023.0,-1.0));" _CRLF);
			}
			src->add("attrDecoder.w = as_type<uint>(float(attrDecoder.w));" _CRLF); // unsure?

		}
		else if( attrib->format == FMT_32_32_32_32 && attrib->nfa == 1 && attrib->isSigned == 0 )
		{
			_readBigEndianAttributeU32x4(shaderContext, src, attributeInputIndex);
		}
		else if( attrib->format == FMT_32_32_32 && attrib->nfa == 1 && attrib->isSigned == 0 )
		{
			_readBigEndianAttributeU32x3(shaderContext, src, attributeInputIndex);
		}
		else if( attrib->format == FMT_32_32 && attrib->nfa == 1 && attrib->isSigned == 0 )
		{
			_readBigEndianAttributeU32x2(shaderContext, src, attributeInputIndex);
		}
		else if (attrib->format == FMT_32 && attrib->nfa == 1 && attrib->isSigned == 0)
		{
			_readBigEndianAttributeU32x1(shaderContext, src, attributeInputIndex);
		}
		else if (attrib->format == FMT_32 && attrib->nfa == 1 && attrib->isSigned == 1)
		{
			// we can just read the signed s32 as a u32 since no sign-extension is necessary
			_readBigEndianAttributeU32x1(shaderContext, src, attributeInputIndex);
		}
		else if( attrib->format == FMT_8_8_8_8 && attrib->nfa == 0 && attrib->isSigned == 0 )
		{
			// seen in Minecraft Wii U Edition
			src->addFmt("attrDecoder.xyzw = as_type<uint4>(float4(in.attrDataSem{}.wzyx)/255.0);" _CRLF, attributeInputIndex);
		}
		else if( attrib->format == FMT_8_8_8_8 && attrib->nfa == 0 && attrib->isSigned != 0 )
		{
			// seen in Minecraft Wii U Edition
			src->addFmt("attrDecoder.xyzw = in.attrDataSem{}.wzyx;" _CRLF, attributeInputIndex);
			src->add("if( (attrDecoder.x&0x80) != 0 ) attrDecoder.x |= 0xFFFFFF00;" _CRLF);
			src->add("if( (attrDecoder.y&0x80) != 0 ) attrDecoder.y |= 0xFFFFFF00;" _CRLF);
			src->add("if( (attrDecoder.z&0x80) != 0 ) attrDecoder.z |= 0xFFFFFF00;" _CRLF);
			src->add("if( (attrDecoder.w&0x80) != 0 ) attrDecoder.w |= 0xFFFFFF00;" _CRLF);
			src->add("attrDecoder.x = as_type<uint>(max(float(int(attrDecoder.x))/127.0,-1.0));" _CRLF);
			src->add("attrDecoder.y = as_type<uint>(max(float(int(attrDecoder.y))/127.0,-1.0));" _CRLF);
			src->add("attrDecoder.z = as_type<uint>(max(float(int(attrDecoder.z))/127.0,-1.0));" _CRLF);
			src->add("attrDecoder.w = as_type<uint>(max(float(int(attrDecoder.w))/127.0,-1.0));" _CRLF);
		}
		else if( attrib->format == FMT_8_8_8_8 && attrib->nfa == 1 && attrib->isSigned == 0 )
		{
			// seen in Minecraft Wii U Edition
			src->addFmt("attrDecoder.xyzw = in.attrDataSem{}.wzyx;" _CRLF, attributeInputIndex);
		}
		else if (attrib->format == FMT_8_8_8_8 && attrib->nfa == 2 && attrib->isSigned == 0)
		{
			// seen in Ben 10 Omniverse
			src->addFmt("attrDecoder.xyzw = as_type<uint4>(float4(in.attrDataSem{}.wzyx));" _CRLF, attributeInputIndex);
		}
		else
		{
			cemuLog_log(LogType::Force, "_emitAttributeDecode(): Unsupported fmt {:02x} nfa {} signed {} endian {}\n", attrib->format, attrib->nfa, attrib->isSigned, attrib->endianSwap);
			cemu_assert_unimplemented();
		}
	}
	else if( attrib->endianSwap == LatteConst::VertexFetchEndianMode::SWAP_NONE )
	{
		if( attrib->format == FMT_32_32_32_32_FLOAT && attrib->nfa == 2 )
		{
			_readLittleEndianAttributeU32x4(shaderContext, src, attributeInputIndex);
		}
		else if (attrib->format == FMT_32_32_32_FLOAT && attrib->nfa == 2)
		{
			_readLittleEndianAttributeU32x3(shaderContext, src, attributeInputIndex);
		}
		else if (attrib->format == FMT_32_32_FLOAT && attrib->nfa == 2)
		{
			// seen in Cities of Gold
			_readLittleEndianAttributeU32x2(shaderContext, src, attributeInputIndex);
		}
		else if (attrib->format == FMT_32 && attrib->nfa == 1 && attrib->isSigned == 0)
		{
			// seen in Nano Assault Neo
			_readLittleEndianAttributeU32x1(shaderContext, src, attributeInputIndex);
		}
		else if (attrib->format == FMT_2_10_10_10 && attrib->nfa == 0 && attrib->isSigned == 0)
		{
			// seen in Fast Racing Neo
			_readLittleEndianAttributeU32x1(shaderContext, src, attributeInputIndex);
			src->add("attrDecoder.xyzw = uint4((attrDecoder.x>>0)&0x3FF,(attrDecoder.x>>10)&0x3FF,(attrDecoder.x>>20)&0x3FF,(attrDecoder.x>>30)&0x3);" _CRLF);
			src->add("attrDecoder.x = as_type<uint>(max(float(int(attrDecoder.x))/1023.0,-1.0));" _CRLF);
			src->add("attrDecoder.y = as_type<uint>(max(float(int(attrDecoder.y))/1023.0,-1.0));" _CRLF);
			src->add("attrDecoder.z = as_type<uint>(max(float(int(attrDecoder.z))/1023.0,-1.0));" _CRLF);
			src->add("attrDecoder.w = as_type<uint>(float(attrDecoder.w));" _CRLF); // todo - is this correct?
		}
		else if (attrib->format == FMT_16_16_16_16 && attrib->nfa == 0 && attrib->isSigned != 0)
		{
			// seen in CoD ghosts
			_readLittleEndianAttributeU16x4(shaderContext, src, attributeInputIndex);
			src->add("if( (attrDecoder.x&0x8000) != 0 ) attrDecoder.x |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.y&0x8000) != 0 ) attrDecoder.y |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.z&0x8000) != 0 ) attrDecoder.z |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.w&0x8000) != 0 ) attrDecoder.w |= 0xFFFF0000;" _CRLF);
			src->add("attrDecoder.x = as_type<uint>(max(float(int(attrDecoder.x))/32767.0,-1.0));" _CRLF);
			src->add("attrDecoder.y = as_type<uint>(max(float(int(attrDecoder.y))/32767.0,-1.0));" _CRLF);
			src->add("attrDecoder.z = as_type<uint>(max(float(int(attrDecoder.z))/32767.0,-1.0));" _CRLF);
			src->add("attrDecoder.w = as_type<uint>(max(float(int(attrDecoder.w))/32767.0,-1.0));" _CRLF);
		}
		else if( attrib->format == FMT_16_16_16_16 && attrib->nfa == 2 && attrib->isSigned == 1 )
		{
			// seen in Rabbids Land
			_readLittleEndianAttributeU16x4(shaderContext, src, attributeInputIndex);
			src->add("if( (attrDecoder.x&0x8000) != 0 ) attrDecoder.x |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.y&0x8000) != 0 ) attrDecoder.y |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.z&0x8000) != 0 ) attrDecoder.z |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.w&0x8000) != 0 ) attrDecoder.w |= 0xFFFF0000;" _CRLF);
			src->add("attrDecoder.xyzw = as_type<uint4>(float4(int4(attrDecoder)));" _CRLF);
		}
		else if (attrib->format == FMT_16_16_16_16_FLOAT && attrib->nfa == 2)
		{
			// seen in Giana Sisters: Twisted Dreams
			_readLittleEndianAttributeU16x4(shaderContext, src, attributeInputIndex);
			// TODO: uint4?
			src->add("attrDecoder.xyzw = as_type<uint4>(float4(float2(as_type<half2>(attrDecoder.x|(attrDecoder.y<<16))),float2(as_type<half2>(attrDecoder.z|(attrDecoder.w<<16)))));" _CRLF);
		}
		else if (attrib->format == FMT_16_16 && attrib->nfa == 0 && attrib->isSigned != 0)
		{
			// seen in Nano Assault Neo
			_readLittleEndianAttributeU16x2(shaderContext, src, attributeInputIndex);
			src->add("if( (attrDecoder.x&0x8000) != 0 ) attrDecoder.x |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.y&0x8000) != 0 ) attrDecoder.y |= 0xFFFF0000;" _CRLF);
			src->add("attrDecoder.x = as_type<uint>(max(float(int(attrDecoder.x))/32767.0,-1.0));" _CRLF);
			src->add("attrDecoder.y = as_type<uint>(max(float(int(attrDecoder.y))/32767.0,-1.0));" _CRLF);
		}
		else if (attrib->format == FMT_16_16_FLOAT && attrib->nfa == 2)
		{
			// seen in Giana Sisters: Twisted Dreams
			_readLittleEndianAttributeU16x2(shaderContext, src, attributeInputIndex);
			src->add("attrDecoder.xy = as_type<uint2>(float2(as_type<half2>(attrDecoder.x|(attrDecoder.y<<16))));" _CRLF);
			src->add("attrDecoder.zw = uint2(0);" _CRLF);
		}
		else if( attrib->format == FMT_8_8_8_8 && attrib->nfa == 0 && attrib->isSigned == 0 )
		{
			src->addFmt("attrDecoder.xyzw = as_type<uint4>(float4(in.attrDataSem{}.xyzw)/255.0);" _CRLF, attributeInputIndex);
		}
		else if( attrib->format == FMT_8_8_8_8 && attrib->nfa == 0 && attrib->isSigned != 0 )
		{
			src->addFmt("attrDecoder.xyzw = in.attrDataSem{}.xyzw;" _CRLF, attributeInputIndex);
			src->add("if( (attrDecoder.x&0x80) != 0 ) attrDecoder.x |= 0xFFFFFF00;" _CRLF);
			src->add("if( (attrDecoder.y&0x80) != 0 ) attrDecoder.y |= 0xFFFFFF00;" _CRLF);
			src->add("if( (attrDecoder.z&0x80) != 0 ) attrDecoder.z |= 0xFFFFFF00;" _CRLF);
			src->add("if( (attrDecoder.w&0x80) != 0 ) attrDecoder.w |= 0xFFFFFF00;" _CRLF);
			src->add("attrDecoder.x = as_type<uint>(max(float(int(attrDecoder.x))/127.0,-1.0));" _CRLF);
			src->add("attrDecoder.y = as_type<uint>(max(float(int(attrDecoder.y))/127.0,-1.0));" _CRLF);
			src->add("attrDecoder.z = as_type<uint>(max(float(int(attrDecoder.z))/127.0,-1.0));" _CRLF);
			src->add("attrDecoder.w = as_type<uint>(max(float(int(attrDecoder.w))/127.0,-1.0));" _CRLF);
		}
		else if (attrib->format == FMT_8_8_8_8 && attrib->nfa == 1 && attrib->isSigned == 0)
		{
			src->addFmt("attrDecoder.xyzw = in.attrDataSem{}.xyzw;" _CRLF, attributeInputIndex);
		}
		else if (attrib->format == FMT_8_8_8_8 && attrib->nfa == 1 && attrib->isSigned != 0)
		{
			// seen in Sonic Lost World
			src->addFmt("attrDecoder.xyzw = in.attrDataSem{}.xyzw;" _CRLF, attributeInputIndex);
			src->add("if( (attrDecoder.x&0x80) != 0 ) attrDecoder.x |= 0xFFFFFF00;" _CRLF);
			src->add("if( (attrDecoder.y&0x80) != 0 ) attrDecoder.y |= 0xFFFFFF00;" _CRLF);
			src->add("if( (attrDecoder.z&0x80) != 0 ) attrDecoder.z |= 0xFFFFFF00;" _CRLF);
			src->add("if( (attrDecoder.w&0x80) != 0 ) attrDecoder.w |= 0xFFFFFF00;" _CRLF);
		}
		else if( attrib->format == FMT_8_8_8_8 && attrib->nfa == 2 && attrib->isSigned == 0 )
		{
			// seen in One Piece
			// TODO: uint4?
			src->addFmt("attrDecoder.xyzw = as_type<uint4>(float4(in.attrDataSem{}.xyzw));" _CRLF, attributeInputIndex);
		}
		else if (attrib->format == FMT_8_8 && attrib->nfa == 0 && attrib->isSigned == 0)
		{
			if( (attrib->offset&3) == 2 && LatteGPUState.glVendor == GLVENDOR_AMD && g_renderer->GetType() == RendererAPI::OpenGL )
			{
				// AMD workaround
				src->addFmt("attrDecoder.xy = as_type<uint2>(float2(in.attrDataSem{}.zw)/255.0);" _CRLF, attributeInputIndex);
				src->add("attrDecoder.zw = uint2(0);" _CRLF);
			}
			else
			{
				src->addFmt("attrDecoder.xy = as_type<uint2>(float2(in.attrDataSem{}.xy)/255.0);" _CRLF, attributeInputIndex);
				src->add("attrDecoder.zw = uint2(0);" _CRLF);
			}
		}
		else if (attrib->format == FMT_8_8 && attrib->nfa == 2 && attrib->isSigned == 0)
		{
			// seen in BotW
			if ((attrib->offset & 3) == 2 && LatteGPUState.glVendor == GLVENDOR_AMD && g_renderer->GetType() == RendererAPI::OpenGL)
			{
				// AMD workaround
				src->addFmt("attrDecoder.xy = as_type<uint2>(float2(in.attrDataSem{}.zw));" _CRLF, attributeInputIndex);
				src->add("attrDecoder.zw = uint2(0);" _CRLF);
			}
			else
			{
				src->addFmt("attrDecoder.xy = as_type<uint2>(float2(in.attrDataSem{}.xy));" _CRLF, attributeInputIndex);
				src->add("attrDecoder.zw = uint2(0);" _CRLF);
			}
		}
		else if (attrib->format == FMT_8_8 && attrib->nfa == 0 && attrib->isSigned != 0)
		{
			if ((attrib->offset & 3) == 2 && LatteGPUState.glVendor == GLVENDOR_AMD && g_renderer->GetType() == RendererAPI::OpenGL)
			{
				// AMD workaround
				src->addFmt("attrDecoder.xy = in.attrDataSem{}.zw;" _CRLF, attributeInputIndex);
				src->add("if( (attrDecoder.x&0x80) != 0 ) attrDecoder.x |= 0xFFFFFF00;" _CRLF);
				src->add("if( (attrDecoder.y&0x80) != 0 ) attrDecoder.y |= 0xFFFFFF00;" _CRLF);
				src->add("attrDecoder.x = as_type<uint>(max(float(int(attrDecoder.x))/127.0,-1.0));" _CRLF);
				src->add("attrDecoder.y = as_type<uint>(max(float(int(attrDecoder.y))/127.0,-1.0));" _CRLF);
				src->add("attrDecoder.zw = uint2(0);" _CRLF);
			}
			else
			{
				src->addFmt("attrDecoder.xy = in.attrDataSem{}.xy;" _CRLF, attributeInputIndex);
				src->add("if( (attrDecoder.x&0x80) != 0 ) attrDecoder.x |= 0xFFFFFF00;" _CRLF);
				src->add("if( (attrDecoder.y&0x80) != 0 ) attrDecoder.y |= 0xFFFFFF00;" _CRLF);
				src->add("attrDecoder.x = as_type<uint>(max(float(int(attrDecoder.x))/127.0,-1.0));" _CRLF);
				src->add("attrDecoder.y = as_type<uint>(max(float(int(attrDecoder.y))/127.0,-1.0));" _CRLF);
				src->add("attrDecoder.zw = uint2(0);" _CRLF);
			}
		}
		else if (attrib->format == FMT_8_8 && attrib->nfa == 1 && attrib->isSigned == 0)
		{
			if ((attrib->offset & 3) == 2 && LatteGPUState.glVendor == GLVENDOR_AMD && g_renderer->GetType() == RendererAPI::OpenGL)
			{
				// AMD workaround
				src->addFmt("attrDecoder.xyzw = uint4(in.attrDataSem{}.zw,0,0);" _CRLF, attributeInputIndex);
			}
			else
			{
				src->addFmt("attrDecoder.xyzw = uint4(in.attrDataSem{}.xy,0,0);" _CRLF, attributeInputIndex);
			}
		}
		else if( attrib->format == FMT_8 && attrib->nfa == 0 && attrib->isSigned == 0 )
		{
			// seen in Pikmin 3
			src->addFmt("attrDecoder.x = as_type<uint>(float(in.attrDataSem{}.x)/255.0);" _CRLF, attributeInputIndex);
			src->add("attrDecoder.yzw = uint3(0);" _CRLF);
		}
		else if( attrib->format == FMT_8 && attrib->nfa == 1 && attrib->isSigned == 0 )
		{
			src->addFmt("attrDecoder.xyzw = uint4(in.attrDataSem{}.x,0,0,0);" _CRLF, attributeInputIndex);
		}
		else
		{
			cemuLog_log(LogType::Force, "_emitAttributeDecode(): Unsupported fmt {:02x} nfa {} signed {} endian {}\n", attrib->format, attrib->nfa, attrib->isSigned, attrib->endianSwap);
			cemu_assert_debug(false);
		}
	}
	else if( attrib->endianSwap == LatteConst::VertexFetchEndianMode::SWAP_U16 )
	{
		if( attrib->format == FMT_16_16_16_16_FLOAT && attrib->nfa == 2 )
		{
			_readBigEndianAttributeU16x4(shaderContext, src, attributeInputIndex);
			// TODO: uint4?
			src->add("attrDecoder.xyzw = as_type<uint4>(float4(float2(as_type<half2>(attrDecoder.x|(attrDecoder.y<<16))),float2(as_type<half2>(attrDecoder.z|(attrDecoder.w<<16)))));" _CRLF);
		}
		else if (attrib->format == FMT_16_16_16_16 && attrib->nfa == 0 && attrib->isSigned != 0)
		{
			_readBigEndianAttributeU16x4(shaderContext, src, attributeInputIndex);
			src->add("if( (attrDecoder.x&0x8000) != 0 ) attrDecoder.x |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.y&0x8000) != 0 ) attrDecoder.y |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.z&0x8000) != 0 ) attrDecoder.z |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.w&0x8000) != 0 ) attrDecoder.w |= 0xFFFF0000;" _CRLF);
			src->add("attrDecoder.x = as_type<uint>(max(float(int(attrDecoder.x))/32767.0,-1.0));" _CRLF);
			src->add("attrDecoder.y = as_type<uint>(max(float(int(attrDecoder.y))/32767.0,-1.0));" _CRLF);
			src->add("attrDecoder.z = as_type<uint>(max(float(int(attrDecoder.z))/32767.0,-1.0));" _CRLF);
			src->add("attrDecoder.w = as_type<uint>(max(float(int(attrDecoder.w))/32767.0,-1.0));" _CRLF);
		}
		else if (attrib->format == FMT_16_16_16_16 && attrib->nfa == 0 && attrib->isSigned == 0)
		{
			// seen in BotW
			_readBigEndianAttributeU16x4(shaderContext, src, attributeInputIndex);
			src->add("attrDecoder.x = as_type<uint>(float(int(attrDecoder.x))/65535.0);" _CRLF);
			src->add("attrDecoder.y = as_type<uint>(float(int(attrDecoder.y))/65535.0);" _CRLF);
			src->add("attrDecoder.z = as_type<uint>(float(int(attrDecoder.z))/65535.0);" _CRLF);
			src->add("attrDecoder.w = as_type<uint>(float(int(attrDecoder.w))/65535.0);" _CRLF);
		}
		else if( attrib->format == FMT_16_16_16_16 && attrib->nfa == 2 && attrib->isSigned != 0 )
		{
			// seen in Minecraft Wii U Edition
			_readBigEndianAttributeU16x4(shaderContext, src, attributeInputIndex);
			src->add("if( (attrDecoder.x&0x8000) != 0 ) attrDecoder.x |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.y&0x8000) != 0 ) attrDecoder.y |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.z&0x8000) != 0 ) attrDecoder.z |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.w&0x8000) != 0 ) attrDecoder.w |= 0xFFFF0000;" _CRLF);
			src->add("attrDecoder.x = as_type<uint>(float(int(attrDecoder.x)));" _CRLF);
			src->add("attrDecoder.y = as_type<uint>(float(int(attrDecoder.y)));" _CRLF);
			src->add("attrDecoder.z = as_type<uint>(float(int(attrDecoder.z)));" _CRLF);
			src->add("attrDecoder.w = as_type<uint>(float(int(attrDecoder.w)));" _CRLF);
		}
		else if( attrib->format == FMT_16_16_16_16 && attrib->nfa == 1 && attrib->isSigned != 0 )
		{
			// seen in Minecraft Wii U Edition
			_readBigEndianAttributeU16x4(shaderContext, src, attributeInputIndex);
			src->add("if( (attrDecoder.x&0x8000) != 0 ) attrDecoder.x |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.y&0x8000) != 0 ) attrDecoder.y |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.z&0x8000) != 0 ) attrDecoder.z |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.w&0x8000) != 0 ) attrDecoder.w |= 0xFFFF0000;" _CRLF);
		}
		else if( attrib->format == FMT_16_16_16_16 && attrib->nfa == 1 && attrib->isSigned == 0 )
		{
			_readBigEndianAttributeU16x4(shaderContext, src, attributeInputIndex);
		}
		else if( attrib->format == FMT_16_16_FLOAT && attrib->nfa == 2 )
		{
			_readBigEndianAttributeU16x2(shaderContext, src, attributeInputIndex);
			src->add("attrDecoder.xy = as_type<uint2>(float2(as_type<half2>(attrDecoder.x|(attrDecoder.y<<16))));" _CRLF);
			src->add("attrDecoder.zw = uint2(0);" _CRLF);
		}
		else if( attrib->format == FMT_16_16 && attrib->nfa == 0 && attrib->isSigned == 0 )
		{
			_readBigEndianAttributeU16x2(shaderContext, src, attributeInputIndex);
			src->add("attrDecoder.xy = as_type<uint2>(float2(float(attrDecoder.x), float(attrDecoder.y))/65535.0);" _CRLF);
			src->add("attrDecoder.zw = uint2(0);" _CRLF);
		}
		else if( attrib->format == FMT_16_16 && attrib->nfa == 0 && attrib->isSigned != 0 )
		{
			_readBigEndianAttributeU16x2(shaderContext, src, attributeInputIndex);
			src->add("if( (attrDecoder.x&0x8000) != 0 ) attrDecoder.x |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.y&0x8000) != 0 ) attrDecoder.y |= 0xFFFF0000;" _CRLF);
			src->add("attrDecoder.x = as_type<uint>(max(float(int(attrDecoder.x))/32767.0,-1.0));" _CRLF);
			src->add("attrDecoder.y = as_type<uint>(max(float(int(attrDecoder.y))/32767.0,-1.0));" _CRLF);
			src->add("attrDecoder.zw = uint2(0);" _CRLF);
		}
		else if( attrib->format == FMT_16_16 && attrib->nfa == 1 && attrib->isSigned == 0 )
		{
			_readBigEndianAttributeU16x2(shaderContext, src, attributeInputIndex);
		}
		else if( attrib->format == FMT_16_16 && attrib->nfa == 1 && attrib->isSigned != 0 )
		{
			_readBigEndianAttributeU16x2(shaderContext, src, attributeInputIndex);
			src->add("if( (attrDecoder.x&0x8000) != 0 ) attrDecoder.x |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.y&0x8000) != 0 ) attrDecoder.y |= 0xFFFF0000;" _CRLF);
			src->add("attrDecoder.zw = uint2(0);" _CRLF);
		}
		else if( attrib->format == FMT_16_16 && attrib->nfa == 2 && attrib->isSigned == 0 )
		{
			_readBigEndianAttributeU16x2(shaderContext, src, attributeInputIndex);
			src->add("attrDecoder.xy = as_type<uint2>(float2(float(attrDecoder.x), float(attrDecoder.y)));" _CRLF);
			src->add("attrDecoder.zw = uint2(0);" _CRLF);
		}
		else if( attrib->format == FMT_16_16 && attrib->nfa == 2 && attrib->isSigned != 0 )
		{
			_readBigEndianAttributeU16x2(shaderContext, src, attributeInputIndex);
			src->add("if( (attrDecoder.x&0x8000) != 0 ) attrDecoder.x |= 0xFFFF0000;" _CRLF);
			src->add("if( (attrDecoder.y&0x8000) != 0 ) attrDecoder.y |= 0xFFFF0000;" _CRLF);
			src->add("attrDecoder.xy = as_type<uint2>(float2(float(int(attrDecoder.x)), float(int(attrDecoder.y))));" _CRLF);
			src->add("attrDecoder.zw = uint2(0);" _CRLF);
		}
		else if (attrib->format == FMT_16 && attrib->nfa == 1 && attrib->isSigned == 0)
		{
			_readBigEndianAttributeU16x1(shaderContext, src, attributeInputIndex);
		}
		else if (attrib->format == FMT_16 && attrib->nfa == 0 && attrib->isSigned == 0)
		{
			// seen in CoD ghosts
			_readBigEndianAttributeU16x1(shaderContext, src, attributeInputIndex);
			src->add("attrDecoder.x = as_type<uint>(float(int(attrDecoder.x))/65535.0);" _CRLF);
		}
		else
		{
			cemuLog_logDebug(LogType::Force, "_emitAttributeDecode(): Unsupported fmt {:02x} nfa {} signed {} endian {}", attrib->format, attrib->nfa, attrib->isSigned, attrib->endianSwap);
		}
	}
	else
	{
		cemu_assert_debug(false);
	}
}
