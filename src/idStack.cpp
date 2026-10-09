#include "idStack.h"
#include "helpers.h"
#include <cstring>

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

void IdStack::push(uint64_t id) { seeds.push(id); }

void IdStack::push(const char *name) { push(getId(name)); }

void IdStack::pop() { seeds.pop(); }

uint64_t IdStack::getId(const char *name) const
{
   uint64_t seed = seeds.empty() ? FNV_OFFSET_BASIS : seeds.top();
   return hash_data(name, std::strlen(name), seed);
}

uint64_t IdStack::getLabelId(const char *label) const
{
   size_t length = monstro_id_text_length(label);
   MONSTRO_REQUIRE(length > 0,
                   "label '%s' has no ID: the text before ## is its ID and the text after it is shown; write "
                   "'id##%s', or '%s##' to show nothing",
                   label, monstro_display_text(label), monstro_display_text(label));
   uint64_t seed = seeds.empty() ? FNV_OFFSET_BASIS : seeds.top();
   return hash_data(label, length, seed);
}

} // namespace monstro
