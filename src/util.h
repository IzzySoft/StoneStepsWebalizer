/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util.h
*/

#ifndef __UTIL_H
#define __UTIL_H

#include <stddef.h>
#include <time.h>
#include <stdlib.h>

#include "types.h"
#include "tstring.h"

#ifdef _WIN32
#define F_OK 00      // existance
#define W_OK 02      // write access
#define R_OK 04      // read access
#else
#include <unistd.h>
#endif

//
//
//
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifdef _WIN32
#define PATH_MAX _MAX_PATH
#else
#ifndef PATH_MAX
#define PATH_MAX 255
#endif
#endif

//
//
//
/* Response code defines as per draft ietf HTTP/1.1 rev 6 */
#define RC_CONTINUE           100
#define RC_SWITCHPROTO        101
#define RC_OK                 200
#define RC_CREATED            201
#define RC_ACCEPTED           202
#define RC_NONAUTHINFO        203
#define RC_NOCONTENT          204
#define RC_RESETCONTENT       205
#define RC_PARTIALCONTENT     206
#define RC_MULTIPLECHOICES    300
#define RC_MOVEDPERM          301
#define RC_MOVEDTEMP          302
#define RC_SEEOTHER           303
#define RC_NOMOD              304
#define RC_USEPROXY           305
#define RC_MOVEDTEMPORARILY   307
#define RC_BAD                400
#define RC_UNAUTH             401
#define RC_PAYMENTREQ         402
#define RC_FORBIDDEN          403
#define RC_NOTFOUND           404
#define RC_METHODNOTALLOWED   405
#define RC_NOTACCEPTABLE      406
#define RC_PROXYAUTHREQ       407
#define RC_TIMEOUT            408
#define RC_CONFLICT           409
#define RC_GONE               410
#define RC_LENGTHREQ          411
#define RC_PREFAILED          412
#define RC_REQENTTOOLARGE     413
#define RC_REQURITOOLARGE     414
#define RC_UNSUPMEDIATYPE     415
#define RC_RNGNOTSATISFIABLE  416
#define RC_EXPECTATIONFAILED  417
#define RC_SERVERERR          500
#define RC_NOTIMPLEMENTED     501
#define RC_BADGATEWAY         502
#define RC_UNAVAIL            503
#define RC_GATEWAYTIMEOUT     504
#define RC_BADHTTPVER         505

/* Request types (bit field) */
#define URL_TYPE_UNKNOWN	0x00
#define URL_TYPE_HTTP		0x01
#define URL_TYPE_HTTPS		0x02
#define URL_TYPE_MIXED		0x03			/* HTTP and HTTPS (nothing else) */

//
// Boyer-Moore-Horspool delta table (see strstr_ex for details)
//
class bmh_delta_table {
   private:
      size_t                *deltas;

      static const size_t   table_size;

   public:
      bmh_delta_table(void) {deltas = NULL;}

      bmh_delta_table(const string_t& str) : deltas(NULL) {reset(str);}

      ~bmh_delta_table(void) {if(deltas) delete [] deltas;}

      void reset(const string_t& str);

      bool isvalid(void) const {return !(deltas == NULL);}

      size_t operator [] (size_t chr) const {return deltas[chr];}
};

//
// Converts the UCS-2 character to a UTF-8 sequence and returns the number of bytes 
// in the output.
//
// [Unicode v5 ch.3 p.103]
//
// Scalar Value        First Byte  Second Byte Third Byte
// 00000000 0xxxxxxx   0xxxxxxx
// 00000yyy yyxxxxxx   110yyyyy    10xxxxxx
// zzzzyyyy yyxxxxxx   1110zzzz    10yyyyyy    10xxxxxx
//
inline size_t ucs2utf8(wchar_t wchar, char *out)
{
   if(!out)
      return 0;

   // 1-byte character
	if(wchar <= 0x7F) {
		*out++ = wchar & 0x7F;
		return 1;
	}
	
	// 2-byte sequence
	if(wchar <= 0x7FF) {
		*out++ = (wchar >> 6) | 0xC0;
		*out++ = (wchar & 0x3F) | 0x80;
		return 2;
	}

   // 3-byte sequence
	*out++ = (wchar >> 12) | 0xE0;
	*out++ = ((wchar & 0xFC0) >> 6) | 0x80;
	*out = (wchar & 0x3F) | 0x80;

	return 3;
}

//
// Returns the size of a UTF-8 character corresponding to the specified UCS-2
// character.
//
inline size_t ucs2utf8size(wchar_t wchar)
{
	return (wchar <= 0x7F) ? 1 : (wchar <= 0x7FF) ? 2 : 3;
}

//
// Converts char to unsigned char and checks if the argument is within the range
// defined by the template arguments.
//
template <unsigned char lo, unsigned char hi>
inline bool in_range(unsigned char ch) 
{
   return ch >= lo && ch <= hi;
}

//
// Returns the number of bytes in a UTF-8 character or zero if the sequence has any
// bytes outside of the ranges defined for UTF-8 characters. 
//
// IMPORTANT: this function does not check whether the UTF-8 character is valid and
// will just count bytes based on the ranges described below. For example, the 0x01
// is not a valid character.
//
// [Unicode v5 ch.3 p.104]
//
// Code Points          First Byte  Second Byte Third Byte  Fourth Byte
// U+0000   .. U+007F   00..7F
// U+0080   .. U+07FF   C2..DF      80..BF
// U+0800   .. U+0FFF   E0          A0..BF      80..BF
// U+1000   .. U+CFFF   E1..EC      80..BF      80..BF
// U+D000   .. U+D7FF   ED          80..9F      80..BF
// U+E000   .. U+FFFF   EE..EF      80..BF      80..BF
// U+10000  .. U+3FFFF  F0          90..BF      80..BF      80..BF
// U+40000  .. U+FFFFF  F1..F3      80..BF      80..BF      80..BF
// U+100000 .. U+10FFFF F4          80..8F      80..BF      80..BF
//
inline size_t utf8size(const char *cp)
{
   if(!cp)
      return 0;

   // 1-byte character (including zero terminator)
   if(in_range<'\x00', '\x7F'>(*cp))
      return 1;
   
   // 2-byte sequences
   if(in_range<'\xC2', '\xDF'>(*cp))
      return in_range<'\x80', '\xBF'>(*(cp+1)) ? 2 : 0;
   
   // 3-byte sequences
   if(*cp == '\xE0')
      return in_range<'\xA0', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) ? 3 : 0;
      
   if(in_range<'\xE1', '\xEC'>(*cp))
      return in_range<'\x80', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) ? 3 : 0;
      
   if(*cp == '\xED')
      return in_range<'\x80', '\x9F'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) ? 3 : 0;
      
   if(in_range<'\xEE', '\xEF'>(*cp))
      return in_range<'\x80', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) ? 3 : 0;

   // 4-byte sequences
   if(*cp == '\xF0')
      return in_range<'\x90', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) &&  in_range<'\x80', '\xBF'>(*(cp+3)) ? 4 : 0;

   if(in_range<'\xF1', '\xF3'>(*cp))
      return in_range<'\x80', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) &&  in_range<'\x80', '\xBF'>(*(cp+3)) ? 4 : 0;
      
   if(*cp == '\xF4')
      return in_range<'\x80', '\x8F'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) &&  in_range<'\x80', '\xBF'>(*(cp+3)) ? 4 : 0;
   
   return 0;
}

//
// Converts a UCS-2 string to a UTF-8 string. Returns the size of the result, in bytes,
// not including the null character, if one was inserted. If slen was provided, all slen 
// characters will be converted, whether they contain a null character or not. 
//
size_t ucs2utf8(const wchar_t *str, size_t slen, char *out, size_t bsize);
size_t ucs2utf8(const wchar_t *str, char *out, size_t bsize);

//
// Checks if str is a valid UTF-8 string
//
bool isutf8str(const char *str);
bool isutf8str(const char *str, size_t slen);

//
// Checks if str is a valid UTF-8 string and if not, assumes str to be a Windows 
// 1252 string and converts it to a UTF-8 string. Returns out if conversion was
// successful or NULL otherwise.
//
char *cp1252utf8(const char *str, char *out, size_t bsize, size_t *olen = NULL);
char *cp1252utf8(const char *str, size_t slen, char *out, size_t bsize, size_t *olen = NULL);

//
// Checks if str is a valid UTF-8 string and if not, converts it to UTF-8. Returns 
// a reference to str and clears out in the former case and a reference to out 
// otherwise
// 
// Current implementation assumes Windows 1252 character set for all non-UTF-8 
// strings. Additinoal source character set identifier may be added in the future 
// as a parameter. 
//
const string_t& toutf8(const string_t& str, string_t& out);

//
//
//
inline double AVG(double curavg, double value, uint64_t newcnt) {return curavg+(value-curavg)/(double)newcnt;}
inline double AVG(double curavg, uint64_t value, uint64_t newcnt) {return AVG(curavg, (double) value, newcnt);}

inline double AVG2(double a1, uint64_t n1, double a2, uint64_t n2) {return a1+(a2-a1)*((double)n2/((double)n1+(double)n2));}

inline double PCENT(double val, double max) {return max ? (val/max)*100.0 : 0.0;}
inline double PCENT(uint64_t val, uint64_t max) {return PCENT((double) val, (double) max);}

uint64_t usec2msec(uint64_t usec);

bool is_secure_url(size_t urltype, bool use_https);
size_t url_path_len(const char *url, size_t *urllen);
bool is_http_redirect(size_t respcode);
bool is_http_error(size_t respcode);

const char *strstr_ex(const char *str1, const char *str2, size_t l1, size_t l2 = 0, const bmh_delta_table *delta = NULL);

size_t strncpy_ex(char *dest, size_t destsize, const char *src, size_t srclen);

int strncmp_ex(const char *str1, size_t slen1, const char *str2, size_t slen2);

string_t& url_decode(const string_t& str, string_t& out);

char *html_encode(const char *str, char *buffer, size_t bsize, bool multiline = false, size_t *olen = NULL);
char *xml_encode(const char *str, char *buffer, size_t bsize, bool multiline = false, size_t *olen = NULL);

char from_hex(char);                           /* convert hex to dec  */
const char *from_hex(const char *cp1, char *cp2);

const char *cstr2str(const char *cp, string_t& str);

size_t ul2str(uint64_t value, char *str);
uint64_t str2ul(const char *str, const char **eptr = NULL, size_t len = ~0);
int64_t str2l(const char *str, const char **eptr = NULL, size_t len = ~0);

string_t cur_time(bool local_time);
void cur_time_ex(bool local_time, string_t& date, string_t& time, string_t *tzname);

uint64_t ctry_idx(const char *);
string_t idx_ctry(uint64_t idx);

const char *get_domain(const char *, size_t);       /* return domain name  */
char *get_url_domain(const char *url, char *buffer, size_t bsize);

bool is_abs_path(const char *path);
string_t get_cur_path(void);
string_t make_path(const char *base, const char *path);

bool is_ip4_address(const char *str);

uint64_t elapsed(uint64_t stime, uint64_t etime);

bool isinstrex(const char *str, const char *cp, size_t slen, size_t cplen, bool substr, const bmh_delta_table *deltas);

template <typename char_t>
const char_t *strptr(const char_t *str, const char_t *defstr = NULL);

// avoid any locale-specific checks and conversions
inline bool isalphaex(int ch) {return ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z';}
inline bool isdigitex(int ch) {return ch >= '0' && ch <= '9';}
inline bool isxdigitex(int ch) {return ch >= '0' && ch <= '9' || ch >= 'A' && ch <= 'F' || ch >= 'a' && ch <= 'f';}
inline bool isspaceex(int ch) {return ch == ' ' || ch == '\t';}
inline bool iswspaceex(int ch) {return isspaceex(ch) || ch == '\r' || ch == '\n';}

#endif // __UTIL_H
