// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <vtbackend/Color.h>
#include <vtbackend/Grid.h> // cell attribs
#include <vtbackend/primitives.h>

#include <vtrasterizer/GridMetrics.h>
#include <vtrasterizer/TextureAtlas.h>
#include <vtrasterizer/shared_defines.h>

#include <crispy/size.h>

#include <optional>
#include <vector>

namespace vtbackend
{
struct BackgroundImage;
}

namespace vtrasterizer
{

using ImageSize = vtbackend::ImageSize;

/**
 * Contains the read-out of the state of an texture atlas.
 */
struct AtlasTextureScreenshot
{
    int atlasInstanceId;
    ImageSize size;
    atlas::Format format;
    atlas::Buffer buffer;
};

/**
 * Defines the attributes of a RenderTile, such as render-offset relative
 * to the render target position.
 *
 * For example the later M may be close to the origin (0,0) (bottom left)
 * and have the extent close to the top right of the grid cell size,
 * whereas the `-` symbol may be offset to the vertical middle and have a
 * vertical extent of just a few pixels.
 *
 * This information is usually font specific and produced by (for example)
 * the text shaping engine and/or the glyph rasterizer.
 *
 * For image fragments x/y will most likely be (0, 0) and
 * width/height span the full grid cell.
 *
 * The bitmap's size is already stored in TextureAtlas::TileCreateData.
 */
struct RenderTileAttributes
{
    // clang-format off
    struct X { int value; };
    struct Y { int value; };
    // clang-format on

    // render x-offset relative to pen position
    X x {};

    // render y-offset relative to pen position
    Y y {};

    // Defines how to interpret the texture data.
    // It could for example be gray-scale antialiased, LCD subpixel antialiased,
    // or a simple RGBA texture.
    // See:
    // - FRAGMENT_SELECTOR_IMAGE_BGRA
    // - FRAGMENT_SELECTOR_GLYPH_ALPHA
    // - FRAGMENT_SELECTOR_GLYPH_LCD
    uint32_t fragmentShaderSelector = FRAGMENT_SELECTOR_IMAGE_BGRA;

    atlas::NormalizedTileLocation normalizedLocation {};

    ImageSize targetSize {};
};

/**
 * Terminal render target interface, for example OpenGL, DirectX, or software-rasterization.
 *
 * @see OpenGLRenderer
 */
class RenderTarget
{
  public:
    using RGBAColor = vtbackend::RGBAColor;
    using Width = vtbackend::Width;
    using Height = vtbackend::Height;
    using TextureAtlas = atlas::TextureAtlas<RenderTileAttributes>;

    virtual ~RenderTarget() = default;

    /// Sets the render target's size in pixels.
    /// This is the size that can be rendered to.
    virtual void setRenderSize(ImageSize size) = 0;

    virtual void setMargin(PageMargin margin) = 0;

    virtual atlas::AtlasBackend& textureScheduler() = 0;

    /// Fills a rectangular area with the given solid color.
    virtual void renderRectangle(int x, int y, Width, Height, RGBAColor color) = 0;

    using ScreenshotCallback =
        std::function<void(std::vector<uint8_t> const& /*_rgbaBuffer*/, ImageSize /*_pixelSize*/)>;

    /// Schedules taking a screenshot of the current scene and forwards it to the given callback.
    virtual void scheduleScreenshot(ScreenshotCallback callback) = 0;

    /// Executes all previously scheduled render commands.
    virtual void execute(std::chrono::steady_clock::time_point now) = 0;

    /// Clears any existing caches.
    virtual void clearCache() = 0;

    /// Reads out the given texture atlas.
    virtual std::optional<vtrasterizer::AtlasTextureScreenshot> readAtlas() = 0;

    virtual void inspect(std::ostream& output) const = 0;
};

/**
 * Helper-base class for render subsystems, such as
 * text renderer, decoration renderer, image fragment renderer, etc.
 */
class Renderable
{
  public:
    using TextureAtlas = RenderTarget::TextureAtlas;
    using DirectMappingAllocator = atlas::DirectMappingAllocator<RenderTileAttributes>;
    using DirectMapping = atlas::DirectMapping<RenderTileAttributes>;
    using AtlasTileAttributes = atlas::TileAttributes<RenderTileAttributes>;
    using TileSliceIndex = atlas::TileSliceIndex;

    explicit Renderable(GridMetrics const& gridMetrics);
    virtual ~Renderable() = default;

    virtual void clearCache() {}

    virtual void setRenderTarget(RenderTarget& renderTarget, DirectMappingAllocator& directMappingAllocator);
    virtual void setTextureAtlas(TextureAtlas& atlas) { _textureAtlas = &atlas; }

    [[nodiscard]] TextureAtlas::TileCreateData createTileData(atlas::TileLocation tileLocation,
                                                              std::vector<uint8_t> bitmap,
                                                              atlas::Format bitmapFormat,
                                                              vtbackend::ImageSize bitmapSize,
                                                              RenderTileAttributes::X x,
                                                              RenderTileAttributes::Y y,
                                                              uint32_t fragmentShaderSelector);

    [[nodiscard]] TextureAtlas::TileCreateData createTileData(atlas::TileLocation tileLocation,
                                                              std::vector<uint8_t> bitmap,
                                                              atlas::Format bitmapFormat,
                                                              vtbackend::ImageSize bitmapSize,
                                                              vtbackend::ImageSize renderBitmapSize,
                                                              RenderTileAttributes::X x,
                                                              RenderTileAttributes::Y y,
                                                              uint32_t fragmentShaderSelector);

    [[nodiscard]] Renderable::TextureAtlas::TileCreateData sliceTileData(
        Renderable::TextureAtlas::TileCreateData const& createData,
        TileSliceIndex sliceIndex,
        atlas::TileLocation tileLocation);

    [[nodiscard]] static atlas::RenderTile createRenderTile(
        atlas::RenderTile::X x,
        atlas::RenderTile::Y y,
        vtbackend::RGBAColor color,
        Renderable::AtlasTileAttributes const& attributes);

    void renderTile(atlas::RenderTile::X x,
                    atlas::RenderTile::Y y,
                    vtbackend::RGBAColor color,
                    Renderable::AtlasTileAttributes const& attributes);

    [[nodiscard]] constexpr bool renderTargetAvailable() const noexcept { return _renderTarget; }

    [[nodiscard]] RenderTarget& renderTarget() noexcept
    {
        assert(_renderTarget);
        return *_renderTarget;
    }

    [[nodiscard]] TextureAtlas& textureAtlas() noexcept
    {
        assert(_textureAtlas);
        return *_textureAtlas;
    }

    [[nodiscard]] atlas::AtlasBackend& textureScheduler() noexcept { return *_textureScheduler; }

    virtual void inspect(std::ostream& output) const = 0;

  protected:
    GridMetrics const& _gridMetrics;
    RenderTarget* _renderTarget = nullptr;
    TextureAtlas* _textureAtlas = nullptr;
    atlas::DirectMappingAllocator<RenderTileAttributes>* _directMappingAllocator = nullptr;
    atlas::AtlasBackend* _textureScheduler = nullptr;
};

inline Renderable::TextureAtlas::TileCreateData Renderable::createTileData(atlas::TileLocation tileLocation,
                                                                           std::vector<uint8_t> bitmap,
                                                                           atlas::Format bitmapFormat,
                                                                           vtbackend::ImageSize bitmapSize,
                                                                           RenderTileAttributes::X x,
                                                                           RenderTileAttributes::Y y,
                                                                           uint32_t fragmentShaderSelector)
{
    return createTileData(
        tileLocation, std::move(bitmap), bitmapFormat, bitmapSize, bitmapSize, x, y, fragmentShaderSelector);
}

} // namespace vtrasterizer

// {{{ fmt
template <>
struct std::formatter<vtrasterizer::RenderTileAttributes>: std::formatter<std::string>
{
    auto format(vtrasterizer::RenderTileAttributes value, auto& ctx) const
    {
        return std::formatter<std::string>::format(
            std::format("tile +{}x +{}y", value.x.value, value.y.value), ctx);
    }
};

template <>
struct std::formatter<vtrasterizer::atlas::TileAttributes<vtrasterizer::RenderTileAttributes>>:
    std::formatter<std::string>
{
    auto format(vtrasterizer::atlas::TileAttributes<vtrasterizer::RenderTileAttributes> const& value,
                auto& ctx) const
    {
        return formatter<std::string>::format(
            std::format("(location {}; bitmap {}; {})", value.location, value.bitmapSize, value.metadata),
            ctx);
    }
};
// }}}
