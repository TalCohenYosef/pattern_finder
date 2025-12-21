#pragma once
#include <memory>
#include <cstdint>

struct Node {
    std::shared_ptr<Node> left;     // sibling
    std::shared_ptr<Node> right;    // sibling
    std::shared_ptr<Node> son;      // first child
    std::weak_ptr<Node> parent;     // back-reference (NO ownership)

    uint32_t index;
    int32_t depth;

    Node(int32_t idx, int32_t d = 0)
        : index(idx), depth(d) {}
};

using NodePtr = std::shared_ptr<Node>;