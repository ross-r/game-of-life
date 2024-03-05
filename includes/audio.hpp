#pragma once

#include <singleton.hpp>

#include <xaudio2.h>
#include <memory>
#include <string>

namespace app {
  
  //
  // Allows the ability to play audio files in the .WAV format.
  //
  class Audio {
  private:
    std::wstring m_file_name;
    std::unique_ptr< uint8_t[] > m_buffer;
    size_t m_buffer_size;
    IXAudio2SourceVoice* m_source_voice;
    HANDLE m_file_handle;

    float m_volume;
    float m_frequency;

  private:
    bool read_file();

  public:
    Audio();
    Audio( const std::wstring_view& file_name );
    ~Audio();

    void set_frequency( const float frequency );
    void set_volume( const float volume );

    void play( const bool loop = false );
    void stop();
  };

  //
  // Initializes XAudio2
  //
  class AudioEngine : public Singleton< AudioEngine > {
  private:
    IXAudio2* m_xaudio;
    IXAudio2MasteringVoice* m_master_voice;

    bool init();

  public:
    AudioEngine();
    ~AudioEngine();

    void shutdown();

    IXAudio2* xaudio() {
      return m_xaudio;
    }

    IXAudio2MasteringVoice* master_voice() {
      return m_master_voice;
    }
  };

}