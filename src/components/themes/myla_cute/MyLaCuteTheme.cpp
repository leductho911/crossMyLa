#include "MyLaCuteTheme.h"

#include <Bitmap.h>
#include <Epub/converters/PngToFramebufferConverter.h>
#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>

#include <algorithm>
#include <string>
#include <vector>

#include "CatArt.h"
#include "CrossPointSettings.h"
#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int kCoverRadius = 18;
constexpr int kMenuRadius = 14;
constexpr int kCardRadius = 12;
constexpr int kBottomRadius = 15;
constexpr int kRowRadius = 20;
constexpr int kInteractiveInsetX = 20;
constexpr int kSelectableRowGap = 6;
constexpr int kTitleFontId = UI_12_FONT_ID;
constexpr int kSubtitleFontId = SMALL_FONT_ID;
constexpr int kGuideFontId = SMALL_FONT_ID;
constexpr int kLeftColumnWidth = 224;
constexpr int kIconSize = 32;

void drawScrollBar(const GfxRenderer& renderer, Rect rect, int itemCount, int pageStartIndex, int pageItems) {
  if (itemCount <= 0 || pageItems <= 0 || itemCount <= pageItems) {
    return;
  }

  const int barW = MyLaCuteMetrics::values.scrollBarWidth;
  const int barX = rect.x + rect.width - MyLaCuteMetrics::values.scrollBarRightOffset - barW;
  const int barY = rect.y;
  const int barH = rect.height;

  const int thumbH = std::max(10, (barH * pageItems) / itemCount);
  const int maxStart = std::max(1, itemCount - pageItems);
  const int maxTravel = std::max(1, barH - thumbH);
  const int clampedStart = std::clamp(pageStartIndex, 0, maxStart);
  const int thumbY = barY + (clampedStart * maxTravel) / maxStart;

  renderer.fillRect(barX, thumbY, barW, thumbH);
}

bool s_wallpaperChecked = false;
bool s_hasCustomWallpaper = false;
const char* s_customWallpaperPath = nullptr;
std::string s_recentBookTitle = "";

const uint8_t* cuteIconForName(UIIcon icon, int size) {
  if (size == 24) {
    switch (icon) {
      case UIIcon::Folder:
        return CatFolder24Icon;
      case UIIcon::Book:
        return CatBook24Icon;
      case UIIcon::Recent:
        return CatRecent24Icon;
      case UIIcon::Settings:
        return CatBell24Icon;
      case UIIcon::Transfer:
        return CatPlane24Icon;
      case UIIcon::Library:
      case UIIcon::Wifi:
      case UIIcon::Hotspot:
        return CatCloud24Icon;
      case UIIcon::Bookmark:
        return CatPaw24Icon;
      default:
        return CatPaw24Icon;
    }
  } else {
    switch (icon) {
      case UIIcon::Folder:
        return CatFolder32Icon;
      case UIIcon::Book:
        return CatBook32Icon;
      case UIIcon::Recent:
        return CatRecent32Icon;
      case UIIcon::Settings:
        return CatBell32Icon;
      case UIIcon::Transfer:
        return CatPlane32Icon;
      case UIIcon::Library:
      case UIIcon::Wifi:
      case UIIcon::Hotspot:
        return CatCloud32Icon;
      case UIIcon::Bookmark:
        return CatPaw32Icon;
      default:
        return CatPaw32Icon;
    }
  }
}

const uint8_t* headerIconForTitle(const char* title) {
  if (!title || title[0] == '\0') {
    return CatPaw24Icon;
  }

  // 1. Settings & system configurations
  if (strcmp(title, tr(STR_SETTINGS_TITLE)) == 0 || strcmp(title, tr(STR_CUSTOMISE_STATUS_BAR)) == 0 ||
      strcmp(title, tr(STR_REMAP_FRONT_BUTTONS)) == 0 || strcmp(title, tr(STR_CLOCK_SYNC)) == 0 ||
      strcmp(title, tr(STR_CLOCK_UTC_OFFSET)) == 0 || strcmp(title, tr(STR_UPDATE)) == 0 ||
      strcmp(title, tr(STR_CLEAR_READING_CACHE)) == 0 || strstr(title, "Setting") != nullptr ||
      strstr(title, "setting") != nullptr || strstr(title, "Cài đặt") != nullptr ||
      strstr(title, "cài đặt") != nullptr || strstr(title, "Config") != nullptr || strstr(title, "config") != nullptr) {
    return CatBell24Icon;
  }

  // 2. Browse Files / SD / Folders
  if (strcmp(title, tr(STR_BROWSE_FILES)) == 0 || title[0] == '/' || strstr(title, "File") != nullptr ||
      strstr(title, "file") != nullptr || strstr(title, "Tệp") != nullptr || strstr(title, "tệp") != nullptr ||
      strstr(title, "Folder") != nullptr || strstr(title, "folder") != nullptr || strstr(title, "Thư mục") != nullptr ||
      strstr(title, "thư mục") != nullptr) {
    return CatFolder24Icon;
  }

  // 3. Recent Books / Reading History
  if (strcmp(title, tr(STR_MENU_RECENT_BOOKS)) == 0 || strstr(title, "Recent") != nullptr ||
      strstr(title, "recent") != nullptr || strstr(title, "Gần đây") != nullptr ||
      strstr(title, "gần đây") != nullptr || strstr(title, "History") != nullptr ||
      strstr(title, "history") != nullptr || strstr(title, "Lịch sử") != nullptr ||
      strstr(title, "lịch sử") != nullptr) {
    return CatRecent24Icon;
  }

  // 4. Online Library / OPDS / WiFi
  if (strcmp(title, tr(STR_OPDS_BROWSER)) == 0 || strcmp(title, tr(STR_OPDS_SERVERS)) == 0 ||
      strcmp(title, tr(STR_WIFI_NETWORKS)) == 0 || strstr(title, "OPDS") != nullptr ||
      strstr(title, "opds") != nullptr || strstr(title, "WiFi") != nullptr || strstr(title, "wifi") != nullptr ||
      strstr(title, "Wi-Fi") != nullptr || strstr(title, "wi-fi") != nullptr || strstr(title, "Online") != nullptr ||
      strstr(title, "online") != nullptr || strstr(title, "Trực tuyến") != nullptr ||
      strstr(title, "trực tuyến") != nullptr) {
    return CatCloud24Icon;
  }

  // 5. File Transfer / Calibre / Web Server / KOReader
  if (strcmp(title, tr(STR_FILE_TRANSFER)) == 0 || strcmp(title, tr(STR_CALIBRE_WIRELESS)) == 0 ||
      strcmp(title, tr(STR_KOREADER_SYNC)) == 0 || strstr(title, "Transfer") != nullptr ||
      strstr(title, "transfer") != nullptr || strstr(title, "Calibre") != nullptr ||
      strstr(title, "calibre") != nullptr || strstr(title, "KOReader") != nullptr ||
      strstr(title, "koreader") != nullptr || strstr(title, "Web") != nullptr || strstr(title, "web") != nullptr ||
      strstr(title, "Truyền") != nullptr || strstr(title, "truyền") != nullptr) {
    return CatPlane24Icon;
  }

  // 6. Reading / Reader / Chapters / Footnotes / Text Settings
  if (strcmp(title, tr(STR_CONTINUE_READING)) == 0 || strcmp(title, tr(STR_TEXT_SETTINGS)) == 0 ||
      strcmp(title, tr(STR_SELECT_CHAPTER)) == 0 || strcmp(title, tr(STR_FRONTLIGHT)) == 0 ||
      strstr(title, "Reader") != nullptr || strstr(title, "reader") != nullptr || strstr(title, "Book") != nullptr ||
      strstr(title, "book") != nullptr || strstr(title, "Sách") != nullptr || strstr(title, "sách") != nullptr ||
      strstr(title, "Đọc") != nullptr || strstr(title, "đọc") != nullptr || strstr(title, "Chapter") != nullptr ||
      strstr(title, "chapter") != nullptr || strstr(title, "Chương") != nullptr || strstr(title, "chương") != nullptr ||
      strstr(title, "Text") != nullptr || strstr(title, "text") != nullptr || strstr(title, "Văn bản") != nullptr ||
      strstr(title, "văn bản") != nullptr) {
    return CatBook24Icon;
  }

  return CatPaw24Icon;
}

}  // namespace

void MyLaCuteTheme::fillBatteryIcon(const GfxRenderer& renderer, Rect rect, uint16_t percentage) const {
  const bool charging = gpio.isUsbConnected();

  if (charging) {
    renderer.fillRoundedRect(rect.x + 2, rect.y + 2, rect.width - 4, rect.height - 4, 3, Color::Black);
    drawBatteryLightningBolt(renderer, rect.x + 5, rect.y + 2);
  } else {
    // Cute pill fill with segmented paw dots
    const int fillW = std::clamp(((rect.width - 4) * percentage) / 100, 0, rect.width - 4);
    if (fillW > 0) {
      renderer.fillRoundedRect(rect.x + 2, rect.y + 2, fillW, rect.height - 4, 2, Color::Black);
    }
  }
}

void MyLaCuteTheme::invalidateWallpaperCache() {
  s_wallpaperChecked = false;
  s_hasCustomWallpaper = false;
  s_customWallpaperPath = nullptr;
}

void MyLaCuteTheme::drawHomeScreenHeader(const GfxRenderer& renderer, Rect rect) const {
  (void)rect;
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  // Check SD card once for custom wallpaper; avoid repetitive slow FAT scans on navigation
  if (!s_wallpaperChecked) {
    s_wallpaperChecked = true;
    s_hasCustomWallpaper = false;
    s_customWallpaperPath = nullptr;
    HalFile wpFile;
    if (Storage.openFileForRead("HOME", "/wallpaper.png", wpFile)) {
      s_hasCustomWallpaper = true;
      s_customWallpaperPath = "/wallpaper.png";
    } else if (Storage.openFileForRead("HOME", "/.crosspoint/wallpaper.png", wpFile)) {
      s_hasCustomWallpaper = true;
      s_customWallpaperPath = "/.crosspoint/wallpaper.png";
    } else if (Storage.openFileForRead("HOME", "/wallpaper.bmp", wpFile)) {
      s_hasCustomWallpaper = true;
      s_customWallpaperPath = "/wallpaper.bmp";
    } else if (Storage.openFileForRead("HOME", "/.crosspoint/wallpaper.bmp", wpFile)) {
      s_hasCustomWallpaper = true;
      s_customWallpaperPath = "/.crosspoint/wallpaper.bmp";
    }
  }

  if (s_hasCustomWallpaper && s_customWallpaperPath != nullptr) {
    if (FsHelpers::hasPngExtension(std::string_view(s_customWallpaperPath))) {
      PngToFramebufferConverter converter;
      RenderConfig config{0, 0, pageWidth, pageHeight};
      converter.decodeToFramebuffer(s_customWallpaperPath, const_cast<GfxRenderer&>(renderer), config);
    } else {
      HalFile wpFile;
      if (Storage.openFileForRead("HOME", s_customWallpaperPath, wpFile)) {
        Bitmap wpBitmap(wpFile, true);
        if (wpBitmap.parseHeaders() == BmpReaderError::Ok) {
          renderer.drawBitmap(wpBitmap, 0, 0, pageWidth, pageHeight, 0, 0);
        }
      }
    }
  }

  char timeBuf[16] = "12:00";
  if (halClock.isAvailable()) {
    halClock.formatTime(timeBuf, sizeof(timeBuf), SETTINGS.clockUtcOffsetQ, SETTINGS.clockFormat != 0);
  }

  char dateBuf[32] = "Thu, Aug 30";
  if (halClock.isAvailable()) {
    halClock.formatDate(dateBuf, sizeof(dateBuf), SETTINGS.clockUtcOffsetQ);
  }

  // Draw Large Clock on the top-left (e.g. 12:00)
  const int clockX = MyLaCuteMetrics::values.contentSidePadding + 4;
  const int clockY = 24;
  renderer.drawText(NOTOSERIF_18_FONT_ID, clockX, clockY, timeBuf, true, EpdFontFamily::REGULAR);

  // Draw Date underneath clock (e.g. "Thu, Aug 30")
  const int subY = clockY + renderer.getLineHeight(NOTOSERIF_18_FONT_ID) + 6;
  renderer.drawText(NOTOSERIF_12_FONT_ID, clockX, subY, dateBuf, true, EpdFontFamily::REGULAR);

  // Draw Battery info on the top-right corner
  const uint16_t batt = powerManager.getBatteryPercentage();
  const int battW = MyLaCuteMetrics::values.batteryWidth;
  const int battH = MyLaCuteMetrics::values.batteryHeight;
  const int pad = MyLaCuteMetrics::values.contentSidePadding;
  const int battX = pageWidth - pad - battW - 8;
  const int battY = 28;

  char battText[8];
  snprintf(battText, sizeof(battText), "%u%%", batt);
  const int battTextW = renderer.getTextWidth(SMALL_FONT_ID, battText, EpdFontFamily::REGULAR);
  const int textY = battY + (battH - renderer.getLineHeight(SMALL_FONT_ID)) / 2;
  renderer.drawText(SMALL_FONT_ID, battX - battTextW - 6, textY, battText, true, EpdFontFamily::REGULAR);

  renderer.drawRoundedRect(battX, battY, battW, battH, 1, 2, true);
  renderer.fillRect(battX + battW, battY + battH / 4, 2, battH / 2);
  fillBatteryIcon(renderer, Rect{battX, battY, battW, battH}, batt);
}

void MyLaCuteTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle) const {
  if (rect.height > MyLaCuteMetrics::values.headerHeight || title == nullptr) {
    drawHomeScreenHeader(renderer, rect);
    return;
  }

  BaseTheme::drawHeader(renderer, rect, title, subtitle);
}

void MyLaCuteTheme::drawSubHeader(const GfxRenderer& renderer, Rect rect, const char* label,
                                  const char* rightLabel) const {
  const int pad = MyLaCuteMetrics::values.headerSidePadding;
  renderer.drawText(UI_10_FONT_ID, pad, rect.y + (rect.height - renderer.getLineHeight(UI_10_FONT_ID)) / 2, label, true,
                    EpdFontFamily::BOLD);
  if (rightLabel && rightLabel[0] != '\0') {
    const int rWidth = renderer.getTextWidth(UI_10_FONT_ID, rightLabel);
    renderer.drawText(UI_10_FONT_ID, rect.width - pad - rWidth,
                      rect.y + (rect.height - renderer.getLineHeight(UI_10_FONT_ID)) / 2, rightLabel, true);
  }
  renderer.drawLine(pad, rect.y + rect.height - 1, rect.width - pad, rect.y + rect.height - 1, true);
}

void MyLaCuteTheme::drawTabBar(const GfxRenderer& renderer, Rect rect, const std::vector<TabInfo>& tabs,
                               bool selected) const {
  if (tabs.empty()) return;
  const int slotWidth = rect.width / static_cast<int>(tabs.size());
  const int tabY = rect.y + 3;
  const int tabHeight = rect.height - 6;

  for (size_t i = 0; i < tabs.size(); i++) {
    const int slotX = rect.x + static_cast<int>(i) * slotWidth;
    const int tabX = slotX + 4;
    const int tabWidth = slotWidth - 8;
    const auto& tab = tabs[i];

    if (tab.selected) {
      renderer.fillRoundedRect(tabX, tabY, tabWidth, tabHeight, kCardRadius,
                               selected ? Color::Black : Color::LightGray);
    } else {
      renderer.drawRoundedRect(tabX, tabY, tabWidth, tabHeight, 1, kCardRadius, true);
    }

    const int textWidth = renderer.getTextWidth(SMALL_FONT_ID, tab.label, EpdFontFamily::REGULAR);
    const int textX = slotX + (slotWidth - textWidth) / 2;
    const int textY = tabY + (tabHeight - renderer.getLineHeight(SMALL_FONT_ID)) / 2;
    renderer.drawText(SMALL_FONT_ID, textX, textY, tab.label, !tab.selected || !selected, EpdFontFamily::REGULAR);
  }
}

bool MyLaCuteTheme::tabIndexFromPoint(const GfxRenderer& renderer, Rect rect, const std::vector<TabInfo>& tabs, int x,
                                      int y, int& index) const {
  (void)renderer;
  if (tabs.empty() || y < rect.y || y >= rect.y + rect.height || x < rect.x || x >= rect.x + rect.width) {
    return false;
  }
  const int slotWidth = std::max(1, rect.width / static_cast<int>(tabs.size()));
  index = std::min(static_cast<int>(tabs.size()) - 1, (x - rect.x) / slotWidth);
  return true;
}

int MyLaCuteTheme::getMenuRowHeight(const GfxRenderer& renderer) const {
  (void)renderer;
  return (SETTINGS.mylaDisplayMode == CrossPointSettings::MYLA_DISPLAY_ICON) ? 56
                                                                             : MyLaCuteMetrics::values.menuRowHeight;
}

int MyLaCuteTheme::getMenuTop(const GfxRenderer& renderer, int menuCount) const {
  const auto& metrics = MyLaCuteMetrics::values;
  if (SETTINGS.mylaMenuPosition == CrossPointSettings::MYLA_MENU_BOTTOM_LEFT) {
    const int pageHeight = renderer.getScreenHeight();
    const bool isIconMode = (SETTINGS.mylaDisplayMode == CrossPointSettings::MYLA_DISPLAY_ICON);
    const int rowHeight = isIconMode ? 56 : metrics.menuRowHeight;
    const int rowSpacing = isIconMode ? 10 : metrics.menuSpacing;
    const int totalMenuHeight = menuCount * rowHeight + std::max(0, menuCount - 1) * rowSpacing;
    const int bottomPadding = gpio.hasTouch() ? 24 : (metrics.buttonHintsHeight + 14);
    return std::max(metrics.homeTopPadding + metrics.homeMenuTopOffset, pageHeight - bottomPadding - totalMenuHeight);
  }
  return metrics.homeTopPadding + metrics.homeCoverTileHeight + metrics.homeMenuTopOffset;
}

int MyLaCuteTheme::getListRowStep(bool hasSubtitle) const {
  const int rowHeight =
      hasSubtitle ? MyLaCuteMetrics::values.listWithSubtitleRowHeight : MyLaCuteMetrics::values.listRowHeight;
  return rowHeight + kSelectableRowGap;
}

int MyLaCuteTheme::getListPageItems(int contentHeight, bool hasSubtitle) const {
  return std::max(1, contentHeight / getListRowStep(hasSubtitle));
}

void MyLaCuteTheme::drawList(const GfxRenderer& renderer, Rect rect, int itemCount, int selectedIndex,
                             const std::function<std::string(int index)>& rowTitle,
                             const std::function<std::string(int index)>& rowSubtitle,
                             const std::function<UIIcon(int index)>& rowIcon,
                             const std::function<std::string(int index)>& rowValue, bool highlightValue,
                             const std::function<bool(int index)>& rowDimmed) const {
  (void)rowIcon;
  (void)highlightValue;
  (void)rowDimmed;
  const bool hasSubtitle = static_cast<bool>(rowSubtitle);
  const int titleLineHeight = renderer.getLineHeight(kTitleFontId);
  const int subtitleLineHeight = renderer.getLineHeight(kSubtitleFontId);
  constexpr int subtitleTopPadding = 10;
  constexpr int subtitleBottomPadding = 10;
  constexpr int subtitleInterLineGap = 4;
  const int subtitleRowHeight =
      subtitleTopPadding + titleLineHeight + subtitleInterLineGap + subtitleLineHeight + subtitleBottomPadding;
  const int rowHeight = hasSubtitle ? subtitleRowHeight : MyLaCuteMetrics::values.listRowHeight;
  const int rowStep = rowHeight + kSelectableRowGap;
  const int pageItems = std::max(1, rect.height / rowStep);
  const int pageStartIndex = std::max(0, selectedIndex / pageItems) * pageItems;

  const int sidePadding = MyLaCuteMetrics::values.contentSidePadding;
  const int rowX = rect.x + sidePadding;
  const int rowWidth = rect.width - sidePadding * 2;

  for (int i = pageStartIndex; i < itemCount && i < pageStartIndex + pageItems; i++) {
    const int rowY = rect.y + (i % pageItems) * rowStep;
    const bool isSelected = i == selectedIndex;
    renderer.fillRoundedRect(rowX, rowY, rowWidth, rowHeight, kRowRadius, isSelected ? Color::Black : Color::White);

    constexpr int kMinTitleWidth = 40;
    constexpr int kMinValueGap = kInteractiveInsetX;
    int textAreaWidth = rowWidth - kInteractiveInsetX * 2;
    if (rowValue) {
      std::string valueText = rowValue(i);
      if (!valueText.empty()) {
        const int maxValueWidth = std::max(0, rowWidth - kInteractiveInsetX * 2 - kMinValueGap - kMinTitleWidth);
        if (maxValueWidth > 0) {
          const std::string truncatedValue =
              renderer.truncatedText(kTitleFontId, valueText.c_str(), maxValueWidth, EpdFontFamily::REGULAR);
          const int valueW = renderer.getTextWidth(kTitleFontId, truncatedValue.c_str(), EpdFontFamily::REGULAR);
          renderer.drawText(kTitleFontId, rowX + rowWidth - kInteractiveInsetX - valueW,
                            rowY + (rowHeight - renderer.getLineHeight(kTitleFontId)) / 2, truncatedValue.c_str(),
                            !isSelected, EpdFontFamily::REGULAR);
          textAreaWidth = std::max(0, textAreaWidth - valueW - kMinValueGap);
        }
      }
    }

    if (hasSubtitle) {
      const std::string subtitleRaw = rowSubtitle(i);
      auto title = renderer.truncatedText(kTitleFontId, rowTitle(i).c_str(), textAreaWidth, EpdFontFamily::BOLD);

      if (subtitleRaw.empty()) {
        const int centeredTitleY = rowY + (rowHeight - titleLineHeight) / 2;
        renderer.drawText(kTitleFontId, rowX + kInteractiveInsetX, centeredTitleY, title.c_str(), !isSelected,
                          EpdFontFamily::BOLD);
      } else {
        const int titleY = rowY + subtitleTopPadding;
        const int subtitleY = titleY + titleLineHeight + subtitleInterLineGap;
        auto subtitle =
            renderer.truncatedText(kSubtitleFontId, subtitleRaw.c_str(), textAreaWidth, EpdFontFamily::REGULAR);
        renderer.drawText(kTitleFontId, rowX + kInteractiveInsetX, titleY, title.c_str(), !isSelected,
                          EpdFontFamily::BOLD);
        renderer.drawText(kSubtitleFontId, rowX + kInteractiveInsetX, subtitleY, subtitle.c_str(), !isSelected,
                          EpdFontFamily::REGULAR);
      }
    } else {
      auto title = renderer.truncatedText(kTitleFontId, rowTitle(i).c_str(), textAreaWidth, EpdFontFamily::BOLD);
      renderer.drawText(kTitleFontId, rowX + kInteractiveInsetX,
                        rowY + (rowHeight - renderer.getLineHeight(kTitleFontId)) / 2, title.c_str(), !isSelected,
                        EpdFontFamily::BOLD);
    }
  }

  drawScrollBar(renderer, rect, itemCount, pageStartIndex, pageItems);
}

void MyLaCuteTheme::drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                                    const char* btn4) const {
  if (gpio.hasTouch()) {
    return;
  }

  const GfxRenderer::Orientation origOrientation = renderer.getOrientation();
  renderer.setOrientation(GfxRenderer::Orientation::Portrait);

  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const int sidePadding = 20;
  const int groupGap = 10;
  const int bottomMargin = 10;
  const int hintHeight = MyLaCuteMetrics::values.buttonHintsHeight - 10;
  const int groupWidth = (pageWidth - sidePadding * 2 - groupGap) / 2;
  const int hintY = pageHeight - hintHeight - bottomMargin;
  const int textY = hintY + (hintHeight - renderer.getLineHeight(SMALL_FONT_ID)) / 2;

  const bool backDisabled = (btn1 == nullptr || btn1[0] == '\0');
  const int leftGroupX = sidePadding;
  const int rightGroupX = leftGroupX + groupWidth + groupGap;
  const std::string backLabel = backDisabled ? "" : std::string(btn1);
  const std::string selectText = (btn2 && btn2[0] != '\0') ? std::string(btn2) : "";
  const std::string upText = (btn3 && btn3[0] != '\0') ? std::string(btn3) : "";
  const std::string downText = (btn4 && btn4[0] != '\0') ? std::string(btn4) : "";

  // Clear background under pills
  renderer.fillRect(leftGroupX, hintY, groupWidth, hintHeight, false);
  renderer.fillRect(rightGroupX, hintY, groupWidth, hintHeight, false);

  constexpr int kPillRadius = 15;
  renderer.drawRoundedRect(leftGroupX, hintY, groupWidth, hintHeight, 2, kPillRadius, true);
  const int selectWidth = renderer.getTextWidth(SMALL_FONT_ID, selectText.c_str(), EpdFontFamily::REGULAR);
  const int downWidth = renderer.getTextWidth(SMALL_FONT_ID, downText.c_str(), EpdFontFamily::REGULAR);
  constexpr int innerEdgePadding = 16;

  const int backX = leftGroupX + innerEdgePadding;
  const int selectX = leftGroupX + groupWidth - innerEdgePadding - selectWidth;
  const int upX = rightGroupX + innerEdgePadding;
  const int downX = rightGroupX + groupWidth - innerEdgePadding - downWidth;

  if (!backDisabled) {
    renderer.drawText(SMALL_FONT_ID, backX, textY, backLabel.c_str(), true, EpdFontFamily::REGULAR);
  }
  renderer.drawText(SMALL_FONT_ID, selectX, textY, selectText.c_str(), true, EpdFontFamily::REGULAR);

  renderer.drawRoundedRect(rightGroupX, hintY, groupWidth, hintHeight, 2, kPillRadius, true);

  renderer.drawText(SMALL_FONT_ID, upX, textY, upText.c_str(), true, EpdFontFamily::REGULAR);
  renderer.drawText(SMALL_FONT_ID, downX, textY, downText.c_str(), true, EpdFontFamily::REGULAR);

  renderer.setOrientation(origOrientation);
}

void MyLaCuteTheme::drawSideButtonHints(const GfxRenderer& renderer, const char* topBtn, const char* bottomBtn) const {
  BaseTheme::drawSideButtonHints(renderer, topBtn, bottomBtn);
}

void MyLaCuteTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                   const std::function<std::string(int index)>& buttonLabel,
                                   const std::function<UIIcon(int index)>& rowIcon) const {
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const bool isLandscape = pageWidth > pageHeight;
  const bool isIconMode = (SETTINGS.mylaDisplayMode == CrossPointSettings::MYLA_DISPLAY_ICON);

  const int menuLeft = MyLaCuteMetrics::values.contentSidePadding;
  const int rowHeight = isIconMode ? 56 : MyLaCuteMetrics::values.menuRowHeight;
  const int menuWidth = isIconMode ? 56 : (isLandscape ? (pageWidth * 38 / 100) : (pageWidth * 54 / 100));
  const int rowSpacing = isIconMode ? 10 : MyLaCuteMetrics::values.menuSpacing;
  const int menuTop = rect.y;

  const int iconSize = isIconMode ? 32 : 24;
  constexpr int menuFontId = UI_10_FONT_ID;
  const int lineHeight = renderer.getLineHeight(menuFontId);

  for (int i = 0; i < buttonCount; ++i) {
    const int rowY = menuTop + i * (rowHeight + rowSpacing);
    const bool isSelected = (selectedIndex == i);

    if (isSelected) {
      // Soft light-gray dithered fill with 2px black rounded outline and bold text
      renderer.fillRoundedRect(menuLeft, rowY, menuWidth, rowHeight, kMenuRadius, Color::LightGray);
      renderer.drawRoundedRect(menuLeft, rowY, menuWidth, rowHeight, 2, kMenuRadius, true);
    } else {
      // Soft clean outline for unselected items
      renderer.fillRoundedRect(menuLeft, rowY, menuWidth, rowHeight, kMenuRadius, Color::White);
      renderer.drawRoundedRect(menuLeft, rowY, menuWidth, rowHeight, 1, kMenuRadius, true);
    }

    if (rowIcon != nullptr) {
      const UIIcon icon = rowIcon(i);
      const uint8_t* iconBmp = cuteIconForName(icon, iconSize);
      if (iconBmp) {
        const int iconX = isIconMode ? (menuLeft + (menuWidth - iconSize) / 2) : (menuLeft + 14);
        const int iconY = rowY + (rowHeight - iconSize) / 2;
        renderer.drawIcon(iconBmp, iconX, iconY, iconSize);
      }
    }

    const std::string label = buttonLabel(i);

    if (!isIconMode) {
      const int textX = menuLeft + 14 + iconSize + 10;
      const int maxTextW = std::max(20, menuLeft + menuWidth - textX - (isSelected ? 26 : 10));
      const EpdFontFamily::Style fontStyle = isSelected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
      const std::string truncated = renderer.truncatedText(menuFontId, label.c_str(), maxTextW, fontStyle);
      const int textY = rowY + (rowHeight - lineHeight) / 2;
      renderer.drawText(menuFontId, textX, textY, truncated.c_str(), true, fontStyle);

      if (isSelected) {
        // Cute trailing chevron on the right edge
        renderer.drawText(menuFontId, menuLeft + menuWidth - 20, textY, "›", true, EpdFontFamily::BOLD);
      }
    }

    if (isSelected) {
      const bool isContinueReading =
          (rowIcon != nullptr && rowIcon(i) == UIIcon::Book) || (label == tr(STR_CONTINUE_READING));
      if (isContinueReading && !s_recentBookTitle.empty()) {
        // Display the book name next to "Continue Reading" when selected
        const int cardX = menuLeft + menuWidth + 12;
        const int cardW = pageWidth - cardX - 16;
        if (cardW > 60) {
          const int cardH = rowHeight;
          renderer.fillRoundedRect(cardX, rowY, cardW, cardH, kMenuRadius, Color::White);
          renderer.drawRoundedRect(cardX, rowY, cardW, cardH, 1, kMenuRadius, true);

          const auto lines = renderer.wrappedText(menuFontId, s_recentBookTitle.c_str(), cardW - 20, 2);
          if (!lines.empty()) {
            const int totalTextH = static_cast<int>(lines.size()) * lineHeight;
            int lineY = rowY + (cardH - totalTextH) / 2;
            for (const auto& line : lines) {
              renderer.drawText(menuFontId, cardX + 10, lineY, line.c_str(), true, EpdFontFamily::REGULAR);
              lineY += lineHeight;
            }
          }
        }
      }
    }
  }
}

void MyLaCuteTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                        const int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                        bool& bufferRestored, std::function<bool()> storeCoverBuffer) const {
  (void)renderer;
  (void)rect;
  (void)selectorIndex;
  (void)coverBufferStored;
  (void)bufferRestored;
  (void)storeCoverBuffer;
  if (!recentBooks.empty()) {
    s_recentBookTitle = recentBooks[0].title;
  } else {
    s_recentBookTitle.clear();
  }
  coverRendered = true;
}

Rect MyLaCuteTheme::drawPopup(const GfxRenderer& renderer, const char* message) const {
  const auto pageWidth = renderer.getScreenWidth();
  const int padding = MyLaCuteMetrics::values.popupMarginX;
  const int popupWidth = pageWidth - 2 * padding;
  const int lineHeight = renderer.getLineHeight(UI_12_FONT_ID);
  const auto lines = renderer.wrappedText(UI_12_FONT_ID, message, popupWidth - 36, 4);
  const int popupHeight = static_cast<int>(lines.size()) * lineHeight + 40;
  const int popupX = padding;
  const int popupY = static_cast<int>(renderer.getScreenHeight() * MyLaCuteMetrics::values.popupTopOffsetRatio);

  // Cute double-framed rounded popup
  renderer.fillRoundedRect(popupX, popupY, popupWidth, popupHeight, kMenuRadius, Color::White);
  renderer.drawRoundedRect(popupX, popupY, popupWidth, popupHeight, 2, kMenuRadius, true);
  renderer.drawRoundedRect(popupX + 4, popupY + 4, popupWidth - 8, popupHeight - 8, 1, kMenuRadius - 2, true);

  int textY = popupY + 20;
  for (const auto& line : lines) {
    const int textW = renderer.getTextWidth(UI_12_FONT_ID, line.c_str(), EpdFontFamily::BOLD);
    const int textX = popupX + (popupWidth - textW) / 2;
    renderer.drawText(UI_12_FONT_ID, textX, textY, line.c_str(), true, EpdFontFamily::BOLD);
    textY += lineHeight;
  }

  return Rect{popupX, popupY, popupWidth, popupHeight};
}
