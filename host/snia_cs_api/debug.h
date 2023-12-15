/*******************************************************************************
 * Copyright (C) 2021 Rick Wertenbroek
 * Reconfigurable Embedded Digital Systems (REDS),
 * School of Management and Engineering Vaud (HEIG-VD),
 * University of Applied Sciences and Arts Western Switzerland (HES-SO).
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
 ******************************************************************************/

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdlib.h>
#include <stdio.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#ifdef ERROR
#warning "ERROR should not be defined, undefining..."
#undef ERROR
#endif

#ifdef WARNING
#warning "WARNING should not be defined, undefining..."
#undef WARNING
#endif

#ifdef IMPORTANT
#warning "IMPORTANT should not be defined, undefining..."
#undef IMPORTANT
#endif

#ifdef INFO
#warning "INFO should not be defined, undefining..."
#undef INFO
#endif

#ifdef DEBUG
#warning "DEBUG should not be defined, undefining..."
#undef DEBUG
#endif

#define MSG_PRINT_INTERNAL(level, fmt, ...) MSG_PRINT_##level(fmt, ##__VA_ARGS__)
#define END_FMT_PRINT_INTERNAL(fmt, ...)  ANSI_COLOR_RESET " %s:%d:%s(): " fmt "\n", \
    __FILE__, __LINE__, __func__, ##__VA_ARGS__

#define MSG_PRINT(level, fmt, ...) MSG_PRINT_INTERNAL(level, fmt, ##__VA_ARGS__)

#define TPREFIX "TSP "

#define MSG_PRINT_ERROR(fmt, ...) \
    do { fprintf(stderr, TPREFIX ANSI_COLOR_RED "[Error]" END_FMT_PRINT_INTERNAL(fmt, ##__VA_ARGS__)); } while(0)
#define MSG_PRINT_WARNING(fmt, ...) \
    do { fprintf(stderr, TPREFIX ANSI_COLOR_YELLOW "[Warning]" END_FMT_PRINT_INTERNAL(fmt, ##__VA_ARGS__)); } while(0)
#define MSG_PRINT_IMPORTANT(fmt, ...) \
    do { printf(TPREFIX ANSI_COLOR_MAGENTA "[Important]" END_FMT_PRINT_INTERNAL(fmt, ##__VA_ARGS__)); } while(0)
#define MSG_PRINT_INFO(fmt, ...) \
    do { printf(TPREFIX ANSI_COLOR_CYAN "[Info]" END_FMT_PRINT_INTERNAL(fmt, ##__VA_ARGS__)); } while(0)
#define MSG_PRINT_DEBUG(fmt, ...) \
    do { printf(TPREFIX ANSI_COLOR_GREEN "[Debug]" END_FMT_PRINT_INTERNAL(fmt, ##__VA_ARGS__)); } while(0)

#endif /* __DEBUG_H__ */