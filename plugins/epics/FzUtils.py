import os

'''
convert 'num' bytes in humanized *bytes* with suffix and measurement units
'''
def human_byte(num, suffix='B'):
    for unit in ['','K','M','G','T','P','E','Z']:
        if abs(num) < 1024.0:
            data = {'value': num, 'unit': str(unit + suffix)}
            return data
        num /= 1024.0
    data = {'value': num, 'unit': str("Y" + suffix)}
    return data

'''
convert 'num' bytes in humanized *bits* with suffix and measurement units
'''
def human_bit(num, suffix='b'):
    for unit in ['','k','M','G','T','P','E','Z']:
        if abs(num) < 1000.0:
            data = {'value': num, 'unit': str(unit + suffix)}
            return data
        num /= 1000.0
    data = {'value': num, 'unit': str("Y" + suffix)}
    return data

'''
convert 'num' bytes in *Mbits* 
'''
def to_Mbit(num):
    return num/125000

'''
convert 'num' bytes in *Mbytes* 
'''
def to_Mbyte(num):
    return num/1024/1024

def check_pid(pidfile):
    if pidfile and os.path.exists(pidfile):
        try:
            pid = int(open(pidfile).read().strip())
            #os.kill(pid, 0)
            return pid
        except BaseException:
            return 0
    return 0

def write_pid(pidfile):
    piddir = os.path.dirname(pidfile)
    if piddir != '':
        try:
            os.makedirs(piddir)
        except OSError:
            pass
    with open(pidfile, 'w') as fobj:
        fobj.write(str(os.getpid()))
