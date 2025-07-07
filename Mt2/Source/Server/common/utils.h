#ifndef __M2_COMMON_UTILS__
#define __M2_COMMON_UTILS__

#include <string>
#include <algorithm>

/*----- check int function -----*/
inline bool chr_is_number(char in)
{
	return in >= '0' && in <= '9';
}

inline bool str_is_number(const char *in)
{
	if (0 == in || 0 == in[0])	return false;

	int len = strlen(in);
	for (int i = 0; i < len; ++i)
	{
		if ((in[i] < '0' || in[i] > '9') && (i > 0 || in[i] != '-'))
			return false;
	}

	return true;
}

/*----- atoi function -----*/
inline bool str_to_number (bool& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (strtol(in, NULL, 10) != 0);
	return true;
}

inline bool str_to_number (char& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (char) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned char& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned char) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (short& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (short) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned short& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned short) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (int& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (int) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned int& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned int) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (long& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (long) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned long& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned long) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (long long& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (long long) strtoull(in, NULL, 10);
	return true;
}

inline bool str_to_number(unsigned long long& out, const char *in)
{
	if (0 == in || 0 == in[0])	return false;

	out = (unsigned long long)strtoull(in, NULL, 10);
	return true;
}

inline bool str_to_number (float& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (float) strtof(in, NULL);
	return true;
}

inline bool str_to_number (double& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (double) strtod(in, NULL);
	return true;
}

#ifdef __FreeBSD__
inline bool str_to_number (long double& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (long double) strtold(in, NULL);
	return true;
}
#endif

inline std::string& str_to_lower(const char* in)
{
	static std::string s_sData;
	s_sData = in;
	std::transform(s_sData.begin(), s_sData.end(), s_sData.begin(), ::tolower);
	return s_sData;
}

/*----- atoi function -----*/

// for god's shake DON'T USE IT IF YOU DON'T KNOW WHAT YOU ARE DOING ITS FUCKING UNSAFE HACKING SHIT!!!!!!!
class ArgumentPrinter
{
private:
	char* args;
	size_t length;
	size_t current;

public:
	ArgumentPrinter(size_t defaultLen = 512) : args(new char[defaultLen]), length(defaultLen), current(0)
	{
	}

	~ArgumentPrinter()
	{
		delete[] args;
	}

	void PushArg(const void* src, size_t size)
	{
		if (size <= length - current)
		{
			memcpy(args + current, src, size);
			current += size;
		}
	}

	void PushArg(unsigned long long src)
	{
		if (sizeof(src) <= length - current)
		{
			memcpy(args + current, &src, sizeof(src));
			current += sizeof(src);
		}
	}

	// Make sure that it does not gets deleted if its on the stack ;)
	void PushArg(const char ** src)
	{
		if (sizeof(src) <= length - current)
		{
			memcpy(args + current, src, sizeof(src));
			current += sizeof(src);
		}
	}

	void PushArg(long long src) { PushArg((unsigned long long)src); }
	void PushArg(int src) { PushArg((long long)src); }
	void PushArg(unsigned int src) { PushArg((unsigned long long)src); }

#ifdef WIN32
	bool PrintWithArg(const char* format, char* output, size_t outputLen)
#else
	bool PrintWithArg(const char* format, char* output, size_t outputLen, ...)
#endif
	{
		if (!current)
			return false;
#ifdef WIN32
		vsnprintf(output, outputLen, format, (va_list)args);
#else
		/*ONLY ON UNIX/64BIT*/
		/* va_args will read from the overflow area if the gp_offset
		is greater than or equal to 48 (6 gp registers * 8 bytes/register)
		and the fp_offset is greater than or equal to 304 (gp_offset +
		16 fp registers * 16 bytes/register) */

		/*va_list argList;
		argList[0].reg_save_area = NULL;
		argList[0].overflow_arg_area = args;
		argList[0].gp_offset = 48;
		argList[0].fp_offset = 304;
		vsnprintf(output, outputLen, format, argList);*/

		va_list argList;
		va_start(argList, outputLen);
		int len = vsnprintf(output, outputLen, format, argList);
		va_end(argList);
#endif

		return true;
	}
};

#endif
