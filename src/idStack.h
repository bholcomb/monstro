#ifndef MONSTRO_INTERNAL_ID_STACK_H
#define MONSTRO_INTERNAL_ID_STACK_H

#include <stdint.h>
#include "constants.h"

namespace monstro {

class IdStack {
public:
    IdStack();

    void push(uint64_t id);
    void push(const char* name);
    void pop();

    uint64_t getId(const char* name) const;

    int depth() const { return top_index; }
    bool isBalanced() const { return top_index == 0; }

private:
    uint64_t seeds[MONSTRO_MAX_ID_STACK_DEPTH];
    int top_index = 0;
};

} // namespace monstro

#endif
