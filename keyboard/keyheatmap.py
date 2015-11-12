#!/usr/bin/env python2
import code
import ctypes
import json
import os
import readline
import sys
import time

x11 = ctypes.CDLL('libX11.so')

class XModifierKeymap(ctypes.Structure):
  __slots__ = ['max_keypermod', 'modifiermap']
  _fields_ = [('max_keypermod', ctypes.c_int),
              ('modifiermap', ctypes.POINTER(ctypes.c_ubyte))]

x11.XGetModifierMapping.restype = ctypes.POINTER(XModifierKeymap)
x11.XKeysymToString.restype = ctypes.c_char_p

MODIFIERS = ['Shift', 'Lock', 'Control', 'Alt', 'Mod2', 'Mod3', 'Super', 'Mod5']

def bit(buf, i):
  return ord(buf[i/8]) & (1 << (i % 8)) != 0

def genkeys(delay_ms=10):
  buf_a = ctypes.create_string_buffer(32)
  buf_b = ctypes.create_string_buffer(32)
  current_buf, prev_buf = buf_a, buf_b
  display = x11.XOpenDisplay(os.environ.get('DISPLAY', ':0'))
  if not display:
    return
  x11.XSynchronize(display, True)
  mod_map = x11.XGetModifierMapping(display)[0]
  alt_mods = set()
  x11.XQueryKeymap(display, current_buf)
  while True:
    x11.usleep(delay_ms * 1000)
    current_buf, prev_buf = prev_buf, current_buf
    x11.XQueryKeymap(display, current_buf)
    modifiers = []
    shift = False
    for mask_bit in range(8):
      for i in range(mod_map.max_keypermod):
        modkc = mod_map.modifiermap[mask_bit * mod_map.max_keypermod + i]
        if modkc and bit(current_buf, modkc):
          modifiers.append(MODIFIERS[mask_bit])
          if mask_bit == 0:
            shift = True
    for alt_mod in alt_mods:
      if alt_mod.startswith('Alt') and not 'Alt' in modifiers:
        modifiers.append('Alt')
    modifiers.sort()
    modifiers = '.'.join(modifiers)
    if modifiers:
      modifiers += '.'
    for i in range(32 * 8):
      if bit(current_buf, i) and not bit(prev_buf, i):
        # KeyDown event
        key_name = x11.XKeysymToString(x11.XKeycodeToKeysym(display, i, shift))
        if key_name.startswith('Alt'):
          alt_mods.add(key_name)
        key_name = modifiers + key_name
        yield key_name
      elif bit(prev_buf, i) and not bit(current_buf, i):
        # KeyUp event
        key_name = x11.XKeysymToString(x11.XKeycodeToKeysym(display, i, shift))
        if key_name.startswith('Alt'):
          try:
            alt_mods.remove(key_name)
          except:
            pass

def histinc(histogram, keyname, d=1):
  try: histogram[keyname] += d
  except KeyError: histogram[keyname] = d

def histinc2(histogram, key0, key1, d=1):
  try:
    histogram[key0][key1] += d
  except KeyError:
    try:
      histogram[key0][key1] = d
    except KeyError:
      histogram[key0] = dict([(key1, d)])

def keyheatmap(logfilename, flush_keys=100, digraph_timeout_s=2.0):
  if os.path.exists(logfilename):
    o = json.load(file(logfilename))
    histogram = o.get('histogram', {})
    digraphs = o.get('digraphs', {})
  else:
    histogram, digraphs = {}, {}
    json.dump(dict(histogram=histogram, digraphs=digraphs), file(logfilename, 'w'))
  os.chmod(logfilename, 0600)
  prev_key = None
  prev_key_ts = None
  captured = 0
  for key_name in genkeys():
    ts = time.time()
    histinc(histogram, key_name)
    if prev_key and (prev_key_ts > (ts - digraph_timeout_s)):
      histinc2(digraphs, prev_key, key_name)
    prev_key = key_name
    prev_key_ts = ts
    captured += 1
    if captured >= flush_keys:
      json.dump(dict(histogram=histogram, digraphs=digraphs), file(logfilename, 'w'))
      captured = 0

if __name__ == '__main__':
  try:
    if len(sys.argv) >= 2:
      keyheatmap(sys.argv[1])
    else:
      for k in genkeys():
        print k
  except KeyboardInterrupt:
    pass
