# Camera Plugin Build Guide

## Architecture

The application loads camera implementations from `plugins/cameras` at runtime.
The host executable depends only on `CameraPluginInterface` and
`CameraPluginManager`; vendor SDK code stays inside its plugin DLL.

- `HikCameraPlugin.dll`: Hikrobot/Hikvision MVS implementation.
- `VirtualCameraPlugin.dll`: hardware-independent camera used for development and testing.
- `CameraPluginManager`: plugin discovery, interface/version validation, instance ownership and safe unloading.
- `CameraContext`: UI-facing facade that uses the plugin manager for enumeration and creation.

Plugins create and destroy their own `CameraInterface` objects. This avoids
deallocating plugin-owned C++ objects in the host module.

## Requirements

- Visual Studio with the MSVC x64 toolchain.
- Qt 6.11.1 `msvc2022_64` (or a compatible Qt 6 MSVC build).
- OpenCV 4.10.0 built for the same MSVC runtime.
- Hikrobot MVS SDK development files for building `HikCameraPlugin`.
- Hikrobot MVS runtime for loading the plugin and using physical cameras.

The current `.pro` files use these local paths:

- `D:/Study/QT/6.11.1/msvc2022_64`
- `D:/Study/OpenCV/opencv-cuda-4.10.0/install`
- `../../depends/HikCamera`

Update the `.pro` files if dependencies are installed elsewhere.

## Build with qmake and MSVC

Run from a Visual Studio x64 developer command prompt:

```bat
cd /d D:\Desktop\c++Training\program2\chongming-V2.1.0.0
mkdir build-plugin-release
cd build-plugin-release
D:\Study\QT\6.11.1\msvc2022_64\bin\qmake6.exe ..\src\VCSGUI\VCSGUI.pro CONFIG+=release
nmake
```

`VCSGUI.pro` is a `subdirs` project and builds both plugins, the desktop
application and `PluginSmokeTest`.

## Build with Visual Studio

The generated solution is:

`D:\Desktop\c++Training\program2\chongming-V2.1.0.0\vs-plugin\VCSGUI.sln`

Open it in Visual Studio and build `Release|x64`. To regenerate the solution:

```bat
cd /d D:\Desktop\c++Training\program2\chongming-V2.1.0.0\vs-plugin
D:\Study\QT\6.11.1\msvc2022_64\bin\qmake6.exe -tp vc -r ..\src\VCSGUI\VCSGUI.pro
```

## Runtime Layout

```text
bin/
  VCSGUI.exe
  PluginSmokeTest.exe
  Qt6*.dll
  opencv_world4100.dll
  plugins/
    cameras/
      HikCameraPlugin.dll
      VirtualCameraPlugin.dll
```

By default the application scans `plugins/cameras` relative to the executable.
Set `VCS_CAMERA_PLUGIN_PATH` to test another plugin directory.

`MvCameraControl.dll` is supplied by the Hikrobot MVS runtime. Install MVS or
place its runtime directory on `PATH`; otherwise the manager skips the Hikrobot
plugin and reports the loader error while the virtual plugin remains available.

## Smoke Test

```bat
cd /d D:\Desktop\c++Training\program2\chongming-V2.1.0.0\bin
PluginSmokeTest.exe
```

The test verifies plugin discovery, virtual camera enumeration, instance
creation, connect/start/get-image/stop/disconnect lifecycle and safe plugin
unloading without requiring physical camera hardware.
