#include "GUI.hpp"
#include <png.h> // 需安装libpng（brew install libpng / apt install libpng-dev）
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_RGB_Image.H> // 新增：RGB图像支持

// 全局状态初始化
AppState app_state;

void set_status(const std::string& msg, Fl_Color color) {
    if (!app_state.status_box) return;
    app_state.status_box->copy_label(msg.c_str());
    app_state.status_box->labelcolor(color);
    app_state.status_box->redraw();
}

/**
 * @brief 选择待渲染文件（FLTK原生文件选择器）
 */
void select_file_cb(Fl_Widget*, void*) {
    Fl_Native_File_Chooser file_chooser;
    file_chooser.title("选择待渲染文件");
    file_chooser.type(Fl_Native_File_Chooser::BROWSE_FILE); // 仅选文件
    // 过滤常见文件格式（可根据你的项目修改）
    file_chooser.filter("支持的文件\t*.obj *.txt *.json *.dat\n所有文件\t*");
    file_chooser.directory("."); // 默认当前目录

    // 打开文件选择器
    if (file_chooser.show() == 0) {
        app_state.selected_file = file_chooser.filename();
        set_status("File chosen: " + app_state.selected_file,
               FL_DARK_GREEN);
    }
}

/**
 * @brief 执行渲染逻辑（需替换为你的项目渲染代码）
 */
void render_cb(Fl_Widget*, void*) {
    if (app_state.selected_file.empty()) {
        set_status("No file, unable to render", FL_RED);
        return;
    }

    set_status("Rendering...", FL_BLUE);

    // ========== 替换为你的实际渲染逻辑 ==========
    // fl_message("开始渲染：%s", app_state.selected_file.c_str());
    
    // 示例：模拟渲染缓冲区（你需替换为真实渲染数据）
    app_state.buffer_width = 800;
    app_state.buffer_height = 600;
    // 分配RGB缓冲区（每个像素3字节）
    app_state.render_buffer = malloc(app_state.buffer_width * app_state.buffer_height * 3);
    if (!app_state.render_buffer) {
        fl_alert("渲染缓冲区分配失败！");
        return;
    }
    // 模拟填充渲染数据（白色背景）
    unsigned char* buf = (unsigned char*)app_state.render_buffer;
    for (int i = 0; i < app_state.buffer_width * app_state.buffer_height * 3; i++) {
        buf[i] = 255;
    }
    // ===========================================

    app_state.is_rendered = true;

    set_status("Render completed", FL_DARK_GREEN);
    app_state.render_display_box->redraw();
}

/**
 * @brief 将渲染结果保存为PNG（基于libpng实现）
 */
bool save_render_to_png(const std::string& save_path) {
    if (!app_state.is_rendered || !app_state.render_buffer) {
        return false;
    }

    FILE* fp = fopen(save_path.c_str(), "wb");
    if (!fp) return false;

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!png_ptr || !info_ptr) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return false;
    }

    // 设置PNG输出
    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, app_state.buffer_width, app_state.buffer_height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);

    // 写入像素数据（按行写入）
    unsigned char* buf = (unsigned char*)app_state.render_buffer;
    for (int y = 0; y < app_state.buffer_height; y++) {
        png_write_row(png_ptr, buf + y * app_state.buffer_width * 3);
    }

    // 清理PNG资源
    png_write_end(png_ptr, nullptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return true;
}

/**
 * @brief 保存PNG回调（选择保存路径+执行保存）
 */
void save_png_cb(Fl_Widget*, void*) {
    if (!app_state.is_rendered) {
        set_status("Not render yet, no PNG", FL_RED);
        return;
    }

    // 选择保存路径
    Fl_Native_File_Chooser save_chooser;
    save_chooser.title("保存渲染结果为PNG");
    save_chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    save_chooser.filter("PNG文件\t*.png");
    save_chooser.preset_file("render_result.png"); // 默认文件名

    if (save_chooser.show() == 0) {
        std::string save_path = save_chooser.filename();
        // 自动补全.png后缀
        if (save_path.find(".png") == std::string::npos) {
            save_path += ".png";
        }

        // 执行保存
        if (save_render_to_png(save_path)) {
            set_status("PNG Saved Successfully", FL_DARK_GREEN);
        } else {
            set_status("Save failure", FL_DARK_GREEN);
        }
    }
}

/**
 * @brief 初始化GUI界面
 */
/**
 * @brief 现代美化版初始化GUI界面
 */
Fl_Window* init_gui(int width = 900, int height = 700) { 
    // 1. 设置全局配色方案
    Fl::background(33, 33, 33);      // 主背景：深黑灰
    Fl::background2(45, 45, 45);     // 输入框/列表背景
    Fl::foreground(240, 240, 240);   // 默认文字颜色：近白色
    
    Fl_Window* win = new Fl_Window(width, height, "Ray Tracing Renderer Pro");
    win->color(fl_rgb_color(33, 33, 33)); // 确保窗口颜色应用
    win->begin();

    // ===== 1. 渲染结果显示区域 (加深背景并居中) =====
    int margin = 20;
    int sidebar_w = 200; // 左边栏宽度

    // ===== 新增：左侧场景列表 =====
    Fl_Hold_Browser* browser = new Fl_Hold_Browser(margin, margin, sidebar_w, height - 160, "Scenes");
    browser->color(fl_rgb_color(45, 45, 45));
    browser->textcolor(FL_WHITE);
    browser->has_scrollbar(Fl_Browser_::VERTICAL);
    browser->align(FL_ALIGN_TOP_LEFT);
    app_state.file_browser = browser;

    // ===== 修改：渲染结果显示区域 (坐标 X 增加 sidebar_w + spacing) =====
    int canvas_x = margin + sidebar_w + margin;
    int canvas_w = width - canvas_x - margin;
    Fl_Box* display_box = new Fl_Box(canvas_x, margin, canvas_w, height - 160);
    display_box->box(FL_FLAT_BOX); 
    display_box->color(fl_rgb_color(20, 20, 20));
    app_state.render_display_box = display_box;

    // ===== 修改：状态栏 (拉长以覆盖底部) =====
    Fl_Box* status_box = new Fl_Box(margin, height - 130, width - 2 * margin, 30, " Ready");
    status_box->box(FL_THIN_DOWN_BOX);
    app_state.status_box = status_box;

    // ===== 3. 按钮布局 (扁平化 & 颜色区分) =====
    int btn_h = 45;
    int btn_y = height - 70;
    int spacing = 15;
    int btn_w = (width - 2 * margin - 2 * spacing) / 3;

    // 选择文件按钮
    Fl_Button* select_btn = new Fl_Button(margin, btn_y, btn_w, btn_h, "@refresh  Refresh");
    select_btn->box(FL_GTK_UP_BOX); // 稍微现代一点的边框
    select_btn->color(fl_rgb_color(60, 60, 60));
    select_btn->down_color(fl_rgb_color(80, 80, 80));

    // 执行按钮 (亮蓝色)
    Fl_Button* render_btn = new Fl_Button(margin + btn_w + spacing, btn_y, btn_w, btn_h, "@render  Render Scene");
    render_btn->box(FL_GTK_UP_BOX);
    render_btn->color(fl_rgb_color(0, 90, 160));
    render_btn->labelcolor(FL_WHITE);
    render_btn->labelfont(FL_HELVETICA_BOLD);

    // 保存按钮 (翠绿色)
    Fl_Button* save_btn = new Fl_Button(margin + 2 * (btn_w + spacing), btn_y, btn_w, btn_h, "@filesave  Save Image");
    save_btn->box(FL_GTK_UP_BOX);
    save_btn->color(fl_rgb_color(40, 110, 40));
    save_btn->labelcolor(FL_WHITE);

    // 绑定回调
    select_btn->callback(select_file_cb);
    render_btn->callback(render_cb);
    save_btn->callback(save_png_cb);

    win->end();
    win->resizable(display_box); 
    return win;
}

/**
 * @brief 清理资源（新增图像资源释放）
 */
void cleanup_resources(Fl_Window* win) {
    // 1. 释放显示的图像
    if (app_state.render_display_box && app_state.render_display_box->image()) {
        delete app_state.render_display_box->image();
        app_state.render_display_box->image(nullptr);
    }
    // 2. 释放渲染缓冲区
    if (app_state.render_buffer) {
        free(app_state.render_buffer);
        app_state.render_buffer = nullptr;
    }
    // 3. 释放窗口
    if (win) {
        delete win;
    }
}