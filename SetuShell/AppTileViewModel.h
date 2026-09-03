#pragma once
#include "AppTileViewModel.g.h"
namespace winrt::SetuShell::implementation
{
    struct AppTileViewModel : AppTileViewModelT<AppTileViewModel>
    {
        AppTileViewModel(hstring const& name, hstring const& glyph) : m_name(name), m_glyph(glyph) {}
        hstring Name() { return m_name; }
        hstring Glyph() { return m_glyph; }
    private:
        hstring m_name;
        hstring m_glyph;
    };
}

namespace winrt::SetuShell::factory_implementation
{
    struct AppTileViewModel : AppTileViewModelT<AppTileViewModel, implementation::AppTileViewModel>
    {
    };
}
