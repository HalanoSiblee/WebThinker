#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>

#include "graph/Graph.hpp"
#include "graph/UndoHistory.hpp"
#include "ui/GraphView.hpp"
#include "ui/RightPanel.hpp"
#include "storage/Storage.hpp"

#include <string>

static const int kMenuH = 28;
static const int kStatusH = 22;
static const int kPanelW = 280;
class MainWindow : public Fl_Double_Window {
public:
    MainWindow(int w, int h, const char* title)
        : Fl_Double_Window(w, h, title)
    {
        color(fl_rgb_color(0, 0, 0));

        menu_ = new Fl_Menu_Bar(0, 0, w, kMenuH);
        menu_->box(FL_FLAT_BOX);
        menu_->add("File/New",            FL_CTRL + 'n', onNew, this);
        menu_->add("File/Open...",        FL_CTRL + 'o', onOpen, this);
        menu_->add("File/Save",           FL_CTRL + 's', onSave, this);
        menu_->add("File/Save As...",     FL_CTRL + FL_SHIFT + 's', onSaveAs, this);
        menu_->add("File/Quit",           FL_CTRL + 'q', onQuit, this);
        menu_->add("Edit/Undo",           FL_CTRL + 'z', onUndo, this);
        menu_->add("Edit/Redo",           FL_CTRL + 'y', onRedo, this);
        menu_->add("View/Reset Camera",   'r', onReset, this);
        menu_->add("Help/About WebThinker", 0, onAbout, this);
        //menu_->selection_color(FL_BLACK);
        menu_->color(fl_rgb_color(12, 12, 12));
        menu_->textcolor(fl_rgb_color(200, 200, 200));

        history_ = new UndoHistory(&graph_);

        graph_view_ = new GraphView(0, kMenuH, w - kPanelW, h - kMenuH - kStatusH, &graph_);
        graph_view_->setUndo(history_);
        graph_view_->setDirtyCallback(onDirty, this);
        graph_view_->callback(onGraphChange, this);

        right_panel_ = new RightPanel(w - kPanelW, kMenuH, kPanelW, h - kMenuH - kStatusH, &graph_);
        right_panel_->setUndo(history_);
        right_panel_->setDirtyCallback(onDirty, this);

        status_ = new Fl_Box(0, h - kStatusH, w, kStatusH, "Unsaved");
        status_->box(FL_FLAT_BOX);
        status_->color(fl_rgb_color(8, 8, 8));
        status_->labelsize(11);
        status_->labelfont(FL_HELVETICA);
        status_->labelcolor(fl_rgb_color(110, 110, 110));
        status_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

        resizable(graph_view_);
        end();

        graph_view_->resetView();
        updateStatus();
    }

    ~MainWindow() override {
        delete history_;
        history_ = nullptr;
    }

    int handle(int event) override {
        // Ensure Ctrl+S / Ctrl+Shift+S work even when GraphView has focus
        if (event == FL_SHORTCUT || event == FL_KEYBOARD || event == FL_KEYDOWN) {
            const int key = Fl::event_key();
            const int state = Fl::event_state();
            if ((state & FL_CTRL) && (key == 's' || key == 'S')) {
                if (state & FL_SHIFT) onSaveAs(nullptr, this);
                else onSave(nullptr, this);
                return 1;
            }
        }
        return Fl_Double_Window::handle(event);
    }

    void resize(int X, int Y, int W, int H) override {
        Fl_Double_Window::resize(X, Y, W, H);
        if (menu_) menu_->resize(0, 0, W, kMenuH);
        if (graph_view_) graph_view_->resize(0, kMenuH, W - kPanelW, H - kMenuH - kStatusH);
        if (right_panel_) right_panel_->resize(W - kPanelW, kMenuH, kPanelW, H - kMenuH - kStatusH);
        if (status_) status_->resize(0, H - kStatusH, W, kStatusH);
    }

private:
    Graph graph_;
    UndoHistory* history_ = nullptr;
    GraphView* graph_view_ = nullptr;
    RightPanel* right_panel_ = nullptr;
    Fl_Menu_Bar* menu_ = nullptr;
    Fl_Box* status_ = nullptr;
    std::string current_path_;
    bool dirty_ = true;

    void updateStatus() {
        static std::string buf;
        if (current_path_.empty()) {
            buf = dirty_ ? "  Unsaved — Untitled" : "  Saved — Untitled";
        } else {
            buf = (dirty_ ? "  Unsaved — " : "  Saved — ") + current_path_;
        }
        status_->label(buf.c_str());
        status_->labelcolor(dirty_ ? fl_rgb_color(160, 120, 80) : fl_rgb_color(100, 100, 100));
        status_->redraw();
    }

    void setDirty(bool d) {
        dirty_ = d;
        updateStatus();
    }

    static void onDirty(void* data) {
        static_cast<MainWindow*>(data)->setDirty(true);
    }

    static void onGraphChange(Fl_Widget*, void* data) {
        auto* self = static_cast<MainWindow*>(data);
        self->right_panel_->refreshFromSelection();
        self->graph_view_->redraw();
    }

    static void onNew(Fl_Widget*, void* data) {
        auto* self = static_cast<MainWindow*>(data);
        self->graph_.clear();
        if (self->history_) self->history_->clear();
        self->current_path_.clear();
        self->setDirty(true);
        self->right_panel_->refreshFromSelection();
        self->graph_view_->redraw();
        self->label("WebThinker — Untitled");
    }

    static void onUndo(Fl_Widget*, void* data) {
        auto* self = static_cast<MainWindow*>(data);
        if (self->history_ && self->history_->undo()) {
            self->setDirty(true);
            self->right_panel_->refreshFromSelection();
            self->graph_view_->redraw();
        }
    }

    static void onRedo(Fl_Widget*, void* data) {
        auto* self = static_cast<MainWindow*>(data);
        if (self->history_ && self->history_->redo()) {
            self->setDirty(true);
            self->right_panel_->refreshFromSelection();
            self->graph_view_->redraw();
        }
    }

    static void onOpen(Fl_Widget*, void* data) {
        auto* self = static_cast<MainWindow*>(data);
        Fl_File_Chooser chooser(".", "*.webtnk", Fl_File_Chooser::SINGLE, "Open Project");
        chooser.show();
        while (chooser.shown()) Fl::wait();
        if (chooser.value()) {
            if (Storage::load(self->graph_, chooser.value())) {
                if (self->history_) self->history_->clear();
                self->current_path_ = chooser.value();
                self->setDirty(false);
                self->right_panel_->refreshFromSelection();
                self->graph_view_->redraw();
                self->label(("WebThinker — " + self->current_path_).c_str());
            } else {
                fl_alert("Failed to load project.");
            }
        }
    }

    static void onSave(Fl_Widget*, void* data) {
        auto* self = static_cast<MainWindow*>(data);
        if (self->current_path_.empty()) {
            onSaveAs(nullptr, data);
            return;
        }
        if (Storage::save(self->graph_, self->current_path_)) {
            self->setDirty(false);
        } else {
            fl_alert("Failed to save project.");
        }
    }

    static void onSaveAs(Fl_Widget*, void* data) {
        auto* self = static_cast<MainWindow*>(data);
        Fl_File_Chooser chooser(".", "*.webtnk", Fl_File_Chooser::CREATE, "Save Project As");
        chooser.show();
        while (chooser.shown()) Fl::wait();
        if (chooser.value()) {
            std::string path = chooser.value();
            if (path.size() < 8 || path.substr(path.size() - 8) != ".webtnk")
                path += ".webtnk";
            if (Storage::save(self->graph_, path)) {
                self->current_path_ = path;
                self->setDirty(false);
                self->label(("WebThinker — " + path).c_str());
            } else {
                fl_alert("Failed to save project.");
            }
        }
    }

    static void onQuit(Fl_Widget*, void*) {
        exit(0);
    }

    static void onReset(Fl_Widget*, void* data) {
        static_cast<MainWindow*>(data)->graph_view_->resetView();
    }

    static void onAbout(Fl_Widget*, void*) {
        fl_message(
            "WebThinker\n"
            "FLTK . SQLite . zstd\n\n\n"
            "@@HalanoSiblee - XAi.Grok\n"
            "Version 1.0.0"
        );
    }
};

int main(int argc, char** argv) {
    Fl::visual(FL_RGB);
    auto* win = new MainWindow(1280, 800, "WebThinker — Untitled");
    win->show(argc, argv);
    Fl::set_color(FL_SELECTION_COLOR, 0, 0, 0);
    return Fl::run();
}