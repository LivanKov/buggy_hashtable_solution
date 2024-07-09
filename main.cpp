#include <cstdint>
#include <initializer_list>
#include <sys/mman.h>
#include <iostream>
#include "debug.hpp"

inline void *allocZeros(uint64_t size)
{
   return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

inline uint64_t hashKey(uint64_t k)
{
   // MurmurHash64A
   const uint64_t m = 0xc6a4a7935bd1e995;
   const int r = 47;
   uint64_t h = 0x8445d61a4e774912 ^ (8 * m);
   k *= m;
   k ^= k >> r;
   k *= m;
   h ^= k;
   h *= m;
   h ^= h >> r;
   h *= m;
   h ^= h >> r;
   return h;
}

struct Hashtable
{
   struct Entry
   {
      uint64_t key;
      uint64_t value;
      Entry *next;
   };

   uint64_t htSize;
   uint64_t mask;
   Entry **ht;

   Hashtable(uint64_t size)
   {
      htSize = 1ull << ((8 * sizeof(uint64_t)) - __builtin_clzl(size));
      mask = htSize - 1;
      ht = reinterpret_cast<Entry **>(allocZeros(sizeof(uint64_t) * htSize));
   }

   ~Hashtable()
   {
      for (uint64_t i = 0; i != htSize; ++i)
      {
         auto _current = ht[i];
         while (_current != nullptr)
         {
            auto _next = _current->next;
            delete _current;
            _current = _next;
         }
      }

      munmap(ht, htSize);
   }

   Entry *lookup(uint64_t key)
   {
      for (Entry *e = ht[hashKey(key) & mask]; e; e = e->next)
         if (e->key == key)
            return e;
      return nullptr;
   }

   bool insert(uint64_t key, uint64_t value)
   {
      Entry *e = lookup(key);
      if (e)
      {
         e->value = value;
         return false;
      }
      else
      {
         uint64_t pos = hashKey(key) & mask;
         Entry *newEntry = new Entry();
         if (!ht[pos])
            ht[pos] = newEntry;
         else
         {
            auto current = ht[pos];
            while (current->next)
            {
               current = current->next;
            }
            current->next = newEntry;
         }
         newEntry->key = key;
         newEntry->value = value;
         return true;
      }
   }

   bool erase(uint64_t key)
   {
      uint64_t pos = hashKey(key) & mask;
      auto _current = ht[pos];
      if (!_current)
         return false;
      if (_current->key == key)
      {
         ht[pos] = _current->next;
         delete _current;
         return true;
      }
      while (_current->next)
      {
         if (_current->next->key == key)
         {
            auto _next = _current->next->next;
            delete _current->next;
            _current->next = _next;
            return true;
         }
         _current = _current->next;
      }
      return false;
   }
};

void general_test()
{
   for (uint64_t size : {10, 99, 837, 48329, 384933})
   {
      Hashtable h(size);

      // insert
      for (uint64_t i = 0; i < size; i++)
      {
         Ensure(h.insert(i, 42));
      }
      // update
      for (uint64_t i = 0; i < size; i++)
      {
         Ensure(!h.insert(i, i));
      }
      // lookup
      for (uint64_t i = 0; i < size; i++)
      {
         Hashtable::Entry *e = h.lookup(i);
         Ensure(e);
         Ensure(e->value == i);
      }
      // erase some
      for (uint64_t i = 0; i < size / 2; i += 3)
      {
         Ensure(h.erase(i));
      }
      // erase twice
      for (uint64_t i = 0; i < size / 2; i += 3)
      {
         Ensure(!h.erase(i));
      }
      // lookup
      for (uint64_t i = 0; i < size / 2; i++)
      {
         Hashtable::Entry *e = h.lookup(i);
         if ((i % 3) == 0)
         {
            Ensure(!e);
         }
         else
         {
            Ensure(e);
            Ensure(e->value == i);
         }
      }
      // erase some more
      for (uint64_t i = 0; i < size / 2; i++)
      {
         if ((i % 3) == 0)
         {
            Ensure(!h.erase(i));
         }
         else
         {
            Ensure(h.erase(i));
         }
      }
      // lookup
      for (uint64_t i = 0; i < size / 2; i++)
      {
         Hashtable::Entry *e = h.lookup(i);
         Ensure(!e);
      }
   }
}

void custom_test()
{
   Hashtable h(837);
   /*h.insert(0, 42);
   h.insert(1, 42);
   h.insert(2, 42);
   h.insert(3, 42);
   h.print_map();
   h.insert(2, 46);
   h.print_map();
   h.erase(0);
   h.erase(2);*/
   // h.print_map();
}

int main()
{
   general_test();
   return 0;
}
