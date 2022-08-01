/*
    Borealis, a Nintendo Switch UI Library
    Copyright (C) 2019-2020  natinusala
    Copyright (C) 2019  p-sam

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <borealis/application.hpp>
#include <borealis/label.hpp>
#include <cmath>

namespace brls
{

Label::Label(LabelStyle labelStyle, std::string text, bool multiline)
    : text(text)
    , textTicker(text + "          " + text)
    , multiline(multiline)
    , labelStyle(labelStyle)
{
    this->lineHeight = this->getLineHeight(labelStyle);
    this->fontSize   = this->getFontSize(labelStyle);

    this->updateTextDimensions();

    if (this->labelStyle == LabelStyle::UNFOCUSED_TICKER) {
        this->onParentFocus();
    } else {
        this->parentFocusSubscription = Application::getGlobalFocusChangeEvent()->subscribe([this](View *view) {
            if (view && view == this->getParent())
                this->onParentFocus();
            else
                this->onParentUnfocus();
        });
    }
}

Label::~Label()
{
    this->stopTickerAnimation();

    this->resetTextAnimation();

    if (this->labelStyle != LabelStyle::UNFOCUSED_TICKER)
        Application::getGlobalFocusChangeEvent()->unsubscribe(this->parentFocusSubscription);
}

void Label::setHorizontalAlign(NVGalign align)
{
    this->horizontalAlign = align;

    this->updateTextDimensions();
}

void Label::setVerticalAlign(NVGalign align)
{
    this->verticalAlign = align;

    this->updateTextDimensions();
}

void Label::setFontSize(unsigned size)
{
    this->fontSize = size;

    this->updateTextDimensions();
}

void Label::setText(std::string text, bool invalidateParent)
{
    this->text = text;
    this->textTicker = text + "          " + text;

    this->updateTextDimensions(invalidateParent);
}

void Label::setStyle(LabelStyle style)
{
    if (this->labelStyle == LabelStyle::UNFOCUSED_TICKER)
        return;

    this->labelStyle = style;
    this->lineHeight = this->getLineHeight(style);
    this->fontSize   = this->getFontSize(style);

    this->updateTextDimensions();
}

void Label::layout(NVGcontext* vg, Style* style, FontStash* stash)
{
    if (this->text == "") return;

    NVGalign ver_align = this->multiline ? NVG_ALIGN_TOP : this->verticalAlign;

    nvgSave(vg);
    nvgReset(vg);

    nvgFontSize(vg, this->fontSize);
    nvgFontFaceId(vg, this->getFont(stash));

    nvgTextLineHeight(vg, this->lineHeight);
    nvgTextAlign(vg, this->horizontalAlign | ver_align);

    // Update width or height to text bounds
    if (this->multiline)
    {
        float bounds[4];
        nvgTextBoxBounds(vg, this->x, this->y, this->width, this->text.c_str(), nullptr, bounds);
        this->height = bounds[3] - bounds[1]; // ymax - ymin
    }
    else
    {
        this->oldWidth = this->width;
        this->width = this->textWidth;

        // Offset the position to keep the right alignment
        unsigned boxWidth = (this->oldWidth && this->oldWidth < this->textWidth) ? this->oldWidth : this->textWidth;

        if (this->horizontalAlign == NVG_ALIGN_RIGHT) {
            this->x -= boxWidth;
        } else if (this->horizontalAlign == NVG_ALIGN_CENTER) {
            this->x -= boxWidth / 2;
        }

        // Generate text with ellipsis (…)
        size_t utf8_str_len = this->getUtf8StringLength(this->text);
        unsigned ellipsisWidth = std::ceil(nvgTextBounds(vg, 0, 0, "…", nullptr, nullptr));
        unsigned diff = 0, textEllipsisWidth = 0;

        do {
            diff++;

            this->textEllipsis = this->getUtf8SubString(this->text, 0, (utf8_str_len - diff) * std::min(1.0F, static_cast<float>(this->oldWidth - ellipsisWidth) / static_cast<float>(this->textWidth)));
            this->textEllipsis += "…";

            textEllipsisWidth = std::ceil(nvgTextBounds(vg, 0, 0, this->textEllipsis.c_str(), nullptr, nullptr));
        } while(this->oldWidth && textEllipsisWidth > this->oldWidth);
    }

    nvgRestore(vg);
}

void Label::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, Style* style, FrameContext* ctx)
{
    if (this->text == "") return;

    NVGcolor valueColor = a(this->getColor(ctx->theme));
    float fontSize = static_cast<float>(this->fontSize);

    NVGalign ver_align = this->multiline ? NVG_ALIGN_TOP : this->verticalAlign;

    // Update color and size if we're dealing with value animation
    if (!this->multiline && this->textAnimation < 1.0F) {
        valueColor.a *= this->textAnimation;
        fontSize *= this->textAnimation;
    }

    // Draw
    nvgFillColor(vg, valueColor);

    nvgFontSize(vg, fontSize);
    nvgFontFaceId(vg, this->getFont(ctx->fontStash));

    nvgTextLineHeight(vg, this->lineHeight);
    nvgTextAlign(vg, this->horizontalAlign | ver_align);

    nvgBeginPath(vg);

    if (this->multiline) {
        nvgTextBox(vg, x, y, width, this->text.c_str(), nullptr);
    } else {
        unsigned boxX = x, boxY = y;
        unsigned boxWidth = (this->oldWidth && this->oldWidth < this->textWidth) ? this->oldWidth : this->textWidth; // this->width == width == this->textWidth (check layout)
        unsigned boxHeight = height > this->boundingBoxHeight ? height : this->boundingBoxHeight;

        const char *str = NULL;
        bool use_ticker = false;

        // Select the string to display
        if (this->labelStyle != LabelStyle::FPS && this->textWidth > boxWidth) {
            if (this->textAnimation >= 1.0F && this->tickerActive) {
                str = this->textTicker.c_str();
                use_ticker = true;
            } else {
                str = this->textEllipsis.c_str();
            }
        } else {
            str = this->text.c_str();
        }

        // Adjust horizontal alignment
        if (this->horizontalAlign == NVG_ALIGN_RIGHT) {
            x += boxWidth;
            if (use_ticker) x += this->textWidth + this->textTickerWidth - boxWidth;
        } else if (this->horizontalAlign == NVG_ALIGN_CENTER) {
            x += boxWidth / 2;
            if (use_ticker) x += (this->textWidth + this->textTickerWidth - boxWidth) / 2;
        }

        // Adjust vertical alignment
        if (ver_align == NVG_ALIGN_BOTTOM || ver_align == NVG_ALIGN_BASELINE) {
            y += height;
            boxY = y - this->boundingBoxHeight;
        } else if (ver_align == NVG_ALIGN_MIDDLE) {
            y += height / 2;
            boxY = y - this->boundingBoxHeight / 2;
        }

        // Scissor area, if needed
        if (use_ticker) {
            nvgSave(vg);
            nvgIntersectScissor(vg, boxX, boxY, boxWidth, boxHeight);
            x -= this->tickerOffset;
        }

        // Draw text
        nvgText(vg, x, y, str, nullptr);

        // Restore nvg context, if needed
        if (use_ticker) nvgRestore(vg);
    }
}

void Label::startTickerAnimation()
{
    if (this->text == "") return;

    if (!this->textTickerWidth)
    {
        this->updateTextDimensions();
        this->invalidate(true);
    }

    this->tickerWaitTimerCtx.duration = 1500;
    this->tickerWaitTimerCtx.cb = [&](void *userdata) {
        menu_animation_ctx_tag tag = (uintptr_t)&this->tickerOffset;
        menu_animation_kill_by_tag(&tag);

        this->tickerOffset = 0.0f;

        menu_animation_ctx_entry_t entry;
        entry.cb           = [&](void *userdata) { menu_timer_start(&this->tickerWaitTimer, &this->tickerWaitTimerCtx); };
        entry.duration     = this->textTickerWidth * 15;
        entry.easing_enum  = EASING_LINEAR;
        entry.subject      = &this->tickerOffset;
        entry.tag          = tag;
        entry.target_value = this->textTickerWidth;
        entry.tick         = [](void* userdata) {};
        entry.userdata     = nullptr;

        menu_animation_push(&entry);
    };
    this->tickerWaitTimerCtx.tick = [](void*){};
    this->tickerWaitTimerCtx.userdata = nullptr;

    menu_timer_start(&this->tickerWaitTimer, &this->tickerWaitTimerCtx);
}

void Label::stopTickerAnimation()
{
    menu_timer_kill(&this->tickerWaitTimer);

    menu_animation_ctx_tag tag = (uintptr_t)&this->tickerOffset;
    menu_animation_kill_by_tag(&tag);

    this->tickerOffset = 0.0f;
}

void Label::resetTickerAnimation(void)
{
    if (!this->tickerActive) return;
    this->stopTickerAnimation();
    this->startTickerAnimation();
}

const std::string& Label::getText()
{
    return this->text;
}

unsigned Label::getTextWidth()
{
    return this->textWidth;
}

unsigned Label::getTextHeight()
{
    return this->textHeight;
}

void Label::setColor(NVGcolor color)
{
    this->customColor    = color;
    this->useCustomColor = true;
}

void Label::unsetColor()
{
    this->useCustomColor = false;
}

NVGcolor Label::getColor(Theme* theme)
{
    // Use custom color if any
    if (this->useCustomColor)
        return a(this->customColor);

    switch (this->labelStyle)
    {
        case LabelStyle::DESCRIPTION:
            return a(theme->descriptionColor);
        case LabelStyle::CRASH:
            return RGB(255, 255, 255);
        case LabelStyle::BUTTON_PRIMARY:
            return a(theme->buttonPrimaryEnabledTextColor);
        case LabelStyle::BUTTON_PRIMARY_DISABLED:
            return a(theme->buttonPrimaryDisabledTextColor);
        case LabelStyle::NOTIFICATION:
            return a(theme->notificationTextColor);
        case LabelStyle::BUTTON_DIALOG:
            return a(theme->dialogButtonColor);
        case LabelStyle::BUTTON_BORDERED:
            return a(theme->buttonBorderedTextColor);
        case LabelStyle::BUTTON_REGULAR:
            return a(theme->buttonRegularTextColor);
        case LabelStyle::FPS:
        case LabelStyle::LIST_ITEM_VALUE:
            return a(theme->listItemValueColor);
        case LabelStyle::LIST_ITEM_VALUE_FAINT:
            return a(theme->listItemFaintValueColor);
        default:
            return a(theme->textColor);
    }
}

void Label::setFont(int font)
{
    this->customFont    = font;
    this->useCustomFont = true;

    this->updateTextDimensions();
}

void Label::unsetFont()
{
    this->useCustomFont = false;

    this->updateTextDimensions();
}

int Label::getFont(FontStash* stash)
{
    if (this->useCustomFont)
        return this->customFont;

    return stash->regular;
}

unsigned Label::getFontSize(LabelStyle labelStyle)
{
    Style* style = Application::getStyle();

    switch (labelStyle)
    {
        case LabelStyle::REGULAR:
        case LabelStyle::UNFOCUSED_TICKER:
            return style->Label.regularFontSize;
        case LabelStyle::MEDIUM:
            return style->Label.mediumFontSize;
        case LabelStyle::SMALL:
            return style->Label.smallFontSize;
        case LabelStyle::DESCRIPTION:
            return style->Label.descriptionFontSize;
        case LabelStyle::CRASH:
            return style->Label.crashFontSize;
        case LabelStyle::BUTTON_PRIMARY_DISABLED:
        case LabelStyle::BUTTON_PRIMARY:
        case LabelStyle::BUTTON_BORDERLESS:
        case LabelStyle::BUTTON_DIALOG:
        case LabelStyle::BUTTON_BORDERED:
        case LabelStyle::BUTTON_REGULAR:
            return style->Label.buttonFontSize;
        case LabelStyle::FPS:
        case LabelStyle::LIST_ITEM:
            return style->Label.listItemFontSize;
        case LabelStyle::LIST_ITEM_VALUE:
        case LabelStyle::LIST_ITEM_VALUE_FAINT:
            return style->List.Item.valueSize;
        case LabelStyle::NOTIFICATION:
            return style->Label.notificationFontSize;
        case LabelStyle::DIALOG:
            return style->Label.dialogFontSize;
        case LabelStyle::HINT:
            return style->Label.hintFontSize;
        default:
            return 0;
    }
}

float Label::getLineHeight(LabelStyle labelStyle)
{
    if (!this->multiline) return 1.0f;

    Style* style = Application::getStyle();

    switch (labelStyle)
    {
        case LabelStyle::NOTIFICATION:
            return style->Label.notificationLineHeight;
        default:
            return style->Label.lineHeight;
    }
}

float Label::getTextAnimation()
{
    return this->textAnimation;
}

void Label::resetTextAnimation()
{
    this->textAnimation = 1.0F;

    menu_animation_ctx_tag tag = (uintptr_t) & this->textAnimation;
    menu_animation_kill_by_tag(&tag);
}

void Label::animate(LabelAnimation animation)
{
    if (!this->multiline) return;

    Style* style = Application::getStyle();

    menu_animation_ctx_tag tag = (uintptr_t) & this->textAnimation;
    menu_animation_kill_by_tag(&tag);

    this->textAnimation = animation == LabelAnimation::EASE_IN ? 0.0F : 1.0F;

    menu_animation_ctx_entry_t entry;
    entry.cb           = [this](void *userdata) { this->resetTextAnimation(); };
    entry.duration     = style->AnimationDuration.highlight;
    entry.easing_enum  = EASING_IN_OUT_QUAD;
    entry.subject      = &this->textAnimation;
    entry.tag          = tag;
    entry.target_value = animation == LabelAnimation::EASE_IN ? 1.0F : 0.0F;
    entry.tick         = [](void* userdata) {};
    entry.userdata     = nullptr;

    menu_animation_push(&entry);
}

void Label::onParentFocus()
{
    this->tickerActive = true;
    this->startTickerAnimation();
}

void Label::onParentUnfocus()
{
    this->stopTickerAnimation();
    this->tickerActive = false;
}

void Label::updateTextDimensions(bool invalidateParent)
{
    this->textWidth = this->textHeight = this->textTickerWidth = this->boundingBoxHeight = 0;
    this->textEllipsis = "";

    if (this->multiline || this->text == "") return;

    float bounds[4];
    NVGcontext *vg = Application::getNVGContext();
    FontStash *stash = Application::getFontStash();

    nvgSave(vg);
    nvgReset(vg);

    nvgFontSize(vg, this->fontSize);
    nvgFontFaceId(vg, this->getFont(stash));

    nvgTextAlign(vg, this->horizontalAlign | this->verticalAlign);

    this->textWidth         = std::ceil(nvgTextBounds(vg, 0, 0, this->text.c_str(), nullptr, bounds));
    this->textHeight        = std::ceil(bounds[3] - bounds[1]); // ymax - ymin
    this->boundingBoxHeight = std::ceil((bounds[3] - bounds[1]) * 1.25f); // Fixes scissor bounding box height
    this->textTickerWidth   = std::ceil(nvgTextBounds(vg, 0, 0, this->textTicker.c_str(), nullptr, nullptr)) - this->textWidth;

    nvgRestore(vg);

    if (invalidateParent && this->hasParent()) this->getParent()->invalidate();
}

// Taken from libnx
static ssize_t decode_utf8(uint32_t *out, const uint8_t *in)
{
    uint8_t code1, code2, code3, code4;

    code1 = *in++;
    if(code1 < 0x80)
    {
        /* 1-byte sequence */
        *out = code1;
        return 1;
    }
    else if(code1 < 0xC2)
    {
        return -1;
    }
    else if(code1 < 0xE0)
    {
        /* 2-byte sequence */
        code2 = *in++;
        if((code2 & 0xC0) != 0x80)
        {
            return -1;
        }

        *out = (code1 << 6) + code2 - 0x3080;
        return 2;
    }
    else if(code1 < 0xF0)
    {
        /* 3-byte sequence */
        code2 = *in++;
        if((code2 & 0xC0) != 0x80)
        {
            return -1;
        }
        if(code1 == 0xE0 && code2 < 0xA0)
        {
            return -1;
        }

        code3 = *in++;
        if((code3 & 0xC0) != 0x80)
        {
            return -1;
        }

        *out = (code1 << 12) + (code2 << 6) + code3 - 0xE2080;
        return 3;
    }
    else if(code1 < 0xF5)
    {
        /* 4-byte sequence */
        code2 = *in++;
        if((code2 & 0xC0) != 0x80)
        {
            return -1;
        }
        if(code1 == 0xF0 && code2 < 0x90)
        {
            return -1;
        }
        if(code1 == 0xF4 && code2 >= 0x90)
        {
            return -1;
        }

        code3 = *in++;
        if((code3 & 0xC0) != 0x80)
        {
            return -1;
        }

        code4 = *in++;
        if((code4 & 0xC0) != 0x80)
        {
            return -1;
        }

        *out = (code1 << 18) + (code2 << 12) + (code3 << 6) + code4 - 0x3C82080;
        return 4;
    }

    return -1;
}

size_t Label::getUtf8StringLength(const std::string& str)
{
    if (str == "") return 0;

    const uint8_t *p = reinterpret_cast<const uint8_t*>(str.c_str());
    ssize_t units = 0;
    uint32_t code = 0;

    size_t ret = 0;

    do {
        units = decode_utf8(&code, p);
        if (units < 0) break;
        p += units;
        ret++;
    } while(code >= ' ');

    return ret;
}

std::string Label::getUtf8SubString(const std::string& str, size_t start, size_t len)
{
    if (!len) return "";

    const uint8_t *p = reinterpret_cast<const uint8_t*>(str.c_str());
    ssize_t units = 0;
    uint32_t code = 0;

    size_t i = 0;
    std::string ret = "";

    do {
        units = decode_utf8(&code, p);
        if (units < 0) break;

        if (i >= start) ret.append(reinterpret_cast<const char*>(p), static_cast<size_t>(units));

        p += units;
        i++;
    } while(code >= ' ' && i < (start + len));

    return ret;
}

} // namespace brls
