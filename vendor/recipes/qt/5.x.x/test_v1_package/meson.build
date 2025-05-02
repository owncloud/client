project('test_package', 'cpp')
qt5 = import('qt5')
qt5_dep = dependency('qt5', modules: ['Core', 'Network', 'Sql', 'Concurrent', 'Xml'])
moc_files = qt5.preprocess(moc_headers : '../test_package/greeter.h', qresources : '../test_package/example.qrc')
executable('test_package', '../test_package/test_package.cpp', moc_files,
           dependencies : qt5_dep)
