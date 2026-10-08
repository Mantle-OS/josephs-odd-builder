#pragma once
// https://www.w3.org/TR/SVG11/types.html#ColorKeywords

#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <job_rgb.h>

namespace job_::gui {

class JobColorKeyword {
    using KeywordColors = std::unordered_map<std::string_view, core::JobRgb>;

    JobColorKeyword() = delete;
    ~JobColorKeyword() = delete;

    JobColorKeyword(const JobColorKeyword &) = delete;
    JobColorKeyword &operator=(const JobColorKeyword &) = delete;

    JobColorKeyword(JobColorKeyword &&) noexcept = delete;
    JobColorKeyword &operator=(JobColorKeyword &&) noexcept = delete;

    static const KeywordColors kKeywordMap;

    [[nodiscard]] static inline core::JobRgb rgb(std::string_view name) noexcept
    {
        if(!kKeywordMap.contains(name))
            return {0, 0, 0, 255};
        return kKeywordMap.at(name);
    }
    [[nodiscard]] static inline std::string_view toString(core::JobRgb rgb) noexcept
    {
        for (const auto &[name, value] : kKeywordMap) {
            if (value == rgb)
                return name;
        }
        return "unknown";
    }
};

inline const JobColorKeyword::KeywordColors JobColorKeyword::kKeywordMap = {
    { "aliceblue"               , core::JobRgb{   240, 248, 255   , 255 } },
    { "antiquewhite"            , core::JobRgb{   250, 235, 215   , 255 } },
    { "aqua"                    , core::JobRgb{    0, 255, 255    , 255 } },
    { "aquamarine"              , core::JobRgb{   127, 255, 212   , 255 } },
    { "azure"                   , core::JobRgb{   240, 255, 255   , 255 } },
    { "beige"                   , core::JobRgb{   245, 245, 220   , 255 } },
    { "bisque"                  , core::JobRgb{   255, 228, 196   , 255 } },
    { "black"                   , core::JobRgb{    0, 0, 0        , 255 } },
    { "blanchedalmond"          , core::JobRgb{   255, 235, 205   , 255 } },
    { "blue"                    , core::JobRgb{    0, 0, 255      , 255 } },
    { "blueviolet"              , core::JobRgb{   138, 43, 226    , 255 } },
    { "brown"                   , core::JobRgb{   165, 42, 42     , 255 } },
    { "burlywood"               , core::JobRgb{   222, 184, 135   , 255 } },
    { "cadetblue"               , core::JobRgb{    95, 158, 160   , 255 } },
    { "chartreuse"              , core::JobRgb{   127, 255, 0     , 255 } },
    { "chocolate"               , core::JobRgb{   210, 105, 30    , 255 } },
    { "coral"                   , core::JobRgb{   255, 127, 80    , 255 } },
    { "cornflowerblue"          , core::JobRgb{   100, 149, 237   , 255 } },
    { "cornsilk"                , core::JobRgb{   255, 248, 220   , 255 } },
    { "crimson"                 , core::JobRgb{   220, 20, 60     , 255 } },
    { "cyan"                    , core::JobRgb{    0, 255, 255    , 255 } },
    { "darkblue"                , core::JobRgb{    0, 0, 139      , 255 } },
    { "darkcyan"                , core::JobRgb{    0, 139, 139    , 255 } },
    { "darkgoldenrod"           , core::JobRgb{   184, 134, 11    , 255 } },
    { "darkgray"                , core::JobRgb{   169, 169, 169   , 255 } },
    { "darkgreen"               , core::JobRgb{    0, 100, 0      , 255 } },
    { "darkgrey"                , core::JobRgb{   169, 169, 169   , 255 } },
    { "darkkhaki"               , core::JobRgb{   189, 183, 107   , 255 } },
    { "darkmagenta"             , core::JobRgb{   139, 0, 139     , 255 } },
    { "darkolivegreen"          , core::JobRgb{    85, 107, 47    , 255 } },
    { "darkorange"              , core::JobRgb{   255, 140, 0     , 255 } },
    { "darkorchid"              , core::JobRgb{   153, 50, 204    , 255 } },
    { "darkred"                 , core::JobRgb{   139, 0, 0       , 255 } },
    { "darksalmon"              , core::JobRgb{   233, 150, 122   , 255 } },
    { "darkseagreen"            , core::JobRgb{   143, 188, 143   , 255 } },
    { "darkslateblue"           , core::JobRgb{    72, 61, 139    , 255 } },
    { "darkslategray"           , core::JobRgb{    47, 79, 79     , 255 } },
    { "darkslategrey"           , core::JobRgb{    47, 79, 79     , 255 } },
    { "darkturquoise"           , core::JobRgb{    0, 206, 209    , 255 } },
    { "darkviolet"              , core::JobRgb{   148, 0, 211     , 255 } },
    { "deeppink"                , core::JobRgb{   255, 20, 147    , 255 } },
    { "deepskyblue"             , core::JobRgb{    0, 191, 255    , 255 } },
    { "dimgray"                 , core::JobRgb{   105, 105, 105   , 255 } },
    { "dimgrey"                 , core::JobRgb{   105, 105, 105   , 255 } },
    { "dodgerblue"              , core::JobRgb{    30, 144, 255   , 255 } },
    { "firebrick"               , core::JobRgb{   178, 34, 34     , 255 } },
    { "floralwhite"             , core::JobRgb{   255, 250, 240   , 255 } },
    { "forestgreen"             , core::JobRgb{    34, 139, 34    , 255 } },
    { "fuchsia"                 , core::JobRgb{   255, 0, 255     , 255 } },
    { "gainsboro"               , core::JobRgb{   220, 220, 220   , 255 } },
    { "ghostwhite"              , core::JobRgb{   248, 248, 255   , 255 } },
    { "gold"                    , core::JobRgb{   255, 215, 0     , 255 } },
    { "goldenrod"               , core::JobRgb{   218, 165, 32    , 255 } },
    { "gray"                    , core::JobRgb{   128, 128, 128   , 255 } },
    { "grey"                    , core::JobRgb{   128, 128, 128   , 255 } },
    { "green"                   , core::JobRgb{    0, 128, 0      , 255 } },
    { "greenyellow"             , core::JobRgb{   173, 255, 47    , 255 } },
    { "honeydew"                , core::JobRgb{   240, 255, 240   , 255 } },
    { "hotpink"                 , core::JobRgb{   255, 105, 180   , 255 } },
    { "indianred"               , core::JobRgb{   205, 92, 92     , 255 } },
    { "indigo"                  , core::JobRgb{    75, 0, 130     , 255 } },
    { "ivory"                   , core::JobRgb{   255, 255, 240   , 255 } },
    { "khaki"                   , core::JobRgb{   240, 230, 140   , 255 } },
    { "lavender"                , core::JobRgb{   230, 230, 250   , 255 } },
    { "lavenderblush"           , core::JobRgb{   255, 240, 245   , 255 } },
    { "lawngreen"               , core::JobRgb{   124, 252, 0     , 255 } },
    { "lemonchiffon"            , core::JobRgb{   255, 250, 205   , 255 } },
    { "lightblue"               , core::JobRgb{   173, 216, 230   , 255 } },
    { "lightcoral"              , core::JobRgb{   240, 128, 128   , 255 } },
    { "lightcyan"               , core::JobRgb{   224, 255, 255   , 255 } },
    { "lightgoldenrodyellow"	, core::JobRgb{   250, 250, 210   , 255 } },
    { "lightgray"               , core::JobRgb{   211, 211, 211   , 255 } },
    { "lightgreen"              , core::JobRgb{   144, 238, 144   , 255 } },
    { "lightgrey"               , core::JobRgb{   211, 211, 211   , 255 } },
    { "lightpink"               , core::JobRgb{   255, 182, 193   , 255 } },
    { "lightsalmon"             , core::JobRgb{   255, 160, 122   , 255 } },
    { "lightseagreen"           , core::JobRgb{    32, 178, 170   , 255 } },
    { "lightskyblue"            , core::JobRgb{   135, 206, 250   , 255 } },
    { "lightslategray"          , core::JobRgb{   119, 136, 153   , 255 } },
    { "lightslategrey"          , core::JobRgb{   119, 136, 153   , 255 } },
    { "lightsteelblue"          , core::JobRgb{   176, 196, 222   , 255 } },
    { "lightyellow"             , core::JobRgb{   255, 255, 224   , 255 } },
    { "lime"                    , core::JobRgb{    0, 255, 0      , 255 } },
    { "limegreen"               , core::JobRgb{    50, 205, 50    , 255 } },
    { "linen"                   , core::JobRgb{   250, 240, 230   , 255 } },
    { "magenta"                 , core::JobRgb{   255, 0, 255     , 255 } },
    { "maroon"                  , core::JobRgb{   128, 0, 0       , 255 } },
    { "mediumaquamarine"        , core::JobRgb{   102, 205, 170   , 255 } },
    { "mediumblue"              , core::JobRgb{    0, 0, 205      , 255 } },
    { "mediumorchid"            , core::JobRgb{   186, 85, 211    , 255 } },
    { "mediumpurple"            , core::JobRgb{   147, 112, 219   , 255 } },
    { "mediumseagreen"          , core::JobRgb{    60, 179, 113   , 255 } },
    { "mediumslateblue"         , core::JobRgb{   123, 104, 238   , 255 } },
    { "mediumspringgreen"       , core::JobRgb{    0, 250, 154    , 255 } },
    { "mediumturquoise"         , core::JobRgb{    72, 209, 204   , 255 } },
    { "mediumvioletred"         , core::JobRgb{   199, 21, 133    , 255 } },
    { "midnightblue"            , core::JobRgb{    25, 25, 112    , 255 } },
    { "mintcream"               , core::JobRgb{   245, 255, 250   , 255 } },
    { "mistyrose"               , core::JobRgb{   255, 228, 225   , 255 } },
    { "moccasin"                , core::JobRgb{   255, 228, 181   , 255 } },
    { "navajowhite"             , core::JobRgb{   255, 222, 173   , 255 } },
    { "navy"                    , core::JobRgb{    0, 0, 128      , 255 } },
    { "oldlace"                 , core::JobRgb{   253, 245, 230   , 255 } },
    { "olive"                   , core::JobRgb{   128, 128, 0     , 255 } },
    { "olivedrab"               , core::JobRgb{   107, 142, 35    , 255 } },
    { "orange"                  , core::JobRgb{   255, 165, 0     , 255 } },
    { "orangered"               , core::JobRgb{   255, 69, 0      , 255 } },
    { "orchid"                  , core::JobRgb{   218, 112, 214   , 255 } },
    { "palegoldenrod"           , core::JobRgb{   238, 232, 170   , 255 } },
    { "palegreen"               , core::JobRgb{   152, 251, 152   , 255 } },
    { "paleturquoise"           , core::JobRgb{   175, 238, 238   , 255 } },
    { "palevioletred"           , core::JobRgb{   219, 112, 147   , 255 } },
    { "papayawhip"              , core::JobRgb{   255, 239, 213   , 255 } },
    { "peachpuff"               , core::JobRgb{   255, 218, 185   , 255 } },
    { "peru"                    , core::JobRgb{   205, 133, 63    , 255 } },
    { "pink"                    , core::JobRgb{   255, 192, 203   , 255 } },
    { "plum"                    , core::JobRgb{   221, 160, 221   , 255 } },
    { "powderblue"              , core::JobRgb{   176, 224, 230   , 255 } },
    { "purple"                  , core::JobRgb{   128, 0, 128     , 255 } },
    { "red"                     , core::JobRgb{   255, 0, 0       , 255 } },
    { "rosybrown"               , core::JobRgb{   188, 143, 143   , 255 } },
    { "royalblue"               , core::JobRgb{    65, 105, 225   , 255 } },
    { "saddlebrown"             , core::JobRgb{   139, 69, 19     , 255 } },
    { "salmon"                  , core::JobRgb{   250, 128, 114   , 255 } },
    { "sandybrown"              , core::JobRgb{   244, 164, 96    , 255 } },
    { "seagreen"                , core::JobRgb{    46, 139, 87    , 255 } },
    { "seashell"                , core::JobRgb{   255, 245, 238   , 255 } },
    { "sienna"                  , core::JobRgb{   160, 82, 45     , 255 } },
    { "silver"                  , core::JobRgb{   192, 192, 192   , 255 } },
    { "skyblue"                 , core::JobRgb{   135, 206, 235   , 255 } },
    { "slateblue"               , core::JobRgb{   106, 90, 205    , 255 } },
    { "slategray"               , core::JobRgb{   112, 128, 144   , 255 } },
    { "slategrey"               , core::JobRgb{   112, 128, 144   , 255 } },
    { "snow"                    , core::JobRgb{   255, 250, 250   , 255 } },
    { "springgreen"             , core::JobRgb{    0, 255, 127    , 255 } },
    { "steelblue"               , core::JobRgb{    70, 130, 180   , 255 } },
    { "tan"                     , core::JobRgb{   210, 180, 140   , 255 } },
    { "teal"                    , core::JobRgb{    0, 128, 128    , 255 } },
    { "thistle"                 , core::JobRgb{   216, 191, 216   , 255 } },
    { "tomato"                  , core::JobRgb{   255, 99, 71     , 255 } },
    { "turquoise"               , core::JobRgb{    64, 224, 208   , 255 } },
    { "violet"                  , core::JobRgb{   238, 130, 238   , 255 } },
    { "wheat"                   , core::JobRgb{   245, 222, 179   , 255 } },
    { "white"                   , core::JobRgb{   255, 255, 255   , 255 } },
    { "whitesmoke"              , core::JobRgb{   245, 245, 245   , 255 } },
    { "yellow"                  , core::JobRgb{   255, 255, 0     , 255 } },
    { "yellowgreen"             , core::JobRgb{   154, 205, 50    , 255 } }
};
}
