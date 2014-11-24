/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    hnode.cpp
*/
#include "pch.h"

#include "hnode.h"
#include "serialize.h"
#include "util.h"

const u_int hnode_t::ccode_size = 2;   // in bytes, not counting the zero terminator

// -----------------------------------------------------------------------
//
// hnode_t
//
// -----------------------------------------------------------------------

hnode_t::hnode_t(void) : base_node<hnode_t>()
{
   count = files = pages = visits = visits_conv = 0;
   visit_avg = .0;
   visit_max = 0;
   xfer = 0;
   max_v_hits = max_v_files = max_v_pages = 0;
   max_v_xfer = 0;
   spammer = false;
   robot = false;
   ccode[0] = ccode[1] = ccode[2] = 0;
   visit = NULL;
   dlref = 0;
   grp_visit = NULL;
}

hnode_t::hnode_t(const hnode_t& hnode) : base_node<hnode_t>(hnode)
{
   spammer = hnode.spammer;
   robot = hnode.robot;
   count = hnode.count;
   files = hnode.files;
   pages = hnode.pages;

   xfer = hnode.xfer;

   visits = hnode.visits; 
   visits_conv = hnode.visits_conv;

   visit_avg = hnode.visit_avg;
   visit_max = hnode.visit_max; 

   max_v_hits = hnode.max_v_hits;
   max_v_files = hnode.max_v_files;
   max_v_pages = hnode.max_v_pages;
   max_v_xfer = hnode.max_v_xfer;

   name = hnode.name;

   ccode[0] = hnode.ccode[0];
   ccode[1] = hnode.ccode[1];
   ccode[2] = 0;

   dlref = 0;
   tstamp = hnode.tstamp;

   grp_visit = NULL;                         // grp_visit is not copied

   if((visit = hnode.visit) != NULL)
      visit->hostref++;
}

hnode_t::hnode_t(const string_t& ipaddr) : base_node<hnode_t>(ipaddr)
{
   spammer = false;
   robot = false;
   count = 0;
   files = pages = visits = visits_conv = 0;
   visit_avg = .0;
   visit_max = 0; 
   xfer = 0;
   max_v_hits = max_v_files = max_v_pages = 0;
   max_v_xfer = 0;
   ccode[0] = ccode[1] = ccode[2] = 0;
   visit = NULL;
   dlref = 0;
   grp_visit = NULL;
}

hnode_t::~hnode_t(void)
{
   vnode_t *vptr;

   if(visit) {
      if(!--visit->hostref)
         delete visit;
   }

   // if grp_visit isn't NULL, something went wrong
   while(grp_visit) {
      vptr = grp_visit;
      grp_visit = grp_visit->next;
      delete vptr;
   }
}

void hnode_t::set_visit(vnode_t *vnode)
{
   if(visit == vnode)
      return;

   if(visit)
      visit->hostref--;

   if((visit = vnode) != NULL)
      visit->hostref++;
}

void hnode_t::add_grp_visit(vnode_t *vnode)
{
   if(vnode) {
      vnode->next = grp_visit;
      grp_visit = vnode;
   }
}

vnode_t *hnode_t::get_grp_visit(void)
{
   vnode_t *vnode = grp_visit;
   
   if(vnode)
      grp_visit = vnode->next;
      
   return vnode;
}

void hnode_t::reset(uint64_t nodeid)
{
   base_node<hnode_t>::reset(nodeid);

   spammer = false;
   count = 0;
   files = pages = visits = visits_conv = 0;
   visit_avg = .0;
   visit_max = 0; 
   xfer = 0;
   max_v_hits = max_v_files = max_v_pages = 0;
   max_v_xfer = 0;
   ccode[0] = ccode[1] = ccode[2] = 0;
   dlref = 0;
   tstamp.reset();

   set_visit(NULL);
}

void hnode_t::set_ccode(const char _ccode[])
{
   ccode[0] = _ccode[0];
   ccode[1] = _ccode[1];
   ccode[2] = 0;
}

void hnode_t::reset_ccode(void)
{
   ccode[0] = ccode[1] = ccode[2] = 0;
}

//
// serialization
//

u_int hnode_t::s_data_size(void) const
{
   return base_node<hnode_t>::s_data_size() + 
               sizeof(u_char) * 3 +             // spammer, active, robot
               sizeof(uint64_t) * 3 +             // count, files, pages
               sizeof(uint64_t) * 3 +             // visits, visit_max, visits_conv
               sizeof(uint64_t) * 3 +             // max_v_hits, max_v_files, max_v_pages
               sizeof(uint64_t)     +             // hash(value)  
               s_size_of(tstamp)  +             // tstamp 
               sizeof(double)  +                // visit_avg
               sizeof(uint64_t) * 2 +           // xfer, max_v_xfer
               s_size_of(name)    +             // name
               ccode_size         +             // country code
               s_size_of(city);                 // city
}

u_int hnode_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize, basesize;
   void *ptr;

   basesize = base_node<hnode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   base_node<hnode_t>::s_pack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = serialize(ptr, spammer);
   ptr = serialize(ptr, count);
   ptr = serialize(ptr, files);
   ptr = serialize(ptr, pages);
   ptr = serialize(ptr, xfer);
   ptr = serialize(ptr, visits);
   ptr = serialize(ptr, visit_avg);
   ptr = serialize(ptr, visit_max);
   ptr = serialize(ptr, max_v_hits);
   ptr = serialize(ptr, max_v_files);
   ptr = serialize(ptr, max_v_pages);
   ptr = serialize(ptr, max_v_xfer);
   ptr = serialize(ptr, (visit) ? true : false);

   ptr = serialize(ptr, s_hash_value());
   ptr = serialize(ptr, name);
   ptr = serialize(ptr, ccode, ccode_size);

   ptr = serialize(ptr, robot);
   ptr = serialize(ptr, visits_conv);
   ptr = serialize(ptr, tstamp);

         serialize(ptr, city);

   return datasize;
}

u_int hnode_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   bool active, tmp;
   u_int datasize, basesize;
   u_short version;
   const void *ptr;

   basesize = base_node<hnode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);
   base_node<hnode_t>::s_unpack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = deserialize(ptr, tmp); spammer = tmp;
   ptr = deserialize(ptr, count);
   ptr = deserialize(ptr, files);
   ptr = deserialize(ptr, pages);
   ptr = deserialize(ptr, xfer);
   ptr = deserialize(ptr, visits);
   ptr = deserialize(ptr, visit_avg);
   ptr = deserialize(ptr, visit_max);
   ptr = deserialize(ptr, max_v_hits);
   ptr = deserialize(ptr, max_v_files);
   ptr = deserialize(ptr, max_v_pages);
   ptr = deserialize(ptr, max_v_xfer);
   ptr = deserialize(ptr, active);

   ptr = s_skip_field<uint64_t>(ptr);      // value hash

   ptr = deserialize(ptr, name);
   ptr = deserialize(ptr, ccode, ccode_size);

   if(version >= 2)
      {ptr = deserialize(ptr, tmp); robot = tmp;}
   else
      robot = false;

   if(version >= 3)
      ptr = deserialize(ptr, visits_conv);
   else
      visits_conv = 0;

   if(version >= 4) {
      if(version >= 5)
         ptr = deserialize(ptr, tstamp);
      else {
         uint64_t tmp;
         ptr = deserialize(ptr, tmp);
         tstamp.reset((time_t) tmp);
      }
   }
   else
      tstamp.reset();

   if(version >= 6)
      deserialize(ptr, city);
   else
      city.clear();

   visit = NULL;
   
   if(upcb)
      upcb(*this, active, arg);

   return datasize;
}

u_int hnode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   u_int datasize = base_node<hnode_t>::s_data_size(buffer) + 
               sizeof(u_char) * 2 +    // spammer, active
               sizeof(uint64_t) * 3 +    // count, files, pages 
               sizeof(uint64_t) * 2 +    // visits, visit_max
               sizeof(uint64_t) * 3 +    // max_v_hits, max_v_files, max_v_pages
               sizeof(uint64_t)     +    // hash(value)
               sizeof(double)       +    // visit_avg
               sizeof(uint64_t) * 2;     // xfer, max_v_xfer

   // host name and country code
   datasize += s_size_of<string_t>((u_char*) buffer + datasize) + ccode_size;

   if(version < 2)
      return datasize;

   datasize += sizeof(u_char);         // robot

   if(version < 3)
      return datasize;

   datasize += sizeof(uint64_t);         // visits_conv

   if(version < 4)
      return datasize;
 
   if(version < 5)
      datasize += sizeof(uint64_t);      // tstamp
   else
      datasize += s_size_of<tstamp_t>((u_char*) buffer + datasize);  // tstamp

   if(version < 6)
      return datasize;

   datasize += s_size_of<string_t>((u_char*) buffer + datasize);     // city

   return datasize; 
}

const void *hnode_t::s_field_value_hash(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*)buffer + base_node<hnode_t>::s_data_size(buffer) + 
            sizeof(u_char) * 2 +       // spammer, active
            sizeof(uint64_t) * 3 +     // count, files, pages
            sizeof(uint64_t) * 2 +     // visits, visit_max
            sizeof(uint64_t) * 3 +     // max_v_hits, max_v_files, max_v_pages
            sizeof(double)       +     // visit_avg
            sizeof(uint64_t) * 2;      // xfer, max_v_xfer
}

const void *hnode_t::s_field_xfer(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<hnode_t>::s_data_size(buffer) + 
            sizeof(u_char) +           // spammer 
            sizeof(uint64_t) * 3;      // count, files, pages
}

const void *hnode_t::s_field_hits(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<hnode_t>::s_data_size(buffer) + 
            sizeof(u_char);            // spammer
}

int64_t hnode_t::s_compare_xfer(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}

int64_t hnode_t::s_compare_hits(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}


