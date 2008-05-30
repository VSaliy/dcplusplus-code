def gen_compile(target, source, env):
	f_target = open(str(target[0]), "w")
	f_source = open(str(source[0]), "r")
	f_template = open(str(source[1]), "r")
	f_target.write(f_template.read().replace("<!-- contents -->", """
<h1>Instructions to compile DC++</h1>
<pre>
""" + f_source.read() + "\r\n</pre>", 1))
	f_target.close()
	f_source.close()
	f_template.close()
