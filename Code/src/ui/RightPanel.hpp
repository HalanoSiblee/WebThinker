#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Color_Chooser.H>
#include "../graph/Graph.hpp"

class UndoHistory;

class RightPanel : public Fl_Group {
public:
    RightPanel(int x, int y, int w, int h, Graph* graph);

    void setGraph(Graph* g) { graph_ = g; }
    void setUndo(UndoHistory* u) { undo_ = u; }
    using DirtyCb = void (*)(void*);
    void setDirtyCallback(DirtyCb cb, void* data) { dirty_cb_ = cb; dirty_data_ = data; }
    void refreshFromSelection();
    void applyToSelection();

private:
    Graph* graph_ = nullptr;
    UndoHistory* undo_ = nullptr;
    DirtyCb dirty_cb_ = nullptr;
    void* dirty_data_ = nullptr;

    Fl_Group*  content_ = nullptr;

    Fl_Box*   title_label_ = nullptr;
    Fl_Input* title_input_ = nullptr;

    Fl_Box* id_label_ = nullptr;
    Fl_Box* id_value_ = nullptr;

    Fl_Box*   coord_label_ = nullptr;
    Fl_Input* x_input_ = nullptr;
    Fl_Input* y_input_ = nullptr;

    Fl_Box*   size_label_ = nullptr;
    Fl_Input* w_input_ = nullptr;
    Fl_Input* h_input_ = nullptr;

    Fl_Box*    color_label_ = nullptr;
    Fl_Button* color_btn_ = nullptr;

    Fl_Box*             text_label_ = nullptr;
    Fl_Multiline_Input* text_input_ = nullptr;

    Fl_Box* conn_label_ = nullptr;
    Fl_Box* conn_box_ = nullptr;

    Fl_Button* update_btn_ = nullptr;

    Fl_Box*    def_label_ = nullptr;
    Fl_Button* def_node_btn_ = nullptr;
    Fl_Button* def_square_btn_ = nullptr;
    Fl_Button* def_edge_btn_ = nullptr;

    static void onUpdate(Fl_Widget*, void* data);
    static void onColor(Fl_Widget*, void* data);
    static void onDefNode(Fl_Widget*, void* data);
    static void onDefSquare(Fl_Widget*, void* data);
    static void onDefEdge(Fl_Widget*, void* data);
    void syncDefaultButtons();
    void buildUI();
};
