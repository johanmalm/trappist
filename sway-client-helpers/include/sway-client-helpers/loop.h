/*
 * Copied from https://github.com/swaylock/include/loop.h
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

#ifndef _SWAY_LOOP_H
#define _SWAY_LOOP_H
#include <stdbool.h>

/**
 * This is an event loop system designed for sway clients, not sway itself.
 *
 * The loop consists of file descriptors and timers. Typically the Wayland
 * display's file descriptor will be one of the fds in the loop.
 */

struct loop;
struct loop_timer;

/**
 * Create an event loop.
 */
struct loop *loop_create(void);

/**
 * Destroy the event loop (eg. on program termination).
 */
void loop_destroy(struct loop *loop);

/**
 * Poll the event loop. This will block until one of the fds has data.
 */
void loop_poll(struct loop *loop);

/**
 * Add a file descriptor to the loop.
 */
void loop_add_fd(struct loop *loop, int fd, short mask,
		void (*func)(int fd, short mask, void *data), void *data);

/**
 * Add a timer to the loop.
 *
 * When the timer expires, the timer will be removed from the loop and freed.
 */
struct loop_timer *loop_add_timer(struct loop *loop, int ms,
		void (*callback)(void *data), void *data);

/**
 * Remove a file descriptor from the loop.
 */
bool loop_remove_fd(struct loop *loop, int fd);

/**
 * Remove a timer from the loop.
 */
bool loop_remove_timer(struct loop *loop, struct loop_timer *timer);

#endif
