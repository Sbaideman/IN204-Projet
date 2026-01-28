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
Fl_Window* init_gui(int width = 800, int height = 600) { // 增大窗口尺寸
    // 创建主窗口
    Fl_Window* win = new Fl_Window(width, height, "Rander GUI");
    win->begin();

    // ===== 1. 渲染结果显示区域（占窗口上半部分）=====
    Fl_Box* display_box = new Fl_Box(10, 10, width - 20, height - 100);
    display_box->box(FL_DOWN_BOX); // 带边框的显示框，更易识别
    display_box->label("Display area");
    display_box->align(FL_ALIGN_CENTER); // 文字居中
    display_box->color(FL_GRAY); // 背景色（未渲染时灰色）
    app_state.render_display_box = display_box; // 关联到全局状态

    // ===== 状态栏 =====
    Fl_Box* status_box =
        new Fl_Box(10, height - 130, width - 20, 30, "就绪");
    status_box->box(FL_DOWN_BOX);
    status_box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    status_box->labelsize(14);
    status_box->color(FL_WHITE);

    app_state.status_box = status_box;

    // ===== 2. 按钮布局优化（底部横向均分）=====
    int btn_total_width = width - 40; // 总宽度（留左右边距）
    int btn_width = btn_total_width / 3; // 三个按钮均分
    int btn_y = height - 80; // 按钮Y坐标（底部）
    Fl_Button* select_btn = new Fl_Button(20, btn_y, btn_width, 40, "Choose file");
    Fl_Button* render_btn = new Fl_Button(20 + btn_width + 10, btn_y, btn_width, 40, "Execute");
    Fl_Button* save_btn = new Fl_Button(20 + 2*(btn_width + 10), btn_y, btn_width, 40, "Save PNG");

    // 绑定回调
    select_btn->callback(select_file_cb);
    render_btn->callback(render_cb);
    save_btn->callback(save_png_cb);

    // 样式优化
    select_btn->labelsize(14);
    render_btn->labelsize(14);
    save_btn->labelsize(14);
    // render_btn->color(FL_LIGHT_BLUE); // 渲染按钮高亮
    // save_btn->color(FL_LIGHT_GREEN);  // 保存按钮高亮

    win->end();
    win->resizable(display_box); // 窗口缩放时，显示区域自适应
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