#pragma once

//
// Might be a good idea to contain all instances of Window objects
// inside of the Application class, that way I can track when a window is open or closed
// and prevent closing the application until window related resources have been properly
// cleaned up, as it stands, the deconstructor for the window class will be invoked
// upon application exit and thus any allocations and such can be handled independently of
// the Application class.
//
// I'm not entirely sure on which concept is the more prefered design paradigm.
//
// If we tracked each window locally we can hash the window name and use a hash map
// and that way we can grab the window via a routine, something like:
//    win = app.find_window( name )
//    win.call_something()
//    ...
//
// Maybe the above is better.
//

namespace app {

  class Application;

  //
  // Render routine called from main application loop.
  // 
  //    app: referenced to instance of current application context
  //    dt: "current" frame delta time
  //
  using render_routine_t = void( __cdecl* )( Application& app, const double dt );

  //
  // Physics routine called from main application loop.
  //    
  //    t: total time accumulated
  //    dt: "current" frame delta time
  //
  using physics_routine_t = void( __cdecl* )( Application& app, const double t, const double dt );

  class Application {
  private:
    bool m_running;

    int m_frame_count;
    double m_frame_measure;
    double m_delta_time;

    double m_physics_interval;
    double m_physics_time;
    double m_physics_remainder;
    double m_physics_time_scale;

  public:
    Application();
    ~Application();

  public:
    void exec( render_routine_t render_routine, physics_routine_t physics_routine );
    void close();

    const float delta_time() const {
      return m_delta_time;
    }

    const float frames_per_second() const {
      return 1.F / m_frame_measure;
    }

    const double time_scale() const {
      return m_physics_time_scale;
    }

    void set_time_scale( const double time_scale ) {
      m_physics_time_scale = time_scale;
    }

    const double physics_time() const {
      return m_physics_time;
    }

    const double physics_remainder() const {
      return m_physics_remainder;
    }
  };

}