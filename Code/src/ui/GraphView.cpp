#include "GraphView.hpp"
#include "../graph/UndoHistory.hpp"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/fl_draw.H>
#include <FL/Fl_SVG_Image.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Return_Button.H>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <cstdlib>

// True OLED black
static const Fl_Color OLED_BG      = fl_rgb_color(0, 0, 0);
static const Fl_Color OLED_GRID    = fl_rgb_color(18, 18, 18);
static const Fl_Color OLED_AXIS    = fl_rgb_color(55, 55, 55);
static const Fl_Color OLED_LABEL   = fl_rgb_color(90, 90, 90);
static const Fl_Color OLED_ORIGIN  = fl_rgb_color(140, 140, 140);
static const Fl_Color OLED_EDGE    = fl_rgb_color(70, 90, 120);
static const Fl_Color OLED_TITLE   = fl_rgb_color(200, 200, 200);
static const Fl_Color OLED_SELECT  = fl_rgb_color(255, 255, 255);
static const Fl_Color OLED_LINK    = fl_rgb_color(0, 220, 160);
static const Fl_Color OLED_HINT    = fl_rgb_color(80, 80, 80);

GraphView::GraphView(int x, int y, int w, int h, Graph* graph)
    : Fl_Widget(x, y, w, h), graph_(graph)
{
    box(FL_FLAT_BOX);
    color(OLED_BG);
}

void GraphView::pushUndo() {
    if (undo_) undo_->push();
    if (dirty_cb_) dirty_cb_(dirty_data_);
}


void GraphView::resetView() {
    cam_x_ = 0.0;
    cam_y_ = 0.0;
    scale_ = 40.0;
    link_from_ = 0;
    mode_ = Select;
    redraw();
}

void GraphView::pan(double dx, double dy) {
    // dx/dy in screen pixels → convert to world
    cam_x_ -= dx / scale_;
    cam_y_ += dy / scale_;   // Y is up
    redraw();
}

void GraphView::zoom(double factor, int mx, int my) {
    double wx, wy;
    screenToWorld(mx, my, wx, wy);

    scale_ *= factor;
    scale_ = std::clamp(scale_, 4.0, 500.0);

    // Keep the world point under the mouse fixed
    double nx, ny;
    screenToWorld(mx, my, nx, ny);
    cam_x_ += (wx - nx);
    cam_y_ += (wy - ny);

    redraw();
}

void GraphView::worldToScreen(double wx, double wy, int& sx, int& sy) const {
    sx = static_cast<int>(std::lround(x() + w() * 0.5 + (wx - cam_x_) * scale_));
    sy = static_cast<int>(std::lround(y() + h() * 0.5 - (wy - cam_y_) * scale_));
}

void GraphView::screenToWorld(int sx, int sy, double& wx, double& wy) const {
    wx = cam_x_ + (sx - x() - w() * 0.5) / scale_;
    wy = cam_y_ - (sy - y() - h() * 0.5) / scale_;
}

Node* GraphView::hitTestNode(int sx, int sy, double radius) {
    if (!graph_) return nullptr;
    double best_d2 = radius * radius;
    Node* best = nullptr;

    for (auto* n : graph_->nodes()) {
        int nsx, nsy;
        worldToScreen(n->x, n->y, nsx, nsy);
        double dx = sx - nsx;
        double dy = sy - nsy;
        double d2 = dx * dx + dy * dy;
        if (d2 < best_d2) {
            best_d2 = d2;
            best = n;
        }
    }
    return best;
}


Square* GraphView::hitTestSquare(int sx, int sy) {
    if (!graph_) return nullptr;
    // topmost = last in list that contains point
    Square* best = nullptr;
    for (auto* s : graph_->squares()) {
        int cx, cy;
        worldToScreen(s->x, s->y, cx, cy);
        int hw = static_cast<int>(std::lround((s->w * 0.5) * scale_));
        int hh = static_cast<int>(std::lround((s->h * 0.5) * scale_));
        if (hw < 4) hw = 4;
        if (hh < 4) hh = 4;
        if (sx >= cx - hw && sx <= cx + hw && sy >= cy - hh && sy <= cy + hh)
            best = s;
    }
    return best;
}


VectorObject* GraphView::hitTestVector(int sx, int sy) {
    if (!graph_) return nullptr;
    VectorObject* best = nullptr;
    for (auto* v : graph_->vectors()) {
        int cx, cy;
        worldToScreen(v->x, v->y, cx, cy);
        int half = static_cast<int>(std::lround(VectorObject::kBaseHalf * v->scale * scale_));
        if (half < 8) half = 8;
        if (sx >= cx - half && sx <= cx + half && sy >= cy - half && sy <= cy + half)
            best = v;
    }
    return best;
}

Edge* GraphView::hitTestEdge(int sx, int sy, double threshold) {
    if (!graph_) return nullptr;
    double best = threshold;
    Edge* best_e = nullptr;

    for (auto* e : graph_->edges()) {
        auto* a = graph_->getNode(e->from);
        auto* b = graph_->getNode(e->to);
        if (!a || !b) continue;

        int ax, ay, bx, by;
        worldToScreen(a->x, a->y, ax, ay);
        worldToScreen(b->x, b->y, bx, by);

        double dx = bx - ax;
        double dy = by - ay;
        double len2 = dx * dx + dy * dy;
        if (len2 < 1e-6) continue;

        double t = ((sx - ax) * dx + (sy - ay) * dy) / len2;
        t = std::clamp(t, 0.0, 1.0);
        double px = ax + t * dx;
        double py = ay + t * dy;
        double dist = std::hypot(sx - px, sy - py);

        if (dist < best) {
            best = dist;
            best_e = e;
        }
    }
    return best_e;
}

void GraphView::drawAxes() {
    // Adaptive grid step so lines stay ~40–70 px apart
    double step = 1.0;
    while (step * scale_ < 35.0) step *= 2.0;
    while (step * scale_ > 75.0) step *= 0.5;

    const double half_w = w() * 0.5 / scale_;
    const double half_h = h() * 0.5 / scale_;

    double min_x = cam_x_ - half_w;
    double max_x = cam_x_ + half_w;
    double min_y = cam_y_ - half_h;
    double max_y = cam_y_ + half_h;

    double start_x = std::floor(min_x / step) * step;
    double start_y = std::floor(min_y / step) * step;

    // Grid
    fl_color(OLED_GRID);
    fl_line_style(FL_SOLID, 1);
    for (double gx = start_x; gx <= max_x + step * 0.5; gx += step) {
        int sx, sy;
        worldToScreen(gx, 0, sx, sy);
        if (sx >= x() && sx <= x() + w())
            fl_line(sx, y(), sx, y() + h());
    }
    for (double gy = start_y; gy <= max_y + step * 0.5; gy += step) {
        int sx, sy;
        worldToScreen(0, gy, sx, sy);
        if (sy >= y() && sy <= y() + h())
            fl_line(x(), sy, x() + w(), sy);
    }

    // Main axes (through world origin)
    int ox, oy;
    worldToScreen(0, 0, ox, oy);

    fl_color(OLED_AXIS);
    fl_line_style(FL_SOLID, 2);
    if (oy >= y() && oy <= y() + h())
        fl_line(x(), oy, x() + w(), oy);          // X axis
    if (ox >= x() && ox <= x() + w())
        fl_line(ox, y(), ox, y() + h());          // Y axis
    fl_line_style(0);

    // Numbers
    fl_font(FL_HELVETICA, 11);
    fl_color(OLED_LABEL);
    char buf[32];

    for (double gx = start_x; gx <= max_x + step * 0.5; gx += step) {
        if (std::abs(gx) < step * 0.01) continue;
        int sx, sy;
        worldToScreen(gx, 0, sx, sy);
        if (sx < x() + 4 || sx > x() + w() - 20) continue;
        snprintf(buf, sizeof(buf), "%g", gx);
        int ty = (oy >= y() && oy <= y() + h() - 14) ? oy + 14 : y() + h() - 4;
        fl_draw(buf, sx - 6, ty);
    }
    for (double gy = start_y; gy <= max_y + step * 0.5; gy += step) {
        if (std::abs(gy) < step * 0.01) continue;
        int sx, sy;
        worldToScreen(0, gy, sx, sy);
        if (sy < y() + 12 || sy > y() + h() - 4) continue;
        snprintf(buf, sizeof(buf), "%g", gy);
        int tx = (ox >= x() && ox <= x() + w() - 30) ? ox + 6 : x() + 4;
        fl_draw(buf, tx, sy + 4);
    }

    // Origin
    if (ox >= x() && ox <= x() + w() && oy >= y() && oy <= y() + h()) {
        fl_color(OLED_ORIGIN);
        fl_draw("0", ox + 4, oy + 14);
    }
}

void GraphView::drawEdges() {
    if (!graph_) return;
    fl_line_style(FL_SOLID, 2);

    for (auto* e : graph_->edges()) {
        auto* a = graph_->getNode(e->from);
        auto* b = graph_->getNode(e->to);
        if (!a || !b) continue;

        fl_color(fl_rgb_color(e->r(), e->g(), e->b()));

        int ax, ay, bx, by;
        worldToScreen(a->x, a->y, ax, ay);
        worldToScreen(b->x, b->y, bx, by);
        fl_line(ax, ay, bx, by);
    }
    fl_line_style(0);
}

void GraphView::drawLinkPreview() {
    if (link_from_ == 0 || !graph_) return;
    auto* n = graph_->getNode(link_from_);
    if (!n) return;

    int sx, sy;
    worldToScreen(n->x, n->y, sx, sy);

    fl_color(OLED_LINK);
    fl_line_style(FL_DASH, 2);
    fl_line(sx, sy, link_mx_, link_my_);
    fl_line_style(0);

    // highlight source node
    fl_color(OLED_LINK);
    fl_circle(sx, sy, 12);
}


void GraphView::drawSquares() {
    if (!graph_) return;
    fl_font(FL_HELVETICA, 12);

    std::vector<Square*> ordered = graph_->squares();
    std::sort(ordered.begin(), ordered.end(),
              [](const Square* a, const Square* b) { return a->z < b->z; });

    for (auto* s : ordered) {
        int cx, cy;
        worldToScreen(s->x, s->y, cx, cy);
        int hw = static_cast<int>(std::lround((s->w * 0.5) * scale_));
        int hh = static_cast<int>(std::lround((s->h * 0.5) * scale_));
        if (hw < 4) hw = 4;
        if (hh < 4) hh = 4;

        uint8_t dr, dg, db;
        s->dimmedRGB(dr, dg, db);
        fl_color(fl_rgb_color(dr, dg, db));
        fl_rectf(cx - hw, cy - hh, 2 * hw, 2 * hh);

        fl_color(fl_rgb_color(s->r(), s->g(), s->b()));
        fl_line_style(FL_SOLID, s->selected ? 3 : 2);
        fl_rect(cx - hw, cy - hh, 2 * hw, 2 * hh);
        fl_line_style(0);

        if (s->selected) {
            fl_color(OLED_SELECT);
            fl_line_style(FL_SOLID, 1);
            fl_rect(cx - hw - 3, cy - hh - 3, 2 * hw + 6, 2 * hh + 6);
            fl_line_style(0);
        }

        fl_color(OLED_TITLE);
        int tw = 0, th = 0;
        fl_measure(s->title.c_str(), tw, th);
        fl_draw(s->title.c_str(), cx - tw / 2, cy - hh - 6);
    }
}


void GraphView::drawVectors() {
    if (!graph_) return;
    fl_font(FL_HELVETICA, 12);

    for (auto* v : graph_->vectors()) {
        int cx, cy;
        worldToScreen(v->x, v->y, cx, cy);
        int half = static_cast<int>(std::lround(VectorObject::kBaseHalf * static_cast<double>(v->scale) * scale_));
        if (half < 12) half = 12;
        int dim = half * 2;
        int dx = cx - half;
        int dy = cy - half;

        bool drawn = false;
        if (!v->svg.empty()) {
            Fl_SVG_Image img(nullptr, v->svg.c_str());
            if (!img.fail() && img.data_w() > 0 && img.data_h() > 0) {
                // FLTK: must scale() before draw — draw(X,Y,W,H) alone leaves SVG at native size (top-left)
                // proportional=1 keeps aspect; can_expand=1 allows upscale to fit the box
                img.scale(dim, dim, /*proportional=*/1, /*can_expand=*/1);
                const int dw = img.w();
                const int dh = img.h();
                const int ox = cx - dw / 2;
                const int oy = cy - dh / 2;
                fl_push_clip(dx, dy, dim, dim);
                img.draw(ox, oy);
                fl_pop_clip();
                drawn = true;
            }
        }

        if (!drawn) {
            fl_color(fl_rgb_color(28, 28, 36));
            fl_rectf(dx, dy, dim, dim);
            fl_color(fl_rgb_color(90, 90, 120));
            fl_line_style(FL_DASH, 1);
            fl_rect(dx, dy, dim, dim);
            fl_line_style(0);
            fl_color(OLED_HINT);
            fl_font(FL_HELVETICA, 10);
            const char* tip = "paste SVG in Core Text";
            int tw = 0, th = 0;
            fl_measure(tip, tw, th);
            fl_draw(tip, cx - tw / 2, cy + 4);
            fl_font(FL_HELVETICA, 12);
        }

        // object bounds (always)
        fl_color(fl_rgb_color(50, 50, 60));
        fl_line_style(FL_SOLID, 1);
        fl_rect(dx, dy, dim, dim);
        fl_line_style(0);

        if (v->selected) {
            fl_color(OLED_SELECT);
            fl_line_style(FL_SOLID, 2);
            fl_rect(dx - 3, dy - 3, dim + 6, dim + 6);
            fl_line_style(0);
        }

        fl_color(OLED_TITLE);
        int tw = 0, th = 0;
        fl_measure(v->title.c_str(), tw, th);
        fl_draw(v->title.c_str(), cx - tw / 2, dy - 6);
    }
}

void GraphView::drawBoxSelect() {
    if (!box_selecting_) return;
    int x0 = box_x0_, y0 = box_y0_, x1 = box_x1_, y1 = box_y1_;
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    fl_color(fl_rgb_color(0, 160, 220));
    fl_line_style(FL_DASH, 1);
    fl_rect(x0, y0, x1 - x0, y1 - y0);
    fl_line_style(0);
}

void GraphView::drawNodes() {
    if (!graph_) return;

    fl_font(FL_HELVETICA, 12);

    for (auto* n : graph_->nodes()) {
        int sx, sy;
        worldToScreen(n->x, n->y, sx, sy);

        int r = n->selected ? 8 : 6;
        fl_color(fl_rgb_color(n->r(), n->g(), n->b()));
        fl_pie(sx - r, sy - r, 2 * r, 2 * r, 0, 360);

        if (n->selected) {
            fl_color(OLED_SELECT);
            fl_line_style(FL_SOLID, 2);
            fl_circle(sx, sy, r + 4);
            fl_line_style(0);
        }

        fl_color(OLED_TITLE);
        int tw = 0, th = 0;
        fl_measure(n->title.c_str(), tw, th);
        fl_draw(n->title.c_str(), sx - tw / 2, sy - r - 6);
    }
}

void GraphView::draw() {
    fl_push_clip(x(), y(), w(), h());

    fl_color(OLED_BG);
    fl_rectf(x(), y(), w(), h());

    drawSquares();   // behind axes / nodes
    drawVectors();
    drawAxes();
    drawEdges();
    drawLinkPreview();
    drawNodes();
    drawBoxSelect();

    // HUD
    fl_font(FL_HELVETICA, 11);
    fl_color(OLED_HINT);
    const char* mode_str = "LMB select | Shift+RMB multi | Alt+drag | Ctrl+add | S/V | Shift+R box | Shift+pan | RMB link | Del | F F1-4";
    if (mode_ == BoxSelect || box_selecting_)
        mode_str = "BOX SELECT — drag rectangle, release to select";
    fl_draw(mode_str, x() + 8, y() + 16);

    fl_pop_clip();
}

int GraphView::handle(int event) {
    int mx = Fl::event_x();
    int my = Fl::event_y();

    switch (event) {
    case FL_PUSH: {
        take_focus();
        last_mx_ = mx;
        last_my_ = my;

        // Middle mouse → pan
        if (Fl::event_button() == FL_MIDDLE_MOUSE) {
            panning_ = true;
            return 1;
        }

        // Left mouse
        if (Fl::event_button() == FL_LEFT_MOUSE) {

            // Box-select mode: drag rectangle
            if (mode_ == BoxSelect) {
                box_selecting_ = true;
                box_x0_ = box_x1_ = mx;
                box_y0_ = box_y1_ = my;
                return 1;
            }

            // Shift + LMB → pan (when not box-select)
            if (Fl::event_state() & FL_SHIFT) {
                panning_ = true;
                return 1;
            }

            // Ctrl + LMB → place new node at cursor
            if (Fl::event_state() & FL_CTRL) {
                double wx, wy;
                screenToWorld(mx, my, wx, wy);
                if (graph_) {
                    pushUndo();
                    Node& n = graph_->addNode(wx, wy);
                    graph_->selectNode(n.id);
                    do_callback();
                    redraw();
                }
                return 1;
            }

            Node* hit = hitTestNode(mx, my);
            if (hit) {
                bool already = hit->selected;
                if (!already) graph_->selectNode(hit->id);
                if (Fl::event_state() & FL_ALT) {
                    pushUndo();
                    multi_drag_ = (graph_->selectionCount() > 1) || already;
                    drag_node_ = hit;
                    drag_square_ = nullptr;
                    drag_vector_ = nullptr;
                    dragging_ = true;
                    drag_pushed_ = true;
                    screenToWorld(mx, my, drag_last_wx_, drag_last_wy_);
                }
                do_callback();
                redraw();
                return 1;
            }

            Square* shit = hitTestSquare(mx, my);
            if (shit) {
                bool already = shit->selected;
                if (!already) graph_->selectSquare(shit->id);
                if (Fl::event_state() & FL_ALT) {
                    pushUndo();
                    multi_drag_ = (graph_->selectionCount() > 1) || already;
                    drag_square_ = shit;
                    drag_node_ = nullptr;
                    drag_vector_ = nullptr;
                    dragging_ = true;
                    drag_pushed_ = true;
                    screenToWorld(mx, my, drag_last_wx_, drag_last_wy_);
                }
                do_callback();
                redraw();
                return 1;
            }

            VectorObject* vhit = hitTestVector(mx, my);
            if (vhit) {
                bool already = vhit->selected;
                if (!already) graph_->selectVector(vhit->id);
                if (Fl::event_state() & FL_ALT) {
                    pushUndo();
                    multi_drag_ = (graph_->selectionCount() > 1) || already;
                    drag_vector_ = vhit;
                    drag_node_ = nullptr;
                    drag_square_ = nullptr;
                    dragging_ = true;
                    drag_pushed_ = true;
                    screenToWorld(mx, my, drag_last_wx_, drag_last_wy_);
                }
                do_callback();
                redraw();
                return 1;
            }

            if (graph_) {
                graph_->clearSelection();
                do_callback();
                redraw();
            }
            return 1;
        }

        // Right mouse:
        //   Shift+RMB → multi-select toggle (nodes / squares / vectors)
        //   RMB on node → link selected → target
        //   RMB on edge → unlink
        if (Fl::event_button() == FL_RIGHT_MOUSE) {
            const bool shift = (Fl::event_state() & FL_SHIFT);

            if (shift && graph_) {
                if (Node* hit = hitTestNode(mx, my)) {
                    if (hit->selected) {
                        hit->selected = false;
                        if (graph_->selectedNode() == hit)
                            graph_->clearSelection();
                        // re-pick primary if anything left selected
                        for (auto* n : graph_->nodes())
                            if (n->selected) { graph_->selectNode(n->id, true); break; }
                        for (auto* s : graph_->squares())
                            if (s->selected) { graph_->selectSquare(s->id, true); break; }
                        for (auto* v : graph_->vectors())
                            if (v->selected) { graph_->selectVector(v->id, true); break; }
                    } else {
                        graph_->selectNode(hit->id, true);
                    }
                    do_callback(); redraw(); return 1;
                }
                if (Square* hit = hitTestSquare(mx, my)) {
                    if (hit->selected) {
                        hit->selected = false;
                    } else {
                        graph_->selectSquare(hit->id, true);
                    }
                    do_callback(); redraw(); return 1;
                }
                if (VectorObject* hit = hitTestVector(mx, my)) {
                    if (hit->selected) {
                        hit->selected = false;
                    } else {
                        graph_->selectVector(hit->id, true);
                    }
                    do_callback(); redraw(); return 1;
                }
                return 1;
            }

            Node* hit = hitTestNode(mx, my);
            if (hit && graph_) {
                Node* src = graph_->selectedNode();
                if (src && src->id != hit->id) {
                    pushUndo();
                    graph_->addEdge(src->id, hit->id);
                    do_callback();
                } else if (!src) {
                    graph_->selectNode(hit->id);
                    do_callback();
                }
                redraw();
                return 1;
            }

            Edge* ehit = hitTestEdge(mx, my);
            if (ehit) {
                pushUndo();
                graph_->removeEdge(ehit->id);
                do_callback();
                redraw();
                return 1;
            }

            redraw();
            return 1;
        }
        break;
    }

    case FL_DRAG: {
        if (box_selecting_) {
            box_x1_ = mx;
            box_y1_ = my;
            redraw();
            return 1;
        }
        if (panning_) {
            pan(mx - last_mx_, my - last_my_);
            last_mx_ = mx;
            last_my_ = my;
            return 1;
        }
        if (dragging_ && multi_drag_ && graph_) {
            double wx, wy;
            screenToWorld(mx, my, wx, wy);
            graph_->moveSelection(wx - drag_last_wx_, wy - drag_last_wy_);
            drag_last_wx_ = wx;
            drag_last_wy_ = wy;
            do_callback();
            redraw();
            return 1;
        }
        if (dragging_ && drag_node_) {
            double wx, wy;
            screenToWorld(mx, my, wx, wy);
            drag_node_->x = wx;
            drag_node_->y = wy;
            do_callback();
            redraw();
            return 1;
        }
        if (dragging_ && drag_square_) {
            double wx, wy;
            screenToWorld(mx, my, wx, wy);
            drag_square_->x = wx;
            drag_square_->y = wy;
            do_callback();
            redraw();
            return 1;
        }
        if (dragging_ && drag_vector_) {
            double wx, wy;
            screenToWorld(mx, my, wx, wy);
            drag_vector_->x = wx;
            drag_vector_->y = wy;
            do_callback();
            redraw();
            return 1;
        }
        if (mode_ == Link && link_from_) {
            link_mx_ = mx;
            link_my_ = my;
            redraw();
            return 1;
        }
        break;
    }

    case FL_RELEASE: {
        if (box_selecting_ && graph_) {
            double wx0, wy0, wx1, wy1;
            screenToWorld(box_x0_, box_y0_, wx0, wy0);
            screenToWorld(box_x1_, box_y1_, wx1, wy1);
            graph_->selectInRect(wx0, wy0, wx1, wy1);
            box_selecting_ = false;
            mode_ = Select;
            do_callback();
            redraw();
            return 1;
        }
        panning_ = false;
        dragging_ = false;
        multi_drag_ = false;
        drag_node_ = nullptr;
        drag_square_ = nullptr;
        drag_vector_ = nullptr;
        drag_pushed_ = false;
        return 1;
    }

    case FL_MOUSEWHEEL: {
        int dy = Fl::event_dy();
        double factor = (dy > 0) ? 0.9 : 1.111111;
        zoom(factor, mx, my);
        return 1;
    }

    case FL_KEYDOWN: {
        int key = Fl::event_key();
        // Ctrl+Z undo / Ctrl+Y or Ctrl+Shift+Z redo
        if ((Fl::event_state() & FL_CTRL) && (key == 'z' || key == 'Z')) {
            if (Fl::event_state() & FL_SHIFT) {
                if (undo_ && undo_->redo()) { do_callback(); redraw(); }
            } else {
                if (undo_ && undo_->undo()) { do_callback(); redraw(); }
            }
            return 1;
        }
        if ((Fl::event_state() & FL_CTRL) && (key == 'y' || key == 'Y')) {
            if (undo_ && undo_->redo()) { do_callback(); redraw(); }
            return 1;
        }
        if ((key == 'r' || key == 'R') && !(Fl::event_state() & FL_SHIFT)) {
            resetView();
            return 1;
        }
        if (key == FL_Escape) {
            link_from_ = 0;
            mode_ = Select;
            redraw();
            return 1;
        }
        if (key == FL_Delete || key == FL_BackSpace) {
            if (graph_) {
                if (graph_->selectionCount() > 0) {
                    pushUndo();
                    graph_->deleteSelection();
                    link_from_ = 0;
                    mode_ = Select;
                    do_callback();
                    redraw();
                }
            }
            return 1;
        }
        // F1 — fast XY edit selected node
        if (key == FL_F+1) {
            if (graph_) {
                if (Node* n = graph_->selectedNode()) {
                    char def[64];
                    snprintf(def, sizeof(def), "%.3f %.3f", n->x, n->y);
                    const char* r = fl_input("Coordinates (X Y):", def);
                    if (r) {
                        double nx = n->x, ny = n->y;
                        int got = sscanf(r, "%lf %lf", &nx, &ny);
                        if (got >= 1) {
                            pushUndo();
                            n->x = nx;
                            if (got >= 2) n->y = ny;
                            do_callback();
                            redraw();
                        }
                    }
                } else if (Square* s = graph_->selectedSquare()) {
                    char def[96];
                    snprintf(def, sizeof(def), "%.3f %.3f %.3f %.3f", s->x, s->y, s->w, s->h);
                    const char* r = fl_input("Square X Y W H:", def);
                    if (r) {
                        double nx, ny, nw, nh;
                        int got = sscanf(r, "%lf %lf %lf %lf", &nx, &ny, &nw, &nh);
                        if (got >= 2) {
                            pushUndo();
                            s->x = nx; s->y = ny;
                            if (got >= 3 && nw > 0.05) s->w = nw;
                            if (got >= 4 && nh > 0.05) s->h = nh;
                            do_callback();
                            redraw();
                        }
                    }
                } else if (VectorObject* v = graph_->selectedVector()) {
                    char def[96];
                    snprintf(def, sizeof(def), "%.3f %.3f %.3f", v->x, v->y, (double)v->scale);
                    const char* r = fl_input("Vector X Y Scale:", def);
                    if (r) {
                        double nx, ny, ns;
                        int got = sscanf(r, "%lf %lf %lf", &nx, &ny, &ns);
                        if (got >= 2) {
                            pushUndo();
                            v->x = nx; v->y = ny;
                            if (got >= 3 && ns > 0.01) v->scale = (float)ns;
                            do_callback();
                            redraw();
                        }
                    }
                }
            }
            return 1;
        }
        // F2 — fast rename selected node
        if (key == FL_F+2) {
            if (graph_) {
                if (Node* n = graph_->selectedNode()) {
                    const char* r = fl_input("Rename node:", n->title.c_str());
                    if (r) {
                        pushUndo();
                        n->title = r;
                        do_callback();
                        redraw();
                    }
                } else if (Square* s = graph_->selectedSquare()) {
                    const char* r = fl_input("Rename square:", s->title.c_str());
                    if (r) {
                        pushUndo();
                        s->title = r;
                        do_callback();
                        redraw();
                    }
                } else if (VectorObject* v = graph_->selectedVector()) {
                    const char* r = fl_input("Rename vector:", v->title.c_str());
                    if (r) {
                        pushUndo();
                        v->title = r;
                        do_callback();
                        redraw();
                    }
                }
            }
            return 1;
        }
        // F3 — fast color selected node
        if (key == FL_F+3) {
            if (graph_ && graph_->selectionCount() > 0) {
                uchar r = 200, g = 200, b = 200;
                if (Node* n = graph_->selectedNode()) { r = n->r(); g = n->g(); b = n->b(); }
                else if (Square* s = graph_->selectedSquare()) { r = s->r(); g = s->g(); b = s->b(); }
                if (fl_color_chooser("Selection Color", r, g, b)) {
                    pushUndo();
                    graph_->colorSelection((uint32_t(r) << 16) | (uint32_t(g) << 8) | b);
                    do_callback();
                    redraw();
                }
            }
            return 1;
        }
        // F — search by id or title
        if ((key == 'f' || key == 'F') && !(Fl::event_state() & FL_CTRL)) {
            if (graph_) {
                const char* q = fl_input("Find node/square (id or title):", "");
                if (q && *q) {
                    std::string query = q;
                    // try numeric id
                    bool found = false;
                    try {
                        unsigned long long idv = std::stoull(query);
                        if (Node* n = graph_->getNode(idv)) {
                            graph_->selectNode(n->id);
                            found = true;
                        } else if (Square* s = graph_->getSquare(idv)) {
                            graph_->selectSquare(s->id);
                            found = true;
                        }
                    } catch (...) {}
                    if (!found) {
                        auto lower = [](std::string a) {
                            for (char& c : a) if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
                            return a;
                        };
                        std::string ql = lower(query);
                        for (auto* n : graph_->nodes()) {
                            if (lower(n->title).find(ql) != std::string::npos) {
                                graph_->selectNode(n->id);
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            for (auto* s : graph_->squares()) {
                                if (lower(s->title).find(ql) != std::string::npos) {
                                    graph_->selectSquare(s->id);
                                    found = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (found) {
                        do_callback();
                        redraw();
                    } else {
                        fl_alert("No match for \"%s\"", q);
                    }
                }
            }
            return 1;
        }

        // F4 — square Z-index slider (stacking: -10 back .. 10 front)
        if (key == FL_F+4) {
            if (graph_) {
                Square* s = graph_->selectedSquare();
                if (!s) {
                    fl_alert("Select a square to set Z-index.");
                    return 1;
                }
                const int W = 320, H = 120;
                Fl_Window win(W, H, "Square Z-index");
                win.set_modal();
                win.color(fl_rgb_color(20, 20, 20));

                auto* slider = new Fl_Value_Slider(20, 30, W - 40, 28, "Z  (back) -10 ... 10 (front)");
                slider->type(FL_HOR_NICE_SLIDER);
                slider->bounds(-10, 10);
                slider->step(1);
                slider->value(s->z);
                slider->labelsize(12);
                slider->labelcolor(fl_rgb_color(180, 180, 180));
                slider->selection_color(fl_rgb_color(0, 140, 200));
                slider->color(fl_rgb_color(40, 40, 40));
                slider->textcolor(fl_rgb_color(220, 220, 220));
                slider->align(FL_ALIGN_TOP);

                int done = 0;
                auto* ok = new Fl_Return_Button(W/2 - 50, H - 40, 100, 28, "OK");
                ok->callback([](Fl_Widget* w, void* p) {
                    *static_cast<int*>(p) = 1;
                    w->window()->hide();
                }, &done);

                win.end();
                win.show();
                while (win.shown()) Fl::wait();

                if (done) {
                    int nz = static_cast<int>(std::lround(slider->value()));
                    if (nz < -10) nz = -10;
                    if (nz > 10) nz = 10;
                    pushUndo();
                    for (auto* sq : graph_->squares())
                        if (sq->selected) sq->z = nz;
                    do_callback();
                    redraw();
                }
            }
            return 1;
        }
        // Shift+R — box select mode
        if ((key == 'r' || key == 'R') && (Fl::event_state() & FL_SHIFT)) {
            mode_ = BoxSelect;
            box_selecting_ = false;
            redraw();
            return 1;
        }
        // V — add SVG vector (optional file)
        if ((key == 'v' || key == 'V') && !(Fl::event_state() & FL_CTRL)) {
            if (graph_) {
                const char* title = fl_input("Vector title:", "SVG");
                if (!title) return 1;
                const char* sc = fl_input("Scale:", "1.0");
                float scale = 1.0f;
                if (sc) scale = static_cast<float>(atof(sc));
                if (scale < 0.01f) scale = 1.0f;

                std::string svg_data;
                int load = fl_choice("Load SVG from file?", "Empty (paste later)", "Choose file…", nullptr);
                if (load == 1) {
                    Fl_File_Chooser chooser(".", "SVG Files (*.svg)\t*.svg", Fl_File_Chooser::SINGLE, "Open SVG");
                    chooser.show();
                    while (chooser.shown()) Fl::wait();
                    if (chooser.value()) {
                        std::ifstream in(chooser.value(), std::ios::binary);
                        if (!in) { fl_alert("Could not open file."); return 1; }
                        std::string raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                        auto is_svg = [](const std::string& s) -> bool {
                            size_t i = 0;
                            while (i < s.size() && (s[i]==' '||s[i]=='\t'||s[i]=='\n'||s[i]=='\r')) ++i;
                            if (i + 5 <= s.size() && s.compare(i, 5, "<?xml") == 0) {
                                auto pos = s.find("<svg", i);
                                if (pos == std::string::npos) pos = s.find("<SVG", i);
                                return pos != std::string::npos;
                            }
                            return i + 4 <= s.size() && (s.compare(i, 4, "<svg") == 0 || s.compare(i, 4, "<SVG") == 0);
                        };
                        if (!is_svg(raw)) { fl_alert("Not an SVG file (missing <svg> header)."); return 1; }
                        svg_data = std::move(raw);
                    }
                }
                double wx, wy;
                screenToWorld(mx, my, wx, wy);
                pushUndo();
                VectorObject& vo = graph_->addVector(wx, wy, svg_data, title, scale);
                graph_->selectVector(vo.id);
                do_callback();
                redraw();
            }
            return 1;
        }

        // C — clone selection
        if ((key == 'c' || key == 'C') && !(Fl::event_state() & FL_CTRL)) {
            if (graph_ && graph_->selectionCount() > 0) {
                pushUndo();
                const double off = 0.5;
                std::vector<uint64_t> nids, sids, vids;
                for (auto* n : graph_->nodes()) if (n->selected) nids.push_back(n->id);
                for (auto* s : graph_->squares()) if (s->selected) sids.push_back(s->id);
                for (auto* v : graph_->vectors()) if (v->selected) vids.push_back(v->id);
                graph_->clearSelection();
                for (auto id : nids) {
                    Node* src = graph_->getNode(id);
                    if (!src) continue;
                    Node& n = graph_->addNode(src->x + off, src->y + off, src->title + " copy");
                    n.text = src->text; n.color = src->color;
                    graph_->selectNode(n.id, true);
                }
                for (auto id : sids) {
                    Square* src = graph_->getSquare(id);
                    if (!src) continue;
                    Square& s = graph_->addSquare(src->x + off, src->y + off, src->w, src->h, src->title + " copy");
                    s.text = src->text; s.color = src->color; s.z = src->z;
                    graph_->selectSquare(s.id, true);
                }
                for (auto id : vids) {
                    VectorObject* src = graph_->getVector(id);
                    if (!src) continue;
                    VectorObject& v = graph_->addVector(src->x + off, src->y + off, src->svg, src->title + " copy", src->scale);
                    graph_->selectVector(v.id, true);
                }
                do_callback();
                redraw();
            }
            return 1;
        }
        // S — place square at cursor (not with Ctrl — Ctrl+S is save)
        if ((key == 's' || key == 'S') && !(Fl::event_state() & FL_CTRL)) {
            if (graph_) {
                double wx, wy;
                screenToWorld(mx, my, wx, wy);
                pushUndo();
                Square& sq = graph_->addSquare(wx, wy);
                graph_->selectSquare(sq.id);
                do_callback();
                redraw();
            }
            return 1;
        }
        break;
    }

    case FL_FOCUS:
    case FL_UNFOCUS:
        return 1;

    case FL_MOVE: {
        if (mode_ == Link && link_from_) {
            link_mx_ = mx;
            link_my_ = my;
            redraw();
            return 1;
        }
        break;
    }

    default:
        break;
    }
    return Fl_Widget::handle(event);
}
