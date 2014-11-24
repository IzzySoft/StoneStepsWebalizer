/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util.cpp
*/
#include "pch.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "util.h"

static char *encode_markup(const char *str, char *buffer, u_int bsize, bool resize, bool multiline, bool xml, u_int *slen);
static char *url_decode(const char *str, char *out, u_int *slen = NULL);

//
// Derive table size from the size of the character. Note that when 
// accessing the array, char values have to be cast to u_char to make
// sure that the sign bit isn't extended. That is, if the character
// with the code \xE2 is converted to an integer without a cast to 
// an unsigned char, the resulting index will be -30:
//
//  const char *str = "";           // 0xE2 (226)
//  int n = deltas[str[0]];          // index = -30 ((int) 0xE2)
//  int m = deltas[(u_char)str[0]];  // index = 226 ((int)(u_char) 0xE2)
//
const u_int bmh_delta_table::table_size = 1 << sizeof(u_char) * 8;

void bmh_delta_table::reset(const char *str, u_int slen)
{
   u_int index;

   if(deltas)
      delete [] deltas;
   deltas = NULL;

   if(str == NULL || *str == 0)
      return;

   if(slen == 0)
      slen = strlen(str);

   deltas = new u_int[table_size];

   // use the string length to fill the array
   for(index = 0; index < table_size; index++)
      deltas[index] = slen;

   //
   // Store the offset from the end of the string for each string  
   // character, except the last one. For example, the string 
   // "abcab" would produce this delta array:
   //
   //    deltas['a'] = 1;
   //    deltas['b'] = 3;
   //    deltas['c'] = 2;
   //    deltas[any other character] = 5;
   //
   for(index = 0; index < slen-1; index++)
      deltas[(u_char)str[index]] = slen - index - 1;
}

//
//	strncpy_ex
//
//	Copies at most destsize-1 source bytes to dest and terminates the 
// resulting string with a zero. If srclen is zero, copies the source
// character-by-character.
//
//	Unlike strncpy, this function doesn't pad the destination buffer 
// with zeros.
//
size_t strncpy_ex(char *dest, size_t destsize, const char *src, size_t srclen)
{
	size_t destlen, maxlen = destsize - 1;
   const char *cp1;
   char *cp2;

	if(dest == NULL || src == NULL || destsize == 0)
		return 0;

	if(srclen) {
	   destlen = MIN(srclen, maxlen);
	   memcpy(dest, src, destlen);
   }
   else {
      cp1 = src; cp2 = dest;
      for(destlen = 0; *cp1 && destlen < maxlen; destlen++)
         *cp2++ = *cp1++;
   }

   dest[destlen] = 0;

	return destlen;
}

uint64_t usec2msec(uint64_t usec)
{
	return (usec / 1000) + ((usec % 1000 >= 500) ? 1 : 0);
}

bool is_secure_url(u_int urltype, bool use_https)
{
   return urltype == URL_TYPE_HTTPS || (use_https && (urltype & URL_TYPE_HTTPS || urltype == URL_TYPE_UNKNOWN));
}

u_int url_path_len(const char *url, u_int *urllen)
{
   u_int pathlen = 0;
   const char *cptr = url;

   while(cptr && *cptr) {
      if(*cptr++ == '?')
         break;
      pathlen++;
   }

   if(urllen)
      *urllen = pathlen + strlen(&url[pathlen]); 

   return pathlen;
}

char *html_encode(const char *str, char *buffer, u_int bsize, bool resize, bool multiline, u_int *olen)
{
   return encode_markup(str, buffer, bsize, resize, multiline, false, olen);
}

char *xml_encode(const char *str, char *buffer, u_int bsize, bool resize, bool multiline, u_int *olen)
{
   return encode_markup(str, buffer, bsize, resize, multiline, true, olen);
}

//
// encode_markup encodes markup pointed to by str. When encoding XML,
// enforce UTF-8. Otherwise, characters above 0x7F are not encoded or 
// validated in any way.
//
static char *encode_markup(const char *str, char *buffer, u_int bsize, bool resize, bool multiline, bool xml, u_int *olen)
{
   static const char hex_char[] = "0123456789ABCDEF";
   const u_char *cptr = (const u_char*) str;
   u_int slen = 0, bcnt;

   while(*cptr) {
      if(buffer == NULL || (slen + 16) > bsize) {
         if(!resize) {
            buffer = NULL;    // return NULL if we cannot resize
            break;
         }
         // allocate a bigger buffer
         bsize = (bsize) ? bsize << 1 : 128;
         buffer = (char*) realloc(buffer, bsize);
      }
      
      if(xml) {
         // if encoding XML, check for invalid UTF-8 sequences
         if((bcnt = utf8bcnt(cptr)) == 0) {
            // replace the bad byte with a private-use code point [Unicode v5 ch.3 p.91]
            slen += ucs2utf8((wchar_t) (0xE000 + ((u_char) *cptr)), buffer+slen);
            cptr++;
            continue;
         }
         
         // output valid multibyte UTF-8 characters
         if(bcnt > 1) {
            memcpy(buffer+slen, cptr, bcnt);
            slen += bcnt;
            cptr += bcnt;
            continue;
         }
      }
      
      // single-byte characters
      switch (*cptr) {
         case '<':
            memcpy(&buffer[slen], "&lt;", 4);
            slen += 4;
            break;
         case '>':
            memcpy(&buffer[slen], "&gt;", 4);
            slen += 4;
            break;
         case '&':
            memcpy(&buffer[slen], "&amp;", 5);
            slen += 5;
            break;
         case '"':
            memcpy(&buffer[slen], "&quot;", 6);
            slen += 6;
            break;
         case '\'':
            memcpy(&buffer[slen], xml ? "&apos;" : "&#x27;", 6);
            slen += 6;
            break;
         case '\r':
         case '\n':
            buffer[slen++] = multiline ? *cptr : ' ';
            break;
         default:
            // hex-encode control characters for visibility
            if(*cptr < 0x20 && *cptr != '\t' || *cptr == 0x7F) {
               buffer[slen++] = '[';
               buffer[slen++] = hex_char[(*cptr & 0xF0) >> 4];
               buffer[slen++] = hex_char[(*cptr & 0x0F)];
               buffer[slen++] = ']';
            }
            else
               buffer[slen++] = *cptr;
            break;
      }
      cptr++;
   }

   if(buffer && slen < bsize)
      buffer[slen] = 0;
   
   // update output length, if requested   
   if(olen)
      *olen = slen;

   return buffer;
}

bool is_http_redirect(u_int respcode)
{
   switch (respcode) {
      case RC_MOVEDPERM:         // 301
      case RC_MOVEDTEMP:         // 302
      case RC_SEEOTHER:          // 303
      case RC_MOVEDTEMPORARILY:  // 307
         return true;
   }

   return false;
}

bool is_http_error(u_int respcode)
{
   u_int code = respcode / 100;
   return code == 4 || code == 5;
}

//
// NIGEL HORSPOOL, Practical Fast Searching in Strings, p505
//
//   delta12[*] <- patlen; { initialize whole array }
//   for j <- 1 to patlen - 1 do
//      delta12[pat[j]]. <- patlen - j;
//
//   lastch <- pat[patlen];
//   i <- patlen;
//   while i <= stringlen do
//      begin
//         ch <- string[i];
//         if ch = lastch then
//            if string[i - patlen + 1 ... j] = pat then
//               return i - patlen + 1 ;
//         i <- i + delta12[ch];
//      end;
//    return 0;
//
// For example, the pattern "abcab" will be matched as follows:
//
//    aabcaabcab
//    abcab         move by delta['a'] = 1 
//     abcab        move by delta['a'] = 1 
//      abcab       move by delta['b'] = 3
//         abcab    match
//
const char *strstr_ex(const char *str1, const char *str2, u_int l1, u_int l2, const bmh_delta_table *delta)
{
   u_int i1;
   const char *cp1, *cp2, *cptr;
   char lastch;

   if(str1 == NULL || *str1 == 0 || str2 == NULL || *str2 == 0)
      return NULL;

   if(l1 == 0)
      l1 = strlen(str1);

   if(l2 == 0)
      l2 = strlen(str2);

   if(l2 > l1) 
      return NULL;

   // if a delta table is available, use it
   if(delta && delta->isvalid()) {
      lastch = str2[l2-1];
      i1 = l2 - 1;

      while(i1 < l1) {
         // check if the last character of the pattern matches the string character
         if(str1[i1] == lastch) {
            // if it does, compare all other characters
            if(!memcmp(&str1[i1-l2+1], str2, l2-1))
               return &str1[i1];
         }
         // move the pattern to match the character we just compared
         i1 += (*delta)[(u_char)str1[i1]];
      }
   }
   else {
      cptr = str1;

      while(*cptr && l1 >= l2) {
         // try to match the pattern left-to-right
         cp1 = cptr; cp2 = str2;
         while(*cp1 && *cp2 && *cp1 == *cp2 && cp1 != &cptr[l1]) cp1++, cp2++;

         // if at the end of the pattern, return
         if(*cp2 == 0)
            return cptr;

         // move the string pointer and reduce the remaining length
         cptr++, l1--;
      }
   }

   return NULL;
}

int strncmp_ex(const char *str1, u_int slen1, const char *str2, u_int slen2)
{
   int rc;

   if(slen1 == 0 && slen2 == 0)
      return 0;

   if(slen1 == 0)
      return -*str2;

   if(slen2 == 0)
      return *str1;

   // compare the minimum of the two lengths characters
   if((rc = strncmp(str1, str2, MIN(slen1, slen2))) != 0)
      return rc;

   // return the first character following the base of the longer string
   if(slen1 < slen2)
      return -str2[slen1];

   if(slen1 > slen2)
      return str1[slen2];

   return 0;
}

/*********************************************/
/* UNESCAPE - convert escape seqs to chars   */
/*********************************************/

string_t& url_decode(const string_t& str, string_t& out)
{
   u_int bsize, olen;
   char *optr;
   
   out.reset();
   
   if(!str.isempty()) {
      // allocate enough memory
      out.reserve(str.length());
      
      // and detach the block
      optr = out.detach(&bsize);
      
      // decode the string
      url_decode(str, optr, &olen);
      
      // and attach it back
      out.attach(optr, olen, false, bsize);
   }
   
   return out;
}

char *url_decode(const char *str, char *out, u_int *slen)
{
   const u_char *cp1 = (const u_char*) str;     /* force unsigned so we    */
   u_char *cp2 = (u_char*) out;                 /* can do > 127            */

   if (!str || !out) 
      return NULL;                              /* make sure strings valid */

   while (*cp1) {
      if (*cp1=='%')                            /* Found an escape?        */
      {
         cp1++;
         if(isxdigitex(*cp1) && isxdigitex(*(cp1+1)))/* ensure a hex digit      */
            cp1 = from_hex(cp1, cp2++);
         else 
            *cp2++='%';
      }
      else
         *cp2++ = *cp1++;                       /* if not, just continue   */
   }
   *cp2 = *cp1;                                 /* don't forget terminator */
   
   if(slen)
      *slen = cp2-(u_char*) out;
   
   return out;                                  /* return the string       */
}

/*********************************************/
/* FROM_HEX - convert hex char to decimal    */
/*********************************************/

const u_char *from_hex(const u_char *cp1, u_char *cp2)
{
   if(!cp1 || !cp2)
      return cp1;

   *cp2 = from_hex(*cp1++) << 4;          /* convert hex to an ascii */
   *cp2 |= from_hex(*cp1++);              /* (hopefully) character   */

   if(*cp2 < 32 || *cp2 == 127) 
      *cp2 = '_';                         /* make '_' if its bad   */

   return cp1;
}

u_char from_hex(u_char c)
{
   if(c >= '0' && c <= '9')
      return c - '0';

   c = toupper(c);

   if(c >= 'A' && c <= 'F')
      return c - 'A' + 10;

   return 0;                      /* return 0 if bad...      */
}

/*****************************************************************/
/*                                                               */
/* JDATE  - Julian date calculator                               */
/*                                                               */
/* Calculates the number of days since Jan 1, 0000.              */
/*                                                               */
/* Originally written by Bradford L. Barrett (03/17/1988)        */
/* Returns an unsigned long value representing the number of     */
/* days since January 1, 0000.                                   */
/*                                                               */
/* Note: Due to the changes made by Pope Gregory XIII in the     */
/*       16th Centyry (Feb 24, 1582), dates before 1583 will     */
/*       not return a truely accurate number (will be at least   */
/*       10 days off).  Somehow, I don't think this will         */
/*       present much of a problem for most situations :)        */
/*                                                               */
/* Usage: days = jdate(year, month, day)                         */
/*                                                               */
/* The number returned is adjusted by 5 to facilitate day of     */
/* week calculations.  The mod of the returned value gives the   */
/* day of the week the date is.  (ie: dow = days % 7 ) where     */
/* dow will return 0=Sunday, 1=Monday, 2=Tuesday, etc...         */
/*                                                               */
/*****************************************************************/

uint64_t jdate1(int year, int month, int day)
{
   uint64_t days;                      /* value returned */
   static const int mtable[] = {0,31,59,90,120,151,181,212,243,273,304,334};

   /* First, calculate base number including leap and Centenial year stuff */

   days = ((uint64_t)year*365) + day + mtable[month-1] + year/4 - year/100 + year/400;

   /* now adjust for leap year before March 1st */

   if(month < 3 && year % 4 == 0 && !(year % 100 && year % 400))
      --days;

   /* done, return with calculated value */

   return days;
}

//
// [length]string
//
const char *cstr2str(const char *cp, string_t& str)
{
   size_t slen;
   str.reset();

   if(cp == NULL || *cp != '[')
      return NULL;

   slen = atol(++cp);
   while(*cp++ != ']');

   if(slen)
      str.assign(cp, slen); 

   return &cp[slen];
}

//
// ul2str
//
// Converts unsigned long value to a decimal string. Returns the
// number of characters in the resulting string
//
u_int ul2str(uint64_t value, char *str)
{
   char *cp1, *cp2, *cp3, tmp;

   if(str == NULL)
      return 0;

   cp1 = cp2 = str;

   do {
      *cp2++ = '0' + (u_char) (value % 10);
      value /= 10;
   } while(value);

   *(cp3 = cp2) = 0;

   while(cp1 < --cp2) {
      tmp = *cp1;
      *cp1++ = *cp2;
      *cp2 = tmp;
   }

   return cp3-str;
}

uint64_t str2ul(const char *str, const char **eptr, u_int len)
{
   const char *cp = str;
   uint64_t value;

   if(eptr)
      *eptr = NULL;

   if(str == NULL || *str == 0 || len == 0 || !isdigitex(*cp))
      return 0;

   value = *cp++ - '0';

   while(*cp && (u_int) (cp - str) < len && isdigitex(*cp)) {
      value *= 10;
      value += *cp++ - '0';
   }

   if(eptr)
      *eptr = cp;

   return value;
}

int64_t str2l(const char *str, const char **eptr, u_int len)
{
   const char *cp = str;
   uint64_t value;
   bool neg = false;

   if(str == NULL)
      return 0;

   // check for +/-
   if(*cp == '-') {
      cp++; 
      neg = true;
   }
   else if(*cp == '+') 
      cp++;

   value = str2ul(cp, eptr, len);

   return neg ? -((int64_t)value) : value;
}

/*********************************************/
/* CUR_TIME - return date/time as a string   */
/*********************************************/

string_t cur_time(bool local_time)
{
   char    timestamp[256];
   time_t  now;

   /* get system time */
   now = time(NULL);
   
   /* convert to timestamp string (compatibility mode) */
   if (local_time)
      strftime(timestamp,sizeof(timestamp),"%d-%b-%Y %H:%M %Z", localtime(&now));
   else
      strftime(timestamp,sizeof(timestamp),"%d-%b-%Y %H:%M GMT", gmtime(&now));
      
   return string_t(timestamp);
}

void cur_time_ex(bool local_time, string_t& date, string_t& time, string_t *tzname)
{
   const struct tm *tmp;
   char timestamp[256];
   wchar_t wctstamp[256];
   tstamp_t local, utc;
   int offset, offshrs, offsmin;
   time_t  now;
   u_int slen;

   if(tzname)
      tzname->clear();

   /* get system time */
   now = ::time(NULL);
   
   // initialize the UTC time stamp first
   utc.reset(now);

   // if UTC time requested, print it out and return
   if(!local_time) {
      slen = snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d", utc.year, utc.month, utc.day);
      date.assign(timestamp, slen);
      
      slen = snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d", utc.hour, utc.min, utc.sec);
      time.assign(timestamp, slen);
      
      return;
   }
   
   *timestamp = 0;
   
   // obtain a pointer to the tm structure populated using local time
   if((tmp = localtime(&now)) != NULL) {

      // get the name of the time zone, if requested
      if(tzname) {
         // obtain the name in UCS-2 and convert it to UTF-8
         slen = wcsftime(wctstamp, sizeof(wctstamp)/sizeof(wchar_t), L"%Z", tmp);
         slen = ucs2utf8(wctstamp, slen, timestamp, sizeof(timestamp));
         tzname->assign(timestamp, slen);
      }

      local.reset(tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
      
      // subtract UTC time from local time
      offset = (int) (local.mktime() - utc.mktime());
      
      // compute hour and minute offset values
      offshrs = abs(offset) / 3600;
      offsmin = (abs(offset) / 60) % 60;
      
      // print the date
      slen = snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d", local.year, local.month, local.day);
      date.assign(timestamp, slen);

      // and the time with UTC offset (by convention, -00:00 represents unknown offset, RFC-3339)
      slen = snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d%c%02d:%02d", local.hour, local.min, local.sec, (offset>=0)?'+':'-', offshrs, offsmin);
      time.assign(timestamp, slen);
   }
}

/*********************************************/
/* CTRY_IDX - create unique # from domain    */
/*********************************************/

uint64_t ctry_idx(const char *str)
{
   uint64_t idx=0;
   const char *cp1=str;

   if(str) {
      if(*str == '*')
         return 0;
	   while(*cp1) {
		   idx = (idx <<= 5) | (*cp1 - 'a' + 1);
		   cp1++;
	   }
   }

   return idx;
}

string_t idx_ctry(uint64_t idx)
{
   char ch, buf[7], *cp;
   string_t ccode;
   
   // if index is zero, return the unknown country code
   if(idx == 0) {
      ccode = "*";
      return ccode;
   }
   
   // set the pointer to one character past the buffer
   cp = buf+sizeof(buf);
   
   // loop through 5-bit integers and fill the buffer
   while(idx) {
      ch = (char) (idx & 0x1F);
      *--cp = (ch + 'a' - 1);
      idx >>= 5;
   }
   
   // assign and return
   ccode.assign(cp, buf+sizeof(buf)-cp);
   
   return ccode;
}

/*********************************************/
/* GET_DOMAIN - Get domain portion of host   */
/*********************************************/

const char *get_domain(const char *str, u_int labelcnt)
{
   const char *cp;
   int  i=labelcnt+1;

	if(str == NULL || *str == 0)
		return NULL;

   cp = &str[strlen(str)-1];

   if (isdigitex(*cp)) return NULL;   /* ignore IP addresses */

   while(cp != str) {
      if(*cp == '.')
         if(!(--i)) return ++cp;
      cp--;
   }

   return cp;
}

char *get_url_domain(const char *url, char *buffer, u_int bsize)
{
   const char *cp1 = url, *cp2;
   u_int nlen;

   if(!cp1 || !*cp1 || *cp1 == '/')
      return NULL;

   //
   // [http[s]://]www.site.com[:port]/path
   //
   if(!strncasecmp(cp1, "http", 4)) {
      cp1 += 4;

      if(tolower(*cp1) == 's')
         cp1++;

      if(strncasecmp(cp1, "://", 3))
         return NULL;

      cp1 += 3;
   }

   if(isdigitex(*cp1)) return NULL;   // ignore IP addresses

   for(cp2 = cp1; *cp2 && *cp2 != '/' && *cp2 != ':'; cp2++);

   nlen = cp2 - cp1;

   return (strncpy_ex(buffer, bsize, cp1, nlen) == nlen) ? buffer : NULL;
}

bool is_abs_path(const char *path)
{
   if(path == NULL || *path == 0)
      return false;

   if(*path == '/')
      return true;

#ifdef _WIN32
   // check for \dir\file or \\server\share\file
   if(*path == '\\')
      return true;

   // check for c:\dir\file
   if(isalpha(*path) && path[1] == ':')
      return true;
#endif
   
   return false;
}

string_t get_cur_path(void)
{
   u_int bufsize = 128;
   char *buffer = new char[bufsize];
   string_t path;

   while(!getcwd(buffer, bufsize-1)) {
      delete [] buffer;
      bufsize <<= 1;
      if(bufsize >= BUFSIZE)
         return path;
      buffer = new char[bufsize];
   }

   path = buffer;
   delete [] buffer;

   return path;
}

string_t make_path(const char *base, const char *path)
{
   const char *cp1;
   string_t result;

   // if there's no path, return base
   if(path == NULL || *path == 0)
      return result = base;

   // if there's no base, return path
   if(base == NULL || *base == 0)
      return result = path;

   // if path is an absolute path, return it 
   if(is_abs_path(path))
      return result = path;

   // otherwise, combine base and path
   result = base;

   cp1 = &base[strlen(base) - 1];

   if(*cp1 != '/' && *cp1 != '\\')
      result += '/';

   return result + path;
}

bool is_ip4_address(const char *cp)
{
   u_int dcnt, gcnt;    // digit and group counts

   if(!cp)
      return false;

   for(gcnt = 0, dcnt = 0; *cp; cp++) {
      if(isdigitex(*cp)) {
         // if it's the first digit in the group, increment group count
         if(!dcnt) gcnt++;
         
         // can't be more than three digits
         if(++dcnt > 3) 
            return false;
      }
      else if(*cp == '.') {
         // can't start with a dot or have more than four groups
         if(dcnt == 0 || gcnt == 4) 
            return false;
         
         // reset digit count   
         dcnt = 0;
      }
      else
         return false;
   }

   return gcnt == 4 ? true : false;
}

uint64_t elapsed(uint64_t stime, uint64_t etime)
{
   return stime < etime ? etime-stime : (~stime+1)+etime;
}

//
// isinstrex checks if the string matches the pattern
//
//    str  - the string to match
//
//    cp   - the pattern; may take four forms:
//
//       pattern*  - match the beginning of the string
//       *pattern  - match the end of the string
//       pattern   - match the sub-string (substr == true)
//       pattern   - match the word (substr == false)
//
//    slen  - length of the string; if non-zero, then the string will be 
//            scanned only up to slen characters
// 
//    cplen - length of the pattern; if non-zero, it *must* indicate the 
//            exact length of the pattern
//
bool isinstrex(const char *str, const char *cp, u_int slen, u_int cplen, bool substr, const bmh_delta_table *deltas)
{
   const char *cp1,*cp2,*eos;

   if(str == NULL || *str == 0 || cp == NULL || *cp == 0)
      return 0;

   if(slen == 0)
      slen = strlen(str);
   eos = &str[slen];

   if(cplen == 0)
      cplen = strlen(cp);

   cp1 = &cp[cplen-1];
   if (*cp=='*')
   {
      if(slen < cplen-1)
         return false;

      /* if leading wildcard, start from end */
      cp2 = &str[slen-1];
      while (cp1 != cp && cp2 != str)
      {
         if (*cp1 == '*') return true;
         if (*cp1-- != *cp2--) return false;
      }
      return (cp1 == cp) ? true : false;
   }
   else
   {
      if(substr && *cp1 != '*') {
         /* if no leading/trailing wildcard, just strstr */
         return (strstr_ex(str, cp, slen, cplen, deltas) != NULL);
      }

      if(*cp1 == '*') {
         if(slen < cplen-1) 
            return false;
      }
      else if(slen < cplen) 
         return false;

      /* otherwise do normal forward scan */
      cp1 = cp; cp2 = str;
      while (cp2 != eos)
      {
         if (*cp1=='*') return true;
         if (*cp1++!=*cp2++) return false;
      }
      return (*cp1 == '*' || (*cp1 == 0 && cp2 == eos)) ? true : false;
   }
}

template <typename char_t>
const char_t *strptr(const char_t *str, const char_t *defstr)
{
   static const char_t empty[1] = {0};
   
   if(str)
      return str;
      
   if(defstr)
      return defstr;
      
   return empty;
}

u_int ucs2utf8(const wchar_t *cp, u_int slen, char *out, u_int bsize)
{
   char *op = out;
   
   while(*cp && slen-- && bsize-(op-out) >= ucs2utf8(*cp, NULL))
      op += ucs2utf8(*cp++, op);
      
   return op - out;
}

//
// Instantiate template functions
//
template const char *strptr<char>(const char *str, const char *defstr);
template const wchar_t *strptr<wchar_t>(const wchar_t *str, const wchar_t *defstr);
