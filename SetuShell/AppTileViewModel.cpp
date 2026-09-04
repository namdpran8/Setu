#include "pch.h"
#include "AppTileViewModel.h"
#if __has_include("AppTileViewModel.g.cpp")
#include "AppTileViewModel.g.cpp"
#endif

namespace winrt::SetuShell::implementation
{
    void AppTileViewModel::IsRunning(bool value)
    {
        if (m_isRunning != value)
        {
            m_isRunning = value;
            m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{L"IsRunning"});
        }
    }

    winrt::event_token AppTileViewModel::PropertyChanged(winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void AppTileViewModel::PropertyChanged(winrt::event_token const& token) noexcept
    {
        m_propertyChanged.remove(token);
    }
    
    winrt::Microsoft::UI::Xaml::Visibility AppTileViewModel::BoolToVis(bool value)
    {
        return value ? winrt::Microsoft::UI::Xaml::Visibility::Visible : winrt::Microsoft::UI::Xaml::Visibility::Collapsed;
    }
}
