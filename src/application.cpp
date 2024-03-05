#include <application.hpp>

#define NOMINMAX
#include <windows.h>

#include <cstdio>
#include <algorithm>

#undef min
#undef max

app::Application::Application() {
  m_running = false;
  m_physics_interval = ( 1.0 / 60.0 );
  m_physics_time_scale = 1.0;
  m_physics_time = 0.0;
  m_delta_time = 0.0;
  m_frame_count = 0;
  m_frame_measure = 0.0;
  m_physics_remainder = 0.0;
}

app::Application::~Application() {}

void app::Application::exec( render_routine_t render_routine, physics_routine_t physics_routine ) {
  m_running = true;

  LARGE_INTEGER freq;
  QueryPerformanceFrequency( &freq );

  double accumulator = 0.0;

  LARGE_INTEGER current_time;
  QueryPerformanceCounter( &current_time );

  while( m_running ) {
    MSG msg;
    if( PeekMessageW( &msg, nullptr, 0, 0, PM_REMOVE ) != 0 ) {
      TranslateMessage( &msg );
      DispatchMessageW( &msg );

      if( msg.message == WM_QUIT ) {
        break;
      }
    }

    LARGE_INTEGER new_time;
    QueryPerformanceCounter( &new_time );
    
    // Calculate delta time and ceil it to 0.25 (1/4th of 1 second, 4 frames / sec, min.)
    m_delta_time = std::min( ( double ) ( new_time.QuadPart - current_time.QuadPart ) / freq.QuadPart, 0.25 );
    
    current_time = new_time;

    //
    // Physics update
    //
    {
      accumulator += m_delta_time;

      // Allow the interval to be scaled by a factor - this way we can better debug physics updates and such.
      const double physics_interval_scaled = m_physics_interval * ( 1.0 / m_physics_time_scale );

      while( accumulator >= physics_interval_scaled ) {
        physics_routine( *this, m_physics_time, physics_interval_scaled );

        m_physics_time += physics_interval_scaled;
        accumulator -= physics_interval_scaled;
      }

      m_physics_remainder = accumulator / physics_interval_scaled;
    }

    // lerp:  value * m_physics_remainder + prev_value * ( 1.0 - m_physics_remainder )
    //        
    //        if this was a game engine, i'd invoke functions to take snapshots of game states
    //        for rendering interpolation

    //
    // Render update
    //
    render_routine( *this, m_delta_time );

    // Update frame metrics.
    m_frame_count++;
    m_frame_measure = ( m_frame_measure * 0.9F ) + ( m_delta_time * ( 1.F - 0.9F ) );
  }
}

void app::Application::close() {
  m_running = false;
}