#include "LyraCarouselTheme.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "components/icons/book.h"
#include "components/icons/cover.h"
#include "components/icons/folder.h"
#include "components/icons/library.h"
#include "components/icons/recent.h"
#include "components/icons/settings2.h"
#include "components/icons/transfer.h"
#include "fontIds.h"

namespace {
constexpr int kCenterCoverMaxW = LyraCarouselTheme::kCenterCoverW;
constexpr int kCenterCoverMaxH = LyraCarouselTheme::kCenterCoverH;
constexpr int kCenterThumbW = LyraCarouselTheme::kCenterThumbW;
constexpr int kCenterThumbH = LyraCarouselTheme::kCenterThumbH;
constexpr int kSideCoverMaxW = LyraCarouselTheme::kSideCoverW;
constexpr int kSideCoverMaxH = LyraCarouselTheme::kSideCoverH;
constexpr int kCoverTopPad = 18;
constexpr int kCenterCoverVisualInset = LyraCarouselTheme::kCenterCoverVisualInset;
constexpr int kBaseDisplayCenterW = LyraCarouselTheme::kBaseDisplayCenterW;
constexpr int kBaseDisplayCenterH = LyraCarouselTheme::kBaseDisplayCenterH;
constexpr int kDisplayCenterW = LyraCarouselTheme::kDisplayCenterW;
constexpr int kDisplayCenterH = LyraCarouselTheme::kDisplayCenterH;
constexpr int kNearSideW = (kBaseDisplayCenterW * 26) / 100;
constexpr int kFarSideW = (kBaseDisplayCenterW * 21) / 100;
constexpr int kNearSideInnerH = (kBaseDisplayCenterH * 90) / 100;
constexpr int kNearSideOuterH = (kBaseDisplayCenterH * 82) / 100;
constexpr int kFarSideInnerH = (kBaseDisplayCenterH * 84) / 100;
constexpr int kFarSideOuterH = (kBaseDisplayCenterH * 74) / 100;
constexpr int kSideCornerRadius = 5;

constexpr int kTitleFontId = UI_12_FONT_ID;
constexpr int kMenuLabelFontId = SMALL_FONT_ID;
constexpr int kDotSize = 8;
constexpr int kDotGap = 6;
constexpr int kTitleTopClearance = 4;
constexpr int kTitleBottomGap = 8;
constexpr int kMenuLabelTopGap = 3;
constexpr int kMenuLabelBottomGap = 4;
constexpr int kMenuRowDrop = 31;

constexpr int kCornerRadius = 6;
constexpr int kThinOutlineW = 1;
constexpr int kSelectionLineW = 3;
constexpr int kCenterOutlineW = 4;

constexpr int kMenuIconSize = 32;
constexpr int kMenuIconPad = 14;
constexpr int kHighlightPad = 7;
constexpr int kButtonHintsH = LyraCarouselMetrics::values.buttonHintsHeight;

struct MenuLayoutMetrics {
  int tileH;
  int tileW;
  int labelLineHeight;
  int rowY;
  int labelY;
};

MenuLayoutMetrics computeMenuLayout(const GfxRenderer& renderer, int buttonCount) {
  const int tileH = kMenuIconPad + kMenuIconSize + kMenuIconPad;
  const int labelLineHeight = renderer.getLineHeight(kMenuLabelFontId);
  const int rowY = renderer.getScreenHeight() - kButtonHintsH - tileH - kMenuLabelTopGap - labelLineHeight -
                   kMenuLabelBottomGap + kMenuRowDrop;
  return {
      tileH, renderer.getScreenWidth() / buttonCount, labelLineHeight, rowY, rowY - kMenuLabelTopGap - labelLineHeight,
  };
}

std::atomic<int> lastCarouselSelectorIndex{-1};
Rect lastCenterCoverRect{0, 0, 0, 0};
Rect cachedCenterCoverRects[LyraCarouselMetrics::values.homeRecentBooksCount];

Rect shrinkCenterCoverRect(const Rect& rect) {
  const int insetWidth = rect.width - kCenterCoverVisualInset * 2;
  const int insetHeight = rect.height - kCenterCoverVisualInset * 2;
  const int width = std::max(0, insetWidth);
  const int height = std::max(0, insetHeight);
  return Rect{rect.x + (rect.width - width) / 2, rect.y + (rect.height - height) / 2, width, height};
}

Rect computeCenterCoverSlotRect(const GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks) {
  (void)recentBooks;
  const int screenW = renderer.getScreenWidth();
  const int centerX = (screenW - kDisplayCenterW) / 2;
  const int titleLineHeight = renderer.getLineHeight(kTitleFontId);
  const int reservedTitleBlockHeight = titleLineHeight * 2;
  const int titleY = rect.y + kTitleTopClearance;
  const int centerTileY = std::max(rect.y + kCoverTopPad, titleY + reservedTitleBlockHeight + kTitleBottomGap);
  return Rect{centerX, centerTileY, kDisplayCenterW, kDisplayCenterH};
}

void drawPerspectiveOutline(const GfxRenderer& renderer, int x, int y, int width, int leftHeight, int rightHeight) {
  const int hMax = std::max(leftHeight, rightHeight);
  const int yTopL = y + (hMax - leftHeight) / 2;
  const int yBottomL = yTopL + leftHeight - 1;
  const int yTopR = y + (hMax - rightHeight) / 2;
  const int yBottomR = yTopR + rightHeight - 1;
  const int rightX = x + width - 1;

  renderer.drawLine(x, yTopL, rightX, yTopR, true);
  renderer.drawLine(rightX, yTopR, rightX, yBottomR, true);
  renderer.drawLine(rightX, yBottomR, x, yBottomL, true);
  renderer.drawLine(x, yBottomL, x, yTopL, true);
}

void fillPerspectiveSilhouette(const GfxRenderer& renderer, int x, int y, int width, int leftHeight, int rightHeight) {
  const int hMax = std::max(leftHeight, rightHeight);
  for (int dx = 0; dx < width; ++dx) {
    const int colH = (width == 1) ? leftHeight : (leftHeight + (rightHeight - leftHeight) * dx / (width - 1));
    if (colH <= 0) continue;
    const int colTop = y + (hMax - colH) / 2;
    renderer.fillRect(x + dx, colTop, 1, colH, false);
  }
}

const uint8_t* iconBitmapForMenu(UIIcon icon) {
  switch (icon) {
    case UIIcon::Folder:
      return FolderIcon;
    case UIIcon::Book:
      return BookIcon;
    case UIIcon::Recent:
      return RecentIcon;
    case UIIcon::Library:
      return LibraryIcon;
    case UIIcon::Settings:
      return Settings2Icon;
    case UIIcon::Transfer:
      return TransferIcon;
    default:
      return nullptr;
  }
}
}  // namespace

void LyraCarouselTheme::setPreRenderIndex(int idx) {
  lastCarouselSelectorIndex.store(idx, std::memory_order_relaxed);
  if (idx >= 0 && idx < LyraCarouselMetrics::values.homeRecentBooksCount) {
    const Rect cachedRect = cachedCenterCoverRects[idx];
    if (cachedRect.width > 0 && cachedRect.height > 0) lastCenterCoverRect = cachedRect;
  }
}

void LyraCarouselTheme::drawCarouselBorder(GfxRenderer& renderer, Rect coverRect,
                                           const std::vector<RecentBook>& recentBooks, int centerIdx,
                                           bool inCarouselRow) const {
  if (!inCarouselRow) return;
  Rect borderRect = shrinkCenterCoverRect(computeCenterCoverSlotRect(renderer, coverRect, recentBooks));
  renderer.drawRoundedRect(borderRect.x, borderRect.y, borderRect.width, borderRect.height, kSelectionLineW,
                           kCornerRadius, true);
}

void LyraCarouselTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect,
                                            const std::vector<RecentBook>& recentBooks, const int selectorIndex,
                                            bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                                            std::function<bool()> storeCoverBuffer) const {
  (void)bufferRestored;
  if (recentBooks.empty()) {
    drawEmptyRecents(renderer, rect);
    return;
  }

  const int bookCount = static_cast<int>(recentBooks.size());
  const bool inCarouselRow = (selectorIndex < bookCount);
  const int lastSelectorIndex = lastCarouselSelectorIndex.load(std::memory_order_relaxed);
  int centerIdx = inCarouselRow ? selectorIndex : (lastSelectorIndex >= 0 ? lastSelectorIndex : 0);

  if (centerIdx >= bookCount) {
    centerIdx = bookCount - 1;
    coverRendered = false;
    coverBufferStored = false;
  }

  if (centerIdx != lastSelectorIndex) {
    coverRendered = false;
    coverBufferStored = false;
  }

  const int screenW = renderer.getScreenWidth();
  const Rect centerCoverSlotRect = computeCenterCoverSlotRect(renderer, rect, recentBooks);
  const int centerDrawY = centerCoverSlotRect.y;
  const int sideMaxHeight = std::max(kNearSideInnerH, kNearSideOuterH);
  const int sideTileY = centerDrawY + (kDisplayCenterH - sideMaxHeight) / 2;

  const int centerX = centerCoverSlotRect.x;
  const int nearOverlap = 4;
  const int farOverlap = 2;
  constexpr int nearCoverInset = 10;
  const int baseLeftNearX = centerX - kNearSideW + nearOverlap;
  const int baseRightNearX = centerX + kDisplayCenterW - nearOverlap;
  const int leftNearX = baseLeftNearX + nearCoverInset;
  const int rightNearX = baseRightNearX - nearCoverInset;
  const int leftFarX = std::max(0, baseLeftNearX - kFarSideW + farOverlap);
  const int rightFarX = std::min(screenW - kFarSideW, baseRightNearX + kNearSideW - farOverlap);

  auto drawCenterCover = [&](int bookIdx, Rect& outRect) -> bool {
    if (bookIdx < 0 || bookIdx >= bookCount) return false;
    const RecentBook& book = recentBooks[bookIdx];
    outRect = shrinkCenterCoverRect(centerCoverSlotRect);

    if (!book.coverBmpPath.empty()) {
      const std::string thumbPath = UITheme::getCoverThumbPath(book.coverBmpPath, kCenterThumbW, kCenterThumbH);
      HalFile file;
      if (Storage.openFileForRead("HOME", thumbPath, file)) {
        Bitmap bitmap(file);
        if (bitmap.parseHeaders() == BmpReaderError::Ok && bitmap.getWidth() > 0 && bitmap.getHeight() > 0) {
          const float srcW = static_cast<float>(bitmap.getWidth());
          const float srcH = static_cast<float>(bitmap.getHeight());
          const float srcRatio = srcW / srcH;
          const float safeTargetHeight = outRect.height == 0 ? 1.0f : static_cast<float>(outRect.height);
          const float targetRatio = static_cast<float>(outRect.width) / safeTargetHeight;
          float cropX = 0.0f;
          float cropY = 0.0f;

          if (srcRatio > targetRatio) {
            cropX = std::max(0.0f, 1.0f - (targetRatio / srcRatio));
          } else if (srcRatio < targetRatio) {
            cropY = std::max(0.0f, 1.0f - (srcRatio / targetRatio));
          }

          renderer.fillRect(outRect.x - kCenterOutlineW, outRect.y - kCenterOutlineW,
                            outRect.width + 2 * kCenterOutlineW, outRect.height + 2 * kCenterOutlineW, false);
          renderer.drawBitmap(bitmap, outRect.x, outRect.y, outRect.width, outRect.height, cropX, cropY);
          renderer.maskRoundedRectOutsideCorners(outRect.x, outRect.y, outRect.width, outRect.height, kCornerRadius,
                                                 Color::White);
          file.close();
          return true;
        }
        file.close();
      }
    }

    renderer.fillRect(outRect.x - kCenterOutlineW, outRect.y - kCenterOutlineW, outRect.width + 2 * kCenterOutlineW,
                      outRect.height + 2 * kCenterOutlineW, false);
    renderer.drawRoundedRect(outRect.x, outRect.y, outRect.width, outRect.height, 1, kCornerRadius, true);
    renderer.fillRoundedRect(outRect.x, outRect.y + outRect.height / 3, outRect.width, 2 * outRect.height / 3,
                             kCornerRadius, false, false, true, true, Color::Black);
    const int iconX = outRect.x + outRect.width / 2 - 16;
    const int iconY = outRect.y + outRect.height / 3 + 14;
    renderer.drawIcon(CoverIcon, iconX, iconY, 32);
    return false;
  };

  auto drawSideCover = [&](int bookIdx, int x, int width, int leftHeight, int rightHeight) -> bool {
    if (bookIdx < 0 || bookIdx >= bookCount) return false;
    const RecentBook& book = recentBooks[bookIdx];

    if (!book.coverBmpPath.empty()) {
      const std::string thumbPath = UITheme::getCoverThumbPath(book.coverBmpPath, kSideCoverMaxW, kSideCoverMaxH);
      HalFile file;
      if (Storage.openFileForRead("HOME", thumbPath, file)) {
        Bitmap bitmap(file);
        if (bitmap.parseHeaders() == BmpReaderError::Ok) {
          const int sideHeight = std::max(leftHeight, rightHeight);
          renderer.fillRect(x, sideTileY, width, sideHeight, false);
          renderer.drawPerspectiveBitmap(bitmap, x, sideTileY, width, leftHeight, rightHeight);
          renderer.maskRoundedRectOutsideCorners(x, sideTileY, width, sideHeight, kSideCornerRadius, Color::White);
          file.close();
          drawPerspectiveOutline(renderer, x, sideTileY, width, leftHeight, rightHeight);
          return true;
        }
        file.close();
      }
    }

    fillPerspectiveSilhouette(renderer, x, sideTileY, width, leftHeight, rightHeight);
    renderer.maskRoundedRectOutsideCorners(x, sideTileY, width, std::max(leftHeight, rightHeight), kSideCornerRadius,
                                           Color::White);
    return false;
  };

  if (!coverRendered) {
    lastCarouselSelectorIndex.store(centerIdx, std::memory_order_relaxed);

    renderer.fillRect(rect.x, rect.y, rect.width, rect.height, false);

    const int leftNearIdx = (centerIdx + bookCount - 1) % bookCount;
    const int leftFarIdx = (centerIdx + bookCount - 2) % bookCount;
    const int rightNearIdx = (centerIdx + 1) % bookCount;
    const int rightFarIdx = (centerIdx + 2) % bookCount;

    if (bookCount >= 5) drawSideCover(leftFarIdx, leftFarX, kFarSideW, kFarSideInnerH, kFarSideOuterH);
    if (bookCount >= 4) drawSideCover(rightFarIdx, rightFarX, kFarSideW, kFarSideOuterH, kFarSideInnerH);
    if (bookCount >= 2) drawSideCover(leftNearIdx, leftNearX, kNearSideW, kNearSideInnerH, kNearSideOuterH);
    if (bookCount >= 3) drawSideCover(rightNearIdx, rightNearX, kNearSideW, kNearSideOuterH, kNearSideInnerH);

    Rect centerCoverRect{};
    drawCenterCover(centerIdx, centerCoverRect);
    lastCenterCoverRect = centerCoverRect;
    if (centerIdx >= 0 && centerIdx < LyraCarouselMetrics::values.homeRecentBooksCount) {
      cachedCenterCoverRects[centerIdx] = centerCoverRect;
    }

    // Title sits above the center cover
    const int textMaxWidth = std::min(screenW - 40, kCenterCoverMaxW + 40);
    const auto titleLines =
        renderer.wrappedText(kTitleFontId, recentBooks[centerIdx].title.c_str(), textMaxWidth, 2, EpdFontFamily::BOLD);
    const int titleLineHeight = renderer.getLineHeight(kTitleFontId);
    int curTitleY = rect.y + kTitleTopClearance;
    for (const auto& line : titleLines) {
      const int lineW = renderer.getTextWidth(kTitleFontId, line.c_str(), EpdFontFamily::BOLD);
      renderer.drawText(kTitleFontId, (screenW - lineW) / 2, curTitleY, line.c_str(), true, EpdFontFamily::BOLD);
      curTitleY += titleLineHeight;
    }

    // Position dots below center cover
    const int dotsY = centerCoverSlotRect.y + centerCoverSlotRect.height + 8;
    const int totalDotsW = bookCount * kDotSize + (bookCount - 1) * kDotGap;
    int dotX = centerCoverSlotRect.x + (centerCoverSlotRect.width - totalDotsW) / 2;
    for (int i = 0; i < bookCount; ++i) {
      if (i == centerIdx) {
        renderer.fillRect(dotX, dotsY, kDotSize, kDotSize, true);
      } else {
        renderer.drawRect(dotX, dotsY, kDotSize, kDotSize, true);
      }
      dotX += kDotSize + kDotGap;
    }

    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
  } else if (lastCenterCoverRect.width <= 0 || lastCenterCoverRect.height <= 0) {
    lastCenterCoverRect = shrinkCenterCoverRect(centerCoverSlotRect);
  }

  const int outlineW = inCarouselRow ? kSelectionLineW : kThinOutlineW;
  renderer.drawRoundedRect(lastCenterCoverRect.x, lastCenterCoverRect.y, lastCenterCoverRect.width,
                           lastCenterCoverRect.height, outlineW, kCornerRadius, true);
}

void LyraCarouselTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                       const std::function<std::string(int index)>& buttonLabel,
                                       const std::function<UIIcon(int index)>& rowIcon) const {
  (void)rect;
  if (buttonCount <= 0) return;

  const MenuLayoutMetrics metrics = computeMenuLayout(renderer, buttonCount);

  for (int i = 0; i < buttonCount; ++i) {
    const int tileX = i * metrics.tileW;
    const int iconX = tileX + (metrics.tileW - kMenuIconSize) / 2;
    const int iconY = metrics.rowY + kMenuIconPad;

    const bool selected = (selectedIndex == i);
    if (selected) {
      const int highlightSize = kMenuIconSize + 2 * kHighlightPad;
      const int highlightY = metrics.rowY + (metrics.tileH - highlightSize) / 2;
      renderer.fillRoundedRect(iconX - kHighlightPad, highlightY, highlightSize, highlightSize, kCornerRadius,
                               Color::Black);
    }

    if (rowIcon != nullptr) {
      const UIIcon icon = rowIcon(i);
      const uint8_t* bmp = iconBitmapForMenu(icon);
      if (bmp != nullptr) {
        if (selected) {
          renderer.drawIconInverted(bmp, iconX, iconY, kMenuIconSize, kMenuIconSize);
        } else {
          renderer.drawIcon(bmp, iconX, iconY, kMenuIconSize);
        }
      }
    }
  }

  renderer.fillRect(0, metrics.labelY, renderer.getScreenWidth(), metrics.labelLineHeight, false);
  if (selectedIndex >= 0 && selectedIndex < buttonCount && buttonLabel != nullptr) {
    std::string label = buttonLabel(selectedIndex);
    const int labelWidth = renderer.getTextWidth(kMenuLabelFontId, label.c_str(), EpdFontFamily::REGULAR);
    renderer.drawText(kMenuLabelFontId, (renderer.getScreenWidth() - labelWidth) / 2, metrics.labelY + 2, label.c_str(),
                      true, EpdFontFamily::REGULAR);
  }
}
