xhook
=====


Overview
--------

xhook is a PLT hook library for Android native ELF (executables and Libraries).


Feature
-------

* supports armeabi, armeabi-v7a and arm64-v8a
* supports Android 4.0+ (API level 14+)
* supports symbol lookup via DT_HASH and DT_GNU_HASH
* supports reloc lookup via DT_ANDROID_REL and DT_ANDROID_RELA
* hook symbols for specified ELF or all ELF
* do NOT need root permission
* small library, pure C code


Compile
-------

* compile the libraries

        ./build_libs.sh

* install the libraries to android project's libs path

        ./install_libs.sh

* clean the libraries

        ./clean_libs.sh
