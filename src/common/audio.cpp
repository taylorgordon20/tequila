#include "src/common/audio.hpp"

#include <SFML/Audio.hpp>
#include <string>

#include "src/common/errors.hpp"
#include "src/common/files.hpp"

namespace tequila {

Sound::Sound(const std::string& file) {
  ENFORCE(buffer_.loadFromFile(resolvePathOrThrow(file)));
  sound_.setBuffer(buffer_);
}

void Sound::play() {
  sound_.play();
}

Music::Music(const std::string& file) {
  music_.openFromFile(resolvePathOrThrow(file));
  music_.setLoop(true);
  music_.setVolume(50.0f);
}

void Music::play() {
  music_.play();
}

void Music::pause() {
  music_.pause();
}

void Music::stop() {
  music_.pause();
}

}  // namespace tequila