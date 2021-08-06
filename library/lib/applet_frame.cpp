/*
    Borealis, a Nintendo Switch UI Library
    Copyright (C) 2019  natinusala
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

#include <borealis/applet_frame.hpp>
#include <borealis/application.hpp>
#include <borealis/i18n.hpp>
#include <cmath>

using namespace brls::i18n::literals;

namespace brls
{

AppletFrame::AppletFrame(bool padLeft, bool padRight)
{
    Style* style = Application::getStyle();
    std::string* commonFooter = Application::getCommonFooter();

    if (padLeft)
        this->leftPadding = style->AppletFrame.separatorSpacing;

    if (padRight)
        this->rightPadding = style->AppletFrame.separatorSpacing;

    this->footer = new Label(LabelStyle::UNFOCUSED_TICKER, *commonFooter, false);
    this->footer->setFontSize(style->AppletFrame.footerTextSize);
    this->footer->setParent(this);

    this->hint = new Hint();
    this->hint->setParent(this);

    this->registerAction("brls/hints/back"_i18n, Key::B, [this] { return this->onCancel(); });
}

void AppletFrame::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, Style* style, FrameContext* ctx)
{
    bool hasTitle         = this->title && this->title->getText() != "";
    bool hasSubTitleLeft  = this->headerStyle == HeaderStyle::POPUP && this->subTitleLeft && this->subTitleLeft->getText() != "";
    bool hasSubTitleRight = this->headerStyle == HeaderStyle::POPUP && this->subTitleRight && this->subTitleRight->getText() != "";
    bool hasFooter        = this->footer->getText() != "";

    unsigned subTitleRightPadding   = style->PopupFrame.imageLeftPadding + 2;
    unsigned subTitleLeftPadding    = this->icon ? style->PopupFrame.subTitleLeftPadding : subTitleRightPadding;
    unsigned subTitleSeparatorWidth = 1 + style->PopupFrame.subTitleSpacing * 2;
    unsigned subTitleWidth          = this->width - subTitleLeftPadding - subTitleRightPadding;
    unsigned sideSubTitleWidth      = (subTitleWidth - subTitleSeparatorWidth) / 2;

    // Header line
    nvgBeginPath(vg);
    nvgFillColor(vg, a(ctx->theme->textColor));
    nvgRect(vg, x + style->AppletFrame.separatorSpacing, y + (this->headerStyle == HeaderStyle::REGULAR ? style->AppletFrame.headerHeightRegular : style->AppletFrame.headerHeightPopup) - 1, \
            width - style->AppletFrame.separatorSpacing * 2, 1);
    nvgFill(vg);

    // Title
    if (hasTitle) {
        NVGcolor titleColor = a(ctx->theme->textColor);
        if (this->headerStyle == HeaderStyle::REGULAR && this->contentView) titleColor.a *= this->contentView->getAlpha();
        this->title->setColor(titleColor);
        this->title->frame(ctx);
    }

    // Left Subtitle
    if (hasSubTitleLeft) {
        this->subTitleLeft->setColor(a(ctx->theme->descriptionColor));
        this->subTitleLeft->frame(ctx);
    }

    // Sub title separator
    if (hasSubTitleLeft && hasSubTitleRight) {
        nvgFillColor(vg, a(ctx->theme->descriptionColor)); // we purposely don't apply opacity
        nvgBeginPath(vg);
        nvgRect(vg, x + subTitleLeftPadding + sideSubTitleWidth + style->PopupFrame.subTitleSpacing, y + style->PopupFrame.subTitleSeparatorTopPadding, 1, style->PopupFrame.subTitleSeparatorHeight);
        nvgFill(vg);
    }

    // Right Subtitle
    if (hasSubTitleRight) {
        this->subTitleRight->setColor(a(ctx->theme->descriptionColor));
        this->subTitleRight->frame(ctx);
    }

    // Footer
    if (hasFooter) {
        NVGcolor footerColor = a(ctx->theme->textColor);

        if (this->slideIn)
            footerColor.a = 0.0f;
        else if (this->slideOut)
            footerColor.a = 1.0f;

        this->footer->setColor(footerColor);
        this->footer->frame(ctx);
    }

    // Hint
    this->hint->frame(ctx);

    // Icon
    if (this->icon)
        this->icon->frame(ctx);

    // Separators
    nvgFillColor(vg, a(ctx->theme->separatorColor));

    // Footer line
    nvgBeginPath(vg);
    nvgRect(vg, x + style->AppletFrame.separatorSpacing, y + height - style->AppletFrame.footerHeight, width - style->AppletFrame.separatorSpacing * 2, 1);
    nvgFill(vg);

    // Content view
    if (contentView)
    {
        float slideAlpha = 1.0f - this->contentView->alpha;

        if ((this->slideIn && this->animation == ViewAnimation::SLIDE_LEFT) || (this->slideOut && this->animation == ViewAnimation::SLIDE_RIGHT))
            slideAlpha = 1.0f - slideAlpha;

        int translation = (int)((float)style->AppletFrame.slideAnimation * slideAlpha);

        if ((this->slideIn && this->animation == ViewAnimation::SLIDE_LEFT) || (this->slideOut && this->animation == ViewAnimation::SLIDE_RIGHT))
            translation -= style->AppletFrame.slideAnimation;

        if (this->slideOut || this->slideIn)
            nvgTranslate(vg, -translation, 0);

        contentView->frame(ctx);

        if (this->slideOut || this->slideIn)
            nvgTranslate(vg, translation, 0);
    }
}

View* AppletFrame::getDefaultFocus()
{
    if (this->contentView)
        return this->contentView->getDefaultFocus();

    return nullptr;
}

void AppletFrame::layout(NVGcontext* vg, Style* style, FontStash* stash)
{
    bool hasTitle         = this->title && this->title->getText() != "";
    bool hasSubTitleLeft  = this->headerStyle == HeaderStyle::POPUP && this->subTitleLeft && this->subTitleLeft->getText() != "";
    bool hasSubTitleRight = this->headerStyle == HeaderStyle::POPUP && this->subTitleRight && this->subTitleRight->getText() != "";
    bool hasFooter        = this->footer->getText() != "";

    unsigned subTitleRightPadding   = style->PopupFrame.imageLeftPadding + 2;
    unsigned subTitleLeftPadding    = this->icon ? style->PopupFrame.subTitleLeftPadding : subTitleRightPadding;
    unsigned subTitleSeparatorWidth = 1 + style->PopupFrame.subTitleSpacing * 2;
    unsigned subTitleWidth          = this->width - subTitleLeftPadding - subTitleRightPadding;
    unsigned sideSubTitleWidth      = (subTitleWidth - subTitleSeparatorWidth) / 2;

    unsigned footerPadding          = style->AppletFrame.separatorSpacing + style->AppletFrame.footerTextSpacing;
    unsigned footerSeparatorWidth   = style->AppletFrame.separatorSpacing;
    unsigned footerWidth            = this->width - footerPadding * 2;
    unsigned sideFooterWidth        = (footerWidth - footerSeparatorWidth) / 2;

    unsigned hintWidth              = hasFooter ? sideFooterWidth : footerWidth;

    // Title
    if (hasTitle) {
        unsigned titleFontSize     = this->headerStyle == HeaderStyle::REGULAR ? style->AppletFrame.titleSize : style->PopupFrame.headerFontSize;
        unsigned titleRightPadding = this->headerStyle == HeaderStyle::REGULAR ? style->AppletFrame.imageLeftPadding : style->PopupFrame.imageLeftPadding;
        unsigned titleLeftPadding  = this->headerStyle == HeaderStyle::REGULAR ? (this->icon ? style->AppletFrame.titleStart : titleRightPadding) : \
                                                                                 (this->icon ? style->PopupFrame.headerTextLeftPadding : titleRightPadding);

        unsigned titleY = this->headerStyle == HeaderStyle::REGULAR ? (style->AppletFrame.headerHeightRegular / 2 + style->AppletFrame.titleOffset) : style->PopupFrame.headerTextTopPadding;

        this->title->setFontSize(titleFontSize);
        this->title->setBoundaries(x + titleLeftPadding, y + titleY, this->width - titleLeftPadding - titleRightPadding, 0);
        this->title->invalidate();
    }

    // Left Subtitle
    if (hasSubTitleLeft) {
        this->subTitleLeft->setBoundaries(x + subTitleLeftPadding, y + style->PopupFrame.subTitleTopPadding, hasSubTitleRight ? sideSubTitleWidth : subTitleWidth, 0);
        this->subTitleLeft->invalidate();
    }

    // Right Subtitle
    if (hasSubTitleRight) {
        unsigned subTitleRightX = hasSubTitleLeft ? (subTitleLeftPadding + sideSubTitleWidth + subTitleSeparatorWidth) : subTitleLeftPadding;
        this->subTitleRight->setBoundaries(x + subTitleRightX, y + style->PopupFrame.subTitleTopPadding, hasSubTitleLeft ? sideSubTitleWidth : subTitleWidth, 0);
        this->subTitleRight->invalidate();
    }

    // Footer
    if (hasFooter) {
        this->footer->setBoundaries(x + footerPadding, y + height - style->AppletFrame.footerHeight / 2, sideFooterWidth, 0);
        this->footer->invalidate();
    }

    // Hint
    this->hint->setBoundaries(
        this->x + this->width - footerPadding - hintWidth,
        this->y + this->height - style->AppletFrame.footerHeight,
        hintWidth,
        style->AppletFrame.footerHeight);
    this->hint->invalidate();

    // Icon
    if (this->icon)
    {
        if (this->headerStyle == HeaderStyle::REGULAR)
        {
            this->icon->setBoundaries(style->AppletFrame.imageLeftPadding, style->AppletFrame.imageTopPadding, style->AppletFrame.imageSize, style->AppletFrame.imageSize);
            this->icon->invalidate();
        }
        else if (this->headerStyle == HeaderStyle::POPUP)
        {
            this->icon->setBoundaries(style->PopupFrame.edgePadding + style->PopupFrame.imageLeftPadding, style->PopupFrame.imageTopPadding, style->PopupFrame.imageSize, style->PopupFrame.imageSize);
            this->icon->invalidate();
        }
    }

    // Content
    if (this->contentView)
    {
        if (this->headerStyle == HeaderStyle::REGULAR)
            this->contentView->setBoundaries(this->x + leftPadding, this->y + style->AppletFrame.headerHeightRegular, this->width - this->leftPadding - this->rightPadding, this->height - style->AppletFrame.footerHeight - style->AppletFrame.headerHeightRegular);
        else if (this->headerStyle == HeaderStyle::POPUP)
            this->contentView->setBoundaries(this->x + leftPadding, this->y + style->AppletFrame.headerHeightPopup, this->width - this->leftPadding - this->rightPadding, this->height - style->AppletFrame.footerHeight - style->AppletFrame.headerHeightPopup);

        this->contentView->invalidate();
    }
}

void AppletFrame::setContentView(View* view)
{
    this->contentView = view;

    if (this->contentView)
    {
        this->contentView->setParent(this);
        this->contentView->willAppear();
    }

    this->invalidate();
}

bool AppletFrame::hasContentView()
{
    return this->contentView;
}

void AppletFrame::setTitle(std::string title)
{
    if (!this->title) {
        this->title = new Label(LabelStyle::UNFOCUSED_TICKER, title, false);
        this->title->setParent(this);
    } else {
        this->title->setText(title);
    }
}

void AppletFrame::setFooterText(std::string footerText)
{
    this->footer->setText(footerText);
}

void AppletFrame::setSubtitle(std::string left, std::string right)
{
    Style* style = Application::getStyle();

    if (!this->subTitleLeft) {
        this->subTitleLeft = new Label(LabelStyle::UNFOCUSED_TICKER, left, false);
        this->subTitleLeft->setFontSize(style->PopupFrame.subTitleFontSize);
        this->subTitleLeft->setVerticalAlign(NVG_ALIGN_TOP);
        this->subTitleLeft->setParent(this);
    } else {
        this->subTitleLeft->setText(left);
    }

    if (!this->subTitleRight) {
        this->subTitleRight = new Label(LabelStyle::UNFOCUSED_TICKER, right, false);
        this->subTitleRight->setFontSize(style->PopupFrame.subTitleFontSize);
        this->subTitleRight->setVerticalAlign(NVG_ALIGN_TOP);
        this->subTitleRight->setParent(this);
    } else {
        this->subTitleRight->setText(right);
    }
}

void AppletFrame::setIcon(const unsigned char* buffer, size_t bufferSize)
{
    if (!this->icon)
    {
        Image* icon = new Image();
        icon->setImage(buffer, bufferSize);
        icon->setScaleType(ImageScaleType::SCALE);
        icon->setParent(this);

        this->icon = icon;
    }
    else if (Image* icon = dynamic_cast<Image*>(this->icon))
    {
        icon->setImage(buffer, bufferSize);
    }

    this->icon->invalidate();
}

void AppletFrame::setIconRGBA(const unsigned char* buffer, size_t width, size_t height)
{
    if (!this->icon)
    {
        Image* icon = new Image();
        icon->setImageRGBA(buffer, width, height);
        icon->setScaleType(ImageScaleType::SCALE);
        icon->setParent(this);

        this->icon = icon;
    }
    else if (Image* icon = dynamic_cast<Image*>(this->icon))
    {
        icon->setImageRGBA(buffer, width, height);
    }

    this->icon->invalidate();
}

void AppletFrame::setIcon(const std::string &imagePath)
{
    if (!this->icon)
    {
        Image* icon = new Image();
        icon->setImage(imagePath);
        icon->setScaleType(ImageScaleType::SCALE);
        icon->setParent(this);

        this->icon = icon;
    }
    else if (Image* icon = dynamic_cast<Image*>(this->icon))
    {
        icon->setImage(imagePath);
    }

    this->icon->invalidate();
}

void AppletFrame::setIcon(View* view)
{
    if (this->icon)
        delete this->icon;

    if (view != nullptr)
        view->setParent(this);

    this->icon = view;
}

void AppletFrame::setHeaderStyle(HeaderStyle headerStyle)
{
    this->headerStyle = headerStyle;

    this->invalidate();
}

void AppletFrame::rebuildHints()
{
    this->hint->rebuildHints(true);
}

AppletFrame::~AppletFrame()
{
    if (this->title)
        delete this->title;

    if (this->subTitleLeft)
        delete this->subTitleLeft;

    if (this->subTitleRight)
        delete this->subTitleRight;

    if (this->footer)
        delete this->footer;

    if (this->contentView)
    {
        this->contentView->willDisappear(true);
        delete this->contentView;
    }

    if (this->icon)
        delete this->icon;

    delete this->hint;
}

void AppletFrame::willAppear(bool resetState)
{
    if (this->icon)
        this->icon->willAppear(resetState);

    if (this->contentView)
        this->contentView->willAppear(resetState);

    this->hint->willAppear(resetState);
}

void AppletFrame::willDisappear(bool resetState)
{
    if (this->icon)
        this->icon->willDisappear(resetState);

    if (this->contentView)
        this->contentView->willDisappear(resetState);

    this->hint->willDisappear(resetState);
}

void AppletFrame::show(std::function<void(void)> cb, bool animated, ViewAnimation animation)
{
    this->animation = animation;

    if (animated && (animation == ViewAnimation::SLIDE_LEFT || animation == ViewAnimation::SLIDE_RIGHT) && this->contentView)
    {
        this->slideIn = true;

        this->contentView->show([this]() {
            this->slideIn = false;
        },
            true, animation);
    }
    else if (this->contentView && this->contentView->isHidden())
    {
        this->contentView->show([]() {}, animated, animation);
    }

    View::show(cb, animated, animation);
}

void AppletFrame::hide(std::function<void(void)> cb, bool animated, ViewAnimation animation)
{
    this->animation = animation;

    if (animated && (animation == ViewAnimation::SLIDE_LEFT || animation == ViewAnimation::SLIDE_RIGHT) && this->contentView)
    {
        this->slideOut = true;

        this->contentView->hide([this, cb]() {
            this->slideOut = false;
        },
            true, animation);
    }
    else if (this->contentView && !this->contentView->isHidden())
    {
        this->contentView->hide([]() {}, animated, animation);
    }

    View::hide(cb, animated, animation);
}

bool AppletFrame::onCancel()
{
    /*
     * TODO: this assumes AppletFrames are used as "activities" in the app, which may be wrong
     * so we should instead change the view stack to an "activity" stack and have them popped when
     * the user presses B on the root view of an "activity"
     */
    Application::popView();
    return true;
}

void AppletFrame::onWindowSizeChanged()
{
    if (this->contentView)
        this->contentView->onWindowSizeChanged();

    if (this->icon)
        this->icon->onWindowSizeChanged();

    if (this->hint)
        this->hint->onWindowSizeChanged();
}

} // namespace brls
