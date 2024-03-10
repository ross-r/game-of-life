#pragma once

// forward delcarations.
namespace app {
  class Application;
  class Window;
}

template< typename T >
struct Vec2 {
  T x;
  T y;

  const bool operator==( const Vec2< T >& other ) const {
    return x == other.x && y == other.y;
  }
};

using Vec2i = Vec2< int >;
using Vec2f = Vec2< float >;

// For better code readability.
using Tile = Vec2i;
using Point = Vec2f;

using SizeI = Vec2i;
using SizeF = Vec2f;