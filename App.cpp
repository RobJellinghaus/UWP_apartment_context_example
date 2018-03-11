// NowSound library by Rob Jellinghaus, https://github.com/RobJellinghaus/NowSound
// Licensed under the MIT license

#include "pch.h"

#include <future>
#include <sstream>

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

TimeSpan timeSpanFromSeconds(double seconds)
{
	// TimeSpan is in 100ns units
	return TimeSpan((int)(seconds * TicksPerSecond));
}

struct App : ApplicationT<App>
{
	const std::wstring AppStateString = L"App state: ";

    // Apartment context instance variable.  Why does this instance variable not get set properly
    // for use inside the Click lambda?
    apartment_context _ui_thread;

    int _currentTick{ 0 };

    TextBlock _textBlock{ nullptr };
    Button _button1{ nullptr };

    void UpdateTextBlock(const std::wstring& suffix)
    {
        std::wstring str(AppStateString);
        str.append(suffix);
        _textBlock.Text(str);
    }

    // Tick forever, updating the text block at each two-second tick.
    // Clicking the button multiple times will create multiple concurrent instances of this loop.
    // But because of the context switching, these are effectively mutually excluded from updating
    // the text block racefully, and these concurrent loops coexist just fine!  Each loop causes the
    // block to be updated an additional 100x per second.
    IAsyncAction TickForever()
    {
        for (;;)
        {
            // first, require that we are in the background while we wait
            co_await resume_background();

            // wait two seconds
            co_await resume_after(timeSpanFromSeconds(0.01));

            // switch to UI thread in order to update _textBlock.
           co_await _ui_thread;

            std::wstringstream wstr{};
            wstr << L"Tick #" << (_currentTick++);
            UpdateTextBlock(wstr.str());
        }
    }

	void OnLaunched(LaunchActivatedEventArgs const&)
    {
		_textBlock = TextBlock();
		_textBlock.Text(AppStateString);

		_button1 = Button();
		_button1.Content(IReference<hstring>(L"Start Ticking"));

		_button1.Click([&](IInspectable const&, RoutedEventArgs const&)
		{
            // start ticking forever
            // QUESTION: Is this a valid way to enter an infinite asynchronous loop?  Does this need to be co_awaited?
            // Would it cause any type of context/thread starvation if it were awaited and effectively looped forever?
            TickForever();
		});

		Window xamlWindow = Window::Current();

		StackPanel stackPanel = StackPanel();
		stackPanel.Children().Append(_textBlock);
		stackPanel.Children().Append(_button1);

		xamlWindow.Content(stackPanel);
		xamlWindow.Activate();

		// and here goes
		Async();
	}

    fire_and_forget Async()
    {
        // We are being called on the UI thread, so capture its context.
        apartment_context ui_thread;

        // attempt to save the apartment_context in an instance variable.
        // QUESTION: Is this a valid operation? This certainly compiles without complaint... should it be expected to work?
        _ui_thread = ui_thread;

        // Switch away from UI thread and wait two seconds.
        co_await resume_background();
        co_await resume_after(timeSpanFromSeconds(2));

        // Switch back and update text.
        co_await ui_thread;
        UpdateTextBlock(L"Initializing");

        // Switch away from UI thread and wait two seconds again.
        co_await resume_background();
        co_await resume_after(timeSpanFromSeconds(2));

        // Switch back and update text again.
        co_await ui_thread;
        UpdateTextBlock(L"Waiting");
	}
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	Application::Start([](auto &&) { make<App>(); });
}
