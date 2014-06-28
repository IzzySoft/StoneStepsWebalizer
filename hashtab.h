/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   hashtab.h
*/
#ifndef _HASHTAB_H
#define _HASHTAB_H

#include "tstring.h"
#include "types.h"

#define LMAXHASH     1048576ul
#define MAXHASH      16384ul
#define SMAXHASH     1024ul

// -----------------------------------------------------------------------
//
// hash functions
//
// -----------------------------------------------------------------------
u_long hash_bin(u_long hashval, const u_char *buf, u_int blen);
u_long hash_str(u_long hashval, const char *str, u_int slen);
template <typename type_t> u_long hash_num(u_long hashval, type_t num);

inline u_long hash_byte(u_long hashval, u_char b) {return ((hashval & (~0ul << (32-5))) >> (32-5)) ^ (hashval << 5) ^ b;}

inline u_long hash_ex(u_long hashval, const char *str) {return (str && *str) ? hash_str(hashval, str, strlen(str)) : 0;}
inline u_long hash_ex(u_long hashval, const string_t& str) {return hash_str(hashval, str.c_str(), str.length());}
inline u_long hash_ex(u_long hashval, u_int data) {return hash_num(hashval, data);}

// -----------------------------------------------------------------------
//
// generic hash table node
//
// -----------------------------------------------------------------------
template <typename node_t, typename key_t = string_t> 
struct htab_node_t {
      node_t   *next;                // pointer to next node

      public:
         htab_node_t(void) {next = NULL;}

         virtual ~htab_node_t(void) {next = NULL;}

         virtual const key_t& key(void) const = 0;

         virtual bool istype(u_int type) const = 0;
};

// -----------------------------------------------------------------------
//
// hash table object
//
// -----------------------------------------------------------------------
template <typename node_t, typename key_t = string_t>
class hash_table {
   private:
      enum swap_code {swap_ok, swap_failed, swap_inuse};

      struct bucket_t {
         node_t   *head;
         u_int    count;

         public:
            bucket_t(void) {head = NULL; count = 0;}
      };

   public:
      //
      // Evaluation callback is called first. Returns true if the node
      // can be swapped out, false otherwise. In the latter case the
      // swap-out callback will not be called.
      //
      typedef bool (*eval_cb_t)(const node_t *node, void *arg);

      //
      // If evaluation was successful, the swap-out callback is called.
      // If swap-out callback returns false, the swap_out method exits 
      // the loop and returns false.
      //
      typedef bool (*swap_cb_t)(node_t *node, void *arg);

   public:
      class iterator {
         friend class hash_table<node_t, key_t>;

         private:
            bucket_t *htab;
            u_long   index;
            u_long   maxhash;
            node_t   *nptr;

         protected:
            iterator(bucket_t _htab[], u_long _maxhash) {htab = _htab; index = 0; maxhash = _maxhash; nptr = NULL;}

         public:
            iterator(void) {htab = NULL; nptr = NULL; index = 0; maxhash = 0;}

            node_t *item(void) {return nptr;}

            node_t *next(void)
            {
               if(htab == NULL)
                  return NULL;

               if(nptr && nptr->next) {
                  nptr = nptr->next;
                  return nptr;
               }

               while(index < maxhash) {
                  if((nptr = htab[index++].head) != NULL)
                     return nptr;
               }

               return NULL;
            }
      };

      class const_iterator : public iterator {
         friend class hash_table<node_t, key_t>;

         private:
            const_iterator(bucket_t _htab[], u_long _maxhash) : iterator(_htab, _maxhash) {}

         public:
            const_iterator(void) : iterator() {}

            const node_t *item(void) {return iterator::item();}

            const node_t *next(void) {return iterator::next();}
      };

   private:
      u_long      count;      // number of hash table entries
      u_long      maxhash;    // number of buckets in the hash table
      u_long      emptycnt;   // number of empty buckets
      bucket_t    *htab;      // buckets

      bool        swap;       // true, if some data is swapped out
      bool        cleared;    // true when the table has been cleared

      eval_cb_t   evalcb;     // swap out evaluation callback
      swap_cb_t   swapcb;     // swap out callback
      void        *cbarg;     // swap out callback argument

   private:
      node_t *move_node(node_t *nptr, node_t *prev, node_t **next) const
      {
         prev->next = nptr->next;
         nptr->next = *next;
         *next = nptr;
         return nptr;
      }

      swap_code swap_out_node(bucket_t& bucket, node_t **pptr);

      bool swap_out_bucket(bucket_t& bucket);

      virtual bool compare(const node_t *nptr, const void *params) const {return false;}

   protected:
      virtual bool load_array_check(const node_t *) const {return true;}

   public:
      hash_table(u_long maxhash = MAXHASH, eval_cb_t evalcb = NULL, swap_cb_t swapcb = NULL, void *cbarg = NULL);

      ~hash_table(void);

      //
      // informational
      //
      u_long size(void) const {return count;}

      u_long buckets(void) const {return maxhash;} 

      u_long empty_buckets(void) const {return emptycnt;}

      //
      // swap-out interface
      //
      bool is_cleared(void) const {return cleared;}

      void set_cleared(bool value) {cleared = value;}

      bool is_swapped_out(void) const {return swap;}

      void set_swapped_out(bool value) {swap = value;}

      void set_swap_out_cb(eval_cb_t evalcb, swap_cb_t swapcb, void *arg);

      bool swap_out(void);

      //
      // iterators
      //
      iterator begin(void) {return iterator(htab, maxhash);}

      const_iterator begin(void) const {return const_iterator(htab, maxhash);}

      //
      // hash table interface
      //
      void clear(void);

      bool find_key(const key_t& key) const {return (const_cast<hash_table<node_t, key_t>*>(this))->find_node(key) ? true : false;}

      const node_t *find_node(const key_t& key) const {return find_node(hash_ex(0, key), key);}

      node_t *find_node(const key_t& key) {return find_node(hash_ex(0, key), key);}

      const node_t *find_node(u_long hashval, const key_t& key) const;

      node_t *find_node(u_long hashval, const key_t& key) {return const_cast<node_t*>(const_cast<const hash_table<node_t, key_t>*>(this)->find_node(hashval, key));}

      node_t *find_node(u_long hashval, const key_t& str, u_int type);

      node_t *find_node(u_long hashval, const void *params);

      node_t *put_node(const key_t& key, node_t *nptr) {return put_node(hash_ex(0, key), nptr);}

      node_t *put_node(u_long hashval, node_t *nptr);

      void pop_node(u_long hashval, node_t *nptr);

      //
      // miscellaneous
      //
      u_long load_array(const node_t *array[]) const;

      u_long load_array(const node_t *array[], u_int type, u_long& typecnt) const;
};

#endif  /* _HASHTAB_H */
