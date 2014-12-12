/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    inode.h
*/
#ifndef __INODE_H
#define __INODE_H

#include "hashtab.h"
#include "basenode.h"
#include "types.h"

/* host hash table struct    */
typedef struct inode_t : public base_node<inode_t> {
      uint64_t   count;
      uint64_t   files;
      uint64_t   visit;
      tstamp_t tstamp;               // last request time
      uint64_t xfer;
      double   avgtime;				    // average processing time (sec)
      double   maxtime;              // maximum processing time (sec)

      public:
         typedef void (*s_unpack_cb_t)(inode_t& rnode, void *arg);

      public:
         inode_t(void);
         inode_t(const inode_t& inode);
         inode_t(const char *ident);

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
} *INODEPTR;

//
// Users
//
class i_hash_table : public hash_table<inode_t> {
   public:
      i_hash_table(void) : hash_table<inode_t>(LMAXHASH) {}
};

#endif // __INODE_H
