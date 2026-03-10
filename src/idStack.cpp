#include "idStack.h"
#include "helpers.h"
#include <cstring>
#include <stdexcept>

namespace monstro
{

/* ------------------------------------------------------------------------- */
/* Internal Hash Helpers                                                     */
/* ------------------------------------------------------------------------- */

static constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
static constexpr uint64_t FNV_PRIME = 1099511628211ULL;

static uint64_t hash_data(const void *data, size_t size, uint64_t seed)
{
   const uint8_t *bytes = (const uint8_t *)data;
   uint64_t hash = seed;
   for (size_t i = 0; i < size; i++)
   {
      hash ^= bytes[i];
      hash *= FNV_PRIME;
   }
   return hash;
}

IdStack::IdStack() { seeds[0] = FNV_OFFSET_BASIS; }

void IdStack::push(uint64_t id)
{
   if (top_index >= MONSTRO_MAX_ID_STACK_DEPTH - 1)
   {
      throw std::runtime_error("Id stack depth exceeded");
   }

   top_index++;
   seeds[top_index] = id;
}

void IdStack::push(const char *name) { push(getId(name)); }

void IdStack::pop()
{
   if (top_index <= 0)
   {
      throw std::runtime_error("Id stack underflow");
   }

   top_index--;
}

uint64_t IdStack::getId(const char *name) const { return hash_data(name, std::strlen(name), seeds[top_index]); }

} // namespace monstro
