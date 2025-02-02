// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <vtbackend/Color.h>
#include <vtbackend/primitives.h>

#include <vtparser/ParserExtension.h>

#include <crispy/range.h>

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace vtbackend
{

/// Sixel Stream Parser API.
///
/// Parses a sixel stream without any Sixel introducer CSI or ST to leave sixel mode,
/// that must be done by the parent parser.
///
/// TODO: make this parser O(1) with state table lookup tables, just like the VT parser.
class SixelParser: public ParserExtension
{
  public:
    enum class State : uint8_t
    {
        Ground,           // Sixel data
        RasterSettings,   // '"', configuring the raster
        RepeatIntroducer, // '!'
        ColorIntroducer,  // '#', color-set or color-use
        ColorParam        // color parameter
    };

    enum class Colorspace : uint8_t
    {
        RGB,
        HSL
    };

    /// SixelParser's event handler
    class Events
    {
      public:
        virtual ~Events() = default;

        /// Defines a new color at given register index.
        virtual void setColor(unsigned index, RGBColor const& color) = 0;

        /// Uses the given color for future paints
        virtual void useColor(unsigned index) = 0;

        /// moves sixel-cursor to the left border
        virtual void rewind() = 0;

        /// moves the sixel-cursorto the left border of the next sixel-band
        virtual void newline() = 0;

        /// Defines the aspect ratio (pan / pad = aspect ratio) and image dimensions in pixels for
        /// the upcoming pixel data.
        virtual void setRaster(unsigned int pan, unsigned int pad, std::optional<ImageSize> imageSize) = 0;

        /// renders a given sixel at the current sixel-cursor position.
        virtual void render(int8_t sixel) = 0;

        /// Finalizes the image by optimizing the underlying storage to its minimal dimension in storage.
        virtual void finalize() = 0;
    };

    using OnFinalize = std::function<void()>;
    explicit SixelParser(Events& events, OnFinalize finalizer = {});

    using iterator = char const*;

    void parseFragment(iterator begin, iterator end)
    {
        for (auto const ch: crispy::range(begin, end))
            parse(ch);
    }

    void parseFragment(std::string_view range) { parseFragment(range.data(), range.data() + range.size()); }

    void parse(char value);
    void done();

    static void parse(std::string_view range, Events& events)
    {
        auto parser = SixelParser { events };
        parser.parseFragment(range.data(), range.data() + range.size());
        parser.done();
    }

    // ParserExtension overrides
    void pass(char ch) override;
    void finalize() override;

  private:
    void paramShiftAndAddDigit(unsigned value);
    void transitionTo(State newState);
    void enterState();
    void leaveState();
    void fallback(char value);

  private:
    State _state = State::Ground;
    std::vector<unsigned> _params;

    Events& _events;
    OnFinalize _finalizer;
};

class SixelColorPalette
{
  public:
    SixelColorPalette(unsigned int size, unsigned int maxSize);

    void reset();

    [[nodiscard]] unsigned int size() const noexcept { return static_cast<unsigned int>(_palette.size()); }
    void setSize(unsigned int newSize);

    [[nodiscard]] unsigned int maxSize() const noexcept { return _maxSize; }
    void setMaxSize(unsigned int value) { _maxSize = value; }

    void setColor(unsigned int index, RGBColor const& color);
    [[nodiscard]] RGBColor at(unsigned int index) const noexcept;

  private:
    std::vector<RGBColor> _palette;
    unsigned int _maxSize;
};

/// Sixel Image Builder API
///
/// Implements the SixelParser::Events event listener to construct a Sixel image.
class SixelImageBuilder: public SixelParser::Events
{
  public:
    using Buffer = std::vector<uint8_t>;

    SixelImageBuilder(ImageSize maxSize,
                      int aspectVertical,
                      int aspectHorizontal,
                      RGBAColor backgroundColor,
                      std::shared_ptr<SixelColorPalette> colorPalette);

    [[nodiscard]] ImageSize maxSize() const noexcept { return _maxSize; }
    [[nodiscard]] ImageSize size() const noexcept { return _size; }
    [[nodiscard]] unsigned int aspectRatio() const noexcept { return _aspectRatio; }
    [[nodiscard]] RGBColor currentColor() const noexcept { return _colors->at(_currentColor); }

    [[nodiscard]] RGBAColor at(CellLocation coord) const noexcept;

    [[nodiscard]] Buffer const& data() const noexcept { return _buffer; }
    [[nodiscard]] Buffer& data() noexcept { return _buffer; }

    void clear(RGBAColor fillColor);

    void setColor(unsigned index, RGBColor const& color) override;
    void useColor(unsigned index) override;
    void rewind() override;
    void newline() override;
    void setRaster(unsigned int pan, unsigned int pad, std::optional<ImageSize> imageSize) override;
    void render(int8_t sixel) override;
    void finalize() override;

    [[nodiscard]] CellLocation const& sixelCursor() const noexcept { return _sixelCursor; }

  private:
    void write(CellLocation const& coord, RGBColor const& value) noexcept;

  private:
    ImageSize const _maxSize;
    std::shared_ptr<SixelColorPalette> _colors;
    ImageSize _size;
    Buffer _buffer; /// RGBA buffer
    CellLocation _sixelCursor {};
    unsigned _currentColor = 0;
    bool _explicitSize = false;
    // This is an int because vt3xx takes the given ratio pan/pad and rounds up the ratio
    // to nearest integers. So 1:3 = 0.33 and it  becomes 1;
    unsigned int _aspectRatio;
    // Height of sixel band in pixels
    unsigned int _sixelBandHeight;
};

} // namespace vtbackend
