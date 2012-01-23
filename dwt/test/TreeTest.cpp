#ifdef _MSC_VER
#include <map> // fix some include trouble on MSVC
#endif

#include <dwt/widgets/Window.h>
#include <dwt/widgets/Tree.h>
#include <dwt/Texts.h>

#include <iostream>

namespace dwt {
tstring Texts::get(Text text) { return _T("test"); }
}

using dwt::tstring;

int dwtMain(dwt::Application& app)
{
	auto window = new dwt::Window();
	window->create();
	window->onClosing([] { return ::PostQuitMessage(0), true; });

	auto tree = window->addChild(dwt::Tree::Seed());
	//tree.setHeaderVisible(true);

	tree->addColumn(_T("Column 1"), 200, dwt::Column::LEFT);
	tree->addColumn(_T("Column 2"), 200, dwt::Column::CENTER);
	tree->addColumn(_T("Column 3"), 200, dwt::Column::RIGHT);

	for (int i = 0; i < 4; i++) {
		tstring name(_T("item 1"));
		name.back() += i;

		auto item = tree->insert(name, NULL, 1);
		tree->setText(item, 1, _T("sub") + name);

		assert(tree->getData(item) == 1);
		for (int j = 0; j < 4; j++) {
			tstring subname(_T("item 1"));
			subname.back() += j;
			auto subItem = tree->insert(subname, item);
			tree->setText(subItem, 1, _T("sub") + subname);
		}
	}

	tree->resize(dwt::Rectangle(window->getClientSize()));

	app.run();

	return 0;
}




