def get_rev_id(env):
    try:
        import subprocess
        ret = subprocess.check_output('hg tip -T \"r{short(node)}, {date(date, \'%Y.%m.%d %H:%M\')}\"', shell=True)
        if ret:
            return ret
    except:
        pass
    return '[unknown]'


def gen_rev_id(target, source, env):
    f = open(str(target[0]), 'wb')
    f.write('#define DCPP_REVISION "%s"\n' % get_rev_id(env))
    f.close()
