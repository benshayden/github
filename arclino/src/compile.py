#!python
import os
src = os.path.dirname(os.path.realpath(__file__))
sh = file(os.path.join(src, 'arclino.sh')).read()
def fill(slug, fn, fix=lambda x:x):
  return sh.replace(slug + '\n' + slug, slug + '\n' + fix(file(os.path.join(src, fn)).read()) + '\n' + slug)
sh = sh.replace('\nCONF_SH\n', file(os.path.join(src, 'conf.sh')).read())
sh = fill('ARDUINO_H', 'Arduino.h')
sh = fill('MAKEFILE_DOC', 'arclino.mk', lambda s:s.replace('$','\\$'))
sh = fill('ARCLINO_TEST_CC', 'arclino_test.cc')
sh = fill('ARCLINO_TEST_H', 'arclino_test.h')
sh = sh.replace('DEMANGLE_CC', file(os.path.join(src, 'demangle.cc')).read())
file(os.path.join(src, '..', 'arclino'), 'w').write(sh)
