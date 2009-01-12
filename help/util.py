def get_lcid(lang):
	from locale import windows_locale

	lang = lang.replace('-', '_')

	# look for an exact match
	for (id, name) in windows_locale.iteritems():
		if name == lang:
			return id

	# ignore the "sub-language" part
	lang = lang.split('_')[0]
	for (id, name) in windows_locale.iteritems():
		if name.split('_')[0] == lang:
			return id

	return 0x409 # default: en-US

def get_win_cp(lcid):
	import ctypes

	LOCALE_IDEFAULTANSICODEPAGE = 0x1004
	LOCALE_RETURN_NUMBER = 0x20000000

	buf = ctypes.c_int()
	ctypes.windll.kernel32.GetLocaleInfoA(lcid, LOCALE_RETURN_NUMBER | LOCALE_IDEFAULTANSICODEPAGE, ctypes.byref(buf), 6)

	if buf.value != 0:
		return 'cp' + str(buf.value)
	return 'cp1252'
