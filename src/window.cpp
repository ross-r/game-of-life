#include <window.hpp>

#define NOMINMAX
#include <windows.h>

#include <ext/imgui/imgui.h>
#include <imgui/imgui_impl_win32.hpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

app::global_win_proc_list g_global_win_proc_list = {};

LRESULT __stdcall app::global_winproc( HWND window, UINT message, WPARAM wparam, LPARAM lparam ) {
  // Call all window callbacks to handle windows messages.
  if( !g_global_win_proc_list.empty() ) {
    for( const auto& def : g_global_win_proc_list ) {
      // Make sure the message we're intercepting is for the correct window.
      if( def.m_window_handle != window ) {
        continue;
      }

      // If the handler is a valid function pointer, call it.
      if( def.m_handler != nullptr ) {
        const bool bRet = def.m_handler( message, wparam, lparam );

        // If the handler does not want to call into the default behaviour, exit out now.
        if( !bRet ) {
          return true;
        }
      }
    }
  }

  //switch( message ) {
  //  case WM_DESTROY:
  //    PostQuitMessage( 0 );
  //    return 0;
  //}

  // Pass through to the operating system to handle this message for us automatically.
  return DefWindowProcW( window, message, wparam, lparam );
}

const HWND app::Window::create() {
  const WNDCLASSEX wndclass = {
    sizeof( WNDCLASSEX ),
    CS_DBLCLKS | CS_DROPSHADOW,
    global_winproc,
    0,
    0,
    GetModuleHandleA( nullptr ),
    nullptr,
    LoadCursor( nullptr, IDC_ARROW ),
    HBRUSH( COLOR_WINDOWFRAME ),
    nullptr,
    m_class_name.c_str(),
    nullptr
  };

  if( RegisterClassExW( &wndclass ) == 0 ) {
    return {};
  }

  return CreateWindowExW(
    NULL,
    m_class_name.c_str(),
    m_window_title.c_str(),
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    m_width,
    m_height,
    nullptr,
    nullptr,
    GetModuleHandleA( nullptr ),
    nullptr
  );
}

app::Window::Window() : 
  m_class_name(), 
  m_window_title(), 
  m_width(), 
  m_height(), 
  m_handle( nullptr ),
  m_imgui_context( nullptr ) {}

app::Window::Window( 
  const std::wstring_view& title, 
  const int width, 
  const int height 
) : Window( TEXT( "AppClass001" ), title, width, height ) {
  // ..
}

app::Window::Window( 
  const std::wstring_view& class_name, 
  const std::wstring_view& title, 
  const int width, 
  const int height 
) : Window() {
  m_class_name = class_name;
  m_window_title = title;
  m_width = width;
  m_height = height;
  m_handle = create();

  if( !m_renderer.initialize( *this ) ) {
    // TODO: raise error
    printf( "Failed to initialize renderer instance for window %p\n", m_handle );
  }

  m_imgui_context = m_renderer.imgui_context();
}

void app::Window::shutdown() {
  m_renderer.shutdown();

  if( m_handle ) {
    CloseWindow( m_handle );
    m_handle = nullptr;
  }
}

const void app::Window::show() const {
  if( m_handle == nullptr ) {
    // TODO: raise an error
    return;
  }

  ShowWindow( m_handle, SW_SHOWDEFAULT );
}

const void app::Window::hide() const {
  ShowWindow( m_handle, SW_HIDE );
}

const void app::Window::center() const {
  if( m_handle == nullptr ) {
    // TODO: raise an error
    return;
  }

  // TODO: Allow choice of desktop window if multi-desktop is used.

  // Get the desktop bounds.
  // In Windows, this is the desktop that is considered the "primary" one.
  RECT desktoprect{};
  GetWindowRect( GetDesktopWindow(), &desktoprect );

  // Get the windows width and height.
  RECT winrect{};
  GetWindowRect( m_handle, &winrect );

  const int width = static_cast< int >( winrect.right - winrect.left );
  const int height = static_cast< int >( winrect.bottom - winrect.top );
  
  // Move the window into the center of the desktop, resize the window if needed and repaint it.
  MoveWindow( m_handle,
              ( ( desktoprect.right - desktoprect.left ) / 2 ) - ( width / 2 ),
              ( ( desktoprect.bottom - desktoprect.top ) / 2 ) - ( height / 2 ),
              width, height, TRUE );
}

const HWND app::Window::handle() const {
  return m_handle;
}

const int app::Window::width() const {
  return m_width;
}

const int app::Window::height() const {
  return m_height;
}

void app::Window::set_message_handler( const message_handler_t& handler ) {
  g_global_win_proc_list.push_back( message_handler_defintiion_t{ m_handle, &handler } );
}

bool app::Window::imgui_message_handler( UINT message, WPARAM wparam, LPARAM lparam ) {
  ImGui::SetCurrentContext( ( ImGuiContext* ) m_imgui_context );

  if( ImGui_ImplWin32_WndProcHandler( m_handle, message, wparam, lparam ) ) {
    return true;
  }

  return false;
}

void app::Window::draw( void( *draw_routine )( Renderer& renderer ) ) {
  if( draw_routine == nullptr ) {
    return;
  }

  m_renderer.begin();

  draw_routine( m_renderer );

  m_renderer.end();
}
