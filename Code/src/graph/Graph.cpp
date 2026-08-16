#include "Graph.hpp"
#include "UndoHistory.hpp"
#include <algorithm>
#include <cmath>

uint64_t Graph::allocNodeId() {
    if (!free_node_ids_.empty()) {
        uint64_t id = *free_node_ids_.begin();
        free_node_ids_.erase(free_node_ids_.begin());
        return id;
    }
    return next_node_id_++;
}
uint64_t Graph::allocSquareId() {
    if (!free_square_ids_.empty()) {
        uint64_t id = *free_square_ids_.begin();
        free_square_ids_.erase(free_square_ids_.begin());
        return id;
    }
    return next_square_id_++;
}
uint64_t Graph::allocVectorId() {
    if (!free_vector_ids_.empty()) {
        uint64_t id = *free_vector_ids_.begin();
        free_vector_ids_.erase(free_vector_ids_.begin());
        return id;
    }
    return next_vector_id_++;
}

uint64_t Graph::nextNodeId() const {
    return free_node_ids_.empty() ? next_node_id_ : *free_node_ids_.begin();
}
uint64_t Graph::nextSquareId() const {
    return free_square_ids_.empty() ? next_square_id_ : *free_square_ids_.begin();
}
uint64_t Graph::nextVectorId() const {
    return free_vector_ids_.empty() ? next_vector_id_ : *free_vector_ids_.begin();
}

Node& Graph::addNode(double x, double y, const std::string& title) {
    return addNodeWithId(allocNodeId(), x, y, title);
}
Node& Graph::addNodeWithId(uint64_t id, double x, double y, const std::string& title) {
    free_node_ids_.erase(id);
    auto* n = new Node();
    n->id = id; n->x = x; n->y = y;
    n->title = title.empty() ? ("Node " + std::to_string(id)) : title;
    n->color = def_node_color_;
    nodes_.push_back(n);
    node_map_[id] = n;
    if (id >= next_node_id_) next_node_id_ = id + 1;
    return *n;
}
bool Graph::removeNode(uint64_t id) {
    auto it = node_map_.find(id);
    if (it == node_map_.end()) return false;
    Node* node = it->second;
    edges_.erase(std::remove_if(edges_.begin(), edges_.end(),
        [&](Edge* e) {
            if (e->from == id || e->to == id) {
                edge_map_.erase(e->id); delete e; return true;
            }
            return false;
        }), edges_.end());
    nodes_.erase(std::remove(nodes_.begin(), nodes_.end(), node), nodes_.end());
    node_map_.erase(it); delete node;
    free_node_ids_.insert(id);
    if (sel_kind_ == SelKind::Node && selected_id_ == id) {
        selected_id_ = 0; sel_kind_ = SelKind::None;
    }
    return true;
}
Node* Graph::getNode(uint64_t id) {
    auto it = node_map_.find(id);
    return it != node_map_.end() ? it->second : nullptr;
}
const Node* Graph::getNode(uint64_t id) const {
    auto it = node_map_.find(id);
    return it != node_map_.end() ? it->second : nullptr;
}
std::vector<Node*>& Graph::nodes() { return nodes_; }
const std::vector<Node*>& Graph::nodes() const { return nodes_; }

Square& Graph::addSquare(double x, double y, double w, double h, const std::string& title) {
    return addSquareWithId(allocSquareId(), x, y, w, h, title);
}
Square& Graph::addSquareWithId(uint64_t id, double x, double y, double w, double h, const std::string& title) {
    free_square_ids_.erase(id);
    auto* s = new Square();
    s->id = id; s->x = x; s->y = y;
    s->w = w > 0.05 ? w : 2.0; s->h = h > 0.05 ? h : 2.0;
    s->title = title.empty() ? ("Square " + std::to_string(id)) : title;
    s->color = def_square_color_;
    squares_.push_back(s);
    square_map_[id] = s;
    if (id >= next_square_id_) next_square_id_ = id + 1;
    return *s;
}
bool Graph::removeSquare(uint64_t id) {
    auto it = square_map_.find(id);
    if (it == square_map_.end()) return false;
    Square* sq = it->second;
    squares_.erase(std::remove(squares_.begin(), squares_.end(), sq), squares_.end());
    square_map_.erase(it); delete sq;
    free_square_ids_.insert(id);
    if (sel_kind_ == SelKind::Square && selected_id_ == id) {
        selected_id_ = 0; sel_kind_ = SelKind::None;
    }
    return true;
}
Square* Graph::getSquare(uint64_t id) {
    auto it = square_map_.find(id);
    return it != square_map_.end() ? it->second : nullptr;
}
const Square* Graph::getSquare(uint64_t id) const {
    auto it = square_map_.find(id);
    return it != square_map_.end() ? it->second : nullptr;
}
std::vector<Square*>& Graph::squares() { return squares_; }
const std::vector<Square*>& Graph::squares() const { return squares_; }

VectorObject& Graph::addVector(double x, double y, const std::string& svg,
                               const std::string& title, float scale) {
    return addVectorWithId(allocVectorId(), x, y, scale, title, svg);
}
VectorObject& Graph::addVectorWithId(uint64_t id, double x, double y, float scale,
                                    const std::string& title, const std::string& svg) {
    free_vector_ids_.erase(id);
    auto* v = new VectorObject();
    v->id = id; v->x = x; v->y = y;
    v->scale = scale > 0.01f ? scale : 1.0f;
    v->title = title.empty() ? ("SVG " + std::to_string(id)) : title;
    v->svg = svg;
    vectors_.push_back(v);
    vector_map_[id] = v;
    if (id >= next_vector_id_) next_vector_id_ = id + 1;
    return *v;
}
bool Graph::removeVector(uint64_t id) {
    auto it = vector_map_.find(id);
    if (it == vector_map_.end()) return false;
    VectorObject* v = it->second;
    vectors_.erase(std::remove(vectors_.begin(), vectors_.end(), v), vectors_.end());
    vector_map_.erase(it); delete v;
    free_vector_ids_.insert(id);
    if (sel_kind_ == SelKind::Vector && selected_id_ == id) {
        selected_id_ = 0; sel_kind_ = SelKind::None;
    }
    return true;
}
VectorObject* Graph::getVector(uint64_t id) {
    auto it = vector_map_.find(id);
    return it != vector_map_.end() ? it->second : nullptr;
}
const VectorObject* Graph::getVector(uint64_t id) const {
    auto it = vector_map_.find(id);
    return it != vector_map_.end() ? it->second : nullptr;
}
std::vector<VectorObject*>& Graph::vectors() { return vectors_; }
const std::vector<VectorObject*>& Graph::vectors() const { return vectors_; }

Edge* Graph::addEdge(uint64_t from, uint64_t to) {
    if (from == to) return nullptr;
    if (!getNode(from) || !getNode(to)) return nullptr;
    for (auto* e : edges_) {
        if ((e->from == from && e->to == to) || (e->from == to && e->to == from))
            return e;
    }
    auto* e = new Edge();
    e->id = next_edge_id_++;
    e->from = from; e->to = to;
    e->color = edge_color_;
    edges_.push_back(e);
    edge_map_[e->id] = e;
    return e;
}
bool Graph::removeEdge(uint64_t edgeId) {
    auto it = edge_map_.find(edgeId);
    if (it == edge_map_.end()) return false;
    Edge* e = it->second;
    edge_map_.erase(it);
    edges_.erase(std::remove(edges_.begin(), edges_.end(), e), edges_.end());
    delete e;
    return true;
}
bool Graph::removeEdgeBetween(uint64_t from, uint64_t to) {
    for (auto* e : edges_) {
        if ((e->from == from && e->to == to) || (e->from == to && e->to == from))
            return removeEdge(e->id);
    }
    return false;
}
Edge* Graph::getEdge(uint64_t id) {
    auto it = edge_map_.find(id);
    return it != edge_map_.end() ? it->second : nullptr;
}
std::vector<Edge*>& Graph::edges() { return edges_; }
const std::vector<Edge*>& Graph::edges() const { return edges_; }

void Graph::clearSelection() {
    for (auto* n : nodes_) n->selected = false;
    for (auto* s : squares_) s->selected = false;
    for (auto* v : vectors_) v->selected = false;
    selected_id_ = 0;
    sel_kind_ = SelKind::None;
}

void Graph::selectNode(uint64_t id, bool additive) {
    if (!additive) clearSelection();
    if (auto* n = getNode(id)) {
        n->selected = true;
        selected_id_ = id;
        sel_kind_ = SelKind::Node;
    }
}
void Graph::selectSquare(uint64_t id, bool additive) {
    if (!additive) clearSelection();
    if (auto* s = getSquare(id)) {
        s->selected = true;
        selected_id_ = id;
        sel_kind_ = SelKind::Square;
    }
}
void Graph::selectVector(uint64_t id, bool additive) {
    if (!additive) clearSelection();
    if (auto* v = getVector(id)) {
        v->selected = true;
        selected_id_ = id;
        sel_kind_ = SelKind::Vector;
    }
}

void Graph::selectInRect(double x0, double y0, double x1, double y1) {
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    clearSelection();
    uint64_t last = 0;
    SelKind kind = SelKind::None;
    for (auto* n : nodes_) {
        if (n->x >= x0 && n->x <= x1 && n->y >= y0 && n->y <= y1) {
            n->selected = true; last = n->id; kind = SelKind::Node;
        }
    }
    for (auto* s : squares_) {
        // any overlap with square AABB
        double hw = s->w * 0.5, hh = s->h * 0.5;
        if (s->x + hw >= x0 && s->x - hw <= x1 && s->y + hh >= y0 && s->y - hh <= y1) {
            s->selected = true; last = s->id; kind = SelKind::Square;
        }
    }
    for (auto* v : vectors_) {
        double h = VectorObject::kBaseHalf * v->scale;
        if (v->x + h >= x0 && v->x - h <= x1 && v->y + h >= y0 && v->y - h <= y1) {
            v->selected = true; last = v->id; kind = SelKind::Vector;
        }
    }
    selected_id_ = last;
    sel_kind_ = kind;
}

Node* Graph::selectedNode() {
    return (sel_kind_ == SelKind::Node) ? getNode(selected_id_) : nullptr;
}
const Node* Graph::selectedNode() const {
    return (sel_kind_ == SelKind::Node) ? getNode(selected_id_) : nullptr;
}
Square* Graph::selectedSquare() {
    return (sel_kind_ == SelKind::Square) ? getSquare(selected_id_) : nullptr;
}
const Square* Graph::selectedSquare() const {
    return (sel_kind_ == SelKind::Square) ? getSquare(selected_id_) : nullptr;
}
VectorObject* Graph::selectedVector() {
    return (sel_kind_ == SelKind::Vector) ? getVector(selected_id_) : nullptr;
}
const VectorObject* Graph::selectedVector() const {
    return (sel_kind_ == SelKind::Vector) ? getVector(selected_id_) : nullptr;
}

int Graph::selectionCount() const {
    int c = 0;
    for (auto* n : nodes_) if (n->selected) ++c;
    for (auto* s : squares_) if (s->selected) ++c;
    for (auto* v : vectors_) if (v->selected) ++c;
    return c;
}

void Graph::deleteSelection() {
    std::vector<uint64_t> nids, sids, vids;
    for (auto* n : nodes_) if (n->selected) nids.push_back(n->id);
    for (auto* s : squares_) if (s->selected) sids.push_back(s->id);
    for (auto* v : vectors_) if (v->selected) vids.push_back(v->id);
    for (auto id : nids) removeNode(id);
    for (auto id : sids) removeSquare(id);
    for (auto id : vids) removeVector(id);
    clearSelection();
}

void Graph::moveSelection(double dx, double dy) {
    for (auto* n : nodes_) if (n->selected) { n->x += dx; n->y += dy; }
    for (auto* s : squares_) if (s->selected) { s->x += dx; s->y += dy; }
    for (auto* v : vectors_) if (v->selected) { v->x += dx; v->y += dy; }
}

void Graph::colorSelection(uint32_t color) {
    for (auto* n : nodes_) if (n->selected) n->color = color;
    for (auto* s : squares_) if (s->selected) s->color = color;
}

void Graph::clear() {
    for (auto* n : nodes_) delete n;
    for (auto* s : squares_) delete s;
    for (auto* v : vectors_) delete v;
    for (auto* e : edges_) delete e;
    nodes_.clear(); squares_.clear(); vectors_.clear(); edges_.clear();
    node_map_.clear(); square_map_.clear(); vector_map_.clear(); edge_map_.clear();
    free_node_ids_.clear(); free_square_ids_.clear(); free_vector_ids_.clear();
    next_node_id_ = next_square_id_ = next_vector_id_ = next_edge_id_ = 1;
    selected_id_ = 0; sel_kind_ = SelKind::None;
}

void Graph::recolorAllEdges(uint32_t c) {
    edge_color_ = c;
    for (auto* e : edges_) e->color = c;
}

void Graph::restoreFromSnap(const GraphSnap& s) {
    for (auto* n : nodes_) delete n;
    for (auto* sq : squares_) delete sq;
    for (auto* v : vectors_) delete v;
    for (auto* e : edges_) delete e;
    nodes_.clear(); squares_.clear(); vectors_.clear(); edges_.clear();
    node_map_.clear(); square_map_.clear(); vector_map_.clear(); edge_map_.clear();
    free_node_ids_.clear(); free_square_ids_.clear(); free_vector_ids_.clear();

    free_node_ids_ = s.free_node_ids;
    free_square_ids_ = s.free_square_ids;
    free_vector_ids_ = s.free_vector_ids;
    next_node_id_ = s.next_node_id;
    next_square_id_ = s.next_square_id;
    next_vector_id_ = s.next_vector_id;
    next_edge_id_ = s.next_edge_id;
    selected_id_ = 0; sel_kind_ = SelKind::None;

    for (const auto& ns : s.nodes) {
        auto* n = new Node();
        n->id = ns.id; n->x = ns.x; n->y = ns.y;
        n->title = ns.title; n->text = ns.text; n->color = ns.color;
        nodes_.push_back(n); node_map_[n->id] = n;
    }
    for (const auto& ss : s.squares) {
        auto* sq = new Square();
        sq->id = ss.id; sq->x = ss.x; sq->y = ss.y; sq->w = ss.w; sq->h = ss.h;
        sq->title = ss.title; sq->text = ss.text; sq->color = ss.color;
        squares_.push_back(sq); square_map_[sq->id] = sq;
    }
    for (const auto& vs : s.vectors) {
        auto* v = new VectorObject();
        v->id = vs.id; v->x = vs.x; v->y = vs.y; v->scale = vs.scale;
        v->title = vs.title; v->svg = vs.svg;
        vectors_.push_back(v); vector_map_[v->id] = v;
    }
    for (const auto& es : s.edges) {
        auto* e = new Edge();
        e->id = es.id; e->from = es.from; e->to = es.to; e->color = es.color;
        edges_.push_back(e); edge_map_[e->id] = e;
    }
    if (s.sel_kind == 1 && s.selected_id && getNode(s.selected_id)) {
        selectNode(s.selected_id);
    } else if (s.sel_kind == 2 && s.selected_id && getSquare(s.selected_id)) {
        selectSquare(s.selected_id);
    } else if (s.sel_kind == 3 && s.selected_id && getVector(s.selected_id)) {
        selectVector(s.selected_id);
    }
}
