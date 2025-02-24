import rtconfig
from building import *

cwd = GetCurrentDir()
src = Glob('*.c')
CPPPATH = [cwd, ]
    
group = DefineGroup('lcd_device', src, depend = [''], CPPPATH = CPPPATH)

Return('group')
