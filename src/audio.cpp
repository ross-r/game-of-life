#include <audio.hpp>

#define NOMINMAX
#include <windows.h>

//
// Utility functions taken from MSDN
//
namespace {

  #define fourccRIFF 'FFIR'
  #define fourccDATA 'atad'
  #define fourccFMT ' tmf'
  #define fourccWAVE 'EVAW'
  #define fourccXWMA 'AMWX'
  #define fourccDPDS 'sdpd'

  HRESULT FindChunk( HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition ) {
    HRESULT hr = S_OK;
    if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ) )
      return HRESULT_FROM_WIN32( GetLastError() );

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while( hr == S_OK ) {
      DWORD dwRead;
      if( 0 == ReadFile( hFile, &dwChunkType, sizeof( DWORD ), &dwRead, NULL ) )
        hr = HRESULT_FROM_WIN32( GetLastError() );

      if( 0 == ReadFile( hFile, &dwChunkDataSize, sizeof( DWORD ), &dwRead, NULL ) )
        hr = HRESULT_FROM_WIN32( GetLastError() );

      switch( dwChunkType ) {
      case 'FFIR':
        dwRIFFDataSize = dwChunkDataSize;
        dwChunkDataSize = 4;
        if( 0 == ReadFile( hFile, &dwFileType, sizeof( DWORD ), &dwRead, NULL ) )
          hr = HRESULT_FROM_WIN32( GetLastError() );
        break;

      default:
        if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, dwChunkDataSize, NULL, FILE_CURRENT ) )
          return HRESULT_FROM_WIN32( GetLastError() );
      }

      dwOffset += sizeof( DWORD ) * 2;

      if( dwChunkType == fourcc ) {
        dwChunkSize = dwChunkDataSize;
        dwChunkDataPosition = dwOffset;
        return S_OK;
      }

      dwOffset += dwChunkDataSize;

      if( bytesRead >= dwRIFFDataSize ) return S_FALSE;
    }

    return S_OK;

  }

  HRESULT ReadChunkData( HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset ) {
    HRESULT hr = S_OK;
    if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, bufferoffset, NULL, FILE_BEGIN ) )
      return HRESULT_FROM_WIN32( GetLastError() );
    DWORD dwRead;
    if( 0 == ReadFile( hFile, buffer, buffersize, &dwRead, NULL ) )
      hr = HRESULT_FROM_WIN32( GetLastError() );
    return hr;
  }

}

app::AudioEngine::AudioEngine() : m_xaudio( nullptr ), m_master_voice( nullptr ) {
  if( !init() ) {
    // ...
  }
}

app::AudioEngine::~AudioEngine() {
  shutdown();
}

void app::AudioEngine::shutdown() {
  if( m_master_voice ) {
    m_master_voice->DestroyVoice();
    m_master_voice = nullptr;
  }

  if( m_xaudio ) {
    m_xaudio->StopEngine();
    m_xaudio->Release();
    m_xaudio = nullptr;
  }
}

bool app::AudioEngine::init() {
  //
  // https://learn.microsoft.com/en-us/windows/win32/xaudio2/how-to--initialize-xaudio2
  //

  if( m_xaudio != nullptr || m_master_voice != nullptr ) {
    return false;
  }

  HRESULT hr = S_OK;
  if( FAILED( hr = CoInitializeEx( nullptr, COINIT_MULTITHREADED ) ) ) {
    return false;
  }

  if( FAILED( hr = XAudio2Create( &m_xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR ) ) ) {
    return false;
  }

  if( FAILED( hr = m_xaudio->CreateMasteringVoice( &m_master_voice ) ) ) {
    return false;
  }

  if( FAILED( hr = m_xaudio->StartEngine() ) ) {
    return false;
  }

  return hr == S_OK;
}

app::Audio::Audio() :
  m_file_name(),
  m_buffer( nullptr ),
  m_buffer_size( 0 ),
  m_source_voice( nullptr ),
  m_file_handle( nullptr ),
  m_volume( 1.F ),
  m_frequency( 1.F ) {}

app::Audio::Audio( const std::wstring_view& file_name ) : Audio() {
  m_file_name = file_name;
  read_file();
}

app::Audio::~Audio() {
  m_buffer.release();
  m_buffer = nullptr;
  m_buffer_size = 0;
}

bool app::Audio::read_file() {
  HRESULT hr = S_OK;

  if( m_file_handle ) {
    CloseHandle( m_file_handle );
    m_file_handle = nullptr;
  }

  m_file_handle = CreateFileW(
    m_file_name.c_str(),
    GENERIC_READ,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    0,
    NULL
  );

  if( m_file_handle == INVALID_HANDLE_VALUE ) {
    return false;
  }

  if( SetFilePointer(
    m_file_handle,
    0,
    NULL,
    FILE_BEGIN
  ) == INVALID_SET_FILE_POINTER ) {
    return false;
  }

  DWORD chunk_size;
  DWORD chunk_pos;
  DWORD audio_type;

  if( FAILED( hr = FindChunk( m_file_handle, fourccRIFF, chunk_size, chunk_pos ) ) ) {
    return false;
  }

  if( FAILED( hr = ReadChunkData( m_file_handle, &audio_type, sizeof( DWORD ), chunk_pos ) ) ) {
    return false;
  }

  if( audio_type != fourccWAVE ) {
    return false;
  }

  WAVEFORMATEXTENSIBLE wfx = { 0 };
  FindChunk( m_file_handle, fourccFMT, chunk_size, chunk_pos );
  ReadChunkData( m_file_handle, &wfx, chunk_size, chunk_pos );

  FindChunk( m_file_handle, fourccDATA, chunk_size, chunk_pos );
  
  m_buffer = std::make_unique< uint8_t[] >( chunk_size );
  m_buffer_size = chunk_size;
  ReadChunkData( m_file_handle, m_buffer.get(), chunk_size, chunk_pos );

  AudioEngine::get()->xaudio()->CreateSourceVoice( &m_source_voice, &wfx.Format );

  return hr == S_OK;
}

void app::Audio::set_frequency( const float frequency ) {
  m_frequency = frequency;

  if( m_source_voice != nullptr ) {
    m_source_voice->SetFrequencyRatio( m_frequency );
  }
}

void app::Audio::set_volume( const float volume ) {
  m_volume = volume;

  if( m_source_voice != nullptr ) {
    m_source_voice->SetVolume( m_volume );
  }
}

void app::Audio::play( const bool loop ) {
  if( m_source_voice == nullptr ) {
    return;
  }

  XAUDIO2_BUFFER buffer = { 0 };
  buffer.AudioBytes = m_buffer_size;
  buffer.pAudioData = m_buffer.get();
  buffer.Flags = XAUDIO2_END_OF_STREAM;
  buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
  m_source_voice->SubmitSourceBuffer( &buffer );
  m_source_voice->SetVolume( m_volume );
  m_source_voice->SetFrequencyRatio( 1.F );
  m_source_voice->Start( 0 );
}

void app::Audio::stop() {
  if( m_source_voice == nullptr ) {
    return;
  }

  m_source_voice->Stop();
  m_source_voice->FlushSourceBuffers();
}