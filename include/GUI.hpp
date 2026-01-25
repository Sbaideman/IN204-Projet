#ifndef GUI_HPP
#define GUI_HPP

#pragma once
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/fl_ask.H>
#include <string>
#include <cstdio>

// 全局状态（存储选中文件、渲染状态）
struct AppState {
    std::string selected_file;  // 选中的待渲染文件路径
    bool is_rendered = false;   // 渲染完成标记
    void* render_buffer = nullptr; // 渲染结果缓冲区（替换为你的渲染数据类型）
    int buffer_width = 0;       // 渲染缓冲区宽
    int buffer_height = 0;      // 渲染缓冲区高
};

extern AppState app_state; // 全局状态实例

#endif