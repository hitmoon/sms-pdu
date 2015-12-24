//
// Created by hitmoon on 15-12-17.
//

#ifndef SMS_UTF_H
#define SMS_UTF_H

#include <stddef.h>
#include <sys/types.h>

typedef unsigned int UTF32;
/* at least 32 bits */
typedef unsigned short UTF16;
/* at least 16 bits */
typedef unsigned char UTF8;
/* typically 8 bits */
typedef unsigned char Boolean; /* 0 or 1 */

/* Some fundamental constants */
#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_BMP (UTF32)0x0000FFFF
#define UNI_MAX_UTF16 (UTF32)0x0010FFFF
#define UNI_MAX_UTF32 (UTF32)0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32 (UTF32)0x0010FFFF

typedef enum {
    conversionOK, /* conversion successful */
            sourceExhausted, /* partial character in source, but hit end */
            targetExhausted, /* insuff. room in target for conversion */
            sourceIllegal        /* source sequence is illegal/malformed */
} ConversionResult;

typedef enum {
    strictConversion = 0,
    lenientConversion
} ConversionFlags;

/* This is for C++ and does no harm in C */
#ifdef __cplusplus
extern "C" {
#endif

ConversionResult ConvertUTF8toUTF16(
        const UTF8 **sourceStart, const UTF8 *sourceEnd,
        UTF16 **targetStart, UTF16 *targetEnd, ConversionFlags flags);

ConversionResult ConvertUTF16toUTF8(
        const UTF16 **sourceStart, const UTF16 *sourceEnd,
        UTF8 **targetStart, UTF8 *targetEnd, ConversionFlags flags);

ConversionResult ConvertUTF8toUTF32(
        const UTF8 **sourceStart, const UTF8 *sourceEnd,
        UTF32 **targetStart, UTF32 *targetEnd, ConversionFlags flags);

ConversionResult ConvertUTF32toUTF8(
        const UTF32 **sourceStart, const UTF32 *sourceEnd,
        UTF8 **targetStart, UTF8 *targetEnd, ConversionFlags flags);

ConversionResult ConvertUTF16toUTF32(
        const UTF16 **sourceStart, const UTF16 *sourceEnd,
        UTF32 **targetStart, UTF32 *targetEnd, ConversionFlags flags);

ConversionResult ConvertUTF32toUTF16(
        const UTF32 **sourceStart, const UTF32 *sourceEnd,
        UTF16 **targetStart, UTF16 *targetEnd, ConversionFlags flags);

Boolean isLegalUTF8Sequence(const UTF8 *source, const UTF8 *sourceEnd);

#ifdef __cplusplus
}
#endif

// 获得下一个char的起始地址
u_int32_t next_char(unsigned char **string);

const unsigned char *utf32toutf8(wchar_t *source, unsigned char *target, size_t size,  int *len);

unsigned char *utf16toutf8(unsigned short *source, unsigned char *target, size_t size,  int *len);
unsigned short *utf8toutf16(unsigned char *source, unsigned short *target, size_t size,  int *len);

int utf8len(unsigned char *string);
int is_acsii(unsigned char *string);
size_t utf8_get_size(unsigned char *source, size_t num);

#endif //SMS_UTF_H
