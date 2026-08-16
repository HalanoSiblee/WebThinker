#pragma once

#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include "../graph/Graph.hpp"

class UndoHistory;

class GraphView : public Fl_Widget {
public:
    GraphView(int x, int y, int w, int h, Graph* graph);

    void setGraph(Graph* g) { graph_ = g; }
    Graph* graph() const { return graph_; }

    void setUndo(UndoHistory* u) { undo_ = u; }
    UndoHistory* undo() const { return undo_; }

    using DirtyCb = void (*)(void*);
    void setDirtyCallback(DirtyCb cb, void* data) { dirty_cb_ = cb; dirty_data_ = data; }

    void resetView();
    void zoom(double factor, int mx, int my);
    void pan(double dx, double dy);

    enum Mode { Select, Pen, Link, BoxSelect };

protected:
    void draw() override;
    int handle(int event) override;

private:
    Graph* graph_ = nullptr;
    UndoHistory* undo_ = nullptr;
    DirtyCb dirty_cb_ = nullptr;
    void* dirty_data_ = nullptr;

    double cam_x_ = 0.0;
    double cam_y_ = 0.0;
    double scale_ = 40.0;

    Mode mode_ = Select;

    uint64_t link_from_ = 0;
    bool     panning_   = false;
    bool     dragging_  = false;
    bool     drag_pushed_ = false;
    bool     box_selecting_ = false;
    Node*    drag_node_ = nullptr;
    Square*  drag_square_ = nullptr;
    VectorObject* drag_vector_ = nullptr;
    bool     multi_drag_ = false;
    double   drag_last_wx_ = 0, drag_last_wy_ = 0;
    int      last_mx_   = 0;
    int      last_my_   = 0;
    int      link_mx_   = 0;
    int      link_my_   = 0;
    int      box_x0_ = 0, box_y0_ = 0, box_x1_ = 0, box_y1_ = 0;

    void pushUndo();
    void worldToScreen(double wx, double wy, int& sx, int& sy) const;
    void screenToWorld(int sx, int sy, double& wx, double& wy) const;
    Node* hitTestNode(int sx, int sy, double radius = 14.0);
    Square* hitTestSquare(int sx, int sy);
    VectorObject* hitTestVector(int sx, int sy);
    Edge* hitTestEdge(int sx, int sy, double threshold = 8.0);

    void drawAxes();
    void drawNodes();
    void drawSquares();
    void drawVectors();
    void drawEdges();
    void drawLinkPreview();
    void drawBoxSelect();
};
