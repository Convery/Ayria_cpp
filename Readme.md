# Ayria_cpp
Base modules for the Ayria project.

#### **Ayria**
Contains a console and debug window app.

#### **Injector** 
Used to inject any library to an application.

#### **Localnetworking** 
Server backend handler.

#### **Platformwrapper** 
A wrapper for different platform integrations

#### **Plugintemplate**
A sample base plugin

# How to build
The projects are setup for cmake. It requires cmake v3.1, vcpkg and a c++20 compatible compiller.

x86 dependencies
```
vcpkg.exe install abseil[cxx17]:x86-windows asmjit:x86-windows lz4:x86-windows mhook:x86-windows nlohmann-json:x86-windows openssl:x86-windows ring-span-lite:x86-windows vectorclass:x86-windows xxhash:x86-windows zydis:x86-windows 
vcpkg.exe install abseil[cxx17]:x86-windows-static asmjit:x86-windows-static lz4:x86-windows-static mhook:x86-windows-static nlohmann-json:x86-windows-static openssl:x86-windows-static ring-span-lite:x86-windows-static vectorclass:x86-windows-static xxhash:x86-windows-static zydis:x86-windows-static 
```
x64 dependencies
```
vcpkg.exe install abseil[cxx17]:x64-windows asmjit:x64-windows lz4:x64-windows mhook:x64-windows nlohmann-json:x64-windows openssl:x64-windows ring-span-lite:x64-windows vectorclass:x64-windows xxhash:x64-windows zydis:x64-windows 
vcpkg.exe install abseil[cxx17]:x64-windows-static asmjit:x64-windows-static lz4:x64-windows-static mhook:x64-windows-static nlohmann-json:x64-windows-static openssl:x64-windows-static ring-span-lite:x64-windows-static vectorclass:x64-windows-static xxhash:x64-windows-static zydis:x64-windows-static
```