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

#include <math.h>

#include <borealis/animations.hpp>
#include <borealis/application.hpp>
#include <borealis/dropdown.hpp>
#include <borealis/header.hpp>
#include <borealis/i18n.hpp>
#include <borealis/list.hpp>
#include <borealis/swkbd.hpp>
#include <borealis/table.hpp>

using namespace brls::i18n::literals;

namespace brls
{

ListContentView::ListContentView(List* list, size_t defaultFocus)
    : BoxLayout(BoxLayoutOrientation::VERTICAL, defaultFocus)
    , list(list)
{
    Style* style = Application::getStyle();
    this->setMargins(style->List.marginTopBottom, style->List.marginLeftRight, style->List.marginTopBottom, style->List.marginLeftRight);
    this->setSpacing(style->List.spacing);
    this->setRememberFocus(true);
}

void ListContentView::customSpacing(View* current, View* next, int* spacing)
{
    // Don't add spacing to the first list item
    // if it doesn't have a description and the second one is a
    // list item too
    if (ListItem* currentItem = dynamic_cast<ListItem*>(current))
    {
        if (currentItem->getReduceDescriptionSpacing())
        {
            if (next != nullptr)
                *spacing /= 2;
        }
        else if (ListItem* nextItem = dynamic_cast<ListItem*>(next))
        {
            if (!currentItem->hasDescription())
            {
                *spacing = 2;

                nextItem->setDrawTopSeparator(currentItem->isCollapsed());
            }
        }
        else if (dynamic_cast<Table*>(next))
        {
            *spacing /= 2;
        }
    }
    // Table custom spacing
    else if (dynamic_cast<Table*>(current))
    {
        *spacing /= 2;
    }
    // ListItemGroupSpacing custom spacing
    else if (dynamic_cast<ListItemGroupSpacing*>(current))
    {
        *spacing /= 2;
    }
    // Header custom spacing
    else if (dynamic_cast<Header*>(current) || dynamic_cast<Header*>(next))
    {
        if (dynamic_cast<Header*>(current) && dynamic_cast<ListItem*>(next))
        {
            *spacing = 1;
        }
        else if (dynamic_cast<Label*>(current) && dynamic_cast<Header*>(next))
        {
            // Keep default spacing
        }
        else
        {
            Style* style = Application::getStyle();
            *spacing     = style->Header.padding;
        }
    }

    // Call list custom spacing
    if (this->list)
        this->list->customSpacing(current, next, spacing);
}

ListItem::ListItem(std::string label, std::string description, std::string subLabel)
{
    Style* style = Application::getStyle();

    this->setHeight(subLabel != "" ? style->List.Item.heightWithSubLabel : style->List.Item.height);

    this->labelView = new Label(LabelStyle::LIST_ITEM, label, false);
    this->labelView->setParent(this);

    if (description != "")
    {
        this->descriptionView = new Label(LabelStyle::DESCRIPTION, description, true);
        this->descriptionView->setParent(this);
    }

    if (subLabel != "")
    {
        this->subLabelView = new Label(LabelStyle::DESCRIPTION, subLabel, false);
        this->subLabelView->setVerticalAlign(NVG_ALIGN_TOP);
        this->subLabelView->setParent(this);
    }

    this->registerAction("brls/hints/ok"_i18n, Key::A, [this] { return this->onClick(); });
}

void ListItem::setThumbnail(Image* image)
{
    if (this->thumbnailView) {
        delete this->thumbnailView;
        this->thumbnailView = nullptr;
    }

    if (image)
    {
        this->thumbnailView = image;
        this->thumbnailView->setParent(this);
        this->invalidate();
    }
}

void ListItem::setThumbnail(const std::string &imagePath)
{
    if (!this->thumbnailView)
        this->thumbnailView = new Image();

    this->thumbnailView->setImage(imagePath);
    this->thumbnailView->setParent(this);
    this->thumbnailView->setScaleType(ImageScaleType::FIT);
    this->invalidate();
}

void ListItem::setThumbnail(const unsigned char* buffer, size_t bufferSize)
{
    if (!this->thumbnailView)
        this->thumbnailView = new Image();

    this->thumbnailView->setImage(buffer, bufferSize);
    this->thumbnailView->setParent(this);
    this->thumbnailView->setScaleType(ImageScaleType::FIT);
    this->invalidate();
}

void ListItem::setThumbnailRGBA(const unsigned char* buffer, size_t width, size_t height)
{
    if (!this->thumbnailView)
        this->thumbnailView = new Image();

    this->thumbnailView->setImageRGBA(buffer, width, height);
    this->thumbnailView->setParent(this);
    this->thumbnailView->setScaleType(ImageScaleType::FIT);
    this->invalidate();
}

bool ListItem::getReduceDescriptionSpacing()
{
    return this->reduceDescriptionSpacing;
}

void ListItem::setReduceDescriptionSpacing(bool value)
{
    this->reduceDescriptionSpacing = value;
}

void ListItem::setIndented(bool indented)
{
    this->indented = indented;
}

void ListItem::setTextSize(unsigned textSize)
{
    this->labelView->setFontSize(textSize);
}

void ListItem::setChecked(bool checked)
{
    this->checked = checked;
}

bool ListItem::onClick()
{
    return this->clickEvent.fire(this);
}

GenericEvent* ListItem::getClickEvent()
{
    return &this->clickEvent;
}

void ListItem::layout(NVGcontext* vg, Style* style, FontStash* stash)
{
    unsigned baseHeight = this->height;
    bool hasSubLabel    = this->subLabelView && this->subLabelView->getText() != "";
    bool hasThumbnail   = this->thumbnailView;
    bool hasValue       = this->valueView && this->valueView->getText() != "";

    if (this->descriptionView)
        baseHeight -= this->descriptionView->getHeight() + style->List.Item.descriptionSpacing;

    unsigned leftPadding        = hasThumbnail ? this->thumbnailView->getWidth() + style->List.Item.thumbnailPadding * 2 : style->List.Item.padding;

    unsigned itemAvailableWidth = this->width - leftPadding - style->List.Item.padding;

    unsigned labelTextWidth     = this->labelView->getTextWidth();
    unsigned labelBoxWidth      = 0;

    unsigned subLabelTextWidth  = hasSubLabel ? this->subLabelView->getTextWidth() : 0;
    unsigned subLabelBoxWidth   = 0;

    unsigned valueTextWidth     = hasValue ? this->valueView->getTextWidth() : 0;
    unsigned valueBoxWidth      = hasValue ? this->width / 3 : 0;

    // Calculate text box widths
    if (this->checked) {
        // Skip sublabel and value calculations if we're dealing with a check mark
        labelBoxWidth = itemAvailableWidth - (style->List.Item.selectRadius * 2) - style->List.Item.padding;
    } else
    if (hasValue) {
        unsigned newItemAvailableWidth = itemAvailableWidth - style->List.Item.padding;

        if (valueBoxWidth >= valueTextWidth)
        {
            // Steal horizontal space for the label / sublabel
            newItemAvailableWidth -= valueTextWidth;
            valueBoxWidth          = valueTextWidth;

            if (hasSubLabel) {
                labelBoxWidth    = itemAvailableWidth;
                subLabelBoxWidth = newItemAvailableWidth;
            } else {
                labelBoxWidth    = newItemAvailableWidth;
            }
        } else {
            // Steal horizontal space for the value (if possible)
            newItemAvailableWidth -= valueBoxWidth;

            if (hasSubLabel)
            {
                labelBoxWidth = itemAvailableWidth;

                if (subLabelTextWidth < newItemAvailableWidth) {
                    subLabelBoxWidth = subLabelTextWidth;
                    valueBoxWidth    = itemAvailableWidth - subLabelBoxWidth - style->List.Item.padding;
                } else {
                    subLabelBoxWidth = newItemAvailableWidth;
                }
            } else {
                if (labelTextWidth < newItemAvailableWidth) {
                    labelBoxWidth = labelTextWidth;
                    valueBoxWidth = itemAvailableWidth - labelBoxWidth - style->List.Item.padding;
                } else {
                    labelBoxWidth = newItemAvailableWidth;
                }
            }
        }
    } else {
        // Use all the available item width for both label and sublabel
        labelBoxWidth    = itemAvailableWidth;
        subLabelBoxWidth = hasSubLabel ? itemAvailableWidth : 0;
    }

    // Label
    this->labelView->setBoundaries(this->x + leftPadding, this->y + (baseHeight / (hasSubLabel ? 3 : 2)), labelBoxWidth, 0);
    this->labelView->invalidate();

    // Value
    if (hasValue) {
        unsigned valueX = this->x + this->width - style->List.Item.padding;
        unsigned valueY = this->y + (hasSubLabel ? baseHeight - baseHeight / 3 : baseHeight / 2);

        this->valueView->setBoundaries(valueX, valueY, valueBoxWidth, 0);
        this->valueView->setVerticalAlign(hasSubLabel ? NVG_ALIGN_TOP : NVG_ALIGN_MIDDLE);
        this->valueView->invalidate();

        this->oldValueView->setBoundaries(valueX, valueY, valueBoxWidth, 0);
        this->oldValueView->setVerticalAlign(hasSubLabel ? NVG_ALIGN_TOP : NVG_ALIGN_MIDDLE);
        this->oldValueView->invalidate();
    }

    // Sub Label
    if (hasSubLabel)
    {
        this->subLabelView->setBoundaries(this->x + leftPadding, this->y + baseHeight - baseHeight / 3, subLabelBoxWidth, 0);
        this->subLabelView->invalidate();
    }

    // Description
    if (this->descriptionView)
    {
        unsigned indent = style->List.Item.descriptionIndent;

        if (this->indented)
            indent += style->List.Item.indent;

        this->height = style->List.Item.height;
        this->descriptionView->setBoundaries(this->x + indent, this->y + this->height + style->List.Item.descriptionSpacing, this->width - indent * 2, 0);
        this->descriptionView->invalidate(true); // we must call layout directly
        this->height += this->descriptionView->getHeight() + style->List.Item.descriptionSpacing;
    }

    // Thumbnail
    if (this->thumbnailView)
    {
        Style* style           = Application::getStyle();
        unsigned thumbnailSize = this->height - style->List.Item.thumbnailPadding * 2;

        this->thumbnailView->setBoundaries(
            this->x + style->List.Item.thumbnailPadding,
            this->y + style->List.Item.thumbnailPadding,
            thumbnailSize,
            thumbnailSize);
        this->thumbnailView->invalidate();
    }
}

void ListItem::getHighlightInsets(unsigned* top, unsigned* right, unsigned* bottom, unsigned* left)
{
    Style* style = Application::getStyle();
    View::getHighlightInsets(top, right, bottom, left);

    if (descriptionView)
        *bottom = -(descriptionView->getHeight() + style->List.Item.descriptionSpacing);

    if (indented)
        *left = -style->List.Item.indent;
}

void ListItem::setValue(std::string value, bool faint, bool animate)
{
    this->oldValueFaint = this->valueFaint;
    this->valueFaint    = faint;

    if (!this->valueView) {
        this->valueView = new Label(this->valueFaint ? LabelStyle::LIST_ITEM_VALUE_FAINT : LabelStyle::LIST_ITEM_VALUE, value, false);
        this->valueView->setHorizontalAlign(NVG_ALIGN_RIGHT);
        this->valueView->setParent(this);

        this->oldValueView = new Label(this->oldValueFaint ? LabelStyle::LIST_ITEM_VALUE_FAINT : LabelStyle::LIST_ITEM_VALUE, "", false);
        this->oldValueView->setHorizontalAlign(NVG_ALIGN_RIGHT);
        this->oldValueView->setParent(this);
    } else {
        this->oldValueView->setText(this->valueView->getText());
        this->oldValueView->setStyle(this->oldValueFaint ? LabelStyle::LIST_ITEM_VALUE_FAINT : LabelStyle::LIST_ITEM_VALUE);
        this->oldValueView->resetTickerAnimation();

        this->valueView->setText(value);
        this->valueView->setStyle(this->valueFaint ? LabelStyle::LIST_ITEM_VALUE_FAINT : LabelStyle::LIST_ITEM_VALUE);
        this->valueView->resetTickerAnimation();

        if (animate && this->oldValueView->getText() != "")
        {
            this->oldValueView->animate(LabelAnimation::EASE_OUT);
            this->valueView->animate(LabelAnimation::EASE_IN);
        } else {
            this->valueView->resetTextAnimation();
            this->oldValueView->resetTextAnimation();
        }
    }
}

std::string ListItem::getValue()
{
    return this->valueView ? this->valueView->getText() : "";
}

void ListItem::setDrawTopSeparator(bool draw)
{
    this->drawTopSeparator = draw;
}

View* ListItem::getDefaultFocus()
{
    if (this->collapseState != 1.0f)
        return nullptr;

    return this;
}

void ListItem::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, Style* style, FrameContext* ctx)
{
    unsigned baseHeight = this->height;
    bool hasSubLabel    = this->subLabelView && this->subLabelView->getText() != "";
    bool hasThumbnail   = this->thumbnailView;
    bool hasValue       = this->valueView && this->valueView->getText() != "";

    if (this->indented)
    {
        x += style->List.Item.indent;
        width -= style->List.Item.indent;
    }

    // Description
    if (this->descriptionView)
    {
        // Don't count description as part of list item
        baseHeight -= this->descriptionView->getHeight() + style->List.Item.descriptionSpacing;
        this->descriptionView->frame(ctx);
    }

    // Value
    if (hasValue) {
        if (this->valueView->getTextAnimation() != 1.0f)
        {
            this->valueView->frame(ctx);
            this->oldValueView->frame(ctx);
        } else {
            this->valueView->frame(ctx);
        }
    }

    // Checked marker
    if (this->checked)
    {
        unsigned radius  = style->List.Item.selectRadius;
        unsigned centerX = x + width - radius - style->List.Item.padding;
        unsigned centerY = y + baseHeight / 2;

        float radiusf = (float)radius;

        int thickness = roundf(radiusf * 0.10f);

        // Background
        nvgFillColor(vg, a(ctx->theme->listItemValueColor));
        nvgBeginPath(vg);
        nvgCircle(vg, centerX, centerY, radiusf);
        nvgFill(vg);

        // Check mark
        nvgFillColor(vg, a(ctx->theme->backgroundColorRGB));

        // Long stroke
        nvgSave(vg);
        nvgTranslate(vg, centerX, centerY);
        nvgRotate(vg, -NVG_PI / 4.0f);

        nvgBeginPath(vg);
        nvgRect(vg, -(radiusf * 0.55f), 0, radiusf * 1.3f, thickness);
        nvgFill(vg);
        nvgRestore(vg);

        // Short stroke
        nvgSave(vg);
        nvgTranslate(vg, centerX - (radiusf * 0.65f), centerY);
        nvgRotate(vg, NVG_PI / 4.0f);

        nvgBeginPath(vg);
        nvgRect(vg, 0, -(thickness / 2), radiusf * 0.53f, thickness);
        nvgFill(vg);

        nvgRestore(vg);
    }

    // Label
    this->labelView->frame(ctx);

    // Sub Label
    if (hasSubLabel)
        this->subLabelView->frame(ctx);

    // Thumbnail
    if (hasThumbnail)
        this->thumbnailView->frame(ctx);

    // Separators
    // Offset by one to be hidden by highlight
    nvgFillColor(vg, a(ctx->theme->listItemSeparatorColor));

    // Top
    if (this->drawTopSeparator)
    {
        nvgBeginPath(vg);
        nvgRect(vg, x, y - 1, width, 1);
        nvgFill(vg);
    }

    // Bottom
    nvgBeginPath(vg);
    nvgRect(vg, x, y + 1 + baseHeight, width, 1);
    nvgFill(vg);
}

bool ListItem::hasDescription()
{
    return this->descriptionView;
}

void ListItem::setLabel(std::string label)
{
    this->labelView->setText(label);
}

std::string ListItem::getLabel()
{
    return this->labelView->getText();
}

void ListItem::setSubLabel(std::string subLabel)
{
    if (!this->subLabelView) {
        this->subLabelView = new Label(LabelStyle::DESCRIPTION, subLabel, false);
        this->subLabelView->setVerticalAlign(NVG_ALIGN_TOP);
        this->subLabelView->setParent(this);
    } else {
        this->subLabelView->setText(subLabel);
    }

    Style* style = Application::getStyle();
    this->setHeight(subLabel != "" ? style->List.Item.heightWithSubLabel : style->List.Item.height);
}

std::string ListItem::getSubLabel()
{
    return this->subLabelView ? this->subLabelView->getText() : "";
}

ListItem::~ListItem()
{
    delete this->labelView;

    if (this->valueView)
        delete this->valueView;

    if (this->oldValueView)
        delete this->oldValueView;

    if (this->descriptionView)
        delete this->descriptionView;

    if (this->subLabelView)
        delete this->subLabelView;

    if (this->thumbnailView)
        delete this->thumbnailView;
}

ToggleListItem::ToggleListItem(std::string label, bool initialValue, std::string description, std::string onValue, std::string offValue)
    : ListItem(label, description)
    , toggleState(initialValue)
    , onValue(onValue)
    , offValue(offValue)
{
    this->updateValue();
}

void ToggleListItem::updateValue()
{
    if (this->toggleState)
        this->setValue(this->onValue, false);
    else
        this->setValue(this->offValue, true);
}

bool ToggleListItem::onClick()
{
    this->toggleState = !this->toggleState;
    this->updateValue();

    ListItem::onClick();
    return true;
}

void ToggleListItem::setToggleState(bool state)
{
    this->toggleState = state;
    this->updateValue();
}

bool ToggleListItem::getToggleState()
{
    return this->toggleState;
}

InputListItem::InputListItem(std::string label, std::string initialValue, std::string helpText, std::string description, int maxInputLength, int kbdDisableBitmask)
    : ListItem(label, description)
    , helpText(helpText)
    , maxInputLength(maxInputLength)
    , kbdDisableBitmask(kbdDisableBitmask)
{
    this->setValue(initialValue, false);
}

bool InputListItem::onClick()
{
    Swkbd::openForText([&](std::string text) {
        this->setValue(text, false);
    },
        this->helpText, "", this->maxInputLength, this->getValue(), this->kbdDisableBitmask);

    ListItem::onClick();
    return true;
}

IntegerInputListItem::IntegerInputListItem(std::string label, int initialValue, std::string helpText, std::string description, int maxInputLength, int kbdDisableBitmask)
    : InputListItem(label, std::to_string(initialValue), helpText, description, maxInputLength, kbdDisableBitmask)
{
}

bool IntegerInputListItem::onClick()
{
    Swkbd::openForNumber([&](int number) {
        this->setValue(std::to_string(number), false);
    },
        this->helpText, "", this->maxInputLength, this->getValue(), "", "", this->kbdDisableBitmask);

    ListItem::onClick();
    return true;
}

ListItemGroupSpacing::ListItemGroupSpacing(bool separator)
    : Rectangle(nvgRGBA(0, 0, 0, 0))
{
    Theme* theme = Application::getTheme();

    if (separator)
        this->setColor(theme->listItemSeparatorColor);
}

SelectListItem::SelectListItem(std::string label, std::vector<std::string> values, unsigned selectedValue, std::string description, bool displayValue, bool registerExit, bool registerFps)
    : ListItem(label, description)
    , values(values)
    , selectedValue(selectedValue)
    , displayValue(displayValue)
    , registerExit(registerExit)
    , registerFps(registerFps)
{
    if (this->displayValue)
        this->setValue(values[selectedValue], false, false);

    this->getClickEvent()->subscribe([this](View* view) {
        if (this->values.empty()) return;

        ValueSelectedEvent::Callback valueCallback = [this](int result) {
            if (result < 0 || result >= static_cast<int>(this->values.size()))
                return;

            if (this->displayValue)
                this->setValue(this->values[result], false, false);

            this->selectedValue = result;

            this->valueEvent.fire(result);
        };

        Dropdown::open(this->getLabel(), this->values, valueCallback, this->displayValue ? this->selectedValue : -1, this->registerExit, this->registerFps);
    });
}

void SelectListItem::setSelectedValue(unsigned value)
{
    if (value >= 0 && value < this->values.size())
    {
        this->selectedValue = value;

        if (this->displayValue)
            this->setValue(this->values[value], false, false);
    }
}

unsigned SelectListItem::getSelectedValue()
{
    return this->selectedValue;
}

ValueSelectedEvent* SelectListItem::getValueSelectedEvent()
{
    return &this->valueEvent;
}

void SelectListItem::updateValues(const std::vector<std::string>& values)
{
    this->values = values;
    this->setSelectedValue(0);

    bool forcePopDropdown = values.empty();

    // Pop the current Dropdown and push a new Dropdown with the updated values if it's currently being displayed.
    // Alternatively, pop the current Dropdown if the provided vector is empty.
    std::vector<View*> *view_stack = Application::getViewStack();
    size_t view_stack_size = (view_stack ? view_stack->size() : 0);

    std::vector<View*> *focus_stack = Application::getFocusStack();
    size_t focus_stack_size = (focus_stack ? focus_stack->size() : 0);

    if (view_stack_size < 2 || focus_stack_size < 1 || focus_stack->at(focus_stack_size - 1) != this) return;

    Dropdown *dropdown = dynamic_cast<Dropdown*>(view_stack->at(view_stack_size - 1));
    if (!dropdown || dropdown->getTitle() != this->getLabel()) return;

    if (forcePopDropdown) {
        dropdown->onCancel();
    } else {
        ValueSelectedEvent::Callback valueCallback = [this](int result) {
            if (result < 0 || result >= static_cast<int>(this->values.size()))
                return;

            if (this->displayValue)
                this->setValue(this->values[result], false, false);

            this->selectedValue = result;

            this->valueEvent.fire(result);
        };

        Application::swapView(new Dropdown(this->getLabel(), this->values, valueCallback, this->displayValue ? this->selectedValue : -1), ViewAnimation::FADE, this->registerExit, this->registerFps);
    }
}

List::List(size_t defaultFocus, bool drawScrollBar) : ScrollView(drawScrollBar)
{
    this->layout = new ListContentView(this, defaultFocus);

    this->layout->setResize(true);
    this->layout->setParent(this);

    this->setContentView(this->layout);
}

// Wrapped BoxLayout methods

void List::addView(View* view, bool fill)
{
    this->layout->addView(view, fill);
}

void List::removeView(int index, bool free)
{
    this->layout->removeView(index, free);
}

void List::clear(bool free)
{
    this->layout->clear(free);
}

size_t List::getViewsCount()
{
    return this->layout->getViewsCount();
}

View* List::getChild(size_t i)
{
    return this->layout->getChild(i);
}

void List::setMargins(unsigned top, unsigned right, unsigned bottom, unsigned left)
{
    this->layout->setMargins(
        top,
        right,
        bottom,
        left);
}

void List::setSpacing(unsigned spacing)
{
    this->layout->setSpacing(spacing);
}

unsigned List::getSpacing()
{
    return this->layout->getSpacing();
}

void List::setMarginBottom(unsigned bottom)
{
    this->layout->setMarginBottom(bottom);
}

void List::customSpacing(View* current, View* next, int* spacing)
{
    // Nothing to do by default
}

List::~List()
{
    // ScrollView already deletes the content view
}

} // namespace brls
