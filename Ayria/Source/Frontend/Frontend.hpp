/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2022-03-08
    License: MIT
*/

#pragma once
#include <Global.hpp>

namespace Frontend
{
    // Q3-style console.
    std::thread CreateQ3Console();

    // Overlay-console for a window.
    std::thread CreateOverlayconsole();

    // Graphical UI for the client.
    std::thread CreateAppwindow();
}
