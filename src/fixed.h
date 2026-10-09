#ifndef MONSTRO_INTERNAL_FIXED_H
#define MONSTRO_INTERNAL_FIXED_H

#include "fatal.h"
#include <new>
#include <stddef.h>
#include <stdint.h>

/*
 * Fixed-capacity containers. Monstro never grows its storage to hide a UI that
 * outgrew it: these are the few places where capacity is checked, and running
 * out stops the process with the container's name, its capacity and what was
 * needed.
 */

namespace monstro
{

// A stack of at most Capacity entries, for nesting depths that are not configurable.
template <typename T, int Capacity> struct FixedStack
{
   const char *myName;
   T myItems[Capacity] = {};
   int myCount = 0;

   explicit FixedStack(const char *name) : myName(name) {}

   T &push(const T &value)
   {
      MONSTRO_REQUIRE(myCount < Capacity,
                      "%s overflow: capacity %d, needed %d (a pop or end call is missing, or the nesting is "
                      "too deep)",
                      myName, Capacity, myCount + 1);
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

// Records with a capacity taken from a config, allocated once when their owner is created.
template <typename T> struct ConfiguredArray
{
   const char *myName;
   const char *myRemedy;
   T *myItems = nullptr;
   int myCount = 0;
   int myCapacity = 0;

   // remedy says which config field to revisit when the capacity runs out.
   ConfiguredArray(const char *name, const char *remedy) : myName(name), myRemedy(remedy) {}
   ~ConfiguredArray() { delete[] myItems; }
   ConfiguredArray(const ConfiguredArray &) = delete;
   ConfiguredArray &operator=(const ConfiguredArray &) = delete;

   bool init(uint32_t capacity)
   {
      myItems = new (std::nothrow) T[capacity];
      myCapacity = myItems ? (int)capacity : 0;
      return myItems != nullptr;
   }

   T &add()
   {
      MONSTRO_REQUIRE(myCount < myCapacity, "%s full: capacity %d, needed %d; %s", myName, myCapacity, myCount + 1,
                      myRemedy);
      myItems[myCount] = T{};
      return myItems[myCount++];
   }

   T pop()
   {
      MONSTRO_REQUIRE(myCount > 0, "%s underflow", myName);
      return myItems[--myCount];
   }

   T &top()
   {
      MONSTRO_REQUIRE(myCount > 0, "%s is empty", myName);
      return myItems[myCount - 1];
   }

   void removeAt(int index)
   {
      for (int i = index; i + 1 < myCount; i++)
         myItems[i] = myItems[i + 1];
      myCount--;
   }

   int size() const { return myCount; }
   bool empty() const { return myCount == 0; }
   void clear() { myCount = 0; }
   void truncate(int count) { myCount = count < myCount ? count : myCount; }
   size_t bytes() const { return sizeof(T) * (size_t)myCapacity; }
   T *begin() { return myItems; }
   T *end() { return myItems + myCount; }
   T &operator[](int index) { return myItems[index]; }
   const T &operator[](int index) const { return myItems[index]; }
};

} // namespace monstro

#endif
