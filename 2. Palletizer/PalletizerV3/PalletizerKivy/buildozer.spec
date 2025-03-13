[app]
title = Palletizer Control
package.name = palletizer
package.domain = org.yourorg
source.dir = .
source.include_exts = py,png,jpg,kv,atlas
source.include_patterns = palletizer/**/*
version = 0.1
requirements = python3,kivy,pyserial,plyer

# Android specific
android.permissions = INTERNET,READ_EXTERNAL_STORAGE,WRITE_EXTERNAL_STORAGE
android.api = 28
android.minapi = 21
android.ndk = 21d
android.arch = armeabi-v7a
android.accept_sdk_license = True

# Untuk komunikasi serial di Android
android.permissions = INTERNET,USB_PERMISSION,READ_EXTERNAL_STORAGE,WRITE_EXTERNAL_STORAGE
android.features = android.hardware.usb.host

# Kompilasi
android.release_artifact = apk