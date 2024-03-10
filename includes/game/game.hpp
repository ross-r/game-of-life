#pragma once

#define NOMINMAX
#include <windows.h>

#include <dxgi.h>
#include <d3d11.h>

#include <memory>

#include <types.hpp>
#include <colour.hpp>

// forward delcarations.
namespace app {
  class Application;
  class Window;
}

namespace game {

  // For use in ImGui's custom user callback for commands.
  struct RenderCallbackData {
    ID3D11DeviceContext* context;
    ID3D11SamplerState* sampler;
  };

  class Game {
  private:
    bool m_draw_debug;

    // TODO: Probably best to use smart pointers but the scope of this class wont live beyond the pointers of app or window
    app::Application* m_app;
    app::Window* m_window;

    Vec2< size_t > m_bounds;

    // Our own custom texture that we write pixel data to.
    ID3D11Texture2D* m_staging;
    ID3D11Texture2D* m_texture;
    ID3D11ShaderResourceView* m_texture_resource;

    // Texture sampler since we don't want linear interpolation on textures.
    ID3D11SamplerState* m_texture_sampler;

    std::unique_ptr< uint32_t[] > m_pixel_buffer;

    std::unique_ptr< uint8_t[] > m_cells_current;
    std::unique_ptr< uint8_t[] > m_cells_next;
    
    float m_time_scale;

    Colour m_alive_colour;
    Colour m_dead_colour;

    // Temporary values that are used in ImGui colour picker.
    float m_temp_alive_colour[ 4 ];
    float m_temp_dead_colour[ 4 ];

    // Temporary values that are used in input fields to update grid size.
    size_t m_temp_size_x;
    size_t m_temp_size_y;

    bool m_running;

    RenderCallbackData m_callback_data;

  public:
    Game( app::Application* app, app::Window* window );

    ~Game();

    void reset();

    void init( const Vec2< size_t >& bounds );

    void update( const double t, const double dt );

    void draw();
  
  private:
    void create_texture_sampler();

    void update_texture();

    void update_pixel_buffer();

    void draw_debug_metrics();

    const uint8_t get_state( const size_t row, const size_t column );

    void set_state( const size_t row, const size_t column, const uint8_t state );

    void set_states( const size_t row, const size_t column, const uint8_t state );

    void set_pixel( const size_t x, const size_t y, const uint32_t colour );

    const size_t num_alive_neighbors( const size_t row, const size_t column );

    const uint32_t alive_colour() const;
    const uint32_t dead_colour() const;

    void update_colours();
  };

}