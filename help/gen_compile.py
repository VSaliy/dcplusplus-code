def gen_compile(target, source, env):
	from cgi import escape
	f_target = open(str(target[0]), "wb")
	f_source = open(str(source[0]), "rb")
	f_template = open(str(source[1]), "rb")
	f_target.write(f_template.read().replace("<!-- contents -->", """
<h1>Instructions to compile DC++</h1>
<pre>
""" + escape(f_source.read()) + "\n</pre>", 1))
	f_target.close()
	f_source.close()
	f_template.close()
