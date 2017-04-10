#include <dwt/widgets/Window.h>
#include <dwt/widgets/RichTextBox.h>

#include <boost/format.hpp>

// set to 16384 to test a bug when adding too many new lines at once.
// ref: <https://bugs.launchpad.net/dcplusplus/+bug/1681153>.
const auto NEW_LINES_PER_CLICK = 2;

int dwtMain(dwt::Application& app)
{
	auto window = new dwt::Window();
	window->create();
	window->onClosing([] { return ::PostQuitMessage(0), true; });

	auto richtextbox = window->addChild(dwt::RichTextBox::Seed());
	richtextbox->setTextLimit(1024*64*2);

	richtextbox->setText(L"click to add lines");

	auto counter = 0;

	richtextbox->onLeftMouseDown([&](const auto&) {
		richtextbox->addTextSteady(
			L"{\\urtf1\n" +
			dwt::RichTextBox::rtfEscape(
				std::wstring(NEW_LINES_PER_CLICK, '\n') +
				(boost::wformat(L"new line #%d") % counter++).str()
			) +
			L"}\n"
		);
		return true;
	});

	richtextbox->resize(window->getClientSize());
	window->onSized([=](const auto&) { richtextbox->resize(window->getClientSize()); });

	app.run();

	return 0;
}
