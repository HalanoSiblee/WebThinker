#pragma once

#include "Graph.hpp"
#include <vector>
#include <string>
#include <set>
#include <cstdint>

struct NodeSnap {
    uint64_t id = 0;
    double x = 0, y = 0;
    std::string title;
    std::string text;
    uint32_t color = 0;
};

struct SquareSnap {
    uint64_t id = 0;
    double x = 0, y = 0, w = 2, h = 2;
    std::string title;
    std::string text;
    uint32_t color = 0;
};

struct VectorSnap {
    uint64_t id = 0;
    double x = 0, y = 0;
    float scale = 1.0f;
    std::string title;
    std::string svg;
};

struct EdgeSnap {
    uint64_t id = 0;
    uint64_t from = 0;
    uint64_t to = 0;
    uint32_t color = 0;
};

struct GraphSnap {
    std::vector<NodeSnap> nodes;
    std::vector<SquareSnap> squares;
    std::vector<VectorSnap> vectors;
    std::vector<EdgeSnap> edges;
    std::set<uint64_t> free_node_ids;
    std::set<uint64_t> free_square_ids;
    std::set<uint64_t> free_vector_ids;
    uint64_t next_node_id = 1;
    uint64_t next_square_id = 1;
    uint64_t next_vector_id = 1;
    uint64_t next_edge_id = 1;
    uint64_t selected_id = 0;
    int sel_kind = 0; // 0 none, 1 node, 2 square, 3 vector
};

class UndoHistory {
public:
    explicit UndoHistory(Graph* g, size_t limit = 64);
    void setGraph(Graph* g) { graph_ = g; }
    void push();
    bool canUndo() const;
    bool canRedo() const;
    bool undo();
    bool redo();
    void clear();

private:
    Graph* graph_ = nullptr;
    std::vector<GraphSnap> undo_;
    std::vector<GraphSnap> redo_;
    size_t limit_ = 64;
    GraphSnap capture() const;
    void restore(const GraphSnap& s);
};
