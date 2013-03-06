#include "qd.h"
#include "toolbox.h"

#include <cpu/defs.h>
#include <cpu/CpuModule.h>
#include <cpu/fmem.h>

#include <stdlib.h>
#include <string>

#include "stackframe.h"

using ToolBox::Log;


namespace SANE
{

	// long double is an 80-bit extended with an extra 48-bits of 0 padding.
	typedef long double extended;

	template <class T>
	T readnum(uint32_t address);

	template<>
	int16_t readnum<int16_t>(uint32_t address)
	{
		return memoryReadWord(address);
	}

	template<>
	int32_t readnum<int32_t>(uint32_t address)
	{
		return memoryReadLong(address);
	}

	template<>
	float readnum<float>(uint32_t address)
	{
		static_assert(sizeof(float) == 4, "unexpected long double size");

		uint32_t x = memoryReadLong(address);
		return *((float *)&x);
	}

	template<>
	double readnum<double>(uint32_t address)
	{
		static_assert(sizeof(double) == 8, "unexpected long double size");

		uint64_t x = memoryReadLongLong(address);
		return *((double *)&x);
	}

	template<>
	long double readnum<long double>(uint32_t address)
	{
		char buffer[16];

		static_assert(sizeof(long double) == 16, "unexpected long double size");


		// read and swap 10 bytes
		// this is very much little endian.

		for (unsigned i = 0; i < 10; ++i)
		{
			buffer[9 - i] = memoryReadByte(address + i);
		}
		// remainder are 0-filled.
		for (unsigned i = 10; i < 16; ++i)
			buffer[i] = 0;

		// now cast...

		return *((long double *)buffer);		
	}

	template<class T>
	void writenum(T value, uint32_t address);

	template<>
	void writenum<int16_t>(int16_t value, uint32_t address)
	{
		memoryWriteWord(value, address);
	}

	template<>
	void writenum<int32_t>(int32_t value, uint32_t address)
	{
		memoryWriteLong(value, address);
	}

	template<>
	void writenum<float>(float value, uint32_t address)
	{
		static_assert(sizeof(value) == 4, "unexpected float size");

		memoryWriteLong(*((uint32_t *)&value), address);
	}	

	template<>
	void writenum<double>(double value, uint32_t address)
	{
		static_assert(sizeof(value) == 8, "unexpected double size");

		memoryWriteLongLong(*((uint64_t *)&value), address);
	}	

	template<>
	void writenum<long double>(long double value, uint32_t address)
	{
		static_assert(sizeof(value) == 16, "unexpected long double size");

		char buffer[16];

		std::memcpy(buffer, &value, sizeof(value));

		// copy 10 bytes over
		// little-endian specific.
		for(unsigned i = 0; i < 10; ++i)
			memoryWriteByte(buffer[9 - i], address + i);
	}	



	uint16_t fl2x()
	{
		// long to extended (80-bit fp)
		uint16_t op;
		uint32_t dest;
		uint32_t src;

		StackFrame<10>(src, dest, op);

		Log("     FL2X(%08x, %08x, %04x)\n", src, dest, op);

		int32_t i = readnum<int32_t>(src);

		if (ToolBox::Trace)
		{
			std::string tmp1 = std::to_string(i);
			Log("     %s\n", tmp1.c_str());
		}

		writenum<extended>((extended)i, dest);

		return 0;
	}

	uint16_t fdivx()
	{
		// div extended (80-bit fp)
		uint16_t op;
		uint32_t dest;
		uint32_t src;

		StackFrame<10>(src, dest, op);

		Log("     FDIVX(%08x, %08x, %04x)\n", src, dest, op);

		extended s = readnum<extended>(src);
		extended d = readnum<extended>(dest);

		if (ToolBox::Trace)
		{
			std::string tmp1 = std::to_string(d);
			std::string tmp2 = std::to_string(s);
			Log("     %s / %s\n", tmp1.c_str(), tmp2.c_str());
		}

		// dest = dest / src
		d = d / s;

		writenum<extended>((extended)d, dest);

		return 0;
	}

	uint16_t fmulx()
	{
		// multiply extended (80-bit fp)
		uint16_t op;
		uint32_t dest;
		uint32_t src;

		StackFrame<10>(src, dest, op);

		Log("     FMULX(%08x, %08x, %04x)\n", src, dest, op);

		extended s = readnum<extended>(src);
		extended d = readnum<extended>(dest);

		if (ToolBox::Trace)
		{
			std::string tmp1 = std::to_string(d);
			std::string tmp2 = std::to_string(s);
			Log("     %s * %s\n", tmp1.c_str(), tmp2.c_str());
		}

		d = d * s;

		writenum<extended>((extended)d, dest);

		return 0;
		return 0;
	}

	uint16_t fx2dec()
	{
		// extended (80-bit fp) to decimal
		// convert a to d based on decform f
		uint16_t op;
		uint32_t f_adr;
		uint32_t a_adr;
		uint32_t d_adr;

		StackFrame<14>(f_adr, a_adr, d_adr, op);

		Log("     FX2DEC(%08x, %08x, %08x, %04x)\n", f_adr, a_adr, d_adr, op);

		fprintf(stderr, "warning: FX2DEC not yet implemented\n");

		extended s = readnum<extended>(a_adr);

		if (ToolBox::Trace)
		{
			std::string tmp1 = std::to_string(s);
			Log("     %s\n", tmp1.c_str());
		}

		// ugh, really don't want to write this code right now.
		memoryWriteWord(0, d_adr);
		memoryWriteWord(0, d_adr + 2);
		memoryWriteWord(0, d_adr + 4);

		return 0;
	}


	template<class SRCTYPE>
	uint16_t fadd(const char *name)
	{
		// faddi, etc.
		// destination is always extended.
		uint16_t op;
		uint32_t dest;
		uint32_t src;

		StackFrame<10>(src, dest, op);

		Log("     %s(%08x, %08x, %04x)\n", name, src, dest, op);

		SRCTYPE s = readnum<SRCTYPE>(src);
		extended d = readnum<extended>(dest);

		if (ToolBox::Trace)
		{
			std::string tmp1 = std::to_string(d);
			std::string tmp2 = std::to_string(s);
			Log("     %s + %s\n", tmp1.c_str(), tmp2.c_str());
		}

		d = d + s;

		writenum<extended>((extended)d, dest);
		return 0;
	}


	uint16_t fp68k(uint16_t trap)
	{
		uint16_t op;
		uint32_t sp;

		sp = cpuGetAReg(7);
		op = memoryReadWord(sp);

		Log("%04x FP68K(%04x)\n", op);

		if (op == 0x0006) return fdivx();
		if (op == 0x000b) return fx2dec();
		if (op == 0x280e) return fl2x();

		if (op == 0x0004) return fmulx();

		switch(op)
		{

			case 0x2000: // faddi
				return fadd<int16_t>("FADDI");
				break;

		}


		fprintf(stderr, "fp68k -- op %04x is not yet supported\n", op);
		exit(1);

		return 0;
	}

	uint16_t NumToString(void)
	{
		/* 
		 * on entry:
		 * A0 Pointer to pascal string
		 * D0 The number
		 *
		 * on exit:
		 * A0 Pointer to pascal string
		 * D0 Result code
		 *
		 */

		int32_t theNum = cpuGetDReg(0);
		uint32_t theString = cpuGetAReg(0);

		//std::string s = ToolBox::ReadPString(theString, false);
		Log("     NumToString(%08x, %08x)\n", theNum, theString);

		std::string s = std::to_string(theNum);

		ToolBox::WritePString(theString, s);

		return 0;
	}

	uint32_t StringToNum(void)
	{
		/* 
		 * on entry:
		 * A0 Pointer to pascal string
		 *
		 * on exit:
		 * D0 the number
		 *
		 */

		uint32_t theString = cpuGetAReg(0);


		std::string s = ToolBox::ReadPString(theString, false);
		Log("     StringToNum(%s)\n", s.c_str());

		bool negative = false;
		uint32_t tmp = 0;

		if (!s.length()) return 0;

		auto iter = s.begin();

		// check for leading +-
		if (*iter == '-')
		{
			negative = true;
			++iter;
		}
		else if (*iter == '+')
		{
			negative = false;
			++iter;
		}

		for ( ; iter != s.end(); ++iter)
		{
			// doesn't actually check if it's a number.
			int value = *iter & 0x0f;
			tmp = tmp * 10 + value;
		}

		if (negative) tmp = -tmp;

		return tmp;
	}

	uint32_t decstr68k(uint16_t trap)
	{
		// this is a strange one since it may be sane or it may be the binary/decimal package.

		uint16_t op;

		StackFrame<2>(op);

		Log("%04x DECSTR68K(%04x)\n", trap, op);

		switch (op)
		{
		case 0x00:
			return NumToString();
			break;
			
		case 0x01:
			return StringToNum();
			break;

		default:
			fprintf(stderr, "decstr68k -- op %04x is not yet supported\n", op);
			exit(1);
		}


		return 0;
	}


}