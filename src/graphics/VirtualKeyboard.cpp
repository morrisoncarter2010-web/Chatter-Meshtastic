#include "configuration.h"
#if HAS_SCREEN
#include "VirtualKeyboard.h"
#include "graphics/Screen.h"
#include "graphics/ScreenFonts.h"
#include "graphics/SharedUIDisplay.h"
#include "main.h"
#include <Arduino.h>
#include <vector>

namespace graphics
{

VirtualKeyboard::VirtualKeyboard()
    : cursorRow(0), cursorCol(0), lastActivityTime(millis()), multiTapFeedbackActive(false), multiTapActiveChar(0),
      multiTapKey(0), multiTapFeedbackTime(0)
{
    initializeKeyboard();
    // Set cursor to H(2, 5)
    cursorRow = 2;
    cursorCol = 5;
}

VirtualKeyboard::~VirtualKeyboard() {}

void VirtualKeyboard::initializeKeyboard()
{
    // New 4 row, 11 column keyboard layout:
    static const char LAYOUT[KEYBOARD_ROWS][KEYBOARD_COLS] = {{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\b'},
                                                              {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '\n'},
                                                              {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', ' '},
                                                              {'z', 'x', 'c', 'v', 'b', 'n', 'm', '.', ',', '?', '\x1b'}};

    // Derive layout dimensions and assert they match the configured keyboard grid
    constexpr int LAYOUT_ROWS = (int)(sizeof(LAYOUT) / sizeof(LAYOUT[0]));
    constexpr int LAYOUT_COLS = (int)(sizeof(LAYOUT[0]) / sizeof(LAYOUT[0][0]));
    static_assert(LAYOUT_ROWS == KEYBOARD_ROWS, "LAYOUT rows must equal KEYBOARD_ROWS");
    static_assert(LAYOUT_COLS == KEYBOARD_COLS, "LAYOUT cols must equal KEYBOARD_COLS");

    // Initialize all keys to empty first
    for (int row = 0; row < LAYOUT_ROWS; row++) {
        for (int col = 0; col < LAYOUT_COLS; col++) {
            keyboard[row][col] = {0, VK_CHAR, 0, 0, 0, 0};
        }
    }

    // Fill keyboard from the 2D layout
    for (int row = 0; row < LAYOUT_ROWS; row++) {
        for (int col = 0; col < LAYOUT_COLS; col++) {
            char ch = LAYOUT[row][col];
            // No empty slots in the simplified layout

            VirtualKeyType type = VK_CHAR;
            if (ch == '\b') {
                type = VK_BACKSPACE;
            } else if (ch == '\n') {
                type = VK_ENTER;
            } else if (ch == '\x1b') { // ESC
                type = VK_ESC;
            } else if (ch == ' ') {
                type = VK_SPACE;
            }

            // Make action keys wider to fit text while keeping the last column aligned
            uint8_t width = (type == VK_BACKSPACE || type == VK_ENTER || type == VK_SPACE) ? (KEY_WIDTH * 3) : KEY_WIDTH;
            keyboard[row][col] = {ch, type, (uint8_t)(col * KEY_WIDTH), (uint8_t)(row * KEY_HEIGHT), width, KEY_HEIGHT};
        }
    }
}

void VirtualKeyboard::draw(OLEDDisplay *display, int16_t offsetX, int16_t offsetY)
{
    // Repeat ticking is driven by NotificationRenderer once per frame
    // Base styles
    display->setColor(WHITE);
    display->setFont(FONT_SMALL);

    // Screen geometry
    const int screenW = display->getWidth();
    const int screenH = display->getHeight();

    // Decide wide-screen mode: if there is comfortable width, allow taller keys and reserve fixed width for last column labels
    // Heuristic: if screen width >= 200px (e.g., 240x135), treat as wide
    const bool isWide = screenW >= 200;

    // Determine last-column label max width
    display->setFont(FONT_SMALL);
    const int wENTER = display->getStringWidth("ENTER");
    int lastColLabelW = wENTER; // ENTER is usually the widest
    // Smaller padding on very small screens to avoid excessive whitespace
    const int lastColPad = (screenW <= 128 ? 2 : 6);
    const int reservedLastColW = lastColLabelW + lastColPad; // reserved width for last column keys

    // Always reserve width for the rightmost text column to avoid overlap on small screens
    int cellW = 0;
    int leftoverW = 0;
    {
        const int leftCols = KEYBOARD_COLS - 1; // 10 input characters
        int usableW = screenW - reservedLastColW;
        if (usableW < leftCols) {
            // Guard: ensure at least 1px per left cell if labels are extremely wide (unlikely)
            usableW = leftCols;
        }
        cellW = usableW / leftCols;
        leftoverW = usableW - cellW * leftCols; // distribute extra pixels over left columns (left to right)
    }

    // Dynamic key geometry
    int cellH = KEY_HEIGHT;
    int keyboardStartY = 0;
    if (screenH <= 64) {
        const int headerHeight = headerText.empty() ? 0 : (FONT_HEIGHT_SMALL - 2);
        const int gapBelowHeader = 0;
        const int singleLineBoxHeight = FONT_HEIGHT_SMALL;
        const int gapAboveKeyboard = 0;
        keyboardStartY = offsetY + headerHeight + gapBelowHeader + singleLineBoxHeight + gapAboveKeyboard;
        if (keyboardStartY < 0)
            keyboardStartY = 0;
        if (keyboardStartY > screenH)
            keyboardStartY = screenH;
        int keyboardHeight = screenH - keyboardStartY;
        cellH = std::max(1, keyboardHeight / KEYBOARD_ROWS);
    } else if (isWide) {
        // For wide screens (e.g., T114 240x135), prefer square keys: height equals left-column key width.
        cellH = std::max((int)KEY_HEIGHT, cellW);

        // Guarantee at least 2 lines of input are visible by reducing cell height minimally if needed.
        // Replicate the spacing used in drawInputArea(): headerGap=1, box-to-header gap=1, gap above keyboard=1
        display->setFont(FONT_SMALL);
        const int headerHeight = headerText.empty() ? 0 : (FONT_HEIGHT_SMALL + 1);
        const int headerToBoxGap = 1;
        const int gapAboveKb = 1;
        const int minBoxHeightForTwoLines = 2 * FONT_HEIGHT_SMALL + 2; // inner 1px top/bottom
        int maxKeyboardHeight = screenH - (offsetY + headerHeight + headerToBoxGap + minBoxHeightForTwoLines + gapAboveKb);
        int maxCellHAllowed = maxKeyboardHeight / KEYBOARD_ROWS;
        if (maxCellHAllowed < (int)KEY_HEIGHT)
            maxCellHAllowed = KEY_HEIGHT;
        if (maxCellHAllowed > 0 && cellH > maxCellHAllowed) {
            cellH = maxCellHAllowed;
        }
        // Keyboard placement from bottom for wide screens
        int keyboardHeight = KEYBOARD_ROWS * cellH;
        keyboardStartY = screenH - keyboardHeight;
        if (keyboardStartY < 0)
            keyboardStartY = 0;
    } else {
        // Default (non-wide, non-64px) e.g. SH1107 128x128:
        // cellH = FONT_HEIGHT_SMALL - 2 so rows are tighter while still hosting the font
        cellH = std::max((int)KEY_HEIGHT, FONT_HEIGHT_SMALL - 2);
        int keyboardHeight = KEYBOARD_ROWS * cellH;
        keyboardStartY = screenH - keyboardHeight;
        if (keyboardStartY < 0)
            keyboardStartY = 0;
    }

    // Draw input area above keyboard
    drawInputArea(display, offsetX, offsetY, keyboardStartY);

    // While the physical Chatter keypad is actively cycling a multi-tap key,
    // temporarily replace the QWERTY grid with a large visual indicator.
    if (multiTapFeedbackActive) {
        if ((millis() - multiTapFeedbackTime) < MULTITAP_FEEDBACK_MS) {
            drawMultiTapFeedback(display, offsetX, offsetY, keyboardStartY);
            return;
        }
        multiTapFeedbackActive = false;
    }

    // Precompute per-column x and width with leftover distributed over left columns for even spacing
    int colX[KEYBOARD_COLS];
    int colW[KEYBOARD_COLS];
    int runningX = offsetX;
    for (int col = 0; col < KEYBOARD_COLS - 1; ++col) {
        int wcol = cellW + (col < leftoverW ? 1 : 0);
        colX[col] = runningX;
        colW[col] = wcol;
        runningX += wcol;
    }
    // Last column
    colX[KEYBOARD_COLS - 1] = runningX;
    colW[KEYBOARD_COLS - 1] = reservedLastColW;

    // Draw keyboard grid
    for (int row = 0; row < KEYBOARD_ROWS; row++) {
        for (int col = 0; col < KEYBOARD_COLS; col++) {
            const VirtualKey &k = keyboard[row][col];
            if (k.character != 0 || k.type != VK_CHAR) {
                const bool isLastCol = (col == KEYBOARD_COLS - 1);
                int x = colX[col];
                int w = colW[col];
                int y = offsetY + keyboardStartY + row * cellH;
                int h = cellH;
                bool selected = (row == cursorRow && col == cursorCol);
                drawKey(display, k, selected, x, y, (uint8_t)w, (uint8_t)h, isLastCol);
            }
        }
    }
}

void VirtualKeyboard::showMultiTapFeedback(char c)
{
    multiTapActiveChar = c;
    multiTapKey = 0;
    multiTapGroup.clear();

    switch (c) {
    case '.': case ',': case '?': case '1':
        multiTapKey = '1'; multiTapGroup = ".,?1"; break;
    case '!': case '+': case '-':
        multiTapKey = '1'; multiTapGroup = "!+-1"; break;
    case 'a': case 'b': case 'c': case '2':
        multiTapKey = '2'; multiTapGroup = "abc2"; break;
    case 'A': case 'B': case 'C':
        multiTapKey = '2'; multiTapGroup = "ABC2"; break;
    case 'd': case 'e': case 'f': case '3':
        multiTapKey = '3'; multiTapGroup = "def3"; break;
    case 'D': case 'E': case 'F':
        multiTapKey = '3'; multiTapGroup = "DEF3"; break;
    case 'g': case 'h': case 'i': case '4':
        multiTapKey = '4'; multiTapGroup = "ghi4"; break;
    case 'G': case 'H': case 'I':
        multiTapKey = '4'; multiTapGroup = "GHI4"; break;
    case 'j': case 'k': case 'l': case '5':
        multiTapKey = '5'; multiTapGroup = "jkl5"; break;
    case 'J': case 'K': case 'L':
        multiTapKey = '5'; multiTapGroup = "JKL5"; break;
    case 'm': case 'n': case 'o': case '6':
        multiTapKey = '6'; multiTapGroup = "mno6"; break;
    case 'M': case 'N': case 'O':
        multiTapKey = '6'; multiTapGroup = "MNO6"; break;
    case 'p': case 'q': case 'r': case 's':
        multiTapKey = '7'; multiTapGroup = "pqrs"; break;
    case 'P': case 'Q': case 'R': case 'S':
        multiTapKey = '7'; multiTapGroup = "PQRS"; break;
    case '7':
        multiTapKey = '7'; multiTapGroup = "7"; break;
    case 't': case 'u': case 'v': case '8':
        multiTapKey = '8'; multiTapGroup = "tuv8"; break;
    case 'T': case 'U': case 'V':
        multiTapKey = '8'; multiTapGroup = "TUV8"; break;
    case 'w': case 'x': case 'y': case 'z':
        multiTapKey = '9'; multiTapGroup = "wxyz"; break;
    case 'W': case 'X': case 'Y': case 'Z':
        multiTapKey = '9'; multiTapGroup = "WXYZ"; break;
    case '9':
        multiTapKey = '9'; multiTapGroup = "9"; break;
    case ' ':
        multiTapKey = '0'; multiTapGroup = "SPACE"; break;
    case '0':
        multiTapKey = '0'; multiTapGroup = "0"; break;
    default:
        return;
    }

    multiTapFeedbackTime = millis();
    multiTapFeedbackActive = true;
}

void VirtualKeyboard::drawMultiTapFeedback(OLEDDisplay *display, int16_t offsetX, int16_t offsetY, int16_t keyboardStartY)
{
    const int screenW = display->getWidth();
    const int screenH = display->getHeight();
    const int areaTop = offsetY + keyboardStartY;
    const int areaH = std::max(1, screenH - areaTop);

    display->setColor(WHITE);
    display->setFont(FONT_SMALL);
    display->setTextAlignment(TEXT_ALIGN_LEFT);

    // Key number centered near the top of the keyboard area.
    char keyLabel[2] = {multiTapKey, '\0'};
    int keyW = display->getStringWidth(keyLabel);
    int keyY = areaTop + std::max(1, areaH / 8);
    display->drawString(offsetX + (screenW - keyW) / 2, keyY, keyLabel);

    // Center the complete group and invert only the currently selected character.
    const int gap = std::max(4, screenW / 32);
    int totalW = 0;
    for (size_t i = 0; i < multiTapGroup.size(); ++i) {
        char label[2] = {multiTapGroup[i], '\0'};
        totalW += display->getStringWidth(label) + 6;
        if (i + 1 < multiTapGroup.size())
            totalW += gap;
    }

    int x = offsetX + std::max(0, (screenW - totalW) / 2);
    int y = areaTop + std::max(FONT_HEIGHT_SMALL + 3, areaH / 2 - FONT_HEIGHT_SMALL / 2);

    for (size_t i = 0; i < multiTapGroup.size(); ++i) {
        char shown = multiTapGroup[i];
        char label[2] = {shown, '\0'};
        int charW = display->getStringWidth(label);
        int boxW = charW + 6;
        bool selected = (shown == multiTapActiveChar);

        // The physical 0 key emits a space; show SPACE as a single highlighted label.
        if (multiTapGroup == "SPACE") {
            const char *spaceLabel = "SPACE";
            int spaceW = display->getStringWidth(spaceLabel);
            int bx = offsetX + (screenW - (spaceW + 10)) / 2;
            int by = y - 2;
            display->fillRect(bx, by, spaceW + 10, FONT_HEIGHT_SMALL + 4);
            display->setColor(BLACK);
            display->drawString(bx + 5, y, spaceLabel);
            display->setColor(WHITE);
            return;
        }

        if (selected) {
            display->fillRect(x, y - 2, boxW, FONT_HEIGHT_SMALL + 4);
            display->setColor(BLACK);
        }
        display->drawString(x + 3, y, label);
        if (selected)
            display->setColor(WHITE);

        x += boxW + gap;
    }
}

void VirtualKeyboard::drawInputArea(OLEDDisplay *display, int16_t offsetX, int16_t offsetY, int16_t keyboardStartY)
{
    display->setColor(WHITE);

    const int screenWidth = display->getWidth();
    const int screenHeight = display->getHeight();
    // Use the standard small font metrics for input box sizing (restore original size)
    const int inputLineH = FONT_HEIGHT_SMALL;

    // Header uses the standard small (which may be larger on big screens)
    display->setFont(FONT_SMALL);
    int headerHeight = 0;
    if (!headerText.empty()) {
        // Draw header and reserve exact font height (plus a tighter gap) to maximize input area
        display->drawString(offsetX + 2, offsetY, headerText.c_str());
        if (screenHeight <= 64) {
            headerHeight = FONT_HEIGHT_SMALL - 2; // 11px
        } else {
            headerHeight = FONT_HEIGHT_SMALL; // no extra padding baked in
        }
    }

    const int boxX = offsetX;
    const int boxWidth = screenWidth;
    int boxY;
    int boxHeight;
    if (screenHeight <= 64) {
        const int gapBelowHeader = 0;
        const int fixedBoxHeight = inputLineH;
        const int gapAboveKeyboard = 0;
        boxY = offsetY + headerHeight + gapBelowHeader;
        boxHeight = fixedBoxHeight;
        if (boxY + boxHeight + gapAboveKeyboard > keyboardStartY) {
            int over = boxY + boxHeight + gapAboveKeyboard - keyboardStartY;
            boxHeight = std::max(1, fixedBoxHeight - over);
        }
    } else {
        const int gapBelowHeader = 1;
        int gapAboveKeyboard = 1;
        int tmpBoxY = offsetY + headerHeight + gapBelowHeader;
        const int minBoxHeight = inputLineH + 2;
        int availableH = keyboardStartY - tmpBoxY - gapAboveKeyboard;
        if (availableH < minBoxHeight)
            availableH = minBoxHeight;
        boxY = tmpBoxY;
        boxHeight = availableH;
    }

    // Draw box border
    display->drawRect(boxX, boxY, boxWidth, boxHeight);

    display->setFont(FONT_SMALL);

    // Text rendering: multi-line if space allows (>= 2 lines), else single-line with leading ellipsis
    const int textX = boxX + 2;
    const int maxTextWidth = boxWidth - 4;
    const int maxLines = (boxHeight - 2) / inputLineH;
    if (maxLines >= 2) {
        // Inner bounds for caret clamping
        const int innerLeft = boxX + 1;
        const int innerRight = boxX + boxWidth - 2;
        const int innerTop = boxY + 1;
        const int innerBottom = boxY + boxHeight - 2;

        // Wrap text greedily into lines that fit maxTextWidth
        std::vector<std::string> lines;
        {
            std::string remaining = inputText;
            while (!remaining.empty()) {
                int bestLen = 0;
                for (int len = 1; len <= (int)remaining.size(); ++len) {
                    int w = display->getStringWidth(remaining.substr(0, len).c_str());
                    if (w <= maxTextWidth)
                        bestLen = len;
                    else
                        break;
                }
                if (bestLen == 0) {
                    // At least show one character to make progress
                    bestLen = 1;
                }
                lines.emplace_back(remaining.substr(0, bestLen));
                remaining.erase(0, bestLen);
            }
        }

        const bool scrolledUp = ((int)lines.size() > maxLines);
        int caretX = textX;
        int caretY = innerTop;

        // Leave a small top gap to render '...' without replacing the first line
        const int topInset = 2;
        const int lineStep = std::max(1, inputLineH - 1); // slightly tighter than font height
        int lineY = innerTop + topInset;

        if (scrolledUp) {
            // Draw three small dots centered horizontally, vertically at the midpoint of the gap
            // between the inner top and the first line's top baseline. This avoids using a tall glyph.
            const int firstLineTop = lineY;                                   // baseline top for the first visible line
            const int gapMidY = innerTop + (firstLineTop - innerTop) / 2 + 1; // shift down 1px as requested
            const int centerX = boxX + boxWidth / 2;
            const int dotSpacing = 3; // px between dots
            const int dotSize = 1;    // small square dot
            display->fillRect(centerX - dotSpacing, gapMidY, dotSize, dotSize);
            display->fillRect(centerX, gapMidY, dotSize, dotSize);
            display->fillRect(centerX + dotSpacing, gapMidY, dotSize, dotSize);
        }

        // How many lines fit with our top inset and tighter step
        const int linesCapacity = std::max(1, (innerBottom - lineY + 1) / lineStep);
        const int linesToShow = std::min((int)lines.size(), linesCapacity);
        const int startIndex = scrolledUp ? ((int)lines.size() - linesToShow) : 0;

        for (int i = 0; i < linesToShow; ++i) {
            const std::string &chunk = lines[startIndex + i];
            display->drawString(textX, lineY, chunk.c_str());
            caretX = textX + display->getStringWidth(chunk.c_str());
            caretY = lineY;
            lineY += lineStep;
        }

        // Draw caret at end of the last visible line
        int caretPadY = 2;
        if (boxHeight >= inputLineH + 4)
            caretPadY = 3;
        int cursorTop = caretY + caretPadY;
        // Use lineStep so caret height matches the row spacing
        int cursorH = lineStep - caretPadY * 2;
        if (cursorH < 1)
            cursorH = 1;
        // Clamp vertical bounds to stay inside the inner rect
        if (cursorTop < innerTop)
            cursorTop = innerTop;
        if (cursorTop + cursorH - 1 > innerBottom)
            cursorH = innerBottom - cursorTop + 1;
        if (cursorH < 1)
            cursorH = 1;
        // Only draw if cursor is inside inner bounds
        if (caretX >= innerLeft && caretX <= innerRight) {
            display->drawVerticalLine(caretX, cursorTop, cursorH);
        }
    } else {
        std::string displayText = inputText;
        int textW = display->getStringWidth(displayText.c_str());
        std::string scrolled = displayText;
        if (textW > maxTextWidth) {
            // Trim from the left until it fits
            while (textW > maxTextWidth && !scrolled.empty()) {
                scrolled.erase(0, 1);
                textW = display->getStringWidth(scrolled.c_str());
            }
            // Add leading ellipsis and ensure it still fits
            if (scrolled != displayText) {
                scrolled = "..." + scrolled;
                textW = display->getStringWidth(scrolled.c_str());
                // If adding ellipsis causes overflow, trim more after the ellipsis
                while (textW > maxTextWidth && scrolled.size() > 3) {
                    scrolled.erase(3, 1); // remove chars after the ellipsis
                    textW = display->getStringWidth(scrolled.c_str());
                }
            }
        } else {
            // Keep textW in sync with what we draw
            textW = display->getStringWidth(scrolled.c_str());
        }

        int textY;
        if (screenHeight <= 64) {
            textY = boxY + (boxHeight - inputLineH) / 2;
        } else {
            const int innerTop = boxY + 1;
            const int innerBottom = boxY + boxHeight - 2;

            // Center text vertically within inner box for single-line, then clamp so it never overlaps borders
            int innerH = innerBottom - innerTop + 1;
            textY = innerTop + std::max(0, (innerH - inputLineH) / 2);
            // Clamp fully inside the inner rect
            if (textY < innerTop)
                textY = innerTop;
            int maxTop = innerBottom - inputLineH + 1;
            if (textY > maxTop)
                textY = maxTop;
        }

        if (!scrolled.empty()) {
            display->drawString(textX, textY, scrolled.c_str());
        }

        int cursorX = textX + textW;
        if (screenHeight > 64) {
            const int innerRight = boxX + boxWidth - 2;
            if (cursorX > innerRight)
                cursorX = innerRight;
        }

        int cursorTop, cursorH;
        if (screenHeight <= 64) {
            cursorH = 10;
            cursorTop = boxY + (boxHeight - cursorH) / 2;
        } else {
            const int innerLeft = boxX + 1;
            const int innerRight = boxX + boxWidth - 2;
            const int innerTop = boxY + 1;
            const int innerBottom = boxY + boxHeight - 2;

            cursorTop = boxY + 2;
            cursorH = boxHeight - 4;
            if (cursorH < 1)
                cursorH = 1;
            if (cursorTop < innerTop)
                cursorTop = innerTop;
            if (cursorTop + cursorH - 1 > innerBottom)
                cursorH = innerBottom - cursorTop + 1;
            if (cursorH < 1)
                cursorH = 1;

            if (cursorX < innerLeft || cursorX > innerRight)
                return;
        }

        display->drawVerticalLine(cursorX, cursorTop, cursorH);
    }
}

void VirtualKeyboard::drawKey(OLEDDisplay *display, const VirtualKey &key, bool selected, int16_t x, int16_t y, uint8_t width,
                              uint8_t height, bool isLastCol)
{
    // Draw key content
    display->setFont(FONT_SMALL);
    const int fontH = FONT_HEIGHT_SMALL;
    // Build label and metrics first
    std::string keyText;
    if (key.type == VK_BACKSPACE || key.type == VK_ENTER || key.type == VK_SPACE || key.type == VK_ESC) {
        // Keep literal text labels for the action keys on the rightmost column
        keyText = (key.type == VK_BACKSPACE) ? "BACK"
                  : (key.type == VK_ENTER)   ? "ENTER"
                  : (key.type == VK_SPACE)   ? "SPACE"
                  : (key.type == VK_ESC)     ? "ESC"
                                             : "";
    } else {
        char c = getCharForKey(key, false);
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }
        keyText = (key.character == ' ' || key.character == '_') ? "_" : std::string(1, c);
        // Show the common "/" pairing next to "?" like on a real keyboard
        if (key.type == VK_CHAR && key.character == '?') {
            keyText = "?/";
        }
    }

    int textWidth = display->getStringWidth(keyText.c_str());
    // Label alignment
    // - Rightmost action column: right-align text with a small right padding (~2px) so it hugs screen edge neatly.
    // - Other keys: center horizontally; use ceil-style rounding to avoid appearing left-biased on odd widths.
    int textX;
    if (isLastCol) {
        const int rightPad = 1;
        textX = x + width - textWidth - rightPad;
        if (textX < x)
            textX = x; // guard
    } else {
        // Use ceil rounding for all screens (consistent with 128x64 behavior for numbers)
        textX = x + (width - textWidth + 1) / 2;
    }
    int contentTop = y;
    int contentH = height;
    if (selected) {
        display->setColor(WHITE);
        bool isAction = (key.type == VK_BACKSPACE || key.type == VK_ENTER || key.type == VK_SPACE || key.type == VK_ESC);

        if (display->getHeight() <= 64 && !isAction) {
            display->fillRect(x, y, width, height);
        } else if (isAction) {
            const int padX = 1;
            const int padY = 2;
            int hlW = textWidth + padX * 2;
            int hlX = textX - padX;

            if (hlX < x) {
                hlW -= (x - hlX);
                hlX = x;
            }
            int maxW = (x + width) - hlX;
            if (hlW > maxW)
                hlW = maxW;
            if (hlW < 1)
                hlW = 1;

            int hlH = std::min(fontH + padY * 2, (int)height);
            int hlY = y + (height - hlH) / 2;
            display->fillRect(hlX, hlY, hlW, hlH);
            contentTop = hlY;
            contentH = hlH;
        } else {
            display->fillRect(x, y, width, height);
        }
        display->setColor(BLACK);
    } else {
        display->setColor(WHITE);
    }

    int centeredTextY;
    if (display->getHeight() <= 64) {
        centeredTextY = y + (height - fontH) / 2;
    } else {
        centeredTextY = contentTop + (contentH - fontH) / 2;
    }
    if (display->getHeight() > 64) {
        if (centeredTextY < contentTop)
            centeredTextY = contentTop;
        if (centeredTextY + fontH > contentTop + contentH)
            centeredTextY = std::max(contentTop, contentTop + contentH - fontH);
    }

    if (display->getHeight() <= 64 && keyText.size() == 1) {
        char ch = keyText[0];
        if (ch == '.' || ch == ',' || ch == ';') {
            centeredTextY -= 1;
        }
    }
#ifdef MUZI_BASE // Correct issue with character vertical position on MUZI_BASE
    centeredTextY -= 2;
#endif
    display->drawString(textX, centeredTextY, keyText.c_str());
}

char VirtualKeyboard::getCharForKey(const VirtualKey &key, bool isLongPress)
{
    if (key.type != VK_CHAR) {
        return key.character;
    }

    char c = key.character;

    // Long-press: letters become uppercase; for "?" provide "/" like a typical keyboard
    if (isLongPress) {
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        } else if (c == '?') {
            c = '/';
        }
    }

    return c;
}

void VirtualKeyboard::moveCursorDelta(int dRow, int dCol)
{
    resetTimeout();
    // wrap around rows and cols in the 4x11 grid
    int r = (int)cursorRow + dRow;
    int c = (int)cursorCol + dCol;
    if (r < 0)
        r = KEYBOARD_ROWS - 1;
    else if (r >= KEYBOARD_ROWS)
        r = 0;
    if (c < 0)
        c = KEYBOARD_COLS - 1;
    else if (c >= KEYBOARD_COLS)
        c = 0;
    cursorRow = (uint8_t)r;
    cursorCol = (uint8_t)c;
}

void VirtualKeyboard::moveCursorUp()
{
    moveCursorDelta(-1, 0);
}
void VirtualKeyboard::moveCursorDown()
{
    moveCursorDelta(1, 0);
}
void VirtualKeyboard::moveCursorLeft()
{
    resetTimeout();

    if (cursorCol > 0) {
        cursorCol--;
    } else {
        if (cursorRow > 0) {
            cursorRow--;
            cursorCol = KEYBOARD_COLS - 1;
        } else {
            cursorRow = KEYBOARD_ROWS - 1;
            cursorCol = KEYBOARD_COLS - 1;
        }
    }
}
void VirtualKeyboard::moveCursorRight()
{
    resetTimeout();

    if (cursorCol < KEYBOARD_COLS - 1) {
        cursorCol++;
    } else {
        if (cursorRow < KEYBOARD_ROWS - 1) {
            cursorRow++;
            cursorCol = 0;
        } else {
            cursorRow = 0;
            cursorCol = 0;
        }
    }
}

void VirtualKeyboard::handlePress()
{
    resetTimeout(); // Reset timeout on any input activity

    const VirtualKey &key = keyboard[cursorRow][cursorCol];

    // Don't handle press if the key is empty (but allow special keys)
    if (key.character == 0 && key.type == VK_CHAR) {
        return;
    }

    // For character keys, insert lowercase character
    if (key.type == VK_CHAR) {
        insertCharacter(getCharForKey(key, false)); // false = lowercase/normal char
        return;
    }

    // Handle non-character keys immediately
    switch (key.type) {
    case VK_BACKSPACE:
        deleteCharacter();
        break;
    case VK_ENTER:
        submitText();
        break;
    case VK_SPACE:
        insertCharacter(' ');
        break;
    case VK_ESC:
        if (onTextEntered) {
            std::function<void(const std::string &)> callback = onTextEntered;
            onTextEntered = nullptr;
            inputText = "";
            callback("");
        }
        return;
    default:
        break;
    }
}

void VirtualKeyboard::handleLongPress()
{
    resetTimeout(); // Reset timeout on any input activity

    const VirtualKey &key = keyboard[cursorRow][cursorCol];

    // Don't handle press if the key is empty (but allow special keys)
    if (key.character == 0 && key.type == VK_CHAR) {
        return;
    }

    // For character keys, insert uppercase/alternate character
    if (key.type == VK_CHAR) {
        insertCharacter(getCharForKey(key, true)); // true = uppercase/alternate char
        return;
    }

    switch (key.type) {
    case VK_BACKSPACE:
        // One-shot: delete up to 5 characters on long press
        for (int i = 0; i < 5; ++i) {
            if (inputText.empty())
                break;
            deleteCharacter();
        }
        break;
    case VK_ENTER:
        submitText();
        break;
    case VK_SPACE:
        insertCharacter(' ');
        break;
    case VK_ESC:
        if (onTextEntered) {
            onTextEntered("");
        }
        break;
    default:
        break;
    }
}

void VirtualKeyboard::insertCharacter(char c)
{
    if (inputText.length() < 160) { // Reasonable text length limit
        inputText += c;
    }
}

void VirtualKeyboard::deleteCharacter()
{
    if (!inputText.empty()) {
        inputText.pop_back();
    }
}

void VirtualKeyboard::submitText()
{
    LOG_INFO("Virtual keyboard: submitting text '%s'", inputText.c_str());

    // Only submit if text is not empty
    if (!inputText.empty() && onTextEntered) {
        // Store callback and text to submit before clearing callback
        std::function<void(const std::string &)> callback = onTextEntered;
        std::string textToSubmit = inputText;
        onTextEntered = nullptr;
        // Don't clear inputText here - let the calling module handle cleanup
        // inputText = "";  // Removed: keep text visible until module cleans up
        callback(textToSubmit);
    } else if (inputText.empty()) {
        // For empty text, just ignore the submission - don't clear callback
        // This keeps the virtual keyboard responsive for further input
        LOG_INFO("Virtual keyboard: empty text submitted, ignoring - keyboard remains active");
    } else {
        // No callback available
        if (screen) {
            screen->setFrames(graphics::Screen::FOCUS_PRESERVE);
        }
    }
}

void VirtualKeyboard::setInputText(const std::string &text)
{
    inputText = text;
}

std::string VirtualKeyboard::getInputText() const
{
    return inputText;
}

void VirtualKeyboard::setHeader(const std::string &header)
{
    headerText = header;
}

void VirtualKeyboard::setCallback(std::function<void(const std::string &)> callback)
{
    onTextEntered = callback;
}

void VirtualKeyboard::resetTimeout()
{
    lastActivityTime = millis();
}

bool VirtualKeyboard::isTimedOut() const
{
    return (millis() - lastActivityTime) > TIMEOUT_MS;
}

} // namespace graphics
#endif
