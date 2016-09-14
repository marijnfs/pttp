PTTP
========

Code Structure:
===========
- src: contains the c++ files (all files end with .cc)
- hdr: contains the header files
- bin: contains source files that get compiled to binaries

Build:
=====
A typical build works as follows:

```
cd build
ccmake ..
make
sudo make install
```

the `ccmake` step will complain when dependencies are missing, resolve them by installen the appropriate libraries.