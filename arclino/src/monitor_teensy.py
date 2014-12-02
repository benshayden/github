#!/usr/bin/python
import os, socket, sys

def open_gateway(name = 'teensy_gateway'):
  for port in [28541,4984,18924,16924,27183,31091]:
    sock = socket.socket()
    try: sock.connect(('localhost', port))
    except: continue
    if sock.recv(len(name)) == name: return sock

def pipe_sock_file(sock, fd):
  buf = ''
  while True:
    c = sock.recv(1)
    buf += c
    if c == '\n':
      fd.write(buf)
      fd.flush()
      buf = ''

def main(basename):
  pidf = basename + '.pid'
  sock = open_gateway()
  try:
    file(pidf, 'w').write(str(os.getpid()))
    pipe_sock_file(sock, file(basename, 'w'))
  finally:
    try:
      os.unlink(pidf)
    except OSError:
      pass

if __name__ == '__main__': main(*sys.argv[1:])
