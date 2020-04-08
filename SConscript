from building import *
Import('rtconfig')

src   = []
cwd   = GetCurrentDir()

# add max30102 src files.
if GetDepend('PKG_USING_MAX30102'):
    src += Glob('src/max30102.c')
if GetDepend('PKG_USING_MAX330102_ALGORITHM'):
    src += Glob('src/algorithm_by_RF.c')
    src += Glob('src/max30102_algorithm.c')
if GetDepend('RT_USING_SENSOR'):
    src += Glob('src/sensor_inven_max30102.c')

if GetDepend('PKG_USING_MAX30102_SAMPLE'):
    src += Glob('samples/max30102_sample.c')

# add max30102 include path.
path  = [cwd + '/inc']

# add src and include to group.
group = DefineGroup('max30102', src, depend = ['PKG_USING_MAX30102'], CPPPATH = path)
group = DefineGroup('max30102/samples', src, depend = ['PKG_USING_MAX30102_SAMPLE'], CPPPATH = path)
group = DefineGroup('max30102/algorithms', src, depend = ['PKG_USING_MAX330102_ALGORITHM'], CPPPATH = path)
Return('group')