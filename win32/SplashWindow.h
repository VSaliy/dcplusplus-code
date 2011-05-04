#ifndef SPLASHWINDOW_H_
#define SPLASHWINDOW_H_

#include <dwt/widgets/Window.h>

class SplashWindow : public dwt::Window  {
public:
	SplashWindow();
	virtual ~SplashWindow();
	void operator()(const string& str);
private:
	dwt::Window* tmp;
	dwt::TextBoxPtr text;
};

#endif /*SPLASHWINDOW_H_*/
