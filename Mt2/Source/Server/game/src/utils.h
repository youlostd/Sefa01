
#ifndef __INC_METIN_II_UTILS_H__
#define __INC_METIN_II_UTILS_H__

#include "../../common/tables.h"
#include <math.h>
#include <sstream>

#define IS_SET(flag, bit)		((flag) & (bit))
#define SET_BIT(var, bit)		((var) |= (bit))
#define REMOVE_BIT(var, bit)	((var) &= ~(bit))
#define TOGGLE_BIT(var, bit)	((var) = (var) ^ (bit))

inline float DISTANCE_SQRT(long dx, long dy)
{
	return ::sqrt((float)dx * dx + (float)dy * dy);
}

inline int DISTANCE_APPROX(int dx, int dy)
{
	int min, max;

	if (dx < 0)
		dx = -dx;

	if (dy < 0)
		dy = -dy;

	if (dx < dy)
	{
		min = dx;
		max = dy;
	}
	else
	{
		min = dy;
		max = dx;
	}

	// coefficients equivalent to ( 123/128 * max ) and ( 51/128 * min )
	return ((( max << 8 ) + ( max << 3 ) - ( max << 4 ) - ( max << 1 ) +
		( min << 7 ) - ( min << 5 ) + ( min << 3 ) - ( min << 1 )) >> 8 );
}

#ifndef __WIN32__
inline WORD MAKEWORD(BYTE a, BYTE b)
{
	return static_cast<WORD>(a) | (static_cast<WORD>(b) << 8);
}
#endif

extern void set_global_time(time_t t);
extern time_t get_global_time();

extern int	dice(int number, int size);
extern size_t str_lower(const char * src, char * dest, size_t dest_size);

extern void	skip_spaces(char **string);

extern const char *	one_argument(const char *argument, char *first_arg, size_t first_size);
extern const char *	two_arguments(const char *argument, char *first_arg, size_t first_size, char *second_arg, size_t second_size);
extern const char *	first_cmd(const char *argument, char *first_arg, size_t first_arg_size, size_t *first_arg_len_result);

extern int CalculateDuration(int iSpd, int iDur);

extern float gauss_random(float avg = 0, float sigma = 1);

extern int parse_time_str(const char* str);

extern bool WildCaseCmp(const char *w, const char *s);

extern int is_twobyte(const char * str);
extern int check_name(const char * str, bool allow_special_chars = false);

extern char* strchr_new(char* string, const char* chrs, bool ignore_escaped = false, bool only_escaped = false);
extern const char* strchr_new(const char* string, const char* chrs, bool ignore_escaped = false, bool only_escaped = false);
extern char* strchr_new(char* string, char chr, bool ignore_escaped = false, bool only_escaped = false);
extern const char* strchr_new(const char* string, char chr, bool ignore_escaped = false, bool only_escaped = false);
extern const char* strescape(const char* string, const char* chrs);
extern const char* strunescape(const char* string, const char* chrs);
extern const char* strreplace(const char* string, const char* strs_old, const char* strs_new);
extern const char* strreplace(const char* string, const std::vector<std::string>& strs_old, const std::vector<std::string>& strs_new);
extern const char* strreplace_large(const char* string, const std::vector<std::string>& strs_old, const std::vector<std::string>& strs_new);
extern const char* strlower(const char* string);
extern const char* strformat(const char* string, ...);

extern std::string strReplaceAll(std::string const& original, std::string const& from, std::string const& to);

inline void split_string(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
}

#endif /* __INC_METIN_II_UTILS_H__ */

