#include "RightPanel.hpp"
#include "../graph/UndoHistory.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include <sstream>
#include <cstdio>

static const Fl_Color P_PANEL  = fl_rgb_color(12, 12, 12);
static const Fl_Color P_INPUT  = fl_rgb_color(20, 20, 20);
static const Fl_Color P_LABEL  = fl_rgb_color(120, 120, 120);
static const Fl_Color P_TEXT   = fl_rgb_color(210, 210, 210);
static const Fl_Color P_BTN    = fl_rgb_color(40, 40, 40);
static const Fl_Color P_ACCENT = fl_rgb_color(0, 0, 0);

RightPanel::RightPanel(int x, int y, int w, int h, Graph* graph)
    : Fl_Group(x, y, w, h), graph_(graph)
{
    box(FL_FLAT_BOX);
    color(P_PANEL);
    buildUI();
    end();
}

void RightPanel::buildUI() {
    const int pad = 10;
    const int row_h = 28;
    int cy = pad;

    const int content_w = w();
    content_ = new Fl_Group(x(), y(), content_w, h());
    content_->box(FL_FLAT_BOX);
    content_->color(P_PANEL);

    auto makeLabel = [&](const char* txt) -> Fl_Box* {
        auto* b = new Fl_Box(x() + pad, y() + cy, content_w - 2 * pad, 18, txt);
        b->labelsize(11);
        b->labelfont(FL_HELVETICA);
        b->labelcolor(P_LABEL);
        b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        cy += 20;
        return b;
    };

    auto styleInput = [&](Fl_Input* inp) {
        inp->color(P_INPUT);
        inp->textcolor(P_TEXT);
        inp->textsize(13);
        inp->box(FL_FLAT_BOX);
    };

    title_label_ = makeLabel("Title");
    title_input_ = new Fl_Input(x() + pad, y() + cy, content_w - 2 * pad, row_h);
    styleInput(title_input_);
    cy += row_h + 8;

    id_label_ = makeLabel("ID");
    id_value_ = new Fl_Box(x() + pad, y() + cy, content_w - 2 * pad, row_h, "—");
    id_value_->box(FL_FLAT_BOX);
    id_value_->color(P_INPUT);
    id_value_->labelcolor(P_TEXT);
    id_value_->labelsize(13);
    id_value_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    cy += row_h + 8;

    coord_label_ = makeLabel("Coordinates (X, Y)");
    int half = (content_w - 2 * pad - 8) / 2;
    x_input_ = new Fl_Input(x() + pad, y() + cy, half, row_h);
    y_input_ = new Fl_Input(x() + pad + half + 8, y() + cy, half, row_h);
    styleInput(x_input_);
    styleInput(y_input_);
    cy += row_h + 8;

    size_label_ = makeLabel("Size (W, H) — squares");
    w_input_ = new Fl_Input(x() + pad, y() + cy, half, row_h);
    h_input_ = new Fl_Input(x() + pad + half + 8, y() + cy, half, row_h);
    styleInput(w_input_);
    styleInput(h_input_);
    cy += row_h + 8;

    color_label_ = makeLabel("Color (0x00RRGGBB)");
    color_btn_ = new Fl_Button(x() + pad, y() + cy, 80, row_h, "Pick");
    color_btn_->color(P_BTN);
    color_btn_->labelcolor(P_TEXT);
    color_btn_->callback(onColor, this);
    cy += row_h + 12;

    text_label_ = makeLabel("Core Text");
    text_input_ = new Fl_Multiline_Input(x() + pad, y() + cy, content_w - 2 * pad, 140);
    text_input_->color(P_INPUT);
    text_input_->textcolor(P_TEXT);
    text_input_->textsize(13);
    text_input_->box(FL_FLAT_BOX);
    text_input_->wrap(1);
    cy += 148;

    conn_label_ = makeLabel("Connected To");
    conn_box_ = new Fl_Box(x() + pad, y() + cy, content_w - 2 * pad, 60, "(none)");
    conn_box_->box(FL_FLAT_BOX);
    conn_box_->color(P_INPUT);
    conn_box_->labelcolor(P_TEXT);
    conn_box_->labelsize(12);
    conn_box_->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    cy += 70;

    update_btn_ = new Fl_Button(x() + pad, y() + cy, content_w - 2 * pad, 36, "Update");
    update_btn_->color(P_ACCENT);
    update_btn_->labelcolor(fl_rgb_color(240, 240, 240));
    update_btn_->labelfont(FL_HELVETICA_BOLD);
    update_btn_->labelsize(13);
    update_btn_->callback(onUpdate, this);
    cy += 50;

    def_label_ = makeLabel("Defaults (new items / link color)");
    int bw = (content_w - 2 * pad - 16) / 3;
    def_node_btn_ = new Fl_Button(x() + pad, y() + cy, bw, row_h, "Node");
    def_square_btn_ = new Fl_Button(x() + pad + bw + 8, y() + cy, bw, row_h, "Square");
    def_edge_btn_ = new Fl_Button(x() + pad + 2 * (bw + 8), y() + cy, bw, row_h, "Line");
    for (auto* b : {def_node_btn_, def_square_btn_, def_edge_btn_}) {
        b->labelcolor(fl_rgb_color(240, 240, 240));
        b->labelsize(11);
    }
    def_node_btn_->callback(onDefNode, this);
    def_square_btn_->callback(onDefSquare, this);
    def_edge_btn_->callback(onDefEdge, this);
    cy += row_h + 12;
    syncDefaultButtons();

    content_->size(content_w, cy + pad);
    content_->end();
}

void RightPanel::refreshFromSelection() {
    syncDefaultButtons();
    if (!graph_) return;

    Node* n = graph_->selectedNode();
    Square* sq = graph_->selectedSquare();
    VectorObject* vo = graph_->selectedVector();

    if (!n && !sq && !vo) {
        title_input_->value("");
        id_value_->label("—");
        x_input_->value("");
        y_input_->value("");
        w_input_->value("");
        h_input_->value("");
        text_input_->value("");
        conn_box_->label("(none)");
        color_btn_->color(P_BTN);
        redraw();
        return;
    }

    if (vo) {
        title_input_->value(vo->title.c_str());
        static char idbuf[48];
        snprintf(idbuf, sizeof(idbuf), "V%llu", (unsigned long long)vo->id);
        id_value_->label(idbuf);
        static char xbuf[32], ybuf[32], sbuf[32];
        snprintf(xbuf, sizeof(xbuf), "%.3f", vo->x);
        snprintf(ybuf, sizeof(ybuf), "%.3f", vo->y);
        snprintf(sbuf, sizeof(sbuf), "%.3f", vo->scale);
        x_input_->value(xbuf);
        y_input_->value(ybuf);
        w_input_->value(sbuf);
        h_input_->value("");
        text_input_->value(vo->svg.c_str());
        conn_box_->label("(svg vector)");
        redraw();
        return;
    }

    if (sq) {
        title_input_->value(sq->title.c_str());
        static char idbuf[48];
        snprintf(idbuf, sizeof(idbuf), "S%llu  z=%d", (unsigned long long)sq->id, sq->z);
        id_value_->label(idbuf);

        static char xbuf[32], ybuf[32], wbuf[32], hbuf[32];
        snprintf(xbuf, sizeof(xbuf), "%.3f", sq->x);
        snprintf(ybuf, sizeof(ybuf), "%.3f", sq->y);
        snprintf(wbuf, sizeof(wbuf), "%.3f", sq->w);
        snprintf(hbuf, sizeof(hbuf), "%.3f", sq->h);
        x_input_->value(xbuf);
        y_input_->value(ybuf);
        w_input_->value(wbuf);
        h_input_->value(hbuf);

        text_input_->value(sq->text.c_str());
        color_btn_->color(fl_rgb_color(sq->r(), sq->g(), sq->b()));
        static char zinfo[48];
        snprintf(zinfo, sizeof(zinfo), "z=%d (-10..10, F4)", sq->z);
        conn_box_->label(zinfo);
        redraw();
        return;
    }

    title_input_->value(n->title.c_str());
    static char idbuf[32];
    snprintf(idbuf, sizeof(idbuf), "%llu", (unsigned long long)n->id);
    id_value_->label(idbuf);

    static char xbuf[32], ybuf[32];
    snprintf(xbuf, sizeof(xbuf), "%.3f", n->x);
    snprintf(ybuf, sizeof(ybuf), "%.3f", n->y);
    x_input_->value(xbuf);
    y_input_->value(ybuf);
    w_input_->value("");
    h_input_->value("");

    text_input_->value(n->text.c_str());
    color_btn_->color(fl_rgb_color(n->r(), n->g(), n->b()));

    std::ostringstream oss;
    bool first = true;
    for (auto* e : graph_->edges()) {
        uint64_t other = 0;
        if (e->from == n->id) other = e->to;
        else if (e->to == n->id) other = e->from;
        if (other) {
            if (auto* on = graph_->getNode(other)) {
                if (!first) oss << ", ";
                oss << on->title << " (" << on->id << ")";
                first = false;
            }
        }
    }
    if (first) oss << "(none)";
    static std::string conn_str;
    conn_str = oss.str();
    conn_box_->label(conn_str.c_str());
    redraw();
}

void RightPanel::applyToSelection() {
    if (!graph_) return;

    if (VectorObject* vo = graph_->selectedVector()) {
        if (undo_) undo_->push();
        if (dirty_cb_) dirty_cb_(dirty_data_);
        vo->title = title_input_->value();
        vo->svg = text_input_->value();  // Core Text = SVG markup
        try {
            vo->x = std::stod(x_input_->value());
            vo->y = std::stod(y_input_->value());
            float sc = static_cast<float>(std::stof(w_input_->value()));
            if (sc > 0.01f) vo->scale = sc;
        } catch (...) {}
        if (window()) window()->redraw();
        return;
    }

    if (Square* sq = graph_->selectedSquare()) {
        if (undo_) undo_->push();
        if (dirty_cb_) dirty_cb_(dirty_data_);
        sq->title = title_input_->value();
        sq->text  = text_input_->value();
        try {
            sq->x = std::stod(x_input_->value());
            sq->y = std::stod(y_input_->value());
            double nw = std::stod(w_input_->value());
            double nh = std::stod(h_input_->value());
            if (nw > 0.05) sq->w = nw;
            if (nh > 0.05) sq->h = nh;
        } catch (...) {}
        return;
    }

    Node* n = graph_->selectedNode();
    if (!n) return;
    if (undo_) undo_->push();
    if (dirty_cb_) dirty_cb_(dirty_data_);
    n->title = title_input_->value();
    n->text  = text_input_->value();
    try {
        n->x = std::stod(x_input_->value());
        n->y = std::stod(y_input_->value());
    } catch (...) {}
}

void RightPanel::onUpdate(Fl_Widget*, void* data) {
    auto* self = static_cast<RightPanel*>(data);
    self->applyToSelection();
    if (self->parent()) self->parent()->redraw();
}

void RightPanel::onColor(Fl_Widget*, void* data) {
    auto* self = static_cast<RightPanel*>(data);
    if (!self->graph_) return;

    if (Square* sq = self->graph_->selectedSquare()) {
        uchar r = sq->r(), g = sq->g(), b = sq->b();
        if (fl_color_chooser("Square Border Color", r, g, b)) {
            if (self->undo_) self->undo_->push();
            if (self->dirty_cb_) self->dirty_cb_(self->dirty_data_);
            sq->setRGB(r, g, b);
            self->color_btn_->color(fl_rgb_color(r, g, b));
            self->color_btn_->redraw();
            if (self->parent()) self->parent()->redraw();
        }
        return;
    }

    Node* n = self->graph_->selectedNode();
    if (!n) return;
    uchar r = n->r(), g = n->g(), b = n->b();
    if (fl_color_chooser("Node Color", r, g, b)) {
        if (self->undo_) self->undo_->push();
        if (self->dirty_cb_) self->dirty_cb_(self->dirty_data_);
        n->setRGB(r, g, b);
        self->color_btn_->color(fl_rgb_color(r, g, b));
        self->color_btn_->redraw();
        if (self->parent()) self->parent()->redraw();
    }
}

void RightPanel::syncDefaultButtons() {
    if (!graph_ || !def_node_btn_) return;
    auto setBtn = [](Fl_Button* b, uint32_t c) {
        uchar r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, bl = c & 0xFF;
        b->color(fl_rgb_color(r, g, bl));
        int lum = (r * 3 + g * 6 + bl) / 10;
        b->labelcolor(lum > 140 ? fl_rgb_color(20, 20, 20) : fl_rgb_color(240, 240, 240));
        b->redraw();
    };
    setBtn(def_node_btn_, graph_->defaultNodeColor());
    setBtn(def_square_btn_, graph_->defaultSquareColor());
    setBtn(def_edge_btn_, graph_->edgeColor());
}

void RightPanel::onDefNode(Fl_Widget*, void* data) {
    auto* self = static_cast<RightPanel*>(data);
    if (!self->graph_) return;
    uint32_t c = self->graph_->defaultNodeColor();
    uchar r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
    if (fl_color_chooser("Default Node Color", r, g, b)) {
        self->graph_->setDefaultNodeColor((uint32_t(r) << 16) | (uint32_t(g) << 8) | b);
        self->syncDefaultButtons();
        if (self->dirty_cb_) self->dirty_cb_(self->dirty_data_);
    }
}

void RightPanel::onDefSquare(Fl_Widget*, void* data) {
    auto* self = static_cast<RightPanel*>(data);
    if (!self->graph_) return;
    uint32_t c = self->graph_->defaultSquareColor();
    uchar r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
    if (fl_color_chooser("Default Square Color", r, g, b)) {
        self->graph_->setDefaultSquareColor((uint32_t(r) << 16) | (uint32_t(g) << 8) | b);
        self->syncDefaultButtons();
        if (self->dirty_cb_) self->dirty_cb_(self->dirty_data_);
    }
}

void RightPanel::onDefEdge(Fl_Widget*, void* data) {
    auto* self = static_cast<RightPanel*>(data);
    if (!self->graph_) return;
    uint32_t c = self->graph_->edgeColor();
    uchar r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
    if (fl_color_chooser("Link / Line Color", r, g, b)) {
        uint32_t packed = (uint32_t(r) << 16) | (uint32_t(g) << 8) | b;
        self->graph_->setEdgeColor(packed);  // only affects newly created links
        self->syncDefaultButtons();
        if (self->dirty_cb_) self->dirty_cb_(self->dirty_data_);
        // Redraw whole window so GraphView picks up new line colors
        if (self->window()) self->window()->redraw();
    }
}
