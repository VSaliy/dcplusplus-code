import glob
import sys
import os

class Dev:
	def __init__(self, mode, tools, env):
		
		self.mode = mode
		self.tools = tools
		self.env = env

		self.build_root = '#/build/' + self.mode + '-' + self.tools
		if env['arch'] != 'x86':
			self.build_root += '-' + env['arch']
		self.build_root += '/'
	
	def prepare(self):
		if not self.env['verbose']:
			self.env['CCCOMSTR'] = "Compiling $TARGET (static)"
			self.env['SHCCCOMSTR'] = "Compiling $TARGET (shared)"
			self.env['CXXCOMSTR'] = "Compiling $TARGET (static)"
			self.env['SHCXXCOMSTR'] = "Compiling $TARGET (shared)"
			self.env['PCHCOMSTR'] = "Compiling $TARGET (precompiled header)"
			self.env['GCHCOMSTR'] = "Compiling $TARGET (static precompiled header)"
			self.env['GCHSHCOMSTR'] = "Compiling $TARGET (shared precompiled header)"
			self.env['SHLINKCOMSTR'] = "Linking $TARGET (shared)"
			self.env['LINKCOMSTR'] = "Linking $TARGET (static)"
			self.env['ARCOMSTR'] = "Archiving $TARGET"
			self.env['RCCOMSTR'] = "Resource $TARGET"
		
		self.env.SConsignFile()
		self.env.SetOption('implicit_cache', '1')
		self.env.SetOption('max_drift', 60*10)
		self.env.Decider('MD5-timestamp')

		if 'mingw' in self.env['TOOLS']:
			self.env.Append(LINKFLAGS=["-Wl,--enable-runtime-pseudo-reloc"])
			
			prefix = ''
			if self.env.get('prefix') is not None:
				prefix = self.env['prefix']
			elif sys.platform != 'win32':
				prefix = 'i386-mingw32-'
			
			self.env['CC'] = prefix + 'gcc'
			self.env['CXX'] = prefix + 'g++'
			self.env['LINK'] = prefix + 'g++'
			self.env['AR'] = prefix + 'ar'
			self.env['RANLIB'] = prefix + 'ranlib'
			self.env['RC'] = prefix + 'windres'
			
			if sys.platform != 'win32':
				self.env['PROGSUFFIX'] = '.exe'
				self.env['LIBPREFIX'] = 'lib'
				self.env['LIBSUFFIX'] = '.a'
				self.env['SHLIBSUFFIX'] = '.dll'
				
	def is_win32(self):
		return sys.platform == 'win32' or 'mingw' in self.env['TOOLS']

	def get_build_root(self):
		return self.build_root

	def get_build_path(self, source_path):
		return self.get_build_root() + source_path
	
	def get_target(self, source_path, name, in_bin = True):
		if in_bin:
			return self.get_build_root() + 'bin/' + name
		else:
			return self.get_build_root() + source_path + name
		
	def get_sources(self, source_path, source_glob):
		return map(lambda x: self.get_build_path(source_path) + x, glob.glob(source_glob))
		
	def prepare_build(self, source_path, name, source_glob = '*.cpp', in_bin = True, precompiled_header = None):
		env = self.env.Clone()
		env.BuildDir(self.get_build_path(source_path), '.', duplicate = 0)

		sources = self.get_sources(source_path, source_glob)

		if precompiled_header is not None:
			for i, source in enumerate(sources):
				if source.find(precompiled_header + '.cpp') != -1:
					# the PCH/GCH builder will take care of this one
					del sources[i]

			if env['CC'] == 'cl': # MSVC
				env['PCHSTOP'] = precompiled_header + '.h'
				env['PCH'] = env.PCH(self.get_target(source_path, precompiled_header + '.pch', False), precompiled_header + '.cpp')[0]
			elif 'gcc' in env['TOOLS']:
				env['Gch'] = env.Gch(self.get_target(source_path, precompiled_header + '.gch', False), precompiled_header + '.h')[0]

		return (env, self.get_target(source_path, name, in_bin), sources)

	def build(self, source_path, local_env = None):
		if not local_env:
			local_env = self.env
		full_path = local_env.Dir('.').path + '/' + source_path	
		return local_env.SConscript(source_path + 'SConscript', exports={'dev' : self, 'source_path' : full_path })

	def i18n (self, source_path, buildenv, sources, name):
		if self.env['mode'] != 'release' and not self.env['i18n']:
			return

		p_oze = glob.glob('po/*.po')

		potfile = 'po/' + name + '.pot'
		buildenv['PACKAGE'] = name
		ret = buildenv.PotBuild(potfile, sources)

		for po_file in p_oze:
			buildenv.Precious(buildenv.PoBuild(po_file, [potfile]))

			lang = os.path.basename(po_file)[:-3]
			locale_path = self.get_target(source_path, 'locale/' + lang + '/')

			buildenv.MoBuild(locale_path + 'LC_MESSAGES/' + name + '.mo', po_file,
					NAME_FILE = buildenv.File(locale_path + 'name.txt'))

#		languages = [ os.path.basename(po).replace ('.po', '') for po in p_oze ]
#		for lang in languages:
#			modir = (os.path.join (install_prefix, 'share/locale/' + lang + '/LC_MESSAGES/'))
#			moname = domain + '.mo'
#			installenv.Alias('install', installenv.InstallAs (os.path.join (modir, moname), lang + '.mo'))

		return ret

	# support installs that only have an asciidoc.py file but no executable
	def get_asciidoc(self):
		if 'PATHEXT' in self.env['ENV']:
			pathext = self.env['ENV']['PATHEXT'] + ';.py'
		else:
			pathext = ''
		asciidoc = self.env.WhereIs('asciidoc', pathext = pathext)
		if asciidoc is None:
			return None
		if asciidoc[-3:] == '.py':
			if self.env.WhereIs('python') is None:
				return None
			asciidoc = 'python ' + asciidoc
		return asciidoc

# source is *one* SCons file node (not a list!) designating the .po file
# env must contain 'NAME_FILE', which is a SCons file node to the target file
def gen_po_name(source, env):
	# rely on the comments at the beginning of the po file to find the language name.
	import codecs, re
	match = re.compile('^# (.+) translation.*', re.I).search(codecs.open(str(source), 'rb', 'utf_8').readline())
	if match:
		name = match.group(1)
		if name != "XXX":
			codecs.open(str(env['NAME_FILE']), 'wb', 'utf_8').write(name)

def CheckPKGConfig(context, version):
	context.Message( 'Checking for pkg-config... ' )
	ret = context.TryAction('pkg-config --atleast-pkgconfig-version=%s' % version)[0]
	context.Result( ret )
	return ret
	
def CheckPKG(context, name):
	context.Message( 'Checking for %s... ' % name )
	ret = context.TryAction('pkg-config --exists "%s"' % name)[0]
	if ret:
		context.env.ParseConfig('pkg-config --cflags --libs "%s"' % name)
		
	context.Result( ret )
	return ret
	   
