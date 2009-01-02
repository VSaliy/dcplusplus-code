# this script generates an HTML Help compatible table of contents

def gen_toc(target, source):
	import codecs
	from HTMLParser import HTMLParser
	from htmlentitydefs import entitydefs
	import re

	spaces = re.compile("\s+")

	# define our HTML parsing class derived from HTMLParser
	class Parser(HTMLParser):
		text = ""
		link = ""
		in_ul = 0
		keep_data = 0

		def handle_starttag(self, tag, attrs):
			if tag == "a" and self.in_ul:
				# attrs is a list of tuples; each tuple being an (attr-name, attr-content) pair
				for attr in attrs:
					if attr[0] == "href":
						self.link = attr[1]
						self.keep_data = 1
			elif tag == "h3":
				# title of a new section
				self.keep_data = 1
			elif tag == "ul":
				f_target.write("\r\n<ul>")
				self.in_ul += 1

		def handle_data(self, data):
			if self.keep_data:
				self.text += data

		def handle_entityref(self, name):
			if self.keep_data:
				self.text += entitydefs[name]

		def handle_endtag(self, tag):
			if tag == "a" and self.in_ul:
					# reached the end of the current link entry
					f_target.write('\r\n<li> <object type="text/sitemap">\r\n<param name="Name" value="')
					f_target.write(spaces.sub(" ", self.text).strip())
					f_target.write('">\r\n<param name="Local" value="')
					f_target.write(self.link)
					f_target.write('">\r\n</object>')
					self.text = ""
					self.link = ""
					self.keep_data = 0
			elif tag == "h3":
				# title of a new section
				f_target.write('\r\n<li> <object type="text/sitemap">\r\n<param name="Name" value="')
				f_target.write(self.text)
				f_target.write('">\r\n</object>')
				self.text = ""
				self.keep_data = 0
			elif tag == "ul":
				f_target.write("\r\n</ul>")
				self.in_ul -= 1

	f_target = codecs.open(target, "w", "latin_1", "xmlcharrefreplace")
	f_target.write("""<html>
<head>
</head>
<body>
<object type=\"text/site properties\">
<param name=\"ImageType\" value=\"Folder\">
</object>
""")

	parser = Parser()
	f_source = codecs.open(source, "r", "utf_8")
	parser.feed(f_source.read())
	f_source.close()
	parser.close()

	f_target.write("\r\n</body>\r\n</html>")
	f_target.close()
