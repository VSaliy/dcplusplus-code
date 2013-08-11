# vim: set filetype: py

EnsureSConsVersion(0, 98, 5)

import os,sys
from build_util import Dev, gen_po_name

# TODO the ipa-cp-clone optimization is disabled; it causes a crash when starting a DL.

# TODO enable LTO when it doesn't ICE... (-flto)

# TODO remove -Wno-format when http://cygwin.com/ml/cygwin/2012-01/msg00061.html
# ("mingw64-i686-gcc-4.5.3-4: -Wformat warnings broken in C++") fixed

# TODO remove -Wno-unused-local-typedefs when boost doesn't trigger it

# TODO add -fdebug-types-section when it doesn't ICE

gcc_flags = {
	'common': ['-g', '-Wall', '-Wextra', '-Wno-unused-local-typedefs', '-Wno-unused-parameter', '-Wno-unused-value', '-Wno-missing-field-initializers', '-Wno-address', '-Wno-unknown-pragmas', '-Wno-format', '-fexceptions'],
	'debug': [], 
	'release' : ['-O3', '-fno-ipa-cp-clone']
}

gcc_xxflags = {
	'common' : ['-std=gnu++11'],
	'debug' : [],
	'release' : []
}

msvc_flags = {
	# 4100: unreferenced formal parameter
	# 4121: alignment of member sensitive to packing
	# 4127: conditional expression is constant
	# 4189: var init'd, unused
	# 4244: possible loss of data on conversion
	# 4290: exception spec ignored
	# 4307: integral constant overflow (size_t -1 in boost lockfree)
	# 4324: structure padded due to __declspec(align())
	# 4355: "this" used in a constructor
	# 4510: no default constructor
	# 4512: assn not generated
	# 4610: no default constructor
	# 4706: assignment within conditional expression
	# 4800: converting from BOOL to bool
	# 4996: fn unsafe, use fn_s
	'common' : ['/W4', '/EHsc', '/Zi', '/Zm200', '/GR', '/FC', '/wd4100', '/wd4121', '/wd4127', '/wd4189', '/wd4244', '/wd4290', '/wd4307', '/wd4324', '/wd4355', '/wd4510', '/wd4512', '/wd4610', '/wd4706', '/wd4800', '/wd4996'],
	'debug' : ['/MDd'],
	'release' : ['/MD', '/O2']
}

msvc_xxflags = {
	'common' : [],
	'debug' : [],
	'release' : []
}

gcc_link_flags = {
	'common' : ['-g', '-Wl,--no-undefined,--nxcompat,--dynamicbase', '-time'],
	'debug' : [],
	'release' : ['-O3']
}

msvc_link_flags = {
	'common' : ['/DEBUG', '/FIXED:NO', '/INCREMENTAL:NO', '/SUBSYSTEM:WINDOWS', '/MANIFESTUAC:NO'],
	'debug' : [],
	'release' : []
}

# TODO remove _VARIADIC_MAX if/when VC has proper variadic template support
msvc_defs = {
	'common' : ['_REENTRANT', '_VARIADIC_MAX=10', 'snprintf=_snprintf'],
	'debug' : ['_DEBUG', '_HAS_ITERATOR_DEBUGGING=0', '_SECURE_SCL=0'],
	'release' : ['NDEBUG']
}

gcc_defs = {
	'common' : ['_REENTRANT', 'NO_VIZ'],
	'debug' : ['_DEBUG'],
	'release' : ['NDEBUG']
}

# defEnv will hold a temporary Environment used to get options that can't be set after the actual
# Environment has been created
defEnv = Environment(ENV = os.environ)
opts = Variables('custom.py', ARGUMENTS)

if sys.platform == 'win32':
	tooldef = 'mingw'
else:
	tooldef = 'default'

opts.AddVariables(
	EnumVariable('tools', 'Toolset to compile with, default = platform default (msvc under windows)', tooldef, ['mingw', 'default']),
	EnumVariable('mode', 'Compile mode', 'debug', ['debug', 'release']),
	BoolVariable('pch', 'Use precompiled headers', 'no'),
	BoolVariable('verbose', 'Show verbose command lines', 'no'),
	BoolVariable('savetemps', 'Save intermediate compilation files (assembly output)', 'no'),
	BoolVariable('unicode', 'Build a Unicode version on Windows', 'yes'),
	BoolVariable('stdatomic', 'Use a standard implementation of <atomic> (turn off to switch to Boost.Atomic)', 'no'),
	BoolVariable('i18n', 'Rebuild i18n files', 'no'),
	BoolVariable('help', 'Build help files (requires i18n=1)', 'yes'),
	BoolVariable('webhelp', 'Build help files for the web (requires help=1)', 'no'),
	('prefix', 'Prefix to use when cross compiling', ''),
	EnumVariable('arch', 'Target architecture', 'x86', ['x86', 'x64', 'ia64']),
	BoolVariable('msvcproj', 'Build MSVC project files', 'no'),
	BoolVariable('distro', 'Produce the official distro (forces tools=mingw, mode=release, unicode=1, i18n=1, help=1, webhelp=1, arch=x86)', 'no')
)

opts.Update(defEnv)
Help(opts.GenerateHelpText(defEnv))

if defEnv['distro']:
	defEnv['tools'] = 'mingw'
	defEnv['mode'] = 'release'
	defEnv['unicode'] = 1
	defEnv['i18n'] = 1
	defEnv['help'] = 1
	defEnv['webhelp'] = 1
	defEnv['arch'] = 'x86'

# workaround for SCons 1.2 which hard-codes possible archs (only allows 'x86' and 'amd64'...)
# TODO remove when SCons knows about all available archs
TARGET_ARCH = defEnv['arch']
if TARGET_ARCH == 'x64':
	TARGET_ARCH = 'amd64'

env = Environment(ENV = os.environ, tools = [defEnv['tools']], options = opts,
		TARGET_ARCH = TARGET_ARCH, MSVS_ARCH = TARGET_ARCH)

if env['distro']:
	env['tools'] = 'mingw'
	env['mode'] = 'release'
	env['unicode'] = 1
	env['i18n'] = 1
	env['help'] = 1
	env['webhelp'] = 1
	env['arch'] = 'x86'

# filter out boost from dependencies to get a speedier rebuild scan
# this means that if boost changes, scons -c needs to be run
# delete .sconsign.dblite to see the effects of this if you're upgrading
def filterBoost(x):
	return [y for y in x if str(y).find('boost') == -1]

SourceFileScanner.function['.c'].recurse_nodes = filterBoost
SourceFileScanner.function['.cpp'].recurse_nodes = filterBoost
SourceFileScanner.function['.h'].recurse_nodes = filterBoost
SourceFileScanner.function['.hpp'].recurse_nodes = filterBoost

dev = Dev(env)
dev.prepare()

env.SConsignFile()

env.Append(CPPPATH = ['#/'])

if dev.is_win32():
	# Windows header defines <http://msdn.microsoft.com/en-us/library/aa383745(VS.85).aspx>
	env.Append(CPPDEFINES = [
		'_WIN32_WINNT=0x502', # Windows XP SP2
		'WINVER=0x502', # Windows XP SP2
		'_WIN32_IE=0x600', # Common Controls 6

		# other defs that influence Windows headers
		'NOMINMAX', 'STRICT', 'WIN32_LEAN_AND_MEAN'])

	if env['unicode']:
		env.Append(CPPDEFINES = ['UNICODE', '_UNICODE'])

	# boost defines
	env.Append(CPPDEFINES = ['BOOST_ALL_NO_LIB', 'BOOST_USE_WINDOWS_H'])

	# zlib defines
	env.Append(CPPDEFINES = ['CASESENSITIVITYDEFAULT_YES', 'ZLIB_WINAPI'])

if 'gcc' in env['TOOLS']:
	if env['savetemps']:
		env.Append(CCFLAGS = ['-save-temps', '-fverbose-asm'])
	else:
		env.Append(CCFLAGS = ['-pipe'])

	# require i686 instructions for atomic<int64_t>, used by boost::lockfree (otherwise lockfree
	# lists won't actually be lock-free).
	if env['arch'] == 'x86':
		env.Append(CCFLAGS = ['-march=i686'])

if 'msvc' in env['TOOLS']:
	env['pch'] = True
if env['pch']:
	env.Append(CPPDEFINES = ['HAS_PCH'])

if 'mingw' in env['TOOLS']:
	env.Append(CCFLAGS = ['-mthreads'])
	env.Append(LINKFLAGS = ['-static-libgcc', '-static-libstdc++', '-mthreads'])

	import sys
	if env['mode'] == 'release' or sys.platform == 'win32':
		env.Append(CCFLAGS = ['-mwindows'])
		env.Append(LINKFLAGS = ['-mwindows'])
	else:
		env.Append(CPPDEFINES = ['CONSOLE'])

	env.Append(CPPPATH = ['#/mingw/preload/', '#/mingw/include/'])
	mingw_lib = '#/mingw/lib/'
	if env['arch'] != 'x86':
		mingw_lib = mingw_lib + env['arch'] + '/'
	env.Append(LIBPATH = [mingw_lib])

if 'msvc' in env['TOOLS']:
	flags = msvc_flags
	xxflags = msvc_xxflags
	link_flags = msvc_link_flags
	defs = msvc_defs

	env.Append(LIBS = ['User32', 'shell32', 'Advapi32'])

else:
	flags = gcc_flags
	xxflags = gcc_xxflags
	link_flags = gcc_link_flags
	defs = gcc_defs

	env.Tool("gch", toolpath=".")

env.Append(CPPDEFINES = defs[env['mode']])
env.Append(CPPDEFINES = defs['common'])

env.Append(CCFLAGS = flags[env['mode']])
env.Append(CCFLAGS = flags['common'])

env.Append(CXXFLAGS = xxflags[env['mode']])
env.Append(CXXFLAGS = xxflags['common'])

env.Append(LINKFLAGS = link_flags[env['mode']])
env.Append(LINKFLAGS = link_flags['common'])

env.SourceCode('.', None)

import SCons.Scanner
SWIGScanner = SCons.Scanner.ClassicCPP(
	"SWIGScan",
	".i",
	"CPPPATH",
	'^[ \t]*[%,#][ \t]*(?:include|import)[ \t]*(<|")([^>"]+)(>|")'
)
env.Append(SCANNERS=[SWIGScanner])

#
# internationalization (ardour.org provided the initial idea)
#

po_args = ['msgmerge', '-q', '--update', '--backup=none', '$TARGET', '$SOURCE']
po_bld = Builder (action = Action([po_args], 'Updating translation $TARGET from $SOURCES'))
env.Append(BUILDERS = {'PoBuild' : po_bld})

mo_args = ['msgfmt', '-c', '-o', '$TARGET', '$SOURCE']
mo_bld = Builder (action = [Action([mo_args], 'Compiling message catalog $TARGET from $SOURCES'),
	Action(lambda target, source, env: gen_po_name(source[0], env), 'Generating $NAME_FILE')])
env.Append(BUILDERS = {'MoBuild' : mo_bld})

pot_args = ['xgettext', '--from-code=UTF-8', '--foreign-user', '--package-name=$PACKAGE',
		'--copyright-holder=Jacek Sieka', '--msgid-bugs-address=dcplusplus-devel@lists.sourceforge.net',
		'--no-wrap', '--keyword=_', '--keyword=T_', '--keyword=TF_', '--keyword=TFN_:1,2',
		'--keyword=F_', '--keyword=gettext_noop', '--keyword=N_', '--keyword=CT_', '--boost', '-s',
		'--output=$TARGET', '$SOURCES']

pot_bld = Builder (action = Action([pot_args], 'Extracting messages to $TARGET from $SOURCES'))
env.Append(BUILDERS = {'PotBuild' : pot_bld})

def CheckFlag(context, flag):
	context.Message('Checking support for the ' + flag + ' flag... ')
	prevFlags = context.env['CCFLAGS']
	context.env.Append(CCFLAGS = [flag])
	ret = context.TryCompile('int main() {}', '.cpp')
	context.env.Replace(CCFLAGS = prevFlags)
	context.Result(ret)
	return ret

conf = Configure(env, custom_tests = { 'CheckFlag': CheckFlag }, conf_dir = dev.get_build_path('.sconf_temp'), log_file = dev.get_build_path('config.log'), clean = False, help = False)

if dev.is_win32():
	if conf.CheckCXXHeader(['windows.h', 'htmlhelp.h'], '<>'):
		conf.env.Append(CPPDEFINES='HAVE_HTMLHELP_H')

if 'mingw' in env['TOOLS']:
	# see whether we're compiling with MinGW or MinGW-w64 (2 different projects that can both build
	# a 32-bit program). the only differentiator is __MINGW64_VERSION_MAJOR.
	if not conf.CheckDeclaration('__MINGW64_VERSION_MAJOR', '#include <windows.h>', 'C++'):
		conf.env.Append(CPPDEFINES='HAVE_OLD_MINGW')

if 'gcc' in conf.env['TOOLS'] and conf.env['mode'] == 'debug':
	if conf.CheckFlag('-Og'):
		conf.env.Append(CCFLAGS = ['-Og'])
		conf.env.Append(LINKFLAGS = ['-Og'])

env = conf.Finish()

# TODO run config tests to determine which libs to build

dev.boost = dev.build('boost/') if dev.is_win32() else []
dev.dwarf = dev.build('dwarf/')
dev.zlib = dev.build('zlib/')
dev.bzip2 = dev.build('bzip2/') if dev.is_win32() else []
dev.geoip = dev.build('geoip/')
dev.intl = dev.build('intl/') if dev.is_win32() else []
dev.miniupnpc = dev.build('miniupnpc/')
dev.natpmp = dev.build('natpmp/')
dev.client = dev.build('dcpp/')

dev.dwt = dev.build('dwt/src/') if dev.is_win32() else []
if dev.is_win32():
	dev.build('dwt/test/')

if dev.is_win32():
	dev.build('help/')

dev.build('test/')
dev.build('utils/')

if dev.is_win32():
	dev.build('win32/')

	dev.build('installer/')

dev.finalize()
