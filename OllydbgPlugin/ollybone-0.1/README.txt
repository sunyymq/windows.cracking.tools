OllyBonE v0.1
Break-on-Execute for OllyDbg
(c)2006 Joe Stewart <joe@joestewart.org>

You will need the OllyDbg PDK and the Windows DDK to compile from source.

Installation:
Copy ollybone.dll and i386/ollybone.sys to your OllyDbg directory.

Known bugs/limitations:
Only one instance of OllyDbg may work with the driver at a time

If you have DEP enabled on XP SP2 you may get a BSOD when the target reaches 
the BoE page - for now the only workaround is to disable DEP under 

Control Panel->System->Advanced->Performance->Settings->Data Execution Prevention. 

If that doesn't work, you may need to add

/noexecute=AlwaysOff

to your Windows partition in boot.ini. Special thanks to Tyler Hudak 
and Danko Krajisnik for helping to debug the issues with DEP.
