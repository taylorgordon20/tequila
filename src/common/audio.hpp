#pragma once

#include <SFML/Audio.hpp>
#include <string>

namespace tequila {

class Sound {
 public:
  Sound(const std::string& file);
  void play();

 private:
  sf::SoundBuffer buffer_;
  sf::Sound sound_;
};

class Music {
 public:
  Music(const std::string& file);
  void play();
  void pause();
  void stop();

 private:
  sf::Music music_;
};

}  // namespace tequila