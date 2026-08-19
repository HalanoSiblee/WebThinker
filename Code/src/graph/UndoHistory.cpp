#include "UndoHistory.hpp"

UndoHistory::UndoHistory(Graph* g, size_t limit) : graph_(g), limit_(limit) {}

GraphSnap UndoHistory::capture() const {
    GraphSnap s;
    if (!graph_) return s;
    for (const auto* n : graph_->nodes()) {
        NodeSnap ns;
        ns.id = n->id; ns.x = n->x; ns.y = n->y;
        ns.title = n->title; ns.text = n->text; ns.color = n->color;
        s.nodes.push_back(std::move(ns));
    }
    for (const auto* sq : graph_->squares()) {
        SquareSnap ss;
        ss.id = sq->id; ss.x = sq->x; ss.y = sq->y; ss.w = sq->w; ss.h = sq->h;
        ss.title = sq->title; ss.text = sq->text; ss.z = sq->z; ss.color = sq->color;
        s.squares.push_back(std::move(ss));
    }
    for (const auto* v : graph_->vectors()) {
        VectorSnap vs;
        vs.id = v->id; vs.x = v->x; vs.y = v->y; vs.scale = v->scale;
        vs.title = v->title; vs.svg = v->svg;
        s.vectors.push_back(std::move(vs));
    }
    for (const auto* e : graph_->edges()) {
        EdgeSnap es;
        es.id = e->id; es.from = e->from; es.to = e->to; es.color = e->color;
        s.edges.push_back(es);
    }
    s.free_node_ids = graph_->freeNodeIds();
    s.free_square_ids = graph_->freeSquareIds();
    s.free_vector_ids = graph_->freeVectorIds();
    s.next_node_id = graph_->rawNextNodeId();
    s.next_square_id = graph_->rawNextSquareId();
    s.next_vector_id = graph_->rawNextVectorId();
    s.next_edge_id = graph_->nextEdgeId();
    s.selected_id = graph_->selectedId();
    switch (graph_->selectedKind()) {
        case Graph::SelKind::Node:   s.sel_kind = 1; break;
        case Graph::SelKind::Square: s.sel_kind = 2; break;
        case Graph::SelKind::Vector: s.sel_kind = 3; break;
        default: s.sel_kind = 0; break;
    }
    return s;
}

void UndoHistory::restore(const GraphSnap& s) {
    if (graph_) graph_->restoreFromSnap(s);
}

void UndoHistory::push() {
    if (!graph_) return;
    undo_.push_back(capture());
    if (undo_.size() > limit_) undo_.erase(undo_.begin());
    redo_.clear();
}

bool UndoHistory::canUndo() const { return !undo_.empty(); }
bool UndoHistory::canRedo() const { return !redo_.empty(); }

bool UndoHistory::undo() {
    if (!graph_ || undo_.empty()) return false;
    redo_.push_back(capture());
    GraphSnap s = std::move(undo_.back());
    undo_.pop_back();
    restore(s);
    return true;
}

bool UndoHistory::redo() {
    if (!graph_ || redo_.empty()) return false;
    undo_.push_back(capture());
    GraphSnap s = std::move(redo_.back());
    redo_.pop_back();
    restore(s);
    return true;
}

void UndoHistory::clear() {
    undo_.clear();
    redo_.clear();
}
