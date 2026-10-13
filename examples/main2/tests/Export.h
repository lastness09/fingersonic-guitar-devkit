// SPDX-License-Identifier: Apache-2.0
#pragma once
#if defined(_WIN32)
#define GROOVE_EXPORT extern "C" __declspec(dllexport)
#else
#define GROOVE_EXPORT extern "C" __attribute__((visibility("default")))
#endif
