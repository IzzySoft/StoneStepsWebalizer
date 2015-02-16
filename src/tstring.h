/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   tstring.h
*/

#ifndef __TSTRING_H
#define __TSTRING_H

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define vsnwprintf _vsnwprintf
#define wcsncasecmp _wcsnicmp
#define wcscasecmp _wcsicmp
#else
#define vsnwprintf vswprintf
#endif

//
// string_base
//
template <typename char_t>
class string_base {
   private:
      char_t   *string;             // string
      size_t   slen     : 31;       // length
      size_t            :  1;       // padding
      size_t   bufsize  : 31;       // buffer size, in characters
      size_t   holder   :  1;

      static char_t empty_string[];

   private:
      void init(void);

      void make_bad_string(void);

      bool realloc_buffer(size_t len);

      string_base& use_string(char_t *str, size_t len, bool cstr, size_t bsize, bool hold);

      static size_t c2b(size_t len) {return len << (sizeof(char_t)-1);}

   public:
      static const size_t npos;

   public:
      string_base(void) {init();};
      string_base(const string_base& str) {init(); assign(str);}
      explicit string_base(const char_t *str) {init(); assign(str, 0, true);}
      explicit string_base(const char_t *str, size_t len, bool cstr = false) {init(); assign(str, len, cstr);}

      ~string_base(void);

      size_t length(void) const {return slen;}

      size_t capacity(void) const {return (bufsize) ? bufsize - 1 : 0;}
      
      const char_t *c_str(void) const {return string;}

      operator const char_t*(void) const {return string;}

      string_base& clear(void);
      string_base& reset(void);
      
      void reserve(size_t len);

      bool isempty(void) const {return slen == 0;}

      string_base& operator = (const string_base& str) {return assign(str.string, str.slen);}
      string_base& operator = (const char_t *str) {return assign(str);}
      string_base& operator = (char_t chr) {return assign(&chr, 1);}

      string_base& assign(const string_base& str) {return assign(str.string, str.slen);}
      string_base& assign(const char_t *str) {reset(); return append(str, 0, true);}
      string_base& assign(const char_t *str, size_t len, bool cstr = false) {reset(); return append(str, len, cstr);}

      string_base& append(const string_base& str) {return append(str.string, str.slen);}
      string_base& append(const char_t *str) {return append(str, 0, true);}
      string_base& append(char_t chr) {return append(&chr, 1);}
      string_base& append(const char_t *str, size_t len, bool cstr = false);

      int compare(const char_t *str, size_t count) const;
      int compare(const char_t *str) const;

      int compare_ci(const char_t *str, size_t count) const;
      int compare_ci(const char_t *str) const;

      string_base& operator += (const string_base& str) {return append(str);}
      string_base& operator += (const char_t *str) {return append(str);}
      string_base& operator += (char_t chr) {return append(&chr, 1);}

      string_base operator + (const string_base& str) const {return string_base(*this).append(str);}
      string_base operator + (const char_t *str) const {return string_base(*this).append(str);}
      string_base operator + (char_t chr) const {return string_base(*this).append(&chr, 1);}

      bool operator == (const string_base& str) const {return (slen != str.slen) ? false : compare(str) == 0 ? true : false;}
      bool operator != (const string_base& str) const {return (slen != str.slen) ? true : compare(str) != 0 ? true : false;}

      bool operator == (const char_t *str) const {return compare(str) == 0 ? true : false;}
      bool operator != (const char_t *str) const {return compare(str) != 0 ? true : false;}
      bool operator >  (const char_t *str) const {return compare(str) >  0 ? true : false;}
      bool operator <  (const char_t *str) const {return compare(str) <  0 ? true : false;}
      bool operator >= (const char_t *str) const {return compare(str) <= 0 ? true : false;}
      bool operator <= (const char_t *str) const {return compare(str) <= 0 ? true : false;}

      string_base& tolower(size_t start = 0, size_t end = 0);
      string_base& toupper(size_t start = 0, size_t end = 0);

      string_base& replace(char_t from, char_t to);

      string_base& truncate(size_t at);

      size_t find(char_t chr, size_t start = 0);
      size_t r_find(char_t chr) const;

      string_base& format(const char_t *fmt, ...);
      string_base& format_va(const char_t *fmt, va_list valist);

      static string_base _format(const char_t *fmt, ...);
      static string_base _format_va(const char_t *fmt, va_list valist);

      char_t *detach(size_t *bsize = NULL);

      string_base& attach(char_t *str) {return attach(str, 0, true, 0);}
      string_base& attach(char_t *str, size_t len, bool cstr = false, size_t bsize = 0) {return use_string(str, len, cstr, bsize, false);}

      string_base& hold(char_t *str) {return hold(str, 0, true, 0);}
      string_base& hold(char_t *str, size_t len, bool cstr = false, size_t bsize = 0) {return use_string(str, len, cstr, bsize, true);}
};

//
//
//
typedef string_base<char> string_t;


#endif // __TSTRING_H
