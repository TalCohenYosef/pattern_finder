#pragma once

#include <boost/graph/adjacency_list.hpp>

struct VertexProperty {
    int32_t color;
};

struct EdgeProperty {
    bool is_reversed;
};


using BoostGraph = boost::adjacency_list<
    boost::vecS,        // edge container
    boost::vecS,        // vertex container (indexable)
    boost::undirectedS,// or boost::directedS if needed
    VertexProperty,     // vertex properties
    EdgeProperty        // edge properties
>;