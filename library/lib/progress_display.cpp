/*
    Borealis, a Nintendo Switch UI Library
    Copyright (C) 2019  Billy Laws
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
#include <borealis/progress_display.hpp>

namespace brls
{

ProgressDisplay::ProgressDisplay(ProgressDisplayFlags progressFlags)
{
    if (progressFlags & ProgressDisplayFlags::PERCENTAGE) {
        this->label = new Label(LabelStyle::DIALOG, "0%", false);
        this->label->setParent(this);
    }

    if (progressFlags & ProgressDisplayFlags::SPINNER) {
        this->spinner = new ProgressSpinner();
        this->spinner->setParent(this);
    }
}

void ProgressDisplay::setProgress(int current, int max)
{
    if (current > max)
        return;

    this->progressPercentage = ((current * 100) / max);

    if (!this->label)
        return;

    std::string labelText = std::to_string((unsigned)this->progressPercentage);
    labelText += "%";
    this->label->setText(labelText);
}

void ProgressDisplay::layout(NVGcontext* vg, Style* style, FontStash* stash)
{
    if (this->label) {
        this->label->setBoundaries(
            this->x + this->width - style->ProgressDisplay.percentageLabelWidth,
            this->y + this->height / 2,
            style->ProgressDisplay.percentageLabelWidth,
            0);
        this->label->invalidate();
    }

    if (this->spinner) {
        this->spinner->setBoundaries(this->x, this->y, this->height, this->height);
        this->spinner->invalidate();
    }
}

void ProgressDisplay::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, Style* style, FrameContext* ctx)
{
    unsigned progressBarWidth = width;
    unsigned progressBarX     = x;

    if (this->label)
    {
        // 1.30 accounts for the extra width of the curve on sides of progress bar
        this->label->frame(ctx);
        progressBarWidth -= style->ProgressDisplay.percentageLabelWidth * 1.30f;
    }

    if (this->spinner)
    {
        // 1.25 accounts for the extra width of the curve on sides of progress bar
        this->spinner->frame(ctx);
        progressBarWidth -= this->spinner->getWidth() * 1.25f;
        progressBarX += this->spinner->getWidth() * 1.25f;
    }

    nvgBeginPath(vg);
    nvgMoveTo(vg, progressBarX, y + height / 2);
    nvgLineTo(vg, progressBarX + progressBarWidth, y + height / 2);
    nvgStrokeColor(vg, a(ctx->theme->listItemSeparatorColor));
    nvgStrokeWidth(vg, height / 3);
    nvgLineCap(vg, NVG_ROUND);
    nvgStroke(vg);

    if (this->progressPercentage > 0.0f)
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, progressBarX, y + height / 2);
        nvgLineTo(vg, progressBarX + ((float)progressBarWidth * this->progressPercentage) / 100, y + height / 2);
        nvgStrokeColor(vg, a(ctx->theme->listItemValueColor));
        nvgStrokeWidth(vg, height / 3);
        nvgLineCap(vg, NVG_ROUND);
        nvgStroke(vg);
    }
}

void ProgressDisplay::willAppear(bool resetState)
{
    if (this->spinner)
        this->spinner->willAppear(resetState);
}

void ProgressDisplay::willDisappear(bool resetState)
{
    if (this->spinner)
        this->spinner->willDisappear(resetState);
}

ProgressDisplay::~ProgressDisplay()
{
    if (this->spinner)
        delete this->spinner;

    if (this->label)
        delete this->label;
}

} // namespace brls
