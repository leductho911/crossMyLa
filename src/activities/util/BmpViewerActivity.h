#pragma once

#include <functional>
#include <string>

#include "MappedInputManager.h"
#include "activities/Activity.h"
#include "components/OptionPopup.h"

class BmpViewerActivity final : public Activity {
 public:
  BmpViewerActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string filePath);

  void onEnter() override;
  void onExit() override;
  void loop() override;

 private:
  void loadSiblingImages();
  void doSetSleepCover();
  bool canSetSleepCover() const;
  void doSetHomeWallpaper();
  bool canSetHomeWallpaper() const;
  bool renderPng();

  std::string filePath;
  std::vector<std::string> siblingImages;
  int currentImageIndex = -1;
  OptionPopup optionPopup;
};
