#ifndef SPLASHWINDOW_H_
#define SPLASHWINDOW_H_

class SplashWindow : public dwt::Window  {
public:
	SplashWindow();
	~SplashWindow();
	void operator()(const string& str);
private:
	dwt::Window* tmp;
	dwt::TextBoxPtr text;
};

#endif /*SPLASHWINDOW_H_*/
