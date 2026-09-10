#pragma once

#include <atomic>
#include <vector>

#include "LyraTheme.h"

class GfxRenderer;

namespace LyraCarouselMetrics {
constexpr ThemeMetrics makeValues() {
  ThemeMetrics v = LyraMetrics::values;
  v.topPadding = 5;
  v.batteryBarHeight = 36;
  v.homeTopPadding = 48;
  v.homeCoverHeight = 600;
  v.homeCoverTileHeight = 620;
  v.homeRecentBooksCount = 5;
  v.listRowHeight = 35;
  v.menuRowHeight = 64;
  v.menuSpacing = 8;
  v.keyboardKeyHeight = 56;
  v.keyboardCenteredText = true;
  return v;
}

constexpr ThemeMetrics values = makeValues();
}  // namespace LyraCarouselMetrics

class LyraCarouselTheme : public LyraTheme {
 public:
  static constexpr int kCenterCoverW = 340;
  static constexpr int kCenterCoverH = LyraCarouselMetrics::values.homeCoverHeight - 60;  // 540
  static constexpr int kCenterCoverVisualInset = 10;
  static constexpr int kBaseDisplayCenterW = (kCenterCoverW * 86) / 100;
  static constexpr int kBaseDisplayCenterH = (kCenterCoverH * 86) / 100;
  static constexpr int kDisplayCenterW =
      ((kBaseDisplayCenterW + 24) < kCenterCoverW) ? (kBaseDisplayCenterW + 24) : kCenterCoverW;
  static constexpr int kDisplayCenterH =
      ((kBaseDisplayCenterH + 24) < kCenterCoverH) ? (kBaseDisplayCenterH + 24) : kCenterCoverH;

  static constexpr int kCenterThumbW = kDisplayCenterW - kCenterCoverVisualInset * 2;  // 296
  static constexpr int kCenterThumbH = kDisplayCenterH - kCenterCoverVisualInset * 2;  // 468
  static constexpr int kSideCoverW = 200;
  static constexpr int kSideCoverH = LyraCarouselMetrics::values.homeCoverHeight - 210;  // 390

  static void setPreRenderIndex(int idx);

  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           const int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
  void drawCarouselBorder(GfxRenderer& renderer, Rect coverRect, const std::vector<RecentBook>& recentBooks,
                          int centerIdx, bool inCarouselRow) const override;
};
