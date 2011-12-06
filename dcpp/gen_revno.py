def get_revision(env):
	try:
		import bzrlib
		import bzrlib.builtins

		tree = bzrlib.builtins.WorkingTree.open_containing(env.Dir("#").abspath)[0]

		return tree.branch.revision_id_to_revno(tree.last_revision())

	except:
		f = None
		try:
			f = open(env.Dir("#").abspath+"/.bzr/branch/last-revision",'r')
			line = f.read()
			pos = line.find(' ')
			if pos > 0:
				return int(line[:pos])
		except:
			pass
		finally:
			if f:
				f.close()
		return 0

def gen_revno(target, source, env):
	f = open(str(target[0]), 'wb')
	f.write('#define DCPP_REVISION ' + str(get_revision(env)) + '\n')
	f.close()
