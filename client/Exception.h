/* 
 * Copyright (C) 2001 Jacek Sieka, jacek@creatio.se
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _EXCEPTION_H
#define _EXCEPTION_H

class Exception  
{
public:
	Exception();
	Exception(const string& aError, const string& aStack);

	virtual const string& getError() const { return error; };
	virtual const string& getStack() const { return stack; };

	virtual ~Exception();
protected:
	string error;
	string stack;
};

#define STANDARD_EXCEPTION(name) class name : public Exception { \
public:\
	name(const Exception& e, const string& aError) : Exception(aError + ":" + e.getError(), #name ":" + e.getStack()) { }; \
	name(const string& aError) : Exception(aError, #name) { }; \
}

STANDARD_EXCEPTION(FileException);

#endif // _EXCEPTION_H

/**
 * @file Exception.h
 * $Id: Exception.h,v 1.1 2001/11/21 17:33:20 arnetheduck Exp $
 * @if LOG
 * $Log: Exception.h,v $
 * Revision 1.1  2001/11/21 17:33:20  arnetheduck
 * Initial revision
 *
 * @endif
 */

