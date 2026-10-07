import gi, time
from gi.repository import Gio, GLib
bus=Gio.bus_get_sync(Gio.BusType.SESSION,None)
def call(path,interface,method,args=None):
 return bus.call_sync('org.fcitx.Fcitx5',path,interface,method,args,None,Gio.DBusCallFlags.NONE,20000,None).unpack()
path,_=call('/org/freedesktop/portal/inputmethod','org.fcitx.Fcitx.InputMethod1','CreateInputContext',GLib.Variant('(a(ss))',([('program','meltype-self-test'),('display','')],)))
interface='org.fcitx.Fcitx.InputContext1'

messages=[]
def on_signal(connection,sender,obj,iface,name,params,data): messages.append((name,params.unpack()))
bus.signal_subscribe('org.fcitx.Fcitx5',interface,None,path,None,Gio.DBusSignalFlags.NONE,on_signal,None)
call(path,interface,'SetCapability',GLib.Variant('(t)',(18,)))
call(path,interface,'FocusIn')
call('/controller','org.fcitx.Fcitx.Controller1','SetCurrentIM',GLib.Variant('(s)',('meltype',)))
def key(sym, states=0):
 result=call(path,interface,'ProcessKeyEvent',GLib.Variant('(uuubu)',(sym,0,states,False,0)))
 while GLib.MainContext.default().pending():GLib.MainContext.default().iteration(False)
 return result
# Shortcut keys are never swallowed, including while text is composing.
for modifier in (4,8,64): assert key(ord('c'),modifier)==(False,)
assert key(ord('k'))==(True,)
assert not [args for name,args in messages if name=='CommitString']
for modifier in (4,8,64): assert key(ord('c'),modifier)==(False,)
key(0xff0d)
assert [args[0] for name,args in messages if name=='CommitString']==['k']
messages.clear()
for char in 'hello':key(ord(char))
key(0xff0d)
for char in 'nihongo':key(ord(char))
key(0x20)
assert key(0xff56)==(True,)
assert key(0xff55)==(True,)
key(0xff0d)
commits=[args[0] for name,args in messages if name=='CommitString']
print('Committed test text:',commits,flush=True)
assert 'hello' in ''.join(commits)
assert any(any(ord(char)>127 for char in text) for text in commits),commits
call(path,interface,'FocusOut');call(path,interface,'DestroyIC')
print('Fcitx adapter English/Japanese commit integration passed',flush=True)
