#pragma once

#include "Node.hpp"
#include "Edge.hpp"
#include "Square.hpp"
#include "VectorObject.hpp"
#include <vector>
#include <unordered_map>
#include <set>
#include <cstdint>

struct GraphSnap;

class Graph {
public:
    Graph() = default;
    ~Graph() { clear(); }

    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;

    // Nodes
    Node& addNode(double x, double y, const std::string& title = "Node");
    Node& addNodeWithId(uint64_t id, double x, double y, const std::string& title = "Node");
    bool removeNode(uint64_t id);
    Node* getNode(uint64_t id);
    const Node* getNode(uint64_t id) const;
    std::vector<Node*>& nodes();
    const std::vector<Node*>& nodes() const;

    // Squares
    Square& addSquare(double x, double y, double w = 2.0, double h = 2.0,
                      const std::string& title = "Square");
    Square& addSquareWithId(uint64_t id, double x, double y, double w, double h,
                            const std::string& title = "Square");
    bool removeSquare(uint64_t id);
    Square* getSquare(uint64_t id);
    const Square* getSquare(uint64_t id) const;
    std::vector<Square*>& squares();
    const std::vector<Square*>& squares() const;

    // Vectors (SVG)
    VectorObject& addVector(double x, double y, const std::string& svg,
                            const std::string& title = "SVG", float scale = 1.0f);
    VectorObject& addVectorWithId(uint64_t id, double x, double y, float scale,
                                  const std::string& title, const std::string& svg);
    bool removeVector(uint64_t id);
    VectorObject* getVector(uint64_t id);
    const VectorObject* getVector(uint64_t id) const;
    std::vector<VectorObject*>& vectors();
    const std::vector<VectorObject*>& vectors() const;

    // Edges
    Edge* addEdge(uint64_t from, uint64_t to);
    bool removeEdge(uint64_t edgeId);
    bool removeEdgeBetween(uint64_t from, uint64_t to);
    Edge* getEdge(uint64_t id);
    std::vector<Edge*>& edges();
    const std::vector<Edge*>& edges() const;

    enum class SelKind { None, Node, Square, Vector };

    void selectNode(uint64_t id, bool additive = false);
    void selectSquare(uint64_t id, bool additive = false);
    void selectVector(uint64_t id, bool additive = false);
    void clearSelection();
    void selectInRect(double x0, double y0, double x1, double y1); // world AABB

    Node* selectedNode();
    const Node* selectedNode() const;
    Square* selectedSquare();
    const Square* selectedSquare() const;
    VectorObject* selectedVector();
    const VectorObject* selectedVector() const;
    SelKind selectionKind() const { return sel_kind_; }

    int selectionCount() const;
    void deleteSelection();
    void moveSelection(double dx, double dy);
    void colorSelection(uint32_t color);

    void clear();
    uint64_t nextNodeId() const;
    uint64_t nextEdgeId() const { return next_edge_id_; }
    uint64_t nextSquareId() const;
    uint64_t nextVectorId() const;

    uint32_t defaultNodeColor() const { return def_node_color_; }
    uint32_t defaultSquareColor() const { return def_square_color_; }
    uint32_t edgeColor() const { return edge_color_; }
    void setDefaultNodeColor(uint32_t c) { def_node_color_ = c; }
    void setDefaultSquareColor(uint32_t c) { def_square_color_ = c; }
    void setEdgeColor(uint32_t c) { edge_color_ = c; }
    void recolorAllEdges(uint32_t c);

    const std::set<uint64_t>& freeNodeIds() const { return free_node_ids_; }
    const std::set<uint64_t>& freeSquareIds() const { return free_square_ids_; }
    const std::set<uint64_t>& freeVectorIds() const { return free_vector_ids_; }
    uint64_t rawNextNodeId() const { return next_node_id_; }
    uint64_t rawNextSquareId() const { return next_square_id_; }
    uint64_t rawNextVectorId() const { return next_vector_id_; }
    uint64_t selectedId() const { return selected_id_; }
    SelKind selectedKind() const { return sel_kind_; }
    void restoreFromSnap(const GraphSnap& s);

private:
    std::vector<Node*> nodes_;
    std::vector<Square*> squares_;
    std::vector<VectorObject*> vectors_;
    std::vector<Edge*> edges_;
    std::unordered_map<uint64_t, Node*> node_map_;
    std::unordered_map<uint64_t, Square*> square_map_;
    std::unordered_map<uint64_t, VectorObject*> vector_map_;
    std::unordered_map<uint64_t, Edge*> edge_map_;

    std::set<uint64_t> free_node_ids_;
    std::set<uint64_t> free_square_ids_;
    std::set<uint64_t> free_vector_ids_;
    uint64_t next_node_id_ = 1;
    uint64_t next_square_id_ = 1;
    uint64_t next_vector_id_ = 1;
    uint64_t next_edge_id_ = 1;
    uint64_t selected_id_  = 0;
    SelKind sel_kind_ = SelKind::None;

    uint32_t def_node_color_   = 0x00C8C8C8;
    uint32_t def_square_color_ = 0x004488CC;
    uint32_t edge_color_       = 0x005A6E8C;

    uint64_t allocNodeId();
    uint64_t allocSquareId();
    uint64_t allocVectorId();
};
