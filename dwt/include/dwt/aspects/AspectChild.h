/*
 * AspectContent.h
 *
 *  Created on: 2008-dec-02
 *      Author: arnetheduck
 */

#ifndef DWT_ASPECTCHILD_H_
#define DWT_ASPECTCHILD_H_

#include "../Widget.h"

namespace dwt {

/** Classes using this aspect may contain a single child window (see AspectContainer) */
template<typename WidgetType>
class AspectChild {
public:
	template<typename SeedType>
	typename SeedType::WidgetType::ObjectType addChild(const SeedType& seed) {
		// Only allow a single child
		Widget* child = getChild();
		if(child != NULL) {
			::DestroyWindow(child->handle());
		}
		return WidgetCreator<typename SeedType::WidgetType>::create(static_cast<WidgetType*>(this), seed);
	}

	Widget* getChild() {
		return hwnd_cast<Widget*>(::GetWindow(static_cast<WidgetType*>(this)->handle(), GW_CHILD));
	}
};

}

#endif /* DWT_ASPECTCHILD_H_ */
