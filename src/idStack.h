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

   // The ID of a name, hashed whole, in the current scope.
   uint64_t getId(const char *name) const;
   // The ID of a label: only the text before its ##, which must not be empty; see monstro_display_text.
   uint64_t getLabelId(const char *label) const;

   int depth() const { return seeds.size(); }
   bool isBalanced() const { return seeds.empty(); }
   void clear() { seeds.clear(); }

 private:
   FixedStack<uint64_t, MONSTRO_MAX_ID_STACK_DEPTH> seeds{"ID stack"};
};

} // namespace monstro

#endif
