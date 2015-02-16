/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    snode.h
*/
#ifndef __SNODE_H
#define __SNODE_H

#include "hashtab.h"
#include "basenode.h"
#include "types.h"

//
// Search strings
//
// 1. Search strings are length-encoded and include the type of the search and 
// search terms. For example, a search for phrase "webalizer css" and any type
// of file (e.g. HTML, PDF, etc) would look like this:
//
//    [6]Phrase[13]webalizer css[9]File Type[3]any
//
// Type length is always present and may be zero to indicate that the type of
// the search term is either missing or unknown.
//
struct snode_t : public base_node<snode_t> {     
      u_short        termcnt;          // search term count
      uint64_t       count;            // request count
      uint64_t       visits;           // visits started

      public:
         typedef void (*s_unpack_cb_t)(snode_t& snode, void *arg);

      public:
         snode_t(void) : base_node<snode_t>() {count = 0; termcnt = 0; visits = 0;}
         snode_t(const snode_t& snode);
         snode_t(const string_t& srch) : base_node<snode_t>(srch) {count = 0; termcnt = 0; visits = 0;}

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

         static size_t s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_hits(const void *buf1, const void *buf2);
};

//
// Search Strings
//
class s_hash_table : public hash_table<snode_t> {
};

#endif // __SNODE_H