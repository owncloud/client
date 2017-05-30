===============================================
Uninstalling the Desktop Synchronization Client
===============================================

Uninstalling the synchronization client isn't always a completely straight-forward process as, sometimes, artifacts can be left behind once the official uninstaller processes completes.

This section of the documentation is intended to guide you through ensuring that, once the official uninstaller processes completes, that you are able to ensure that you can both find an remove any leftover artifacts, ensuring that your system is clean.

Please refer to the section for your operating system to find detailed instructions.

Windows (Vista, 7, 8, & 10)
---------------------------

The official uninstaller often leaves entries behind in the Windows Registry, bloating the registry’s size.
To find them, using `the Windows PowerShell`_, use `the reg query command`_ as in the following example:

:: 

  reg query HKCU\SOFTWARE\Microsoft /s /f ownCloud
  
The query should return three results. 
The keys and values should be:

=============================================================================================================== =================================================
Key                                                                                                             Value
=============================================================================================================== =================================================
``HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run``                                             ``ownCloud``
``HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\UFH\SHC``                                         0
``HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Compatibility Assistant\Store`` ``C:\Program Files (x86)\ownCloud\uninstall.exe``
=============================================================================================================== =================================================

If any exist, you then have to use `the reg delete command`_ to delete them. 
For example, say that the following entry was returned from the query (which has been shortened for brevity):

::
  
  HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\UFH\SHC
    0   REG_MULTI_SZ    C:\ProgramData\Microsoft\Windows\Start Menu\Programs\ownCloud.link\...
    
Then you would delete it by running the following command:

:: 

  reg delete HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\UFH\SHC /v 0
  
If you’re not familiar with the command, what it does is to delete the key with the value ``0`` from the key: ``HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\UFH\SHC``. 

When you have deleted the three entries, then all remnants of the synchronization client will have been removed from your machine.

.. Links
   
.. _the reg delete command: https://technet.microsoft.com/en-us/library/cc742145(v=ws.11).aspx
.. _the reg query command: https://technet.microsoft.com/en-us/library/cc742028(v=ws.11).aspx
.. _the Windows PowerShell: https://msdn.microsoft.com/en-us/powershell
