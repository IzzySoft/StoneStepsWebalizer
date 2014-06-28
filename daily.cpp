/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    daily.cpp
*/
#include "pch.h"

#include "daily.h"
#include "serialize.h"

daily_t::daily_t(u_int month) : keynode_t<u_int>(month)
{
   td_hours = 0;
   tm_hits = tm_files = tm_hosts = tm_pages = tm_visits = 0;
   h_hits_max = h_files_max = h_pages_max = h_visits_max = h_hosts_max = 0;
   h_xfer_max = h_xfer_avg = .0;
   tm_xfer = .0;
   h_hits_avg = h_files_avg = h_pages_avg = h_visits_avg = h_hosts_avg = .0;
}

void daily_t::reset(u_long nodeid)
{
   keynode_t<u_int>::reset(nodeid);
   datanode_t<daily_t>::reset();

   td_hours = 0;
   tm_hits = tm_files = tm_hosts = tm_pages = tm_visits = 0;
   h_hits_max = h_files_max = h_pages_max = h_visits_max = h_hosts_max = 0;
   h_xfer_max = h_xfer_avg = .0;
   tm_xfer = .0;
   h_hits_avg = h_files_avg = h_pages_avg = h_visits_avg = h_hosts_avg = .0;
}

//
// serialization
//

u_int daily_t::s_data_size(void) const
{
   return datanode_t<daily_t>::s_data_size() + sizeof(u_long) * 10 + sizeof(double) * 8 + sizeof(u_short);
}

u_int daily_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize;
   void *ptr;

   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   datanode_t<daily_t>::s_pack_data(buffer, bufsize);
   ptr = (u_char*) buffer + datanode_t<daily_t>::s_data_size();
      
   ptr = serialize(ptr, tm_hits);
   ptr = serialize(ptr, tm_files);
   ptr = serialize(ptr, tm_pages);
   ptr = serialize(ptr, tm_hosts);
   ptr = serialize(ptr, tm_visits);
   ptr = serialize(ptr, tm_xfer);
   
   ptr = serialize(ptr, h_hits_max);
   ptr = serialize(ptr, h_files_max);
   ptr = serialize(ptr, h_pages_max);
   ptr = serialize(ptr, h_xfer_max);
   ptr = serialize(ptr, h_visits_max);
   ptr = serialize(ptr, h_hosts_max);
   
   ptr = serialize(ptr, h_hits_avg);
   ptr = serialize(ptr, h_files_avg);
   ptr = serialize(ptr, h_pages_avg);
   ptr = serialize(ptr, h_xfer_avg);
   ptr = serialize(ptr, h_visits_avg);
   ptr = serialize(ptr, h_hosts_avg);
   
         serialize(ptr, td_hours);

   return datasize;
}

u_int daily_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   bool fixver = (intptr_t) arg == -1;
   u_short version;
   u_int datasize;
   const void *ptr = buffer;

   datasize = s_data_size(buffer, fixver);

   if(bufsize < datasize)
      return 0;

   // see the comment in state_t::restore_state_ex
   if(fixver) {
      version = 1;
      ptr = (u_char*) buffer;
   }
   else {
      version = s_node_ver(buffer);
      ptr = (u_char*) buffer + datanode_t<daily_t>::s_data_size(buffer);
   }

   ptr = deserialize(ptr, tm_hits);
   ptr = deserialize(ptr, tm_files);
   ptr = deserialize(ptr, tm_pages);
   ptr = deserialize(ptr, tm_hosts);
   ptr = deserialize(ptr, tm_visits);
   ptr = deserialize(ptr, tm_xfer);

   if(version >= 2) {
      ptr = deserialize(ptr, h_hits_max);
      ptr = deserialize(ptr, h_files_max);
      ptr = deserialize(ptr, h_pages_max);
      ptr = deserialize(ptr, h_xfer_max);
      ptr = deserialize(ptr, h_visits_max);
      ptr = deserialize(ptr, h_hosts_max);
      
      ptr = deserialize(ptr, h_hits_avg);
      ptr = deserialize(ptr, h_files_avg);
      ptr = deserialize(ptr, h_pages_avg);
      ptr = deserialize(ptr, h_xfer_avg);
      ptr = deserialize(ptr, h_visits_avg);
      ptr = deserialize(ptr, h_hosts_avg);

            deserialize(ptr, td_hours);
   }
   else {
      h_hits_max = h_files_max = h_pages_max = h_visits_max = 0;
      h_hits_avg = h_files_avg = h_pages_avg = h_visits_avg = h_xfer_max = h_xfer_avg = .0;
      td_hours = 0;
   }
   
   if(upcb)
      upcb(*this, arg);
   
   return datasize;
}

u_int daily_t::s_data_size(const void *buffer, bool fixver)
{
   u_short version = s_node_ver(buffer);
   u_long datasize = datanode_t<daily_t>::s_data_size(buffer) + sizeof(u_long) * 5 + sizeof(double);
   
   if(fixver || version < 2)
      return datasize;
      
   return datasize + 
            sizeof(u_short)      +  // td_hours 
            sizeof(u_long) * 5   +  // h_hits_max, h_files_max, h_pages_max, h_visits_max, h_hosts_max
            sizeof(double) * 7;     // h_hits_avg, h_files_avg, h_pages_avg, h_visits_avg, h_hosts_avg, h_xfer_max, hxfer_avg
}
