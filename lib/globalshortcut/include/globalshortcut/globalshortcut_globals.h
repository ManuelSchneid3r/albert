// Copyright (C) 2014-2021 Manuel Schneider

#pragma once

#if defined(_MSC_VER)
    #define EXPORT __declspec(dllexport)
    #define IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    #define EXPORT __attribute__((visibility("default")))
    #define IMPORT
#else
    #define EXPORT
    #define IMPORT
    #pragma warning Unknown dynamic link import/export semantics.
#endif

#ifdef GLOBALSHORTCUT
 #define EXPORT_GLOBALSHORTCUT EXPORT
#else
 #define EXPORT_GLOBALSHORTCUT IMPORT
#endif
