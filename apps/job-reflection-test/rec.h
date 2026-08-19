#pragma once

#include <iostream>
#include <meta>
#include <string_view>
#include <utility>

consteval bool startsWith(std::string_view value, std::string_view prefix)
{
    return value.starts_with(prefix);
}

template <typename T>
consteval auto findSlot(std::string_view name)
{
    constexpr auto ctx =
        std::meta::access_context::unchecked();

    template for (constexpr auto member :
                  std::define_static_array(
                      std::meta::members_of(^^T, ctx)))
    {
        if constexpr (std::meta::has_identifier(member))
        {
            constexpr auto memberName =
                std::meta::identifier_of(member);

            if (memberName == name &&
                memberName.starts_with("slot"))
            {
                return member;
            }
        }
    }

    return std::meta::info{};
}
struct Receiver
{
    void slotValueChanged(int value)
    {
        std::cout << "slotValueChanged(" << value << ")\n";
    }

    void slotFinished()
    {
        std::cout << "slotFinished()\n";
    }

    void ordinaryFunction()
    {
    }
};