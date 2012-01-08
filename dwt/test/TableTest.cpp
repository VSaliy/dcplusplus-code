#include <dwt/widgets/Window.h>
#include <dwt/widgets/Table.h>
#include <dwt/Texts.h>

namespace dwt {
tstring Texts::get(Text text) { return _T("test"); }
}

using dwt::tstring;

int dwtMain(dwt::Application& app)
{
	auto window = new dwt::Window();
	window->create();
	window->onClosing([] { return ::PostQuitMessage(0), true; });

	auto table = window->addChild(dwt::Table::Seed());

	std::vector<tstring> columns;
	columns.push_back(_T("A"));
	columns.push_back(_T("B"));
	table->createColumns(columns);
	table->insert(columns);

	table->resize(dwt::Rectangle(window->getClientSize()));

	app.run();

	return 0;
}
