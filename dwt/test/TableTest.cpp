#include <dwt/widgets/Window.h>
#include <dwt/widgets/Table.h>

using dwt::tstring;

int dwtMain(dwt::Application& app)
{
	auto window = new dwt::Window();
	window->create();
	window->onClosing([] { return ::PostQuitMessage(0), true; });

	auto table = window->addChild(dwt::Table::Seed());

	table->addColumn(dwt::Column(_T("Column A")));
	table->addColumn(dwt::Column(_T("Column B")));

	table->eraseColumn(1);
	table->addColumn(_T("Column C"), dwt::Column::SIZE_TO_HEADER);

	table->setColumnWidth(0, 100);

	auto order = table->getColumnOrder();
	order[0] = 1;
	order[1] = 0;
	table->setColumnOrder(order);

	std::vector<tstring> rows;
	rows.push_back(_T("A"));
	rows.push_back(_T("B"));

	table->insert(rows);

	table->resize(dwt::Rectangle(window->getClientSize()));

	app.run();

	return 0;
}
