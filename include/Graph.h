#include <boost/graph/adjacency_list.hpp>

struct VertexProperty {
    int color;
};

struct EdgeProperty {
    bool is_reversed;
};


using Graph = boost::adjacency_list<
    boost::vecS,        // edge container
    boost::vecS,        // vertex container (indexable)
    boost::undirectedS,// or boost::directedS if needed
    VertexProperty,     // vertex properties
    EdgeProperty        // edge properties
>;