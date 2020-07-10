/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-10
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include "Common/Graphics/Graphics.hpp"
#include "Common/Graphics/Drawing.hpp"
#include "Common/Overlay/Console.hpp"

namespace Pluginloader
{
    void Loadplugins();

    bool InstallTLSCallback();
    void RestoreTLSCallback();

    bool InstallEPCallback();
    void RestoreEPCallback();
}

// Forward declarations for Appmain.
void onStartup();
void onFrame();
