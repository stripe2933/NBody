//
// Created by gomkyung2 on 2023/09/04.
//

#pragma once

#ifndef NDEBUG
#define NOEXCEPT_IF_RELEASE
#else
#define NOEXCEPT_IF_RELEASE noexcept
#endif