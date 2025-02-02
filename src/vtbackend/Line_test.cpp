// SPDX-License-Identifier: Apache-2.0
#include <vtbackend/Line.h>
#include <vtbackend/cell/CellConfig.h>

#include <crispy/escape.h>

#include <catch2/catch_test_macros.hpp>

using namespace std;

using namespace vtbackend;
using namespace crispy;

// Default cell type for testing.
using Cell = PrimaryScreenCell;

TEST_CASE("Line.BufferFragment", "[Line]")
{
    auto constexpr TestText = "0123456789ABCDEF"sv;
    auto pool = buffer_object_pool<char>(16);
    auto bufferObject = pool.allocateBufferObject();
    bufferObject->writeAtEnd(TestText);
    auto const bufferFragment = bufferObject->ref(0, 10);

    auto const externalView = string_view(bufferObject->data(), 10);
    auto const fragment = buffer_fragment(bufferObject, externalView);
    CHECK(fragment.view() == externalView);
}

TEST_CASE("Line.resize", "[Line]")
{
    auto constexpr DisplayWidth = ColumnCount(4);
    auto text = "abcd"sv;
    auto pool = buffer_object_pool<char>(32);
    auto bufferObject = pool.allocateBufferObject();
    bufferObject->writeAtEnd(text);

    auto const bufferFragment = bufferObject->ref(0, 4);

    auto const sgr = GraphicsAttributes {};
    auto const trivial = TrivialLineBuffer { .displayWidth = DisplayWidth,
                                             .textAttributes = sgr,
                                             .fillAttributes = sgr,
                                             .hyperlink = HyperlinkId {},
                                             .usedColumns = DisplayWidth,
                                             .text = bufferFragment };
    CHECK(trivial.text.view() == string_view(text.data()));
    auto lineTrivial = Line<Cell>(LineFlag::None, trivial);
    CHECK(lineTrivial.isTrivialBuffer());

    lineTrivial.resize(ColumnCount(10));
    CHECK(lineTrivial.isTrivialBuffer());

    lineTrivial.resize(ColumnCount(5));
    CHECK(lineTrivial.isTrivialBuffer());

    lineTrivial.resize(ColumnCount(3));
    CHECK(lineTrivial.isTrivialBuffer());
}

TEST_CASE("Line.reflow", "[Line]")
{
    auto constexpr DisplayWidth = ColumnCount(4);
    auto text = "abcd"sv;
    auto pool = buffer_object_pool<char>(32);
    auto bufferObject = pool.allocateBufferObject();
    bufferObject->writeAtEnd(text);

    auto const bufferFragment = bufferObject->ref(0, 4);

    auto const sgr = GraphicsAttributes {};
    auto const trivial = TrivialLineBuffer { .displayWidth = DisplayWidth,
                                             .textAttributes = sgr,
                                             .fillAttributes = sgr,
                                             .hyperlink = HyperlinkId {},
                                             .usedColumns = DisplayWidth,
                                             .text = bufferFragment };
    CHECK(trivial.text.view() == string_view(text.data()));
    auto lineTrivial = Line<Cell>(LineFlag::None, trivial);
    CHECK(lineTrivial.isTrivialBuffer());

    (void) lineTrivial.reflow(ColumnCount(5));
    CHECK(lineTrivial.isTrivialBuffer());

    (void) lineTrivial.reflow(ColumnCount(3));
    CHECK(lineTrivial.isInflatedBuffer());
}

TEST_CASE("Line.inflate", "[Line]")
{
    auto constexpr TestText = "0123456789ABCDEF"sv;
    auto pool = buffer_object_pool<char>(16);
    auto bufferObject = pool.allocateBufferObject();
    bufferObject->writeAtEnd(TestText);
    auto const bufferFragment = bufferObject->ref(0, 10);

    auto sgr = GraphicsAttributes {};
    sgr.foregroundColor = RGBColor(0x123456);
    sgr.backgroundColor = Color::Indexed(IndexedColor::Yellow);
    sgr.underlineColor = Color::Indexed(IndexedColor::Red);
    sgr.flags |= CellFlag::CurlyUnderlined;
    auto const trivial = TrivialLineBuffer { .displayWidth = ColumnCount(10),
                                             .textAttributes = sgr,
                                             .fillAttributes = sgr,
                                             .hyperlink = HyperlinkId {},
                                             .usedColumns = ColumnCount(10),
                                             .text = bufferFragment };

    auto const inflated = inflate<Cell>(trivial);

    CHECK(inflated.size() == 10);
    for (size_t i = 0; i < inflated.size(); ++i)
    {
        auto const& cell = inflated[i];
        INFO(std::format("column {} codepoint {}", i, (char) cell.codepoint(0)));
        CHECK(cell.foregroundColor() == sgr.foregroundColor);
        CHECK(cell.backgroundColor() == sgr.backgroundColor);
        CHECK(cell.underlineColor() == sgr.underlineColor);
        CHECK(cell.codepointCount() == 1);
        CHECK(char(cell.codepoint(0)) == TestText[i]);
    }
}

TEST_CASE("Line.inflate.Unicode", "[Line]")
{
    auto constexpr DisplayWidth = ColumnCount(10);
    auto constexpr TestTextUtf32 = U"0\u2705123456789ABCDEF"sv;
    auto const testTextUtf8 = unicode::convert_to<char>(TestTextUtf32);

    auto pool = buffer_object_pool<char>(32);
    auto bufferObject = pool.allocateBufferObject();
    bufferObject->writeAtEnd(testTextUtf8);

    // Buffer fragment containing 9 codepoints, with one of them using display width of 2.
    auto const bufferFragment = bufferObject->ref(0, 11);

    auto sgr = GraphicsAttributes {};
    sgr.foregroundColor = RGBColor(0x123456);
    sgr.backgroundColor = Color::Indexed(IndexedColor::Yellow);
    sgr.underlineColor = Color::Indexed(IndexedColor::Red);
    sgr.flags |= CellFlag::CurlyUnderlined;
    auto const trivial = TrivialLineBuffer { .displayWidth = DisplayWidth,
                                             .textAttributes = sgr,
                                             .fillAttributes = sgr,
                                             .hyperlink = HyperlinkId {},
                                             .usedColumns = DisplayWidth,
                                             .text = bufferFragment };

    auto const inflated = inflate<Cell>(trivial);

    CHECK(inflated.size() == unbox<size_t>(DisplayWidth));
    for (size_t i = 0, k = 0; i < inflated.size();)
    {
        auto const& cell = inflated[i];
        INFO(std::format("column {}, k {}, codepoint U+{:X}", i, k, (unsigned) cell.codepoint(0)));
        REQUIRE(cell.codepointCount() == 1);
        REQUIRE(cell.codepoint(0) == TestTextUtf32[k]);
        REQUIRE(cell.foregroundColor() == sgr.foregroundColor);
        REQUIRE(cell.backgroundColor() == sgr.backgroundColor);
        REQUIRE(cell.underlineColor() == sgr.underlineColor);
        for (int n = 1; n < cell.width(); ++n)
        {
            INFO(std::format("column.sub: {}\n", n));
            auto const& fillCell = inflated.at(i + static_cast<size_t>(n));
            REQUIRE(fillCell.codepointCount() == 0);
            REQUIRE(fillCell.foregroundColor() == sgr.foregroundColor);
            REQUIRE(fillCell.backgroundColor() == sgr.backgroundColor);
            REQUIRE(fillCell.underlineColor() == sgr.underlineColor);
        }
        i += cell.width();
        k++;
    }
}

TEST_CASE("Line.inflate.Unicode.FamilyEmoji", "[Line]")
{
    // Ensure inflate() is also working for reaaally complex Unicode grapheme clusters.

    auto constexpr DisplayWidth = ColumnCount(5);
    auto constexpr UsedColumnCount = ColumnCount(4);
    auto constexpr TestTextUtf32 = U"A\U0001F468\u200D\U0001F468\u200D\U0001F467B"sv;
    auto const testTextUtf8 = unicode::convert_to<char>(TestTextUtf32);
    auto const familyEmojiUtf8 = unicode::convert_to<char>(U"\U0001F468\u200D\U0001F468\u200D\U0001F467"sv);

    auto pool = buffer_object_pool<char>(32);
    auto bufferObject = pool.allocateBufferObject();
    bufferObject->writeAtEnd(testTextUtf8);

    auto const bufferFragment = bufferObject->ref(0, testTextUtf8.size());

    auto sgr = GraphicsAttributes {};
    sgr.foregroundColor = RGBColor(0x123456);
    sgr.backgroundColor = Color::Indexed(IndexedColor::Yellow);
    sgr.underlineColor = Color::Indexed(IndexedColor::Red);
    sgr.flags |= CellFlag::CurlyUnderlined;

    auto fillSGR = GraphicsAttributes {};
    fillSGR.foregroundColor = RGBColor(0x123456);
    fillSGR.backgroundColor = Color::Indexed(IndexedColor::Yellow);
    fillSGR.underlineColor = Color::Indexed(IndexedColor::Red);
    fillSGR.flags |= CellFlag::CurlyUnderlined;

    auto const trivial = TrivialLineBuffer { .displayWidth = DisplayWidth,
                                             .textAttributes = sgr,
                                             .fillAttributes = fillSGR,
                                             .hyperlink = HyperlinkId {},
                                             .usedColumns = UsedColumnCount,
                                             .text = bufferFragment };

    auto const inflated = inflate<Cell>(trivial);

    CHECK(inflated.size() == unbox<size_t>(DisplayWidth));

    // Check text in 0..3
    // Check @4 is empty text.
    // Check 0..3 has same SGR.
    // Check @4 has fill-SGR.

    REQUIRE(inflated[0].toUtf8() == "A");
    REQUIRE(inflated[1].toUtf8() == familyEmojiUtf8);
    REQUIRE(inflated[2].toUtf8().empty());
    REQUIRE(inflated[3].toUtf8() == "B");
    REQUIRE(inflated[4].toUtf8().empty());

    for (auto const i: { 0u, 2u, 1u, 3u })
    {
        auto const& cell = inflated[i];
        REQUIRE(cell.foregroundColor() == sgr.foregroundColor);
        REQUIRE(cell.backgroundColor() == sgr.backgroundColor);
        REQUIRE(cell.underlineColor() == sgr.underlineColor);
    }

    auto const& cell = inflated[4];
    REQUIRE(cell.foregroundColor() == fillSGR.foregroundColor);
    REQUIRE(cell.backgroundColor() == fillSGR.backgroundColor);
    REQUIRE(cell.underlineColor() == fillSGR.underlineColor);
}
