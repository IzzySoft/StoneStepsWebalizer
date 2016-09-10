/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    spnode.h
*/
#ifndef __SPNODE_H
#define __SPNODE_H

#include "hashtab.h"
#include "basenode.h"
#include "types.h"

//
// Spammer
//
struct spnode_t : public base_node<spnode_t> {
      public:
         spnode_t(void) : base_node<spnode_t>() {}
         spnode_t(const string_t& host) : base_node<spnode_t>(host) {}
};

//
// Spammers
//
// Tracks IP addresses of known spammers. 
// 
// This table is not saved in the database and instead every host node read from 
// the database that has the spammer attribute set is inserted into this table.
// Keeping spammers in a separate hash table allows us to look-up spammers before 
// the host node can be processed.
//
class sp_hash_table : public hash_table<spnode_t> {
};

#endif // __SPNODE_H

