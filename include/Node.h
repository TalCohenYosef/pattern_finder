#pragma once
#include <memory>
#include <unordered_set>
#include <cstdint>

struct Node {

    bool is_alive = true;
    uint32_t index;
    int32_t depth;

    Node(int32_t idx, int32_t d = 0)
        : index(idx), depth(d) {}
};
