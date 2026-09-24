

// Might not be needed The goal of this  is to make it so that JobColor(string_view or const string &hex ) ->  JobColor
// struct JobColorHex{
//  string_view hex;
//  // we are using c++26 so string_view::sub_view :)
// toHexString
// fromHexString
// }
///


// START This goes over over to job_color_basics.h
static constexpr JobColor Transparent{0, 0, 0, 0};   // upper = JobColor
static constexpr string_view transparent{"transparent"}; // lower = string_view


// Add Gray and others as well I am okay with this going up to 15
// Rename to follow
// https://www.w3.org/TR/SVG11/types.html#ColorKeywords
// static constexpr JobColor Black{0, 0, 0};
// static constexpr JobColor White{255, 255, 255};
// static constexpr JobColor Red{255, 0, 0};
// static constexpr JobColor Orange{255, 127, 0};
// static constexpr JobColor Yellow{255, 255, 0};
// static constexpr JobColor Green{0, 255, 0};
// static constexpr JobColor Blue{0, 0, 255};
// static constexpr JobColor Indigo{75, 0, 130};
// static constexpr JobColor Violet{148, 0, 211};
// static constexpr JobColor Cyan{0, 255, 255};
// static constexpr JobColor Magenta{255, 0, 255};


// END This goes over over to job_color_basics.h


// START stays in color
// rules: starts with(# -> hex, anthing elss embedded_list) ->  if !embedded_list  -> length ->
// #
//  12 #RRRRGGGGBBBB
//  9 #RRRGGGBBB
//  8 #AARRGGBB
//  6 #RRGGBB
//  3 #RGB || single digit
//
// [[nodiscard]] JobColor fromString(const string &str)
// [[nodiscard]] JobColor fromStringView(string_view str)
// [[nodiscard]] string toString(const JobColor &str)
// [[nodiscard]] string_view toStringView(JobColor str) << where the parsing happends
//


// [[nodiscard]] inline constexpr JobColor lighter(JobColor color, int percent) noexcept
// [[nodiscard]] inline constexpr JobColor lighter(JobColorHex color, int percent) noexcept

// [[nodiscard]] inline constexpr JobColor darker(JobColor color, int percent) noexcept
// [[nodiscard]] inline constexpr JobColor darker(JobColorHex color, int percent) noexcept

// Dangit need JobRgba JobRgba64
// toRGBA(JobColor color)
// fromRGBA(JobColor)

// toRGBA64(JobColor color)
// fromRGBA64(JobColor)

// toARGB(JobColor)
// fromARGB(JobColor)

// toARGB64(JobColor)
// fromARGB64(JobColor)

// fromInt(JobColor)
// toInt(JobColor)
// END stays in color_utils

// Add HSL[vairants](to from)
// AND CMYK[vairants](to from)




// Terminal Colors (Dangit 2nd time this proves we really need deadicated JobRgba JobRgba64)
// [[nodiscard]] inline constexpr JobColor basicAnsiColor(int index) noexcept
// [[nodiscard]] inline constexpr JobColor brightAnsiColor(int index) noexcept
// [[nodiscard]] inline constexpr std::optional<JobColor> sgrCodeToColor(int code) noexcept
// [[nodiscard]] inline constexpr JobColor fromXterm256Palette(int colorCode) noexcept
// [[nodiscard]] inline constexpr std::array<JobColor, 256> makeXterm256Palette() noexcept
// [[nodiscard]] inline constexpr const std::array<JobColor, 256> &xterm256Palette() noexcept
// [[nodiscard]] inline constexpr std::optional<int> rgbToSGR(const JobColor &color, bool foreground) noexcept
// [[nodiscard]] inline std::string sgr256Color(int index, bool foreground)
// [[nodiscard]] inline std::string sgrTrueColor(const JobColor &color, bool foreground)
// [[nodiscard]] inline std::string sgrFromHex(std::string_view hex, bool foreground = true)
// [[nodiscard]] inline constexpr int findNearestXterm256(const JobColor &color) noexcept


