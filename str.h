#pragma once
/*
 * str.h - function library for nul-terminated strings
 *
 * Copyright © 2021 Jonas Kümmerlin <jonas@kuemmerlin.eu>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// like strlen(3), but str is allowed to be NULL (returns 0 then)
static inline int
str_length(const char *str)
{
    int i = 0;

    while (str && str[i])
        ++i;

    return i;
}

static inline void *
_str_xrealloc(void *p, size_t len)
{
    void *r = realloc(p, len);
    if (!r) {
        perror("realloc(3)");
        abort();
    }
    return r;
}

static inline void
str_realloc(char **ps, int newlen)
{
    assert(ps);
    assert(newlen >= 0);

    *ps = (char*)_str_xrealloc(*ps, (size_t)newlen + 1);
    (*ps)[newlen] = 0; // ensure 0 termination, caller is required to fill in the rest
}

static inline void
str_clear(char **ps)
{
    if (ps && *ps) {
        free(*ps);
        *ps = NULL;
    }
}

static inline void
str_free(char *s)
{
    free(s);
}

static inline void
str_swap(char **pa, char **pb)
{
    char *tmp = *pa;
    *pa = *pb;
    *pb = tmp;
}

static inline void
str_assign_buf(char **target, const char *data, int len)
{
    assert(target);
    assert(len >= 0);
    if (!data) {
        str_clear(target);
    } else {
        assert(*target != data);

        str_realloc(target, len);
        memcpy(*target, data, (size_t)len);
    }
}

static inline void
str_assign(char **target, const char *str)
{
    str_assign_buf(target, str, str_length(str));
}

// like strdup(), but deals with NULL
static inline char *
str_dup(const char *s)
{
    char *r = NULL;
    str_assign(&r, s);
    return r;
}

static inline char *
str_dup_buf(const char *s, int len)
{
    char *r = NULL;
    str_assign_buf(&r, s, len);
    return r;
}

static inline void
str_append_buf(char **target, int target_len, const char *data, int len)
{
    assert(target);

    if (data && len > 0) {
        str_realloc(target, target_len + len);
        memcpy(&(*target)[target_len], data, (size_t)len);
    }
}

static inline void
str_append(char **target, const char *str)
{
    assert(target);

    str_append_buf(target, str_length(*target), str, str_length(str));
}

// NOTE: a or b may be NULL if a_len/b_len is 0
static inline int
str_cmp_buf(const char *a, int a_len, const char *b, int b_len)
{
    int i = 0;
    while (i < a_len && i < b_len && a[i] == b[i])
        ++i;

    if (i < a_len && i < b_len && a[i] != b[i]) {
        return (unsigned char)a[i] - (unsigned char)b[i];
    } else if (a_len < b_len) {
        return -1;
    } else if (a_len > b_len) {
        return 1;
    } else {
        return 0;
    }
}

// like strcmp(3), but treats NULL as empty string
static inline int
str_cmp(const char *a, const char *b)
{
    a = a ? a : "";
    b = b ? b : "";

    size_t i = 0;
    while (a[i] && (a[i] == b[i]))
        ++i;

    return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
}

// NOTE a or b may be NULL if a_len/b_len == 0
static inline bool
str_equal_buf(const char *a, int a_len, const char *b, int b_len)
{
    if (a_len != b_len)
        return false;

    for (int i = 0; i < a_len; ++i)
        if (a[i] != b[i])
            return false;

    return true;
}

// like !strcmp(a, b), but NULL is treated like an empty string
static inline bool
str_equal(const char *a, const char *b)
{
    a = a ? a : "";
    b = b ? b : "";

    while (*a && *b) {
        if (*a++ != *b++)
            return false;
    }

    return *a == *b;
}

static inline void
str_assign_vprintf(char **ps, const char *format, va_list ap)
{
    assert(ps);
    assert(format);

    va_list ap2;
    va_copy(ap2, ap);
    int len = vsnprintf(NULL, 0, format, ap2);
    va_end(ap2);

    str_realloc(ps, len);
    if (len > 0) {
        int len2 = vsnprintf(*ps, (size_t)len+1, format, ap);
        assert(len2 == len);
    }
}

static inline void
str_append_vprintf(char **ps, const char *format, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);
    int len = vsnprintf(NULL, 0, format, ap2);
    va_end(ap2);

    int oldlen = str_length(*ps);

    str_realloc(ps, oldlen + len);
    if (len > 0) {
        int len2 = vsnprintf(*ps + oldlen, (size_t)len+1, format, ap);
        assert(len2 == len);
    }
}

static inline char *
str_vprintf(const char *format, va_list ap)
{
    char *out = NULL;
    str_assign_vprintf(&out, format, ap);
    return out;
}

static inline void
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
str_assign_printf(char **ps, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    str_assign_vprintf(ps, format, ap);
    va_end(ap);
}

static inline void
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
str_append_printf(char **ps, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    str_append_vprintf(ps, format, ap);
    va_end(ap);
}

static inline char *
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
str_printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    char *r = str_vprintf(format, ap);
    va_end(ap);
    return r;
}

// substr like python slicing, supports negative indexes for counting from the end
// s is allowed to be NULL
static inline void
str_assign_substr(char **target, const char *s, int start, int end)
{
    int slen = str_length(s);
    s = s ? s : "";

    if (start < 0)
        start = slen + start;

    if (start < 0)
        start = 0;

    if (start > slen)
        start = slen;

    if (end < 0)
        end = slen + end;

    if (end < 0)
        end = 0;

    if (end > slen)
        end = slen;

    if (end < start)
        end = start;

    str_assign_buf(target, s + start, end - start);
}

static inline char *
str_substr(const char *s, int start, int end)
{
    char *r = NULL;
    str_assign_substr(&r, s, start, end);
    return r;
}

// like the other substr functions, but memmove(3)s the data inside the buffer
static inline void
str_substr_inplace(char *target, int start, int end)
{
    assert(target);
    int slen = str_length(target);

    if (start < 0)
        start = slen + start;

    if (start < 0)
        start = 0;

    if (start > slen)
        start = slen;

    if (end < 0)
        end = slen + end;

    if (end < 0)
        end = 0;

    if (end > slen)
        end = slen;

    if (end < start)
        end = start;

    memmove(target, target + start, (size_t)(end - start));
    target[end-start] = 0;
}

static inline bool
str_starts_with_buf(const char *haystack, int haystacklen, const char *prefix, int prefixlen)
{
    if (prefixlen == 0)
        return true;

    if (prefixlen > haystacklen)
        return false;

    return !memcmp(haystack, prefix, (size_t)prefixlen);
}

static inline bool
str_starts_with(const char *s, const char *prefix)
{
    return str_starts_with_buf(s ? s : "", str_length(s), prefix ? prefix : "", str_length(prefix));
}

static inline bool
str_ends_with_buf(const char *haystack, int haystacklen, const char *suffix, int suffixlen)
{
    if (suffix == 0)
        return true;

    if (suffixlen > haystacklen)
        return false;

    return !memcmp(haystack + (haystacklen - suffixlen), suffix, (size_t)suffixlen);
}

static inline bool
str_ends_with(const char *s, const char *suffix)
{
    return str_ends_with_buf(s ? s : "", str_length(s), suffix ? suffix : "", str_length(suffix));
}

static inline int
str_index_of_buf(const char *haystack, int haystack_len, const char *needle, int needle_len)
{
    // this is the boyer-moore-sunday algorithm
    int last[256];
    for (unsigned c = 0; c <= 255; ++c) {
        last[c] = -1;
    }
    for (int i = 0; i < needle_len; ++i) {
        last[(unsigned char)needle[i]] = i;
    }

    int i = 0;
    while (i + needle_len <= haystack_len) {
        int j;
        for (j = 0; j < needle_len; ++j) {
            if (haystack[i+j] != needle[j])
                break;
        }
        if (j == needle_len)
            return i;

        i += needle_len;
        i -= last[(unsigned char)haystack[i]];
    }

    return -1;
}

static inline int
str_index_of(const char *haystack, const char *needle)
{
    return str_index_of_buf(haystack ? haystack : "", str_length(haystack),
                            needle ? needle : "", str_length(needle));
}

static inline int
str_last_index_of_buf(const char *haystack, int haystack_len, const char *needle, int needle_len)
{
    // this is the boyer-moore-sunday algorithm, in reverse
    int first[256];
    for (unsigned c = 0; c <= 255; ++c) {
        first[c] = -1;
    }
    for (int i = needle_len-1; i >= 0; --i) {
        first[(unsigned char)needle[i]] = needle_len - i - 1;
    }

    int i = haystack_len-1;
    while (i >= needle_len-1) {
        int j;
        for (j = 0; j < needle_len; ++j) {
            if (haystack[i-j] != needle[needle_len-1-j])
                break;
        }
        if (j == needle_len)
            return i - needle_len + 1;

        i -= needle_len;
        i += first[(unsigned char)haystack[i]];
    }

    return -1;
}

static inline int
str_last_index_of(const char *haystack, const char *needle)
{
    return str_last_index_of_buf(haystack ? haystack : "", str_length(haystack),
                                 needle ? needle : "", str_length(needle));
}

static inline void
str_assign_reversed(char **target, const char *s)
{
    assert(target);
    assert(*target != s);

    int l = str_length(s);

    str_realloc(target, l);
    for (int i = 0; i < l; ++i) {
        (*target)[i] = s[l-i-1];
    }
}

static inline char *
str_reversed(const char *s)
{
    char *r = NULL;
    str_assign_reversed(&r, s);
    return r;
}

static inline void
str_reverse_inplace(char *s)
{
    int l = str_length(s);
    for (int i = 0; i < l/2; ++i) {
        char t = s[i];
        s[i] = s[l-i-1];
        s[l-i-1] = t;
    }
}

static inline void
str_assign_replaced(char **target, const char *haystack, const char *needle, const char *replacement)
{
    haystack = haystack ? haystack : "";
    needle = needle ? needle : "";
    replacement = replacement ? replacement : "";

    int haystack_len = str_length(haystack);
    int needle_len = str_length(needle);
    int replacement_len = str_length(replacement);

    assert(target);
    assert(*target != haystack);

    if (needle_len < 1) {
        // special: insert replacement between all chars
        str_realloc(target, replacement_len * (haystack_len + 1) + haystack_len);
        memcpy(*target, replacement, (size_t)replacement_len);
        for (int i = 0; i < haystack_len; ++i) {
            (*target)[replacement_len + i * (replacement_len + 1)] = haystack[i];
            memcpy(*target + (replacement_len + 1) * (i + 1), replacement, (size_t)replacement_len);
        }
    } else {
        // TODO: improve memory allocation strategy

        int iout = 0;
        int iin = 0;

        int imatch = -1;
        while ((imatch = str_index_of_buf(haystack + iin, haystack_len - iin, needle, needle_len)) >= 0) {
            str_realloc(target, iout + imatch + replacement_len);
            memcpy(*target + iout, haystack + iin, (size_t)imatch);
            memcpy(*target + iout + imatch, replacement, (size_t)replacement_len);
            iin += imatch + needle_len;
            iout += imatch + replacement_len;
        }

        str_realloc(target, iout + haystack_len - iin);
        memcpy(*target + iout, haystack + iin, (size_t)(haystack_len - iin));
    }
}

static inline char *
str_replaced(const char *haystack, const char *needle, const char *replacement)
{
    char *r = NULL;
    str_assign_replaced(&r, haystack, needle, replacement);
    return r;
}

static inline void
str_assign_left_padded(char **out, const char *s, int width, char pad)
{
    assert(out);
    assert(*out != s);

    int l = str_length(s);
    if (l >= width) {
        str_assign(out, s);
    } else {
        str_realloc(out, width);
        memset(*out, pad, (size_t)(width - l));
        memcpy(*out + (width - l), s, (size_t)l);
    }
}

static inline char *
str_left_padded(const char *s, int width, char pad)
{
    char *r = NULL;
    str_assign_left_padded(&r, s, width, pad);
    return r;
}

static inline void
str_left_pad_inplace(char *buf, int width, char pad)
{
    int l = str_length(buf);
    if (l > width)
        return;

    memmove(buf + (width - l), buf, (size_t)l + 1);
    memset(buf, pad, (size_t)(width - l));
}

static inline void
str_assign_right_padded(char **out, const char *s, int width, char pad)
{
    assert(out);
    assert(*out != s);

    int l = str_length(s);
    if (l >= width) {
        str_assign(out, s);
    } else {
        str_realloc(out, width);
        memcpy(*out, s, (size_t)l);
        memset(*out + l, pad, (size_t)(width - l));
    }
}

static inline char *
str_right_padded(const char *s, int width, char pad)
{
    char *r = NULL;
    str_assign_right_padded(&r, s, width, pad);
    return r;
}

static inline void
str_right_pad_inplace(char *buf, int width, char pad)
{
    int l = str_length(buf);
    if (l > width)
        return;

    memset(buf + l, pad, (size_t)(width - l));
    buf[width] = 0;
}

static inline bool
_str_is_ascii_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

// trims ascii whitespace (" \t\r\n\v\f") front and back
static inline void
str_assign_trimmed(char **target, const char *s)
{
    s = s ? s : "";

    assert(target);
    assert(*target != s);

    int len = str_length(s);
    int left = 0;
    int right = 0;

    while (left < len && _str_is_ascii_space(s[left]))
        left++;

    while (right + left < len && _str_is_ascii_space(s[len - right - 1]))
        right++;

    str_assign_substr(target, s, left, len - right);
}

static inline char *
str_trimmed(const char *s)
{
    char *r = NULL;
    str_assign_trimmed(&r, s);
    return r;
}

static inline void
str_trim_inplace(char *buf)
{
    assert(buf);

    int len = str_length(buf);
    int left = 0;
    int right = 0;

    while (left < len && _str_is_ascii_space(buf[left]))
        left++;

    while (right + left < len && _str_is_ascii_space(buf[len - right - 1]))
        right++;

    if (left > 0) {
        memmove(buf, buf + left, (size_t)(len - left - right));
    }
    buf[len - left - right] = '\0';
}

// ascii upper/lower functions
static inline char
_str_ascii_upper(char c)
{
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    else
        return c;
}

static inline void
str_assign_uppercased(char **target, const char *s)
{
    assert(target);
    assert(!s || *target != s);

    int len = str_length(s);

    str_realloc(target, len);
    for (int i = 0; i < len; ++i)
        (*target)[i] = _str_ascii_upper(s[i]);
}

static inline char *
str_uppercased(const char *s)
{
    char *r = NULL;
    str_assign_uppercased(&r, s);
    return r;
}

static inline void
str_uppercase_inplace(char *buf)
{
    assert(buf);

    while (*buf) {
        *buf = _str_ascii_upper(*buf);
        ++buf;
    }
}

static inline char
_str_ascii_lower(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    else
        return c;
}

static inline void
str_assign_lowercased(char **target, const char *s)
{
    assert(target);
    assert(!s || *target != s);

    int len = str_length(s);

    str_realloc(target, len);
    for (int i = 0; i < len; ++i)
        (*target)[i] = _str_ascii_lower(s[i]);
}

static inline char *
str_lowercased(const char *s)
{
    char *r = NULL;
    str_assign_lowercased(&r, s);
    return r;
}

static inline void
str_lowercase_inplace(char *buf)
{
    assert(buf);

    while (*buf) {
        *buf = _str_ascii_lower(*buf);
        ++buf;
    }
}

// "natural" comparison similar to windows explorer and php strnatcasecmp()
static inline int
str_natcmp(const char *a, const char *b)
{
    a = a ? a : "";
    b = b ? b : "";

    // skip spaces at the beginning
    while (_str_is_ascii_space(*a))
        a++;
    while (_str_is_ascii_space(*b))
        b++;

    for (;;) {
        // skip consecutive spaces, and spaces at the end of the string
        char ca = *a;
        while (_str_is_ascii_space(ca) && (_str_is_ascii_space(a[1]) || !a[1]))
            ca = *(++a);

        char cb = *b;
        while (_str_is_ascii_space(cb) && (_str_is_ascii_space(b[1]) || !b[1]))
            cb = *(++b);

        // bail on end of string
        if (ca == 0 || cb == 0)
            return (int)(unsigned char)ca - (int)(unsigned char)cb;

        if (ca >= '0' && ca <= '9' && cb >= '0' && cb <= '9') {
            // compare digit runs

            // skip leading zeroes
            while (ca == '0' && a[1] >= '0' && a[1] <= '9')
                ca = *(++a);
            while (cb == '0' && b[1] >= '0' && b[1] <= '9')
                cb = *(++b);

            int r = ca - cb;
            for (;;) {
                ca = *(++a);
                cb = *(++b);

                if (ca >= '0' && ca <= '9' && (cb < '0' || cb > '9')) {
                    // a longer = greater digit run than b
                    return 1;
                } else if (cb >= '0' && cb <= '9' && (ca < '0' || ca > '9')) {
                    // b longer = greater digit run than a
                    return -1;
                } else if (ca >= '0' && ca <= '9' && cb >= '0' && cb <= '9') {
                    // digit run the same length so far and continuing
                    if (r == 0)
                        r = ca - cb; // if exactly the same digits so far, note difference
                } else {
                    // digit run the same length and over
                    if (r != 0) // different digits
                        return r;

                    // identical digit run -> quit processing digits
                    break;
                }
            }
        } else {
            // compare letters and other stuff

            // case insensitivity for ascii letters
            ca = _str_ascii_upper(ca);
            cb = _str_ascii_upper(cb);

            // all types of spaces are equal
            if (_str_is_ascii_space(ca))
                ca = ' ';
            if (_str_is_ascii_space(cb))
                cb = ' ';

            if (ca != cb)
                return (int)(unsigned char)ca - (int)(unsigned char)cb;

            a++;
            b++;
        }
    }
}

// debugging tool: string in c source
static inline void
str_assign_cliteral(char **out, const char *s)
{
    assert(out);

    if (!s) {
        str_assign(out, "NULL");
        return;
    }

    int l = str_length(s);
    str_realloc(out, l * 4 + 2); // worst case estimate if we need octal escape for everything

    int i = 1;
    (*out)[0] = '"';
    while (*s) {
        switch (*s) {
            case '"':
                (*out)[i++] = '\\';
                (*out)[i++] = '"';
                break;
            case '\\':
                (*out)[i++] = '\\';
                (*out)[i++] = '\\';
                break;
            case '\a':
                (*out)[i++] = '\\';
                (*out)[i++] = 'a';
                break;
            case '\b':
                (*out)[i++] = '\\';
                (*out)[i++] = 'b';
                break;
            case '\f':
                (*out)[i++] = '\\';
                (*out)[i++] = 'f';
                break;
            case '\n':
                (*out)[i++] = '\\';
                (*out)[i++] = 'n';
                break;
            case '\r':
                (*out)[i++] = '\\';
                (*out)[i++] = 'r';
                break;
            case '\t':
                (*out)[i++] = '\\';
                (*out)[i++] = 't';
                break;
            case '\v':
                (*out)[i++] = '\\';
                (*out)[i++] = 'v';
                break;
            default:
                if (((unsigned char)*s) < 32 || ((unsigned char)*s) > 126) {
                    if (s[1] >= '0' && s[1] <= '7') { // digit following -> need full 3 digit octal form
                        i += sprintf(&(*out)[i], "\\%03o", (int)(unsigned char)*s);
                    } else { // short octal form suffices
                        i += sprintf(&(*out)[i], "\\%o", (int)(unsigned char)*s);
                    }
                } else {
                    (*out)[i++] = *s;
                }
        }

        s++;
    }
    (*out)[i++] = '"';

    str_realloc(out, i);
}

static inline char *
str_cliteral(const char *s)
{
    char *r = NULL;
    str_assign_cliteral(&r, s);
    return r;
}
