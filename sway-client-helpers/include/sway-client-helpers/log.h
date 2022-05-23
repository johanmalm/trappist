/*
 * Copied from https://github.com/swaylock/include/log.h
 *
 * Copyright (C) 2016-2019 Drew DeVault
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _SWAYLOCK_LOG_H
#define _SWAYLOCK_LOG_H

#include <stdarg.h>
#include <string.h>
#include <errno.h>

enum log_importance {
	LOG_SILENT = 0,
	LOG_ERROR = 1,
	LOG_INFO = 2,
	LOG_DEBUG = 3,
	LOG_IMPORTANCE_LAST,
};

void log_init(enum log_importance verbosity);

#ifdef __GNUC__
#define _ATTRIB_PRINTF(start, end) __attribute__((format(printf, start, end)))
#else
#define _ATTRIB_PRINTF(start, end)
#endif

void _swaylock_log(enum log_importance verbosity, const char *format, ...)
	_ATTRIB_PRINTF(2, 3);

const char *_swaylock_strip_path(const char *filepath);

#define LOG(verb, fmt, ...) \
	_swaylock_log(verb, "[%s:%d] " fmt, _swaylock_strip_path(__FILE__), \
			__LINE__, ##__VA_ARGS__)

#define LOG_ERRNO(verb, fmt, ...) \
	LOG(verb, fmt ": %s", ##__VA_ARGS__, strerror(errno))

#endif
