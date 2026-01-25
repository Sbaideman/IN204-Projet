#include "GUI.hpp"
#include <png.h> // 需安装libpng（brew install libpng / apt install libpng-dev）

// 全局状态初始化
AppState app_state;

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
        fl_message("已选择文件：%s", app_state.selected_file.c_str());
    }
}

/**
 * @brief 执行渲染逻辑（需替换为你的项目渲染代码）
 */
void render_cb(Fl_Widget*, void*) {
    if (app_state.selected_file.empty()) {
        fl_alert("请先选择待渲染文件！");
        return;
    }

    // ========== 替换为你的实际渲染逻辑 ==========
    fl_message("开始渲染：%s", app_state.selected_file.c_str());
    
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
    fl_message("渲染完成！");
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
        fl_alert("请先完成渲染！");
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
            fl_message("PNG保存成功：%s", save_path.c_str());
        } else {
            fl_alert("PNG保存失败！");
        }
    }
}

/**
 * @brief 初始化GUI界面
 */
Fl_Window* init_gui(int width = 400, int height = 300) {
    // 注册图片格式（FLTK内置）
    // fl_register_images();

    // 创建主窗口
    Fl_Window* win = new Fl_Window(width, height, "渲染工具GUI");
    win->begin();

    // 按钮布局（选择文件、渲染、保存PNG）
    Fl_Button* select_btn = new Fl_Button(50, 50, 300, 40, "选择待渲染文件");
    Fl_Button* render_btn = new Fl_Button(50, 120, 300, 40, "执行渲染");
    Fl_Button* save_btn = new Fl_Button(50, 190, 300, 40, "保存渲染结果为PNG");

    // 绑定回调
    select_btn->callback(select_file_cb);
    render_btn->callback(render_cb);
    save_btn->callback(save_png_cb);

    // 样式适配
    select_btn->labelsize(14);
    render_btn->labelsize(14);
    save_btn->labelsize(14);

    win->end();
    // win->center(); // 窗口居中
    return win;
}

/**
 * @brief 清理资源（渲染缓冲区+GUI）
 */
void cleanup_resources(Fl_Window* win) {
    if (app_state.render_buffer) {
        free(app_state.render_buffer);
        app_state.render_buffer = nullptr;
    }
    if (win) {
        delete win;
    }
}