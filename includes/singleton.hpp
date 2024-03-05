#pragma once

template< typename T >
class Singleton {
protected:
  inline static T* m_instance = nullptr;

public:
  static T* get() {
    if( m_instance == nullptr ) {
      m_instance = new T();
    }

    return m_instance;
  }
};