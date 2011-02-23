#include <dwt/widgets/ScrolledContainer.h>

namespace dwt {

Point ScrolledContainer::getPreferredSize() {
	auto child = getChild();
	return child ? child->getPreferredSize() : Point(0, 0);
}

void ScrolledContainer::layout(const Rectangle &rect) {
	BaseType::layout(rect);
	auto child = getChild();
	if(!child) {
		return;
	}

	auto clientSize = getClientSize();
	auto childSize = child->getPreferredSize();

	::ShowScrollBar(handle(), SB_HORZ, childSize.x > clientSize.x);
	::ShowScrollBar(handle(), SB_VERT, childSize.y > clientSize.y);

	clientSize = getClientSize();

	if(childSize.x > clientSize.x) {
		setScrollInfo(SB_HORZ, clientSize.x, childSize.x);
	}

	if(childSize.y > childSize.y) {
		setScrollInfo(SB_VERT, clientSize.y, childSize.y);
	}

	child->layout(Rectangle(childSize));
}

void ScrolledContainer::setScrollInfo(int type, int page, int max, int pos) {
	SCROLLINFO si = { sizeof(SCROLLINFO) };

	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = max - 1;
	si.nPos = pos;
	si.nPage = type == SB_HORZ ? getClientSize().x : getClientSize().y;
	::SetScrollInfo(handle(), type, &si, TRUE);
}

bool ScrolledContainer::handleMessage(const MSG &msg, LRESULT &retVal) {
	if(!(msg.message == WM_HSCROLL || msg.message == WM_VSCROLL)) {
		return false;
	}

	auto child = getChild();

	if(!child) {
		return false;
	}

	auto childSize = child->getWindowSize();

	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };

	auto type = msg.message == WM_HSCROLL ? SB_HORZ : SB_VERT;
    ::GetScrollInfo(handle(), type, &si);

    auto orig = si.nPos;

    switch (LOWORD (msg.wParam)) {
    case SB_TOP:
    	si.nPos = 0;
    	break;
    case SB_BOTTOM:
    	si.nPos = si.nMax;
    	break;
    case SB_LINELEFT:
        si.nPos -= 1;
        break;
    case SB_LINERIGHT:
        si.nPos += 1;
        break;
    case SB_PAGELEFT:
        si.nPos -= si.nPage;
        break;
    case SB_PAGERIGHT:
        si.nPos += si.nPage;
        break;
    case SB_THUMBTRACK:
        si.nPos = si.nTrackPos;
        break;
    }

    ::SetScrollInfo(handle(), type, &si, FALSE);

	::GetScrollInfo(handle(), type, &si);
	auto hDiff = type == SB_HORZ ? orig - si.nPos : 0;
	auto vDiff = type == SB_VERT ? orig - si.nPos : 0;

	::ScrollWindowEx(handle(), hDiff, vDiff, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN);

	return true;
}

}
