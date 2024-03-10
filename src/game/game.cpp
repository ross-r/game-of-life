#include <game/game.hpp>

#include <application.hpp>
#include <window.hpp>

#include <ext/imgui/imgui.h>

#include <colour.hpp>

#include <memory>
#include <random>
#include <algorithm>

template< typename T >
T clamp( const T value, const T min, const T max ) {
  return std::max( std::min( value, max ), min );
}

//
// TODO:  Optimizations are needed.
//        The game can scale exponentially large, i.e., x*y pixels (*4 for rgba)
//        Iterating over every single row and column in a single loop and thread is just not good enough, additionally, using a 1D array seems to cause a lot of cache misses, nearby row column pairs could have
//        largely differing indices and that is suboptimal, iterating over all neighbors for every single pixel will produce thousands of cache misses and making reading/writing memory
//        painfully slow, in reality, this should just be a pixel shader.
//
//  Possible optimization techniques could be:
//    spatial grids combined with parallel processing.
//    double buffered cell parallel processing.
//
// Space saving considerations:
//    use a bit operations to represent 8 cells in a single bytem we only need a 0 or 1 for dead or alive
//

game::Game::Game( app::Application* app, app::Window* window ) :
  m_app( app ),
  m_window( window ),
  m_temp_alive_colour{ 1.F, 1.F, 1.F, 1.F },
  m_temp_dead_colour{ 0.F, 0.F, 0.F, 1.F },
  m_bounds{},
  m_staging{},
  m_texture{},
  m_texture_resource{}
{
  m_draw_debug = true;
  m_time_scale = 1.F;
  m_running = false;
  m_texture_zoom_scale = 1.F;

  update_colours();
}

game::Game::~Game() {
  reset();
}

void game::Game::reset() {
  if( m_staging ) {
    m_staging->Release();
    m_staging = nullptr;
  }

  if( m_texture ) {
    m_texture->Release();
    m_texture = nullptr;
  }

  if( m_texture_resource ) {
    m_texture_resource->Release();
    m_texture_resource = nullptr;
  }

  if( m_pixel_buffer ) {
    m_pixel_buffer.release();
    m_pixel_buffer = nullptr;
  }

  if( m_cells_current ) {
    m_cells_current.release();
    m_cells_current = nullptr;
  }

  if( m_cells_next ) {
    m_cells_next.release();
    m_cells_next = nullptr;
  }
}

void game::Game::init( const Vec2< size_t >& bounds ) {
  reset();

  auto& renderer = m_window->renderer();
  auto device = renderer.device();
  auto context = renderer.context();

  m_bounds = bounds;

  // Just update these so the UI reflects it.
  m_temp_size_x = m_bounds.x;
  m_temp_size_y = m_bounds.y;

  m_pixel_buffer = std::make_unique< uint32_t[] >( m_bounds.x * m_bounds.y );
  memset( m_pixel_buffer.get(), dead_colour(), sizeof( uint32_t ) * m_bounds.x * m_bounds.y );

  m_cells_current = std::make_unique< uint8_t[] >( ( m_bounds.x + 2 ) * ( m_bounds.y + 2 ) );
  memset( m_cells_current.get(), 0, m_bounds.x * m_bounds.y );

  m_cells_next = std::make_unique< uint8_t[] >( ( m_bounds.x + 2 ) * ( m_bounds.y + 2 ) );
  memset( m_cells_next.get(), 0, m_bounds.x * m_bounds.y );

  HRESULT hr = S_OK;

  {
    D3D11_TEXTURE2D_DESC textureDescription{};
    memset( &textureDescription, 0, sizeof( textureDescription ) );

    textureDescription.Width = m_bounds.x;
    textureDescription.Height = m_bounds.y;
    textureDescription.ArraySize = 1;
    textureDescription.SampleDesc.Count = 1;
    textureDescription.SampleDesc.Quality = 0;
    textureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDescription.Usage = D3D11_USAGE_STAGING;
    textureDescription.BindFlags = 0;
    textureDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    textureDescription.MiscFlags = 0;
    textureDescription.MipLevels = 1;

    if( FAILED( hr = device->CreateTexture2D( &textureDescription, nullptr, &m_staging ) ) ) {
      return;
    }
  }

  {
    D3D11_TEXTURE2D_DESC textureDescription{};
    memset( &textureDescription, 0, sizeof( textureDescription ) );

    textureDescription.Width = m_bounds.x;
    textureDescription.Height = m_bounds.y;
    textureDescription.ArraySize = 1;
    textureDescription.SampleDesc.Count = 1;
    textureDescription.SampleDesc.Quality = 0;
    textureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDescription.Usage = D3D11_USAGE_DEFAULT;
    textureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDescription.CPUAccessFlags = 0;
    textureDescription.MiscFlags = 0;
    textureDescription.MipLevels = 1;

    if( FAILED( hr = device->CreateTexture2D( &textureDescription, nullptr, &m_texture ) ) ) {
      return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory( &srvDesc, sizeof( srvDesc ) );
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = textureDescription.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    if( FAILED( hr = device->CreateShaderResourceView( m_texture, &srvDesc, &m_texture_resource ) ) ) {
      return;
    }
  }
}

void game::Game::update( const double t, const double dt ) {
  //
  // Conway's Game of Life
  //    1. Any live cell with fewer than two live neighbors dies, as if by underpopulation.
  //    2. Any live cell with two or three live neighbors lives on to the next generation.
  //    3. Any live cell with more than three live neighbors dies, as if by overpopulation.
  //    4. Any dead cell with exactly three live neighbors becomes a live cell, as if by reproduction.
  //

  m_app->set_time_scale( m_time_scale );

  if( !m_running ) {
    return;
  }

  const size_t rows = m_bounds.x;
  const size_t columns = m_bounds.y;

  for( size_t column{ 1 }; column <= columns; ++column ) {
    for( size_t row{ 1 }; row <= rows; ++row ) {
      const uint8_t state = get_state( row, column );
      const size_t live_neighbors = num_alive_neighbors( row, column );

      if( live_neighbors == 3 ) {
        set_state( row, column, 1 );
      } 
      else if( live_neighbors == 2 ) {
        set_state( row, column, state );
      }
      else if( !( live_neighbors == 3 || live_neighbors == 2 ) ) {
        set_state( row, column, 0 );
      }
    }
  }

  std::swap( m_cells_current, m_cells_next );
}

void game::Game::draw() {
  ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

  if( ImGui::IsKeyPressed( ImGuiKey_G ) ) {
    m_draw_debug = !m_draw_debug;
  }

  if( ImGui::IsKeyPressed( ImGuiKey_R ) ) {
    m_running = !m_running;
  }

  draw_debug_metrics();

  const auto& mouse = ImGui::GetMousePos();

  // If the game is not running, let the user select which pixels are alive before the simulation begins again.
  if( !m_running ) {
    const float remapped_mouse_x = clamp( ( mouse.x / m_window->width() ) * m_bounds.x, 0.F, ( float ) m_bounds.x );
    const float remapped_mouse_y = clamp( ( mouse.y / m_window->height() ) * m_bounds.y, 0.F, ( float ) m_bounds.y );

    if( ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
      const size_t row = ( size_t ) remapped_mouse_y;
      const size_t column = ( size_t ) remapped_mouse_x;

      set_states( row, column, 1 );
    }
  }

  if( m_texture_resource == nullptr ) {
    return;
  }

  update_pixel_buffer();
  update_texture();

  // TODO: Finish this, if the zoom level is >1 we want to use the cursor x/y as the start of the text relative to the center + zoom offset.
  float center_x = ( float ) ( ( m_window->width() ) / 2.F ) - ( ( m_window->width() ) / 2.F ) * m_texture_zoom_scale;
  float center_y = ( float ) ( ( m_window->height() ) / 2.F ) - ( ( m_window->height() ) / 2.F ) * m_texture_zoom_scale;
  
  // Draw the texture and stretch it out to the full window size regardless of the actual textures size.
  draw_list->AddImage(
    m_texture_resource,
    { center_x, center_y },
    { ( float ) m_window->width() * m_texture_zoom_scale, ( float ) m_window->height() * m_texture_zoom_scale }
  );
}

void game::Game::update_texture() {
  //
  // Copies the pixel buffer to the staging textures buffer.
  //

  auto& renderer = m_window->renderer();
  auto device = renderer.device();
  auto context = renderer.context();

  if( m_staging == nullptr || m_texture == nullptr ) {
    return;
  }

  D3D11_MAPPED_SUBRESOURCE subresource;
  if( FAILED( context->Map(
    m_staging,
    0,
    D3D11_MAP_WRITE,
    0,
    &subresource
  ) ) ) {
    return;
  }

  if( subresource.pData == nullptr ) {
    return;
  }

  memcpy(
    subresource.pData,
    m_pixel_buffer.get(),
    sizeof( uint32_t ) * m_bounds.x * m_bounds.y
  );

  context->Unmap( m_staging, 0 );

  //
  // Copy the staging texture to the texture that has a shader resource bound to it.
  //
  context->CopyResource( m_texture, m_staging );
}

void game::Game::update_pixel_buffer() {
  const size_t rows = m_bounds.x;
  const size_t columns = m_bounds.y;

  for( size_t column{ 1 }; column <= columns; ++column ) {
    for( size_t row{ 1 }; row <= rows; ++row ) {
      const uint8_t state = get_state( row, column );
      const uint32_t colour = ( state == 1 ) ? alive_colour() : dead_colour();
      set_pixel( row, column, colour );
    }
  }
}

void game::Game::draw_debug_metrics() {
  if( !m_draw_debug ) {
    return;
  }

  ImGui::Begin( "Settings" );
  {
    ImGui::Text( "FPS: %.2f (%.8f)", m_app->frames_per_second(), m_app->delta_time() );

    ImGui::Checkbox( "Run", &m_running );
    ImGui::SliderFloat( "Time Scale", &m_time_scale, 0.01F, 2.F );

    ImGui::InputScalar( "Grid Size X", ImGuiDataType_U64, &m_temp_size_x );
    ImGui::InputScalar( "Grid Size Y", ImGuiDataType_U64, &m_temp_size_y );

    //ImGui::SliderFloat( "Zoom", &m_texture_zoom_scale, 1.F, 10.F );

    if( ImGui::Button( "Reset" ) ) {
      m_running = false;
      init( { m_temp_size_x, m_temp_size_y } );
    }

    if( ImGui::Button( "Random" ) ) {
      m_running = false;

      init( m_bounds );

      std::random_device r;
      std::default_random_engine e1( r() );
      std::uniform_int_distribution< int > uniform_dist( 0, 1 );

      for( size_t column{ 1 }; column <= m_bounds.y; ++column ) {
        for( size_t row{ 1 }; row <= m_bounds.x; ++row ) {
          set_states( row, column, uniform_dist( e1 ) );
        }
      }

      m_running = true;
    }

    {
      bool update = false;

      update = update || ImGui::ColorPicker4( "Alive Colour", m_temp_alive_colour );
      update = update || ImGui::ColorPicker4( "Dead Colour", m_temp_dead_colour );

      if( update ) {
        update_colours();
      }
    }

    ImGui::End();
  }
}

const uint8_t game::Game::get_state( const size_t row, const size_t column ) {
  return m_cells_current[ row * ( m_bounds.x + 2 ) + column ];
}

void game::Game::set_state( const size_t row, const size_t column, const uint8_t state ) {
  m_cells_next[ row * ( m_bounds.x + 2 ) + column ] = state;
}

void game::Game::set_states( const size_t row, const size_t column, const uint8_t state ) {
  m_cells_current[ row * ( m_bounds.x + 2 ) + column ] = state;
  m_cells_next[ row * ( m_bounds.x + 2 ) + column ] = state;
}

void game::Game::set_pixel( const size_t x, const size_t y, const uint32_t colour ) {
  const size_t index = ( x - 1 ) * m_bounds.x + ( y - 1 );
  m_pixel_buffer[ index ] = colour;
}

const size_t game::Game::num_alive_neighbors( const size_t row, const size_t column ) {
  return m_cells_current[ ( row - 1 ) * ( m_bounds.x + 2 ) + ( column ) ] +
    m_cells_current[ ( row ) * ( m_bounds.x + 2 ) + ( column - 1 ) ] +
    m_cells_current[ ( row - 1 ) * ( m_bounds.x + 2 ) + ( column - 1 ) ] +
    m_cells_current[ ( row + 1 ) * ( m_bounds.x + 2 ) + ( column ) ] +
    m_cells_current[ ( row ) * ( m_bounds.x + 2 ) + ( column + 1 ) ] +
    m_cells_current[ ( row + 1 ) * ( m_bounds.x + 2 ) + ( column + 1 ) ] +
    m_cells_current[ ( row + 1 ) * ( m_bounds.x + 2 ) + ( column - 1 ) ] +
    m_cells_current[ ( row - 1 ) * ( m_bounds.x + 2 ) + ( column + 1 ) ];
}

const uint32_t game::Game::alive_colour() const {
  return m_alive_colour.argb();
}

const uint32_t game::Game::dead_colour() const {
  return m_dead_colour.argb();
}

void game::Game::update_colours() {
  m_alive_colour = Colour{
    ( uint8_t ) ( m_temp_alive_colour[ 0 ] * 255.F ),
    ( uint8_t ) ( m_temp_alive_colour[ 1 ] * 255.F ),
    ( uint8_t ) ( m_temp_alive_colour[ 2 ] * 255.F ),
    ( uint8_t ) ( m_temp_alive_colour[ 3 ] * 255.F )
  };

  m_dead_colour = Colour{
    ( uint8_t ) ( m_temp_dead_colour[ 0 ] * 255.F ),
    ( uint8_t ) ( m_temp_dead_colour[ 1 ] * 255.F ),
    ( uint8_t ) ( m_temp_dead_colour[ 2 ] * 255.F ),
    ( uint8_t ) ( m_temp_dead_colour[ 3 ] * 255.F )
  };
}
