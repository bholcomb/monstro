#ifndef MONSTRO_INTERNAL_ID_STACK_H
#define MONSTRO_INTERNAL_ID_STACK_H

#include "constants.h"
#include "fixed.h"
#include <stdint.h>

namespace monstro
{

class IdStack
{
 public:
   void push(uint64_t id);
   void push(const char *name);
   void pop();

   uint64_t getId(const char *name) const;

   int depth() const { return seeds.size(); }
   bool isBalanced() const { return seeds.empty(); }

 private:
   FixedStack<uint64_t, MONSTRO_MAX_ID_STACK_DEPTH> seeds{"ID stack"};
};

} // namespace monstro

#endif
