#pragma once

#include <game/types.hpp>
#include <game/pathfinder.hpp>

namespace game {

  class Snake;

  class Bot {
  private:
    PathFinder m_path_finder;

    Snake& m_snake;

  public:
    Bot( Snake& snake );

    void update();
  };

}