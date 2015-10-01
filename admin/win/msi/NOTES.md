WiX Install Framework
=====================

This framework will create a Windows-based MSI package for mass-deployment.

Contributions
-------------

Because of the declarative nature of MSI packages, any new resources in the mirall application proper must be defined explicitly here. 

ChangeLog
---------

WiX package 0.0.1
- MSI is built using `make_installer.bat` batch script.
- Initial MSI is considered 'silent', and will install default options only.
- Currently, the MSI must be fresh-installed. ownCloud <1.5.1 must be uninstalled before installing MSI.
