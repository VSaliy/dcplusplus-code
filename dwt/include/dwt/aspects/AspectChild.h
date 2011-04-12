/*
 * AspectContent.h
 *
 *  Created on: 2008-dec-02
 *      Author: arnetheduck
 */

#ifndef DWT_ASPECTCHILD_H_
#define DWT_ASPECTCHILD_H_

#include <dwt/forward.h>
#include <dwt/widgets/Control.h>

namespace dwt {

/** Classes using this aspect may contain a single child window (see AspectContainer) */
template<typename WidgetType>
class AspectChild {
public:
	template<typename SeedType>
	typename SeedType::WidgetType::ObjectType addChild(const SeedType& seed) {
		// Only allow a single child
		auto child = getChild();
		if(child) {
			child->close();
		}
		return WidgetCreator<typename SeedType::WidgetType>::create(static_cast<WidgetType*>(this), seed);
	}

	Control* getChild() {
		return hwnd_cast<Control*>(::GetWindow(static_cast<WidgetType*>(this)->handle(), GW_CHILD));
	}
};

}

#endif /* DWT_ASPECTCHILD_H_ */
