/*
 * BundleItem.h
 *
 *  Created on: 22 apr 2011
 *      Author: arnetheduck
 */

#ifndef BUNDLEITEM_H_
#define BUNDLEITEM_H_

#include "Bundle.h"
#include "Util.h"

namespace dcpp {

class BundleItem {
public:
	BundleItem();
	BundleItem(const string& root, const BundlePtr &bundle) : root(root), bundle(bundle) { }

	GETSET(string, root, Root);
	GETSET(BundlePtr, bundle, Bundle);
};

}

#endif /* BUNDLEITEM_H_ */
