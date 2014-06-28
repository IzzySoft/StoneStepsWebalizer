/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    scnode.cpp
*/
#include "pch.h"

#include "scnode.h"
#include "serialize.h"

sc_hash_table::sc_hash_table(u_int maxcodes) : stcodes(maxcodes, true)
{
   memset(clsindex, 0, sizeof(clsindex));
   stcodes.push(scnode_t(0));                // unknown status code node
}

sc_hash_table::~sc_hash_table(void)
{
}

void sc_hash_table::add_status_code(u_int code)
{
   u_int cls = code / 100;

   // check if in the valid range 
   if(cls < 1 || cls > 5) 
      return;

   // and in the ascending order
   if(stcodes.size() && code <= stcodes[stcodes.size()-1].code)
      return;

   // if first in its class, update the index
   if(clsindex[cls] == 0)
      clsindex[cls] = stcodes.size();

   stcodes.push(scnode_t(code));
}

scnode_t& sc_hash_table::get_status_code(u_int code)
{
   u_int index;
   u_int cls = code / 100;

   // check if class is valid
   if(cls < 1 || cls > 5) 
      return stcodes[0];

   // start with the first code in its class
   for(index = clsindex[cls]; index < stcodes.size(); index++) {
      if(stcodes[index].code == code)
         return stcodes[index];

      // codes are sorted, break
      if(code < stcodes[index].code)
         break;
   }

   // not found
   return stcodes[0];
}

const scnode_t& sc_hash_table::operator [] (u_int index) const
{
   if(index >= stcodes.size())
      return stcodes[0];

   return stcodes[index];
}

//
// serialization
//

u_int scnode_t::s_data_size(void) const
{
   return datanode_t<scnode_t>::s_data_size() + sizeof(u_long);
}

u_int scnode_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize;

   datasize = s_data_size();

   if(bufsize < s_data_size())
      return 0;

   serialize(buffer, count);

   return datasize;
}

u_int scnode_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   u_int datasize;

   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   deserialize(buffer, count);
   
   return datasize;
}

u_int scnode_t::s_data_size(const void *buffer)
{
   return datanode_t<scnode_t>::s_data_size(buffer) + sizeof(u_long);
}
