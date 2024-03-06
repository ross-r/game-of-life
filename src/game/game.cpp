#include <game/game.hpp>

#include <ext/imgui/imgui.h>

#include <application.hpp>
#include <window.hpp>
#include <game/colour.hpp>

#include <algorithm>

#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>

#include <random>

game::Game::Game() {
  m_draw_metrics = true;
  m_paused = false;
}

void game::Game::init( const app::Application& app, const app::Window& window ) {
  m_snake.init( window );
}

void game::Game::resize( const app::Application& app, const app::Window& window ) {}

void game::Game::update( app::Application& app, const double t, const double dt ) {
  app.set_time_scale( 1.0 );

  //
  // Check game over conditions before anything.
  //
  if( m_paused ) {
    return;
  }

  m_snake.update( app, t, dt );
}

void game::Game::draw( const app::Application& app, const app::Window& window ) {
  if( ImGui::IsKeyPressed( ImGuiKey_P ) ) {
    m_paused = !m_paused;
  }

  ImDrawList* draw_list = ImGui::GetForegroundDrawList();
  ImFont* font = ImGui::GetFont();

  const float window_center_x = ( window.width() / 2 );
  const float window_center_y = ( window.height() / 2 );

  m_snake.draw( app, window );

  if( m_paused ) {
    const char* paused_str = "GAME PAUSED";
    const auto& paused_text_size = font->CalcTextSizeA( 32.F, 0.F, 0.F, paused_str );

    draw_list->AddRectFilled( { 0.F, 0.F }, { ( float ) window.width(), ( float ) window.height() }, 0x7F000000 );
    draw_list->AddText( { window_center_x - ( paused_text_size.x / 2.F ), window_center_y - ( paused_text_size.y / 2.F ) }, 0xFFFFFFFF, paused_str );
  }

  if( m_draw_metrics ) {
    char buf[ 256 ] = { '\0' };
    sprintf_s( buf, "FPS: %.0F (%.8F - %.8F)", app.frames_per_second(), app.delta_time(), app.physics_remainder() );
    draw_list->AddText( { 2.F, 2.F }, 0xFFFFFFFF, buf );
  }
}