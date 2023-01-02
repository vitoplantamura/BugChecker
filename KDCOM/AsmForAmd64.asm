.CODE

extrn HalInitSystem:PROC

DllEntryPoint proc ; from VirtualKD
    jmp HalInitSystem
DllEntryPoint endp

END
