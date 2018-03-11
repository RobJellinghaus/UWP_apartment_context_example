// NowSound library by Rob Jellinghaus, https://github.com/RobJellinghaus/NowSound
// Licensed under the MIT license

#include "pch.h"

#include <future>

using namespace std::chrono;
using namespace winrt;

using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::System;

const int TicksPerSecond = 10000000;

TimeSpan timeSpanFromSeconds(int seconds)
{
	// TimeSpan is in 100ns units
	return TimeSpan(seconds * TicksPerSecond);
}

// Wait until the graph state becomes the expected state, or timeoutTime is reached.
IAsyncAction WaitFor(DateTime timeoutTime)
{
	while (winrt::clock::now() < timeoutTime)
	{
		// wait in intervals of 1/100 sec
		co_await resume_after(TimeSpan((int)(TicksPerSecond * 0.01f)));
	}
}

struct App : ApplicationT<App>
{
	const std::wstring AppStateString = L"App state: ";

    // Apartment context instance variable.  Why does this instance variable not get set properly
    // for use inside the Click lambda?
    apartment_context _ui_thread;

    IAsyncAction TickForever()
    {
        // first, require that we are in the background
        co_await resume_background();


    }

	void OnLaunched(LaunchActivatedEventArgs const&)
    {
		m_textBlock = TextBlock();
		m_textBlock.Text(AppStateString);

		m_button1 = Button();
		m_button1.Content(IReference<hstring>(L"Start Ticking"));

		m_button1.Click([&](IInspectable const& sender, RoutedEventArgs const&)
		{
		    
		});

		Window xamlWindow = Window::Current();

		StackPanel stackPanel = StackPanel();
		stackPanel.Children().Append(m_textBlock);
		stackPanel.Children().Append(m_button1);

		xamlWindow.Content(stackPanel);
		xamlWindow.Activate();

		// and here goes
		Async();
	}

    fire_and_forget Async()
    {
        apartment_context ui_thread;

        co_await resume_background();
        // wait only one second (and hopefully much less) for graph to become initialized.
        // 1000 second timeout is for early stage debugging.
        co_await WaitFor(winrt::clock::now() + timeSpanFromSeconds(2));

        {
            co_await ui_thread;
            std::wstring str(AppStateString);
            str.append(L"Initializing");
            m_textBlock.Text(str);
        }

        co_await WaitFor(winrt::clock::now() + timeSpanFromSeconds(2));

        {
            co_await ui_thread;
            std::wstring str(AppStateString);
            str.append(L"Running");
            m_textBlock.Text(str);
        }

        co_await resume_background();
	}

	TextBlock m_textBlock{ nullptr };
	Button m_button1{ nullptr };
	// Button m_button2{ nullptr };
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	Application::Start([](auto &&) { make<App>(); });
}
