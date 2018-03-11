// NowSound library by Rob Jellinghaus, https://github.com/RobJellinghaus/NowSound
// Licensed under the MIT license

#include "pch.h"

#include <future>
#include <sstream>

using namespace concurrency;
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

    TextBlock _textBlock{ nullptr };

    void UpdateTextBlock(const std::wstring& suffix)
    {
        std::wstring str(AppStateString);
        str.append(suffix);
        _textBlock.Text(str);
    }

	void OnLaunched(LaunchActivatedEventArgs const&)
    {
		_textBlock = TextBlock();
		_textBlock.Text(AppStateString);

		Window xamlWindow = Window::Current();

		xamlWindow.Content(_textBlock);
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
        co_await _ui_thread;
        UpdateTextBlock(L"Completed first wait");

        // The lambda created here fails to properly restore its captured "this" reference after the first two co_awaits.
        create_task([this]() -> IAsyncAction
        {
            // Switch away from UI thread and wait two seconds again.
            co_await resume_background();
            co_await resume_after(timeSpanFromSeconds(2));

            // Switch back and update text again.
            // THIS NEXT LINE CRASHES as the value of the captured "this" pointer appears corrupt.
            co_await _ui_thread;

            // This line is not reached.
            UpdateTextBlock(L"Completed second wait");
        });
	}
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	Application::Start([](auto &&) { make<App>(); });
}
