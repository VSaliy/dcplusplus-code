/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2008, Jacek Sieka

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <dwt/widgets/Grid.h>

#include <numeric>

namespace dwt {

Grid::Seed::Seed(size_t rows_, size_t cols_) : BaseType::Seed(_T(""), WS_CHILD | WS_VISIBLE, 0), rows(rows_), cols(cols_) { }

void Grid::create( const Seed & cs )
{
	BaseType::create(cs);
	rows.resize(cs.rows);
	columns.resize(cs.cols);
}

Point Grid::getPreferedSize() {
	std::vector<size_t> rowSize = calcSizes(rows, columns, 0, true);
	std::vector<size_t> colSize = calcSizes(columns, rows, 0, false);
	Point p(std::accumulate(colSize.begin(), colSize.end(), 0), std::accumulate(rowSize.begin(), rowSize.end(), 0));
	return p;
}

Point Grid::getPreferedSize(size_t row, size_t column) const {
	Point ret(0, 0);

	for(WidgetInfoList::const_iterator i = widgetInfo.begin(); i != widgetInfo.end(); ++i) {
		if(i->inCell(row, column)) {
			return i->w->getPreferedSize();
		}
	}
	return ret;
}

bool Grid::WidgetInfo::inCell(size_t r, size_t c) const {
	return r >= row && r < (row + rowSpan) && c >= column && c < (column + colSpan);
}

GridInfo& Grid::row(size_t i) {
	return rows[i];
}

GridInfo& Grid::column(size_t i) {
	return columns[i];
}

std::vector<size_t> Grid::calcSizes(const GridInfoList& x, const GridInfoList& y, size_t cur, bool isRow) const {
	std::vector<size_t> ret(x.size());

	size_t total = 0;
	size_t fills = 0;
	for(size_t i = 0; i < ret.size(); ++i) {
		ret[i] = x[i].size;

		switch(x[i].mode) {
		case GridInfo::STATIC:
			break;
		case GridInfo::FILL:
			fills++;
			break;
		case GridInfo::AUTO:
			for(size_t j = 0; j < y.size(); ++j) {
				ret[i] = std::max(ret[i], static_cast<size_t>(isRow ? getPreferedSize(i, j).y : getPreferedSize(j, i).x));
			}
			break;
		}
		total += ret[i];
	}

	if(total < cur && fills > 0) {
		size_t left = cur - total;
		for(size_t i = 0; i < ret.size(); ++i) {
			if(x[i].mode != GridInfo::FILL)
				continue;
			size_t take = left / fills;
			ret[i] += take;
			left -= take;
			fills--;
		}
	}
	return ret;
}

void Grid::layout() {
	std::vector<HWND> children;

	// TODO find better way of keeping track of children
	for(HWND wnd = ::FindWindowEx(handle(), NULL, NULL, NULL); wnd; wnd = ::FindWindowEx(handle(), wnd, NULL, NULL)) {
		WidgetInfo* wi = getWidgetInfo(wnd);
		if(!wi)
			continue;
		children.push_back(wnd);
	}

	Point size = getSize();

	std::vector<size_t> rowSize = calcSizes(rows, columns, size.y, true);
	std::vector<size_t> colSize = calcSizes(columns, rows, size.x, false);

	for(std::vector<HWND>::iterator i = children.begin(); i != children.end(); ++i) {
		WidgetInfo* wi = getWidgetInfo(*i);
		if(!wi->w)
			continue;

		size_t r = wi->row;
		size_t rs = wi->rowSpan;

		size_t c = wi->column;
		size_t cs = wi->colSpan;

		if(r + rs > rowSize.size() || c + cs > colSize.size()) {
			continue;
		}
		size_t x = std::accumulate(colSize.begin(), colSize.begin() + c, 0);
		size_t y = std::accumulate(rowSize.begin(), rowSize.begin() + r, 0);

		size_t w = std::accumulate(colSize.begin() + c, colSize.begin() + c + cs, 0);
		size_t h = std::accumulate(rowSize.begin() + r, rowSize.begin() + r + rs, 0);

		::MoveWindow(wi->w->handle(), x, y, w, h, TRUE);
	}
}

Grid::WidgetInfo* Grid::getWidgetInfo(HWND hwnd) {
	size_t maxc = 0;

	for(WidgetInfoList::iterator i = widgetInfo.begin(); i != widgetInfo.end(); ++i) {
		if(i->w->handle() == hwnd) {
			return &(*i);
		}
		maxc = std::max(maxc, i->row * columns.size() + i->column);
	}

	if(!widgetInfo.empty())
		maxc++;

	size_t r = maxc / columns.size();
	size_t c = maxc % columns.size();

	if(r >= rows.size() || c >= columns.size()) {
		return 0;
	}

	Widget* w = hwnd_cast<Widget*>(hwnd);
	if(!w) {
		return 0;
	}

	widgetInfo.push_back(WidgetInfo(w, r, c, 1, 1));
	return &widgetInfo.back();
}

}
