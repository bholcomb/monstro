#ifndef MONSTRO_INTERNAL_FIXED_H
#define MONSTRO_INTERNAL_FIXED_H

#include "fatal.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Fixed-capacity containers. Monstro never grows its storage to hide a UI that
 * outgrew it: these are the few places where capacity is checked, and running
 * out stops the process with the container's name and the numbers involved.
 */

namespace monstro
{

// A stack of at most Capacity entries.
template <typename T, int Capacity> struct FixedStack
{
   const char *myName;
   T myItems[Capacity] = {};
   int myCount = 0;

   explicit FixedStack(const char *name) : myName(name) {}

   T &push(const T &value)
   {
      MONSTRO_REQUIRE(myCount < Capacity,
                      "%s overflow: capacity %d (a pop or end call is missing, or the nesting is "
                      "too deep)",
                      myName, Capacity);
      myItems[myCount] = value;
      return myItems[myCount++];
   }

   T pop()
   {
      MONSTRO_REQUIRE(myCount > 0, "%s underflow: a pop or end call without its push or begin", myName);
      return myItems[--myCount];
   }

   T &top()
   {
      MONSTRO_REQUIRE(myCount > 0, "%s is empty", myName);
      return myItems[myCount - 1];
   }

   const T &top() const
   {
      MONSTRO_REQUIRE(myCount > 0, "%s is empty", myName);
      return myItems[myCount - 1];
   }

   int size() const { return myCount; }
   bool empty() const { return myCount == 0; }
   void clear() { myCount = 0; }
   void truncate(int count) { myCount = count < myCount ? count : myCount; }
   T &operator[](int index) { return myItems[index]; }
   const T &operator[](int index) const { return myItems[index]; }
};

// Persistent per-ID state records (windows, tree nodes, tab bars, tables): slots are handed out once and kept.
template <typename T, int Capacity> struct SlotTable
{
   const char *myName;
   T myItems[Capacity] = {};
   int myCount = 0;

   explicit SlotTable(const char *name) : myName(name) {}

   T &allocate()
   {
      MONSTRO_REQUIRE(myCount < Capacity, "%s full: all %d slots are in use", myName, Capacity);
      return myItems[myCount++];
   }

   int size() const { return myCount; }
   T *begin() { return myItems; }
   T *end() { return myItems + myCount; }
   T &operator[](int index) { return myItems[index]; }
};

// Per-frame output storage with a capacity fixed when the context is created (or by the caller).
template <typename T> struct CapacityBuffer
{
   const char *myName;
   T *myData = nullptr;
   uint32_t myCount = 0;
   size_t myCapacity = 0;

   explicit CapacityBuffer(const char *name) : myName(name) {}

   // owner and frame say where the overflow happened; remedy says which capacity to revisit.
   T *reserve(uint32_t count, const char *owner, int frame, const char *remedy)
   {
      size_t needed = (size_t)myCount + count;
      MONSTRO_REQUIRE(needed <= myCapacity,
                      "%s overflow: capacity %zu, needed %zu (window '%s', frame %d); simplify "
                      "that UI or %s",
                      myName, myCapacity, needed, owner ? owner : "?", frame, remedy);
      T *slots = myData + myCount;
      myCount += count;
      return slots;
   }
};

} // namespace monstro

#endif
