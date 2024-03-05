#pragma once

#define NOMINMAX
#include <windows.h>

#include <string>
#include <string_view>
#include <vector>

#include <renderer.hpp>

namespace app {

  using message_handler_t = bool( UINT, WPARAM, LPARAM );

  struct message_handler_defintiion_t {
    HWND m_window_handle;
    message_handler_t* m_handler;
  };

  using global_win_proc_list = std::vector< message_handler_defintiion_t >;
  
  LRESULT __stdcall global_winproc( HWND, UINT, WPARAM, LPARAM );

  class Window {
  private:
    std::wstring m_class_name;
    std::wstring m_window_title;

    int m_width;
    int m_height;

    HWND m_handle;

    // Special case for ImGui implementation so we can set the correct context when intercepting messages.
    void* m_imgui_context;

    // Renderer instance.
    Renderer m_renderer;

  private:
    const HWND create();

  public:
    Window();
    Window( const std::wstring_view& title, const int width, const int height );
    Window( const std::wstring_view& class_name, const std::wstring_view& title, const int width, const int height );
    
  public:
    void shutdown();

    const void show() const;
    const void hide() const;
    const void center() const;

  public:
    const HWND handle() const;
    const int width() const;
    const int height() const;

    Renderer& renderer() {
      return m_renderer;
    }

  public:
    void set_message_handler( const message_handler_t& handler );

    // Calls the ImGui message handler for the window.
    bool imgui_message_handler( UINT message, WPARAM wparam, LPARAM lparam );

    // Draws the window.
    //    The routine should take in a Renderer reference for ease of use.
    void draw( void( *draw_routine )( Renderer& renderer ) );
  };

}