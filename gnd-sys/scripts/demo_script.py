"""
This demo script serves as a template for new user scripts. cFSAT is not
intended to be a full featured ground system so the ScriptRunner context
provides sending commands and receiving/waiting on telemetry values.

The script executes in the context of a ScriptRUnner object which is a
CmdTlmProcess child class.

"""

print ('*** Start Demo ***')

self.send_cfs_cmd('CFE_ES', 'NoopCmd', {})
time.sleep(1)
self.send_cfs_cmd('CFE_EVS', 'NoopCmd', {})
time.sleep(1)
self.send_cfs_cmd('CFE_SB', 'NoopCmd', {})
time.sleep(1)
self.send_cfs_cmd('CFE_TBL', 'NoopCmd', {})
time.sleep(1)
self.send_cfs_cmd('CFE_TIME', 'NoopCmd', {})

for i in range(5):
   self.tlm_val()
   time.sleep(4)
   
print ('*** End Demo ***')


