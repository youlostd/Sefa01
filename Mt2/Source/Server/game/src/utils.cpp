#include "stdafx.h"
#include "utils.h"
#include "mob_manager.h"
#include "config.h"

static int global_time_gap = 0;

time_t get_global_time()
{
	return time(0) + global_time_gap;
}

void set_global_time(time_t t)
{
	global_time_gap = t - time(0);

	char time_str_buf[32];
	snprintf(time_str_buf, sizeof(time_str_buf), "%s", time_str(get_global_time()));

	sys_log(0, "GLOBAL_TIME: %s time_gap %d", time_str_buf, global_time_gap);
}

int dice(int number, int size)
{
	int sum = 0, val;

	if (size <= 0 || number <= 0)
		return (0);

	while (number)
	{
		val = ((thecore_random() % size) + 1);
		sum += val;
		--number;
	}

	return (sum);
}

size_t str_lower(const char * src, char * dest, size_t dest_size)
{
	size_t len = 0;

	if (!dest || dest_size == 0)
		return len;

	if (!src)
	{
		*dest = '\0';
		return len;
	}

	// \0 자리 확보
	--dest_size;

	while (*src && len < dest_size)
	{
		*dest = LOWER(*src); // LOWER 매크로에서 ++나 --하면 안됨!!

		++src;
		++dest;
		++len;
	}

	*dest = '\0';
	return len;
}

void skip_spaces(const char **string)
{   
	for (; **string != '\0' && isnhspace(**string); ++(*string));
}

const char *one_argument(const char *argument, char *first_arg, size_t first_size)
{
	char mark = FALSE;
	size_t first_len = 0;

	if (!argument || 0 == first_size)
	{
		sys_err("one_argument received a NULL pointer!");			   
		*first_arg = '\0';
		return NULL;	
	} 

	// \0 자리 확보
	--first_size;

	skip_spaces(&argument);

	while (*argument && first_len < first_size)
	{ 
		if (*argument == '\"')
		{
			mark = !mark;
			++argument; 
			continue;   
		}

		if (!mark && isnhspace(*argument))	  
			break;

		*(first_arg++) = *argument;
		++argument;	 
		++first_len;
	} 

	*first_arg = '\0';

	skip_spaces(&argument);
	return (argument);
}

const char *two_arguments(const char *argument, char *first_arg, size_t first_size, char *second_arg, size_t second_size)
{
	return (one_argument(one_argument(argument, first_arg, first_size), second_arg, second_size));
}

const char *first_cmd(const char *argument, char *first_arg, size_t first_arg_size, size_t *first_arg_len_result)
{		   
	size_t cur_len = 0;
	skip_spaces(&argument);

	// \0 자리 확보
	first_arg_size -= 1;

	while (*argument && !isnhspace(*argument) && cur_len < first_arg_size)
	{
		*(first_arg++) = LOWER(*argument);
		++argument;
		++cur_len;
	}

	*first_arg_len_result = cur_len;
	*first_arg = '\0';
	return (argument);
}

int CalculateDuration(int iSpd, int iDur)
{
	int i = 100 - iSpd;

	if (i > 0) 
		i = 100 + i;
	else if (i < 0) 
		i = 10000 / (100 - i);
	else
		i = 100;

	return iDur * i / 100;
}
double uniform_random(double a, double b)
{
	return thecore_random() / (RAND_MAX + 1.f) * (b - a) + a;
}

float gauss_random(float avg, float sigma)
{
	static bool haveNextGaussian = false;
	static float nextGaussian = 0.0f;

	if (haveNextGaussian) 
	{
		haveNextGaussian = false;
		return nextGaussian * sigma + avg;
	} 
	else 
	{
		double v1, v2, s;
		do { 
			//v1 = 2 * nextDouble() - 1;   // between -1.0 and 1.0
			//v2 = 2 * nextDouble() - 1;   // between -1.0 and 1.0
			v1 = uniform_random(-1.f, 1.f);
			v2 = uniform_random(-1.f, 1.f);
			s = v1 * v1 + v2 * v2;
		} while (s >= 1.f || fabs(s) < FLT_EPSILON);
		double multiplier = sqrtf(-2 * logf(s)/s);
		nextGaussian = v2 * multiplier;
		haveNextGaussian = true;
		return v1 * multiplier * sigma + avg;
	}
}

int parse_time_str(const char* str)
{
	int tmp = 0;
	int secs = 0;

	while (*str != 0)
	{
		switch (*str)
		{
			case 'm':
			case 'M':
				secs += tmp * 60;
				tmp = 0;
				break;

			case 'h':
			case 'H':
				secs += tmp * 3600;
				tmp = 0;
				break;

			case 'd':
			case 'D':
				secs += tmp * 86400;
				tmp = 0;
				break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				tmp *= 10;
				tmp += (*str) - '0';
				break;

			case 's':
			case 'S':
				secs += tmp;
				tmp = 0;
				break;
			default:
				return -1;
		}
		++str;
	}

	return secs + tmp;
}

bool WildCaseCmp(const char *w, const char *s)
{
	for (;;)
	{
		switch(*w)
		{
			case '*':
				if ('\0' == w[1])
					return true;
				{
					for (size_t i = 0; i <= strlen(s); ++i)
					{
						if (true == WildCaseCmp(w + 1, s + i))
							return true;
					}
				}
				return false;

			case '?':
				if ('\0' == *s)
					return false;

				++w;
				++s;
				break;

			default:
				if (*w != *s)
				{
					if (tolower(*w) != tolower(*s))
						return false;
				}

				if ('\0' == *w)
					return true;

				++w;
				++s;
				break;
		}
	}

	return false;
}

int is_twobyte(const char * str)
{
	return ishan(*str);
}

int check_name(const char * str, bool allow_special_chars)
{
	const char* tmp;

	if (!str || !*str)
		return 0;

	if (strlen(str) < 2)
		return 0;

#ifdef SPECIAL_NAME_LETTERS
	const char* special_char_player_name = " .,_-*+~#?=&%$§!°^;:ß@";
#endif
	const char* special_char_list = " .,_-*+~#?\\=}()[]{/&%$§!°^<>|;:ß@€";
	
	for (tmp = str; *tmp; ++tmp)
	{
		if (isdigit(*tmp) || isalpha(*tmp))
			continue;
		else
		{
			if (allow_special_chars)
			{
				if (strchr(special_char_list, *tmp))
					continue;
			}
#ifdef SPECIAL_NAME_LETTERS
			else
			{
				if (strchr(special_char_player_name, *tmp))
					continue;
			}
#endif
			return 0;
		}
	}

	char szTmp[256];
	str_lower(str, szTmp, sizeof(szTmp));

	if (CMobManager::instance().Get(szTmp, false))
		return 0;

	return 1;
}

char* strchr_new(char* string, const char* chrs, bool ignore_escaped, bool only_escaped)
{
	const char* chrptr;

	while (*string)
	{
		chrptr = chrs;
		if (ignore_escaped && *chrptr == '\\')
		{
			++string;
			if (*string)
				++string;
			continue;
		}

		if (only_escaped)
		{
			if (*(string++) != '\\')
				continue;
		}

		while (*chrptr)
		{
			if (*chrptr == *string)
				return string;
			++chrptr;
		}

		++string;
	}

	return NULL;
}

const char* strchr_new(const char* string, const char* chrs, bool ignore_escaped, bool only_escaped)
{
	const char* chrptr;

	while (*string)
	{
		chrptr = chrs;
		if (ignore_escaped && *string == '\\')
		{
			++string;
			if (*string)
				++string;
			continue;
		}

		if (only_escaped)
		{
			if (*(string++) != '\\')
				continue;
		}

		while (*chrptr)
		{
			if (*chrptr == *string)
				return string;
			++chrptr;
		}

		++string;
	}

	return NULL;
}

char* strchr_new(char* string, char chr, bool ignore_escaped, bool only_escaped)
{
	static char chrs[] = { '\0', '\0' };
	*chrs = chr;
	return strchr_new(string, chrs, ignore_escaped, only_escaped);
}

const char* strchr_new(const char* string, char chr, bool ignore_escaped, bool only_escaped)
{
	static char chrs[] = { '\0', '\0' };
	*chrs = chr;
	return strchr_new(string, chrs, ignore_escaped, only_escaped);
}

const char* strescape(const char* string, const char* chrs)
{
	static char s_szEscapeString[1024 + 1];
	strcpy(s_szEscapeString, string);

	int iCount = 0;

	const char* szPtrFind = string;
	while (szPtrFind = strchr_new(szPtrFind, chrs)) // find next char to escape
	{
		int iNewPos = szPtrFind - string + iCount; // get pos in escape string

		// escape char
		strcpy(&s_szEscapeString[iNewPos + 1], szPtrFind);
		s_szEscapeString[iNewPos] = '\\';

		++szPtrFind;
		++iCount;
	}

	return s_szEscapeString;
}

const char* strunescape(const char* string, const char* chrs)
{
	static char s_szUnescapeString[1024 + 1];
	strcpy(s_szUnescapeString, string);

	int iCount = 0;

	const char* szPtrFind = string;
	while (szPtrFind = strchr_new(szPtrFind, chrs, false, true))
	{
		int iNewPos = szPtrFind - string - iCount; // get pos in unescape string

		// unescape char
		strcpy(&s_szUnescapeString[iNewPos - 1], szPtrFind);

		++szPtrFind;
		++iCount;
	}

	return s_szUnescapeString;
}

const char* strreplace(const char* string, const char* strs_old, const char* strs_new)
{
	if (!*strs_old)
		return string;

	std::vector<std::string> vec_stOld;
	std::vector<std::string> vec_stNew;

	// load replace vectors
	{
		const char* szPtrOld = strs_old;
		const char* szPtrOldOld = strs_old;
		const char* szPtrOldEnd = strs_old + strlen(strs_old);
		const char* szPtrNew = strs_new;
		const char* szPtrNewOld = szPtrNew;
		const char* szPtrNewEnd = strs_new + strlen(strs_new);

		do
		{
			// min len of replace str is 1
			++szPtrOld;

			// find next string replace sequence
			szPtrOld = strchr(szPtrOld, '|');
			if (!szPtrOld)
				szPtrOld = szPtrOldEnd;
			szPtrNew = strchr(szPtrNew, '|');
			if (!szPtrNew)
				szPtrNew = szPtrNewEnd;

			// copy sequences into a string
			std::string stOld, stNew;
			stOld.assign(szPtrOldOld, szPtrOld - szPtrOldOld);
			stNew.assign(szPtrNewOld, szPtrNew - szPtrNewOld);

			// save old position
			szPtrOldOld = ++szPtrOld;
			szPtrNewOld = szPtrNew < szPtrNewEnd ? ++szPtrNew : szPtrNew;

			// save sequences in vector
			vec_stOld.push_back(stOld);
			vec_stNew.push_back(stNew);
		} while (szPtrOld < szPtrOldEnd);
	}

	return strreplace(string, vec_stOld, vec_stNew);
}

const char* strreplace(const char* string, const std::vector<std::string>& strs_old, const std::vector<std::string>& strs_new)
{
	int iReplaceCount = strs_old.size();

	// replace strings
	static char szResultString[1024 + 1];
	strcpy(szResultString, string);
	int iResultLenDif = 0;

	{
		const char* curptr = string;
		while (*curptr)
		{
			for (int i = 0; i < iReplaceCount; ++i)
			{
				if (!strncmp(curptr, strs_old[i].c_str(), strs_old[i].length()))
				{
					strcpy(szResultString + (curptr - string) + iResultLenDif, strs_new[i].c_str());
					strcpy(szResultString + (curptr - string) + iResultLenDif + strs_new[i].length(), curptr + strs_old[i].length());

					iResultLenDif += (int)strs_new[i].length() - (int)strs_old[i].length();
					curptr += strs_old[i].length() - 1;

					break;
				}
			}

			++curptr;
		}
	}

	return szResultString;
}

const char* strreplace_large(const char* string, const std::vector<std::string>& strs_old, const std::vector<std::string>& strs_new)
{
	int iReplaceCount = strs_old.size();

	// replace strings
	int iStringLen = strlen(string);
	char* szResultString = new char[iStringLen * 2 + 1];
	strcpy(szResultString, string);
	int iResultLenDif = 0;

	{
		const char* curptr = string;
		while (*curptr)
		{
			for (int i = 0; i < iReplaceCount; ++i)
			{
				if (!strncmp(curptr, strs_old[i].c_str(), strs_old[i].length()))
				{
					strcpy(szResultString + (curptr - string) + iResultLenDif, strs_new[i].c_str());
					strcpy(szResultString + (curptr - string) + iResultLenDif + strs_new[i].length(), curptr + strs_old[i].length());

					iResultLenDif += (int)strs_new[i].length() - (int)strs_old[i].length();
					curptr += strs_old[i].length() - 1;

					break;
				}
			}

			++curptr;
		}
	}

	return szResultString;
}

const char* strlower(const char* string)
{
	static char s_szLowerString[1024 + 1];
	str_lower(string, s_szLowerString, sizeof(s_szLowerString));
	return s_szLowerString;
}

const char* strformat(const char* string, ...)
{
	static char s_szBuf[1024 + 1];
	va_list args;

	va_start(args, string);
	int len = vsnprintf(s_szBuf, sizeof(s_szBuf), string, args);
	va_end(args);

	return s_szBuf;
}

std::string strReplaceAll(std::string const& original, std::string const& from, std::string const& to)
{
	std::string results;
	std::string::const_iterator end = original.end();
	std::string::const_iterator current = original.begin();
	std::string::const_iterator next = std::search(current, end, from.begin(), from.end());
	while (next != end) {
		results.append(current, next);
		results.append(to);
		current = next + from.size();
		next = std::search(current, end, from.begin(), from.end());
	}
	results.append(current, next);
	return results;
}
