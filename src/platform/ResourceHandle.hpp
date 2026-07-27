#pragma once
#include "raylib.h"


// =========================================================================
// [Chapter 43 추가] RAII 기반 오디오 디바이스 관리자
// =========================================================================
class AudioDeviceRAII final {
public:
    AudioDeviceRAII() noexcept {
        InitAudioDevice(); // 생성 시 즉시 오디오 하드웨어 점유
    }
    ~AudioDeviceRAII() noexcept {
        CloseAudioDevice(); // 스코프 이탈 시 하드웨어 자원 안전하게 반환
    }
    // 오디오 디바이스는 단일(Singleton) 자원이므로 복사/이동 엄격히 금지
    AudioDeviceRAII(const AudioDeviceRAII&) = delete;
    AudioDeviceRAII& operator=(const AudioDeviceRAII&) = delete;
    AudioDeviceRAII(AudioDeviceRAII&&) = delete;
    AudioDeviceRAII& operator=(AudioDeviceRAII&&) = delete;
};

// =========================================================================
// [Chapter 43 추가] RAII 기반 사운드 관리자 (TextureHandle과 동일 구조)
// =========================================================================
class SoundHandle final {
private:
    Sound m_sound;
    bool m_isValid;

public:
    SoundHandle() noexcept : m_sound{}, m_isValid(false) {}

    explicit SoundHandle(const char* fileName) noexcept {
        m_sound = LoadSound(fileName);
        m_isValid = (m_sound.stream.buffer != nullptr);
    }

    // 복사 방지: Double-Free(이중 해제) 버그 차단[cite: 2]
    SoundHandle(const SoundHandle&) = delete;
    SoundHandle& operator=(const SoundHandle&) = delete;

    // 이동 시맨틱: 소유권만 O(1) 로 이전[cite: 2]
    SoundHandle(SoundHandle&& other) noexcept : m_sound(other.m_sound), m_isValid(other.m_isValid) {
        other.m_isValid = false; 
    }

    SoundHandle& operator=(SoundHandle&& other) noexcept {
        if (this != &other) {
            if (m_isValid) UnloadSound(m_sound); 
            m_sound = other.m_sound;
            m_isValid = other.m_isValid;
            other.m_isValid = false;
        }
        return *this;
    }

    ~SoundHandle() noexcept {
        if (m_isValid) {
            UnloadSound(m_sound); // 스코프 이탈 시 자동 메모리 해제[cite: 2]
        }
    }

    // 조건 분기(branch)를 최소화한 인라인 재생 함수
    inline void Play() const noexcept {
        if (m_isValid) PlaySound(m_sound);
    }
};

// =========================================================================
// [Chapter 39 추가] RAII 기반 텍스처 관리자 (Zero-Cost Abstraction)
// =========================================================================
// C 방식의 수동 메모리 관리를 C++ 소멸자로 자동화하여 메모리 누수를 원천 차단합니다.
class TextureHandle final {
private:
    Texture2D m_texture;
    bool m_isValid;

public:
    TextureHandle() noexcept : m_texture{}, m_isValid(false) {}

    // explicit을 통해 암시적 형변환을 막고, 생성 시 즉시 텍스처를 VRAM에 적재합니다.
    // [수정됨] 타겟 해상도와 투명화 처리할 색상을 인자로 추가로 받습니다.
    explicit TextureHandle(const char* fileName, int targetWidth = 0, int targetHeight = 0, Color colorKey = BLACK) noexcept {
        // 1. RAM으로 이미지 로드
        Image img = LoadImage(fileName);
        
        if (img.data != nullptr) {
            // 2. 지정된 색상(기본: 검정)을 투명(BLANK)으로 변환
            ImageColorReplace(&img, colorKey, BLANK);

            // 3. 타겟 사이즈가 지정되었다면 RAM 상태에서 리사이징 (게임 루프 렌더링 오버헤드 제거)
            if (targetWidth > 0 && targetHeight > 0) {
                ImageResize(&img, targetWidth, targetHeight);
            }

            // 4. 최적화된 이미지를 VRAM으로 업로드
            m_texture = LoadTextureFromImage(img);
            m_isValid = (m_texture.id != 0);

            // 5. RAM 데이터 즉시 해제 (Zero-allocation)
            UnloadImage(img);

        } else {
            m_isValid = false;
            m_texture = Texture2D{};
        }
    }

    // 복사 방지: Double-Free(이중 해제) 버그 차단
    TextureHandle(const TextureHandle&) = delete;
    TextureHandle& operator=(const TextureHandle&) = delete;

    // 이동 시맨틱: 소유권만 O(1) 로 이전
    TextureHandle(TextureHandle&& other) noexcept : m_texture(other.m_texture), m_isValid(other.m_isValid) {
        other.m_isValid = false; 
    }

    TextureHandle& operator=(TextureHandle&& other) noexcept {
        if (this != &other) {
            if (m_isValid) UnloadTexture(m_texture); 
            m_texture = other.m_texture;
            m_isValid = other.m_isValid;
            other.m_isValid = false;
        }
        return *this;
    }

    ~TextureHandle() noexcept {
        if (m_isValid) {
            UnloadTexture(m_texture); // 스코프 이탈 시 자동 메모리 해제
        }
    }

    inline const Texture2D& Get() const noexcept { return m_texture; }
    inline bool IsValid() const noexcept { return m_isValid; }
};