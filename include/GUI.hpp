#ifndef GUI_HPP
#define GUI_HPP

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>
#include <string>

// 全局状态结构体（需暴露给main.cpp）
struct AppState {
    std::string selected_file;    // 选中的场景文件路径
    int buffer_width;             // 缓冲区宽度
    int buffer_height;            // 缓冲区高度
    void* render_buffer = nullptr; // 渲染像素缓冲区（RGB格式）
    bool is_rendered = false;     // 是否完成渲染
    Fl_Box* render_display_box = nullptr; // 渲染结果显示框（新增）
    Fl_Box* status_box = nullptr;
    Fl_Hold_Browser* file_browser = nullptr; // 新增：左侧文件列表框
    Fl_Progress* progress_bar = nullptr; // 新增：渲染进度条
};

// 全局状态（extern供main.cpp访问）
extern AppState app_state;

// GUI初始化函数
Fl_Window* init_gui(int width, int height);

// 资源清理函数
void cleanup_resources(Fl_Window* win);

// 保存PNG回调（供main.cpp绑定）
void save_png_cb(Fl_Widget*, void*);
void select_file_cb(Fl_Widget*, void*);
void render_cb(Fl_Widget*, void*);

// 状态栏接口
void set_status(const std::string& msg, Fl_Color color = FL_BLACK);

#endif // GUI_HPP