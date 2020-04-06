/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-07
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
using Blob = std::basic_string<uint8_t>;

namespace Filesharing
{
    Blob Read(std::string_view Sharename, uint32_t Maxsize = 4096);
    bool Write(std::string_view Sharename, Blob &&Data);
    bool Write(std::string_view Sharename, std::string &&Data);
}
