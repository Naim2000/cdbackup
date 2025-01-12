#include <iostream>
#include <string>
#include <iterator>
#include <fstream>
#include <sys/stat.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <vector>
#include "pngu-mod.h"
#include "odh.h"

extern "C"
{
#include <unistd.h>
}

// typedef unsigned char u8;
// typedef unsigned short u16;
// typedef unsigned long u32;
// typedef unsigned long long u64;

// typedef char s8;
// typedef short s16;
// typedef long s32;
// typedef long long s64;

#define RGB565 0
#define RGBA8 1
#define Y8U8V8 2

#define DArJCOL_RES_SUCCESS 0
#define DArJCOL_RES_ERROR 1

#define DArJCOL_COLOR_NUM 3
#define DArJCOL_COLOR_R 0
#define DArJCOL_COLOR_G 1
#define DArJCOL_COLOR_B 2
#define DArJCOL_COLOR_Y 0
#define DArJCOL_COLOR_Cb 1
#define DArJCOL_COLOR_Cr 2

#define DArCONST_BITS 8
#define DArRANGE_MASK 0x3FF
#define DArCONST_BITS 8

#define DAr_FIX_0_382683433 ((s32)98)  /* FIX(0.382683433) */
#define DAr_FIX_0_541196100 ((s32)139) /* FIX(0.541196100) */
#define DAr_FIX_0_707106781 ((s32)181) /* FIX(0.707106781) */
#define DAr_FIX_1_306562965 ((s32)334) /* FIX(1.306562965) */
#define DArFIX_1_082392200 ((s32)277)  /* FIX(1.082392200) */
#define DArFIX_1_414213562 ((s32)362)  /* FIX(1.414213562) */
#define DArFIX_1_847759065 ((s32)473)  /* FIX(1.847759065) */
#define DArFIX_2_613125930 ((s32)669)  /* FIX(2.613125930) */

/* ?p???b?g?e?[?u??????K?v??? */
#define DArSCALEBITS 16 // speediest right-shift on some machines

#define DArCONV_TBL_IDX_Y_R 0
#define DArCONV_TBL_IDX_Y_G 32
#define DArCONV_TBL_IDX_Y_B 64
#define DArCONV_TBL_IDX_Cb_R (DArJCOL_COLOR_Cb * DArJCOL_COLOR_NUM * 32)
#define DArCONV_TBL_IDX_Cb_G (DArJCOL_COLOR_Cb * DArJCOL_COLOR_NUM * 32 + 32)
#define DArCONV_TBL_IDX_Cb_B (DArJCOL_COLOR_Cb * DArJCOL_COLOR_NUM * 32 + 64)
#define DArCONV_TBL_IDX_Cr_R (DArJCOL_COLOR_Cr * DArJCOL_COLOR_NUM * 32)
#define DArCONV_TBL_IDX_Cr_G (DArJCOL_COLOR_Cr * DArJCOL_COLOR_NUM * 32 + 32)
#define DArCONV_TBL_IDX_Cr_B (DArJCOL_COLOR_Cr * DArJCOL_COLOR_NUM * 32 + 64)

/* ?????????->2???? */
#define DArCDJ_IMAGE_DIMENSION 2
/* ?F????[??->3 */
#define DArCDJ_COLOR_DIMENSION 3
/* ??q???e?[?u???? */
#define DArCDJ_QUANTIZE_TABLE_NUM 2
/* DCT?u???b?N?????(1????) */
#define DArCDJ_DCT_SIZE_1D 8
/* DCT?u???b?N?????(2????) */
#define DArCDJ_DCT_SIZE_2D (DArCDJ_DCT_SIZE_1D * DArCDJ_DCT_SIZE_1D)

/* ?z???X/Y??????\???v?f???C???f?b?N?X */
#define DArCDJ_AXIS_X 0
#define DArCDJ_AXIS_Y 1

/* ?z???YCbCr??\???v?f???C???f?b?N?X */
#define DArCDJ_COLOR_Y 0
#define DArCDJ_COLOR_Cb 1
#define DArCDJ_COLOR_Cr 2

/* DCT?o?b?t?@?????????(4??align??????) */
#define DArCDJ_DCT_BUFFER_SIZE 512

/* ???k??\???????T?C?Y */
#define DArCDJ_PIXEL_SIZE_MAX_X 0x07FF
#define DArCDJ_PIXEL_SIZE_MAX_Y 0x07FF

// GBA?pODH???w?b?_??T?C?Y
#define DArCDJ_HEADER_SIZE 16

#define DArNUM_TEXEL_X_1TILE 4 // ?e?N?X?`???f?[?^??1?^?C??????X?????s?N?Z????
#define DArNUM_TEXEL_Y_1TILE 4 // ?e?N?X?`???f?[?^??1?^?C??????Y?????s?N?Z????
#define DArNUM_TEXEL_1TILE (DArNUM_TEXEL_X_1TILE * DArNUM_TEXEL_Y_1TILE)

// SArCDJ_OdhMaster?\?????|?C???^??^??????A?????o?b?t?@?????
// ODH?f?[?^??w?b?_????A?????????k?f?[?^?T?C?Y????o???????}?N??
#define DArGetOdhFileSize(x) (*(u32 *)(&((x)->codeBufferPtr[-8])))

#define DArCDJRESULT_ERR_BIT 0x80000000

#define DArCDJRESULT_SUCCESS 0x00000000
// ?w????????????????
#define DArCDJRESULT_ERR_SIZE (0x00000001 | DArCDJRESULT_ERR_BIT)
// ?w????????i???????
#define DArCDJRESULT_ERR_QUALITY (0x00000002 | DArCDJRESULT_ERR_BIT)
// ?^??????n?t?}???????????
#define DArCDJRESULT_ERR_HUFFCODE (0x00000003 | DArCDJRESULT_ERR_BIT)
// ???k???f?[?^????w???f?[?^????z??????????
#define DArCDJRESULT_ERR_CODE_SIZE (0x00000004 | DArCDJRESULT_ERR_BIT)
// ?^??????f?[?^???????????????i????R?[?h?????j
#define DArCDJRESULT_ERR_INVALID_DATA (0x00000005 | DArCDJRESULT_ERR_BIT)

/* Y??????CbCr??????????T???v?????O??1or2(?????E????????) */
#define DArCDJ_C_Y_RATE_H 1
#define DArCDJ_C_Y_RATE_V 1

/* MCU?????(??????????s?N?Z???P??) */
#define DArCDJ_C_MCU_SIZE_X (DArCDJ_C_Y_RATE_H * DArCDJ_DCT_SIZE_1D)
#define DArCDJ_C_MCU_SIZE_Y (DArCDJ_C_Y_RATE_V * DArCDJ_DCT_SIZE_1D)

/* DCTBlock??2block???k????????????f?[?^?? */
#define DArCDJ_C_MAX_OUT_2BLK 812

#define DArODH_HEADER_FILESIZE_OFFSET 8

/* ?}?N?? */
#define DArDESCALE(x, n) ((x) >> (n))
#define DArMULTIPLY(var, const) (DArDESCALE((var) * (const), DArCONST_BITS))

/* Huffman?????p?\???? */
typedef struct
{
    u32 *mPreviousDC[2];       /* ?O???DC?W?????|?C???^ */
    const u32 *mDCTable;       /* DCHuffmaTable */
    const u32 *mACTable;       /* ACHuffmaTable */
    u8 *mCodeBuffer;           /* CodeBuffer???|?C???^ */
    u32 *mCodeBufferRemain;    /* CodeBuffer??c????L???????????|?C???^ */
    u32 *mHuffmanBuffer;       /* HuffmanBuffer???L???????????|?C???^ */
    u32 *mHuffmanBufferRemain; /* HuffmanBuffer??t???[?r?b?g???L???????????|?C???^ */
    u32 mCodeBufferSize;       /* CodeBuffer??T?C?Y */
} SArCDJ_HuffmanRequest;

/* ODH???C?u????????\???? */
typedef struct
{
    // ?C???[?W?????
    u16 mImageSize[DArCDJ_IMAGE_DIMENSION];
    // ?C???[?W??N?I???e?B
    u8 mQuality;
    // ?C???[?W????????MCU???(X,Y)
    u16 mMCUinImage[DArCDJ_IMAGE_DIMENSION];
    // ???????????MCU???u(X,Y)
    u16 mCurrentMCU[DArCDJ_IMAGE_DIMENSION];

    // Cb?ECr??????_?E???T???v?????O???
    // ?L???v???O????????Q?????????o
    // ?i???k?v???O???????????Q???????j
    u8 mSmpRate[DArCDJ_IMAGE_DIMENSION];

    // huffman?p??o?b?t?@
    u32 mHuffmanBuffer;
    // ?????????FHuffman?o?b?t?@????t???[?r?b?g??(??????r?b?g?????????H?j
    // ???????F?????o?b?t?@???????u??A8?r?b?g?P??????[?r?b?g??
    u32 mHuffmanBits;
    // ?????o?b?t?@??T?C?Y
    u32 mCodeBufferSize;
    // ?????o?b?t?@??c??T?C?Y
    u32 mRemainCodeBuffer;

    // DC????????O??l
    u32 mPreviousDC[DArCDJ_COLOR_DIMENSION];

    // ??????o?b?t?@???|?C???^
    u8 *mImgYCbCrBufferPtr;
    // DCT?o?b?t?@
    u32 mDCTBuffer[DArCDJ_DCT_BUFFER_SIZE / sizeof(u32)];
    // ?????o?b?t?@???|?C???^
    u8 *mCodeBufferPtr;
    // ??q???e?[?u??
    u32 mQuantizationTable[DArCDJ_QUANTIZE_TABLE_NUM][DArCDJ_DCT_SIZE_2D];

    SArCDJ_HuffmanRequest mHuffmanRequest[2];
} SArCDJ_OdhMaster;

typedef struct
{
    s16 R_Cr[256];
    s32 G_Cb[256], G_Cr[256];
    s16 B_Cb[256];
} SArDeconvTbl;

class CArGBAOdh
{
public:
    // ?R???X?g???N?^
    CArGBAOdh(){};
    // ?f?X?g???N?^
    ~CArGBAOdh(){};

    // ??????o?b?t?@?i240?~160?????112.5KByte?j???w??????????
    // ????*work??n???????
    int decompressGbaOdh(u8 *, u8 *, u8 *, int);

    // sizeLimit??K??4??{?????????????
    // ?w??b??????????f?[?^?T?C?Y????
    // ??????o?b?t?@?i240?~160?????112.5KByte?j???w??????????
    // ????*work??n???????
    int compressGbaOdh(u8 *, u8 *, int, int, int, u32, u8 *, int);

    int Mel_round(const float f) const
    {
        if (f < 0.0F)
            return (0);
        else if (f > 255.0F)
            return (255);
        else
            return ((int)(f + 0.5F));
    }
    // ODH?f?[?^???|?C???^??????ODH?f?[?^??T?C?Y????
    // ODH???C?u???????G?????????????????p
    u32 getOdhFileSize(const void *src) const
    {
        return ((u32 *)src)[2];
    }
    // ODH?f?[?^???|?C???^??????ODH??????E????????
    int getOdhWidth(const void *src) const
    {
        return (int)(((u32 *)src)[1] & 0x7ff);
    }
    int getOdhHeight(const void *src) const
    {
        return (int)((((u32 *)src)[1] >> 11) & 0x7ff);
    }
    u8 getOdhSamplingRateH(const void *src) const
    {
        return (u8)(((((u32 *)src)[1] >> 22) & 1) + 1);
    }
    u8 getOdhSamplingRateV(const void *src) const
    {
        return (u8)(((((u32 *)src)[1] >> 23) & 1) + 1);
    }
    u8 getOdhQuality(const void *src) const
    {
        return (u8)((((u32 *)src)[1] >> 24) & 0xff);
    }

private:
    // Cdj_c_Lib.c Function
    u32 cdj_c_initializeCompressOdh(
        SArCDJ_OdhMaster *cdj_ctrl, u16 *imageSize, u8 quality,
        u8 *imgYCbCrBufPtr, u8 *CodeBufPtr, u32 CodeBufSize);
    u32 cdj_c_compressLoop(SArCDJ_OdhMaster *cdj_ctrl);
    u32 cdj_c_flashBuffer(SArCDJ_OdhMaster *cdj_ctrl);
    void cdj_c_setQuantizationTable(SArCDJ_OdhMaster *cdj_ctrl, u32 quality);
    void cdj_c_makeHeader(SArCDJ_OdhMaster *cdj_ctrl, u32 fileSize);

    // Cdj_c_Color.c Function
    u32 cdj_c_colorConv(SArCDJ_OdhMaster *, u8 *, int);
#if (DArCDJ_C_Y_RATE_H == 1) && (DArCDJ_C_Y_RATE_V == 1)
    void LineConv11(u8 *, u8 *, u8 *, u8 *, u16, u16, const s32 *, int);
#elif (DArCDJ_C_Y_RATE_H == 2) && (DArCDJ_C_Y_RATE_V == 1)
    void LineConv21(u8 *, u8 *, u8 *, u8 *, u16, const s32 *, int);
#elif (DArCDJ_C_Y_RATE_H == 1) && (DArCDJ_C_Y_RATE_V == 2)
    void LineConv12(u8 *, u8 *, u8 *, u8 *, u16, const s32 *, int);
#elif (DArCDJ_C_Y_RATE_H == 2) && (DArCDJ_C_Y_RATE_V == 2)
    void LineConv22(u8 *, u8 *, u8 *, u8 *, u16, const s32 *, int);
#endif

    // Cdj_c_Dct.c Function
    void fdct_fast(u32 *dctBlock, u8 *pixelBuffer, u32 pixelOffset, u32 *quantTable);

    // Cdj_c_Huff.c Function
    inline u32 EmitBit(s32 code, s32 len, SArCDJ_HuffmanRequest *huffRequest);
    u32 huffmanCoder(u16 *DCTBuffer, SArCDJ_HuffmanRequest *huffRequest);

    // Cdj_c_Huff.c Data
    u8 *pCodeBuf;

    // Cdj_d_Lib.c Function
    u32 cdj_d_initializeDecompressOdh(
        SArCDJ_OdhMaster *cdj_ctrl, u8 *imgYCbCrBufPtr, u8 *CodeBufPtr);
    u32 cdj_d_decompressLoop(SArCDJ_OdhMaster *cdj_ctrl);
    void cdj_d_setDequantizationTable(SArCDJ_OdhMaster *cdj_ctrl, u32 quality);

    // Cdj_d_Color.c Function
    s32 ScaleLimit(s32 x)
    {
        // ?l??0?`255????????C?????C?????
        if (x < 0)
            x = 0;
        else if (x > 255)
            x = 255;
        return x;
    }
    u32 cdj_d_colorDeconv(SArCDJ_OdhMaster *, u8 *, int);
    void LineDeconv11(u8 *, u8 *, u8 *, u8 *, u16, u16, const SArDeconvTbl *, int);
    void LineDeconv21(u8 *, u8 *, u8 *, u8 *, u16, u16, const SArDeconvTbl *, int);
    void LineDeconv12(u8 *, u8 *, u8 *, u8 *, u16, u16, const SArDeconvTbl *, int);
    void LineDeconv22(u8 *, u8 *, u8 *, u8 *, u16, u16, const SArDeconvTbl *, int);

    // Cdj_d_Huff.c Function
    u32 huffmanDecoder(u32 *DCTBlock,
                       SArCDJ_HuffmanRequest *huffmanRequest, u16 **hufftree, int col);

    // Cdj_d_Idct.c Function
    void idct_fast(const u8 *range_limit, u32 *quant_table,
                   u32 *DCTBuffer, u8 *output_buf, u32 pixelOffset);
};

/*--------------------- ?O???[?o??????? -------------------------*/
/*------------------------------------------------------------------*/

/*--------------------- ?????l?t??????? -------------------------*/
/* ZigZag?I?[?_?[???L?q?????e?[?u?? */
const u8 odh_zigzag_order[DArCDJ_DCT_SIZE_2D] = {
    0, 1, 5, 6, 14, 15, 27, 28,
    2, 4, 7, 13, 16, 26, 29, 42,
    3, 8, 12, 17, 25, 30, 41, 43,
    9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63};

/* FDCT????????????f?[?^?????????????e?[?u?? */
const u8 FDCTOrder1[8] = {
    0, 32, 16, 48, 40, 8, 56, 24};

const u8 FDCTOrder2[8] = {
    5, 1, 7, 3, 2, 6, 0, 4};

/* ?tZigZag?I?[?_?[???L?q?????e?[?u?? */
const u8 odh_natural_order[DArCDJ_DCT_SIZE_2D] = {
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63};

const u8 range_limit[1024] = {
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F};

// ?W????q???e?[?u??(Y?p/CbCr?p)
const u8 gArCdj_std_quant_tbl[2][64] = {
    {16, 11, 10, 16, 24, 40, 51, 61,
     12, 12, 14, 19, 26, 58, 60, 55,
     14, 13, 16, 24, 40, 57, 69, 56,
     14, 17, 22, 29, 51, 87, 80, 62,
     18, 22, 37, 56, 68, 109, 103, 77,
     24, 35, 55, 64, 81, 104, 113, 92,
     49, 64, 78, 87, 103, 121, 120, 101,
     72, 92, 95, 98, 112, 100, 103, 99},
    {17, 18, 24, 47, 99, 99, 99, 99,
     18, 21, 26, 66, 99, 99, 99, 99,
     24, 26, 56, 99, 99, 99, 99, 99,
     47, 66, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99}};

// AAN?}?g???b?N?X(DCT??g?p?EDCT??AAN??)
const u16 gArAANScales[64] = {
    16384, 22725, 21407, 19266, 16384, 12873, 8867, 4520,
    22725, 31521, 29692, 26722, 22725, 17855, 12299, 6270,
    21407, 29692, 27969, 25172, 21407, 16819, 11585, 5906,
    19266, 26722, 25172, 22654, 19266, 15137, 10426, 5315,
    16384, 22725, 21407, 19266, 16384, 12873, 8867, 4520,
    12873, 17855, 16819, 15137, 12873, 10114, 6967, 3552,
    8867, 12299, 11585, 10426, 8867, 6967, 4799, 2446,
    4520, 6270, 5906, 5315, 4520, 3552, 2446, 1247};

/* Y?????pDC?n?t?}???e?[?u?? */
const u32 gArDC_L_Table[16] = {
    0x02000000, 0x03000002, 0x03000003, 0x03000004, 0x03000005, 0x03000006, 0x0400000e, 0x0500001e,
    0x0600003e, 0x0700007e, 0x080000fe, 0x090001fe, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

/* Y?????pAC?n?t?}???e?[?u?? */
const u32 gArAC_L_Table[16 * 16] = {
    0x0400000a, 0x02000000, 0x02000001, 0x03000004, 0x0400000b, 0x0500001a, 0x07000078, 0x080000f8,
    0x0a0003f6, 0x1000ff82, 0x1000ff83, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0400000c, 0x0500001b, 0x07000079, 0x090001f6, 0x0b0007f6, 0x1000ff84, 0x1000ff85,
    0x1000ff86, 0x1000ff87, 0x1000ff88, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0500001c, 0x080000f9, 0x0a0003f7, 0x0c000ff4, 0x1000ff89, 0x1000ff8a, 0x1000ff8b,
    0x1000ff8c, 0x1000ff8d, 0x1000ff8e, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0600003a, 0x090001f7, 0x0c000ff5, 0x1000ff8f, 0x1000ff90, 0x1000ff91, 0x1000ff92,
    0x1000ff93, 0x1000ff94, 0x1000ff95, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0600003b, 0x0a0003f8, 0x1000ff96, 0x1000ff97, 0x1000ff98, 0x1000ff99, 0x1000ff9a,
    0x1000ff9b, 0x1000ff9c, 0x1000ff9d, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0700007a, 0x0b0007f7, 0x1000ff9e, 0x1000ff9f, 0x1000ffa0, 0x1000ffa1, 0x1000ffa2,
    0x1000ffa3, 0x1000ffa4, 0x1000ffa5, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0700007b, 0x0c000ff6, 0x1000ffa6, 0x1000ffa7, 0x1000ffa8, 0x1000ffa9, 0x1000ffaa,
    0x1000ffab, 0x1000ffac, 0x1000ffad, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x080000fa, 0x0c000ff7, 0x1000ffae, 0x1000ffaf, 0x1000ffb0, 0x1000ffb1, 0x1000ffb2,
    0x1000ffb3, 0x1000ffb4, 0x1000ffb5, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x090001f8, 0x0f007fc0, 0x1000ffb6, 0x1000ffb7, 0x1000ffb8, 0x1000ffb9, 0x1000ffba,
    0x1000ffbb, 0x1000ffbc, 0x1000ffbd, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x090001f9, 0x1000ffbe, 0x1000ffbf, 0x1000ffc0, 0x1000ffc1, 0x1000ffc2, 0x1000ffc3,
    0x1000ffc4, 0x1000ffc5, 0x1000ffc6, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x090001fa, 0x1000ffc7, 0x1000ffc8, 0x1000ffc9, 0x1000ffca, 0x1000ffcb, 0x1000ffcc,
    0x1000ffcd, 0x1000ffce, 0x1000ffcf, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0a0003f9, 0x1000ffd0, 0x1000ffd1, 0x1000ffd2, 0x1000ffd3, 0x1000ffd4, 0x1000ffd5,
    0x1000ffd6, 0x1000ffd7, 0x1000ffd8, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0a0003fa, 0x1000ffd9, 0x1000ffda, 0x1000ffdb, 0x1000ffdc, 0x1000ffdd, 0x1000ffde,
    0x1000ffdf, 0x1000ffe0, 0x1000ffe1, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0b0007f8, 0x1000ffe2, 0x1000ffe3, 0x1000ffe4, 0x1000ffe5, 0x1000ffe6, 0x1000ffe7,
    0x1000ffe8, 0x1000ffe9, 0x1000ffea, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x1000ffeb, 0x1000ffec, 0x1000ffed, 0x1000ffee, 0x1000ffef, 0x1000fff0, 0x1000fff1,
    0x1000fff2, 0x1000fff3, 0x1000fff4, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x0b0007f9, 0x1000fff5, 0x1000fff6, 0x1000fff7, 0x1000fff8, 0x1000fff9, 0x1000fffa, 0x1000fffb,
    0x1000fffc, 0x1000fffd, 0x1000fffe, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

/* CbCr?????pDC?n?t?}???e?[?u?? */
const u32 gArDC_C_Table[16] = {
    0x02000000, 0x02000001, 0x02000002, 0x03000006, 0x0400000e, 0x0500001e, 0x0600003e, 0x0700007e,
    0x080000fe, 0x090001fe, 0x0a0003fe, 0x0b0007fe, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

/* CbCr?????pAC?n?t?}???e?[?u?? */
const u32 gArAC_C_Table[16 * 16] = {
    0x02000000, 0x02000001, 0x03000004, 0x0400000a, 0x05000018, 0x05000019, 0x06000038, 0x07000078,
    0x090001f4, 0x0a0003f6, 0x0c000ff4, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0400000b, 0x06000039, 0x080000f6, 0x090001f5, 0x0b0007f6, 0x0c000ff5, 0x1000ff88,
    0x1000ff89, 0x1000ff8a, 0x1000ff8b, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0500001a, 0x080000f7, 0x0a0003f7, 0x0c000ff6, 0x0f007fc2, 0x1000ff8c, 0x1000ff8d,
    0x1000ff8e, 0x1000ff8f, 0x1000ff90, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0500001b, 0x080000f8, 0x0a0003f8, 0x0c000ff7, 0x1000ff91, 0x1000ff92, 0x1000ff93,
    0x1000ff94, 0x1000ff95, 0x1000ff96, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0600003a, 0x090001f6, 0x1000ff97, 0x1000ff98, 0x1000ff99, 0x1000ff9a, 0x1000ff9b,
    0x1000ff9c, 0x1000ff9d, 0x1000ff9e, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0600003b, 0x0a0003f9, 0x1000ff9f, 0x1000ffa0, 0x1000ffa1, 0x1000ffa2, 0x1000ffa3,
    0x1000ffa4, 0x1000ffa5, 0x1000ffa6, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x07000079, 0x0b0007f7, 0x1000ffa7, 0x1000ffa8, 0x1000ffa9, 0x1000ffaa, 0x1000ffab,
    0x1000ffac, 0x1000ffad, 0x1000ffae, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0700007a, 0x0b0007f8, 0x1000ffaf, 0x1000ffb0, 0x1000ffb1, 0x1000ffb2, 0x1000ffb3,
    0x1000ffb4, 0x1000ffb5, 0x1000ffb6, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x080000f9, 0x1000ffb7, 0x1000ffb8, 0x1000ffb9, 0x1000ffba, 0x1000ffbb, 0x1000ffbc,
    0x1000ffbd, 0x1000ffbe, 0x1000ffbf, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x090001f7, 0x1000ffc0, 0x1000ffc1, 0x1000ffc2, 0x1000ffc3, 0x1000ffc4, 0x1000ffc5,
    0x1000ffc6, 0x1000ffc7, 0x1000ffc8, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x090001f8, 0x1000ffc9, 0x1000ffca, 0x1000ffcb, 0x1000ffcc, 0x1000ffcd, 0x1000ffce,
    0x1000ffcf, 0x1000ffd0, 0x1000ffd1, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x090001f9, 0x1000ffd2, 0x1000ffd3, 0x1000ffd4, 0x1000ffd5, 0x1000ffd6, 0x1000ffd7,
    0x1000ffd8, 0x1000ffd9, 0x1000ffda, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x090001fa, 0x1000ffdb, 0x1000ffdc, 0x1000ffdd, 0x1000ffde, 0x1000ffdf, 0x1000ffe0,
    0x1000ffe1, 0x1000ffe2, 0x1000ffe3, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0b0007f9, 0x1000ffe4, 0x1000ffe5, 0x1000ffe6, 0x1000ffe7, 0x1000ffe8, 0x1000ffe9,
    0x1000ffea, 0x1000ffeb, 0x1000ffec, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0e003fe0, 0x1000ffed, 0x1000ffee, 0x1000ffef, 0x1000fff0, 0x1000fff1, 0x1000fff2,
    0x1000fff3, 0x1000fff4, 0x1000fff5, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x0a0003fa, 0x0f007fc3, 0x1000fff6, 0x1000fff7, 0x1000fff8, 0x1000fff9, 0x1000fffa, 0x1000fffb,
    0x1000fffc, 0x1000fffd, 0x1000fffe, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

// cdj_c_plttTable.h Const Data
const s64 gArConvPlttTbl[3][3][32] = {
    // Y
    0x00000000,
    0x00026458,
    0x0004c8b0,
    0x00072d08,
    0x00099160,
    0x000bf5b8,
    0x000e5a10,
    0x0010be68,
    0x001322c0,
    0x00158718,
    0x0017eb70,
    0x001a4fc8,
    0x001cb420,
    0x001f1878,
    0x00217cd0,
    0x0023e128,
    0x00264580,
    0x0028a9d8,
    0x002b0e30,
    0x002d7288,
    0x002fd6e0,
    0x00323b38,
    0x00349f90,
    0x003703e8,
    0x00396840,
    0x003bcc98,
    0x003e30f0,
    0x00409548,
    0x0042f9a0,
    0x00455df8,
    0x0047c250,
    0x004a26a8,
    0x00000000,
    0x0004b230,
    0x00096460,
    0x000e1690,
    0x0012c8c0,
    0x00177af0,
    0x001c2d20,
    0x0020df50,
    0x00259180,
    0x002a43b0,
    0x002ef5e0,
    0x0033a810,
    0x00385a40,
    0x003d0c70,
    0x0041bea0,
    0x004670d0,
    0x004b2300,
    0x004fd530,
    0x00548760,
    0x00593990,
    0x005debc0,
    0x00629df0,
    0x00675020,
    0x006c0250,
    0x0070b480,
    0x007566b0,
    0x007a18e0,
    0x007ecb10,
    0x00837d40,
    0x00882f70,
    0x008ce1a0,
    0x009193d0,
    0x00008000,
    0x00016978,
    0x000252f0,
    0x00033c68,
    0x000425e0,
    0x00050f58,
    0x0005f8d0,
    0x0006e248,
    0x0007cbc0,
    0x0008b538,
    0x00099eb0,
    0x000a8828,
    0x000b71a0,
    0x000c5b18,
    0x000d4490,
    0x000e2e08,
    0x000f1780,
    0x001000f8,
    0x0010ea70,
    0x0011d3e8,
    0x0012bd60,
    0x0013a6d8,
    0x00149050,
    0x001579c8,
    0x00166340,
    0x00174cb8,
    0x00183630,
    0x00191fa8,
    0x001a0920,
    0x001af298,
    0x001bdc10,
    0x001cc588,

    // Cb
    0x00000000,
    0xfffea668,
    0xfffd4cd0,
    0xfffbf338,
    0xfffa99a0,
    0xfff94008,
    0xfff7e670,
    0xfff68cd8,
    0xfff53340,
    0xfff3d9a8,
    0xfff28010,
    0xfff12678,
    0xffefcce0,
    0xffee7348,
    0xffed19b0,
    0xffebc018,
    0xffea6680,
    0xffe90ce8,
    0xffe7b350,
    0xffe659b8,
    0xffe50020,
    0xffe3a688,
    0xffe24cf0,
    0xffe0f358,
    0xffdf99c0,
    0xffde4028,
    0xffdce690,
    0xffdb8cf8,
    0xffda3360,
    0xffd8d9c8,
    0xffd78030,
    0xffd62698,
    0x00000000,
    0xfffd5998,
    0xfffab330,
    0xfff80cc8,
    0xfff56660,
    0xfff2bff8,
    0xfff01990,
    0xffed7328,
    0xffeaccc0,
    0xffe82658,
    0xffe57ff0,
    0xffe2d988,
    0xffe03320,
    0xffdd8cb8,
    0xffdae650,
    0xffd83fe8,
    0xffd59980,
    0xffd2f318,
    0xffd04cb0,
    0xffcda648,
    0xffcaffe0,
    0xffc85978,
    0xffc5b310,
    0xffc30ca8,
    0xffc06640,
    0xffbdbfd8,
    0xffbb1970,
    0xffb87308,
    0xffb5cca0,
    0xffb32638,
    0xffb07fd0,
    0xffadd968,
    0x00807fff,
    0x00847fff,
    0x00887fff,
    0x008c7fff,
    0x00907fff,
    0x00947fff,
    0x00987fff,
    0x009c7fff,
    0x00a07fff,
    0x00a47fff,
    0x00a87fff,
    0x00ac7fff,
    0x00b07fff,
    0x00b47fff,
    0x00b87fff,
    0x00bc7fff,
    0x00c07fff,
    0x00c47fff,
    0x00c87fff,
    0x00cc7fff,
    0x00d07fff,
    0x00d47fff,
    0x00d87fff,
    0x00dc7fff,
    0x00e07fff,
    0x00e47fff,
    0x00e87fff,
    0x00ec7fff,
    0x00f07fff,
    0x00f47fff,
    0x00f87fff,
    0x00fc7fff,

    // Cr
    0x00807fff,
    0x00847fff,
    0x00887fff,
    0x008c7fff,
    0x00907fff,
    0x00947fff,
    0x00987fff,
    0x009c7fff,
    0x00a07fff,
    0x00a47fff,
    0x00a87fff,
    0x00ac7fff,
    0x00b07fff,
    0x00b47fff,
    0x00b87fff,
    0x00bc7fff,
    0x00c07fff,
    0x00c47fff,
    0x00c87fff,
    0x00cc7fff,
    0x00d07fff,
    0x00d47fff,
    0x00d87fff,
    0x00dc7fff,
    0x00e07fff,
    0x00e47fff,
    0x00e87fff,
    0x00ec7fff,
    0x00f07fff,
    0x00f47fff,
    0x00f87fff,
    0x00fc7fff,
    0x00000000,
    0xfffca688,
    0xfff94d10,
    0xfff5f398,
    0xfff29a20,
    0xffef40a8,
    0xffebe730,
    0xffe88db8,
    0xffe53440,
    0xffe1dac8,
    0xffde8150,
    0xffdb27d8,
    0xffd7ce60,
    0xffd474e8,
    0xffd11b70,
    0xffcdc1f8,
    0xffca6880,
    0xffc70f08,
    0xffc3b590,
    0xffc05c18,
    0xffbd02a0,
    0xffb9a928,
    0xffb64fb0,
    0xffb2f638,
    0xffaf9cc0,
    0xffac4348,
    0xffa8e9d0,
    0xffa59058,
    0xffa236e0,
    0xff9edd68,
    0xff9b83f0,
    0xff982a78,
    0x00000000,
    0xffff5978,
    0xfffeb2f0,
    0xfffe0c68,
    0xfffd65e0,
    0xfffcbf58,
    0xfffc18d0,
    0xfffb7248,
    0xfffacbc0,
    0xfffa2538,
    0xfff97eb0,
    0xfff8d828,
    0xfff831a0,
    0xfff78b18,
    0xfff6e490,
    0xfff63e08,
    0xfff59780,
    0xfff4f0f8,
    0xfff44a70,
    0xfff3a3e8,
    0xfff2fd60,
    0xfff256d8,
    0xfff1b050,
    0xfff109c8,
    0xfff06340,
    0xffefbcb8,
    0xffef1630,
    0xffee6fa8,
    0xffedc920,
    0xffed2298,
    0xffec7c10,
    0xffebd588,
};

// Cdj_d_plltTable.h Const Data
const SArDeconvTbl gArDeconvPlttTbl = {
    // Red
    (s16)0xff4d,
    (s16)0xff4e,
    (s16)0xff4f,
    (s16)0xff51,
    (s16)0xff52,
    (s16)0xff54,
    (s16)0xff55,
    (s16)0xff56,
    (s16)0xff58,
    (s16)0xff59,
    (s16)0xff5b,
    (s16)0xff5c,
    (s16)0xff5d,
    (s16)0xff5f,
    (s16)0xff60,
    (s16)0xff62,
    (s16)0xff63,
    (s16)0xff64,
    (s16)0xff66,
    (s16)0xff67,
    (s16)0xff69,
    (s16)0xff6a,
    (s16)0xff6b,
    (s16)0xff6d,
    (s16)0xff6e,
    (s16)0xff70,
    (s16)0xff71,
    (s16)0xff72,
    (s16)0xff74,
    (s16)0xff75,
    (s16)0xff77,
    (s16)0xff78,
    (s16)0xff79,
    (s16)0xff7b,
    (s16)0xff7c,
    (s16)0xff7e,
    (s16)0xff7f,
    (s16)0xff80,
    (s16)0xff82,
    (s16)0xff83,
    (s16)0xff85,
    (s16)0xff86,
    (s16)0xff87,
    (s16)0xff89,
    (s16)0xff8a,
    (s16)0xff8c,
    (s16)0xff8d,
    (s16)0xff8e,
    (s16)0xff90,
    (s16)0xff91,
    (s16)0xff93,
    (s16)0xff94,
    (s16)0xff95,
    (s16)0xff97,
    (s16)0xff98,
    (s16)0xff9a,
    (s16)0xff9b,
    (s16)0xff9c,
    (s16)0xff9e,
    (s16)0xff9f,
    (s16)0xffa1,
    (s16)0xffa2,
    (s16)0xffa3,
    (s16)0xffa5,
    (s16)0xffa6,
    (s16)0xffa8,
    (s16)0xffa9,
    (s16)0xffaa,
    (s16)0xffac,
    (s16)0xffad,
    (s16)0xffaf,
    (s16)0xffb0,
    (s16)0xffb1,
    (s16)0xffb3,
    (s16)0xffb4,
    (s16)0xffb6,
    (s16)0xffb7,
    (s16)0xffb8,
    (s16)0xffba,
    (s16)0xffbb,
    (s16)0xffbd,
    (s16)0xffbe,
    (s16)0xffc0,
    (s16)0xffc1,
    (s16)0xffc2,
    (s16)0xffc4,
    (s16)0xffc5,
    (s16)0xffc7,
    (s16)0xffc8,
    (s16)0xffc9,
    (s16)0xffcb,
    (s16)0xffcc,
    (s16)0xffce,
    (s16)0xffcf,
    (s16)0xffd0,
    (s16)0xffd2,
    (s16)0xffd3,
    (s16)0xffd5,
    (s16)0xffd6,
    (s16)0xffd7,
    (s16)0xffd9,
    (s16)0xffda,
    (s16)0xffdc,
    (s16)0xffdd,
    (s16)0xffde,
    (s16)0xffe0,
    (s16)0xffe1,
    (s16)0xffe3,
    (s16)0xffe4,
    (s16)0xffe5,
    (s16)0xffe7,
    (s16)0xffe8,
    (s16)0xffea,
    (s16)0xffeb,
    (s16)0xffec,
    (s16)0xffee,
    (s16)0xffef,
    (s16)0xfff1,
    (s16)0xfff2,
    (s16)0xfff3,
    (s16)0xfff5,
    (s16)0xfff6,
    (s16)0xfff8,
    (s16)0xfff9,
    (s16)0xfffa,
    (s16)0xfffc,
    (s16)0xfffd,
    (s16)0xffff,
    (s16)0x0000,
    (s16)0x0001,
    (s16)0x0003,
    (s16)0x0004,
    (s16)0x0006,
    (s16)0x0007,
    (s16)0x0008,
    (s16)0x000a,
    (s16)0x000b,
    (s16)0x000d,
    (s16)0x000e,
    (s16)0x000f,
    (s16)0x0011,
    (s16)0x0012,
    (s16)0x0014,
    (s16)0x0015,
    (s16)0x0016,
    (s16)0x0018,
    (s16)0x0019,
    (s16)0x001b,
    (s16)0x001c,
    (s16)0x001d,
    (s16)0x001f,
    (s16)0x0020,
    (s16)0x0022,
    (s16)0x0023,
    (s16)0x0024,
    (s16)0x0026,
    (s16)0x0027,
    (s16)0x0029,
    (s16)0x002a,
    (s16)0x002b,
    (s16)0x002d,
    (s16)0x002e,
    (s16)0x0030,
    (s16)0x0031,
    (s16)0x0032,
    (s16)0x0034,
    (s16)0x0035,
    (s16)0x0037,
    (s16)0x0038,
    (s16)0x0039,
    (s16)0x003b,
    (s16)0x003c,
    (s16)0x003e,
    (s16)0x003f,
    (s16)0x0040,
    (s16)0x0042,
    (s16)0x0043,
    (s16)0x0045,
    (s16)0x0046,
    (s16)0x0048,
    (s16)0x0049,
    (s16)0x004a,
    (s16)0x004c,
    (s16)0x004d,
    (s16)0x004f,
    (s16)0x0050,
    (s16)0x0051,
    (s16)0x0053,
    (s16)0x0054,
    (s16)0x0056,
    (s16)0x0057,
    (s16)0x0058,
    (s16)0x005a,
    (s16)0x005b,
    (s16)0x005d,
    (s16)0x005e,
    (s16)0x005f,
    (s16)0x0061,
    (s16)0x0062,
    (s16)0x0064,
    (s16)0x0065,
    (s16)0x0066,
    (s16)0x0068,
    (s16)0x0069,
    (s16)0x006b,
    (s16)0x006c,
    (s16)0x006d,
    (s16)0x006f,
    (s16)0x0070,
    (s16)0x0072,
    (s16)0x0073,
    (s16)0x0074,
    (s16)0x0076,
    (s16)0x0077,
    (s16)0x0079,
    (s16)0x007a,
    (s16)0x007b,
    (s16)0x007d,
    (s16)0x007e,
    (s16)0x0080,
    (s16)0x0081,
    (s16)0x0082,
    (s16)0x0084,
    (s16)0x0085,
    (s16)0x0087,
    (s16)0x0088,
    (s16)0x0089,
    (s16)0x008b,
    (s16)0x008c,
    (s16)0x008e,
    (s16)0x008f,
    (s16)0x0090,
    (s16)0x0092,
    (s16)0x0093,
    (s16)0x0095,
    (s16)0x0096,
    (s16)0x0097,
    (s16)0x0099,
    (s16)0x009a,
    (s16)0x009c,
    (s16)0x009d,
    (s16)0x009e,
    (s16)0x00a0,
    (s16)0x00a1,
    (s16)0x00a3,
    (s16)0x00a4,
    (s16)0x00a5,
    (s16)0x00a7,
    (s16)0x00a8,
    (s16)0x00aa,
    (s16)0x00ab,
    (s16)0x00ac,
    (s16)0x00ae,
    (s16)0x00af,
    (s16)0x00b1,
    (s16)0x00b2,

    // Green
    (s32)0x002c8d00,
    (s32)0x002c34e6,
    (s32)0x002bdccc,
    (s32)0x002b84b2,
    (s32)0x002b2c98,
    (s32)0x002ad47e,
    (s32)0x002a7c64,
    (s32)0x002a244a,
    (s32)0x0029cc30,
    (s32)0x00297416,
    (s32)0x00291bfc,
    (s32)0x0028c3e2,
    (s32)0x00286bc8,
    (s32)0x002813ae,
    (s32)0x0027bb94,
    (s32)0x0027637a,
    (s32)0x00270b60,
    (s32)0x0026b346,
    (s32)0x00265b2c,
    (s32)0x00260312,
    (s32)0x0025aaf8,
    (s32)0x002552de,
    (s32)0x0024fac4,
    (s32)0x0024a2aa,
    (s32)0x00244a90,
    (s32)0x0023f276,
    (s32)0x00239a5c,
    (s32)0x00234242,
    (s32)0x0022ea28,
    (s32)0x0022920e,
    (s32)0x002239f4,
    (s32)0x0021e1da,
    (s32)0x002189c0,
    (s32)0x002131a6,
    (s32)0x0020d98c,
    (s32)0x00208172,
    (s32)0x00202958,
    (s32)0x001fd13e,
    (s32)0x001f7924,
    (s32)0x001f210a,
    (s32)0x001ec8f0,
    (s32)0x001e70d6,
    (s32)0x001e18bc,
    (s32)0x001dc0a2,
    (s32)0x001d6888,
    (s32)0x001d106e,
    (s32)0x001cb854,
    (s32)0x001c603a,
    (s32)0x001c0820,
    (s32)0x001bb006,
    (s32)0x001b57ec,
    (s32)0x001affd2,
    (s32)0x001aa7b8,
    (s32)0x001a4f9e,
    (s32)0x0019f784,
    (s32)0x00199f6a,
    (s32)0x00194750,
    (s32)0x0018ef36,
    (s32)0x0018971c,
    (s32)0x00183f02,
    (s32)0x0017e6e8,
    (s32)0x00178ece,
    (s32)0x001736b4,
    (s32)0x0016de9a,
    (s32)0x00168680,
    (s32)0x00162e66,
    (s32)0x0015d64c,
    (s32)0x00157e32,
    (s32)0x00152618,
    (s32)0x0014cdfe,
    (s32)0x001475e4,
    (s32)0x00141dca,
    (s32)0x0013c5b0,
    (s32)0x00136d96,
    (s32)0x0013157c,
    (s32)0x0012bd62,
    (s32)0x00126548,
    (s32)0x00120d2e,
    (s32)0x0011b514,
    (s32)0x00115cfa,
    (s32)0x001104e0,
    (s32)0x0010acc6,
    (s32)0x001054ac,
    (s32)0x000ffc92,
    (s32)0x000fa478,
    (s32)0x000f4c5e,
    (s32)0x000ef444,
    (s32)0x000e9c2a,
    (s32)0x000e4410,
    (s32)0x000debf6,
    (s32)0x000d93dc,
    (s32)0x000d3bc2,
    (s32)0x000ce3a8,
    (s32)0x000c8b8e,
    (s32)0x000c3374,
    (s32)0x000bdb5a,
    (s32)0x000b8340,
    (s32)0x000b2b26,
    (s32)0x000ad30c,
    (s32)0x000a7af2,
    (s32)0x000a22d8,
    (s32)0x0009cabe,
    (s32)0x000972a4,
    (s32)0x00091a8a,
    (s32)0x0008c270,
    (s32)0x00086a56,
    (s32)0x0008123c,
    (s32)0x0007ba22,
    (s32)0x00076208,
    (s32)0x000709ee,
    (s32)0x0006b1d4,
    (s32)0x000659ba,
    (s32)0x000601a0,
    (s32)0x0005a986,
    (s32)0x0005516c,
    (s32)0x0004f952,
    (s32)0x0004a138,
    (s32)0x0004491e,
    (s32)0x0003f104,
    (s32)0x000398ea,
    (s32)0x000340d0,
    (s32)0x0002e8b6,
    (s32)0x0002909c,
    (s32)0x00023882,
    (s32)0x0001e068,
    (s32)0x0001884e,
    (s32)0x00013034,
    (s32)0x0000d81a,
    (s32)0x00008000,
    (s32)0x000027e6,
    (s32)0xffffcfcc,
    (s32)0xffff77b2,
    (s32)0xffff1f98,
    (s32)0xfffec77e,
    (s32)0xfffe6f64,
    (s32)0xfffe174a,
    (s32)0xfffdbf30,
    (s32)0xfffd6716,
    (s32)0xfffd0efc,
    (s32)0xfffcb6e2,
    (s32)0xfffc5ec8,
    (s32)0xfffc06ae,
    (s32)0xfffbae94,
    (s32)0xfffb567a,
    (s32)0xfffafe60,
    (s32)0xfffaa646,
    (s32)0xfffa4e2c,
    (s32)0xfff9f612,
    (s32)0xfff99df8,
    (s32)0xfff945de,
    (s32)0xfff8edc4,
    (s32)0xfff895aa,
    (s32)0xfff83d90,
    (s32)0xfff7e576,
    (s32)0xfff78d5c,
    (s32)0xfff73542,
    (s32)0xfff6dd28,
    (s32)0xfff6850e,
    (s32)0xfff62cf4,
    (s32)0xfff5d4da,
    (s32)0xfff57cc0,
    (s32)0xfff524a6,
    (s32)0xfff4cc8c,
    (s32)0xfff47472,
    (s32)0xfff41c58,
    (s32)0xfff3c43e,
    (s32)0xfff36c24,
    (s32)0xfff3140a,
    (s32)0xfff2bbf0,
    (s32)0xfff263d6,
    (s32)0xfff20bbc,
    (s32)0xfff1b3a2,
    (s32)0xfff15b88,
    (s32)0xfff1036e,
    (s32)0xfff0ab54,
    (s32)0xfff0533a,
    (s32)0xffeffb20,
    (s32)0xffefa306,
    (s32)0xffef4aec,
    (s32)0xffeef2d2,
    (s32)0xffee9ab8,
    (s32)0xffee429e,
    (s32)0xffedea84,
    (s32)0xffed926a,
    (s32)0xffed3a50,
    (s32)0xffece236,
    (s32)0xffec8a1c,
    (s32)0xffec3202,
    (s32)0xffebd9e8,
    (s32)0xffeb81ce,
    (s32)0xffeb29b4,
    (s32)0xffead19a,
    (s32)0xffea7980,
    (s32)0xffea2166,
    (s32)0xffe9c94c,
    (s32)0xffe97132,
    (s32)0xffe91918,
    (s32)0xffe8c0fe,
    (s32)0xffe868e4,
    (s32)0xffe810ca,
    (s32)0xffe7b8b0,
    (s32)0xffe76096,
    (s32)0xffe7087c,
    (s32)0xffe6b062,
    (s32)0xffe65848,
    (s32)0xffe6002e,
    (s32)0xffe5a814,
    (s32)0xffe54ffa,
    (s32)0xffe4f7e0,
    (s32)0xffe49fc6,
    (s32)0xffe447ac,
    (s32)0xffe3ef92,
    (s32)0xffe39778,
    (s32)0xffe33f5e,
    (s32)0xffe2e744,
    (s32)0xffe28f2a,
    (s32)0xffe23710,
    (s32)0xffe1def6,
    (s32)0xffe186dc,
    (s32)0xffe12ec2,
    (s32)0xffe0d6a8,
    (s32)0xffe07e8e,
    (s32)0xffe02674,
    (s32)0xffdfce5a,
    (s32)0xffdf7640,
    (s32)0xffdf1e26,
    (s32)0xffdec60c,
    (s32)0xffde6df2,
    (s32)0xffde15d8,
    (s32)0xffddbdbe,
    (s32)0xffdd65a4,
    (s32)0xffdd0d8a,
    (s32)0xffdcb570,
    (s32)0xffdc5d56,
    (s32)0xffdc053c,
    (s32)0xffdbad22,
    (s32)0xffdb5508,
    (s32)0xffdafcee,
    (s32)0xffdaa4d4,
    (s32)0xffda4cba,
    (s32)0xffd9f4a0,
    (s32)0xffd99c86,
    (s32)0xffd9446c,
    (s32)0xffd8ec52,
    (s32)0xffd89438,
    (s32)0xffd83c1e,
    (s32)0xffd7e404,
    (s32)0xffd78bea,
    (s32)0xffd733d0,
    (s32)0xffd6dbb6,
    (s32)0xffd6839c,
    (s32)0xffd62b82,
    (s32)0xffd5d368,
    (s32)0xffd57b4e,
    (s32)0xffd52334,
    (s32)0xffd4cb1a,
    (s32)0x005b6900,
    (s32)0x005ab22e,
    (s32)0x0059fb5c,
    (s32)0x0059448a,
    (s32)0x00588db8,
    (s32)0x0057d6e6,
    (s32)0x00572014,
    (s32)0x00566942,
    (s32)0x0055b270,
    (s32)0x0054fb9e,
    (s32)0x005444cc,
    (s32)0x00538dfa,
    (s32)0x0052d728,
    (s32)0x00522056,
    (s32)0x00516984,
    (s32)0x0050b2b2,
    (s32)0x004ffbe0,
    (s32)0x004f450e,
    (s32)0x004e8e3c,
    (s32)0x004dd76a,
    (s32)0x004d2098,
    (s32)0x004c69c6,
    (s32)0x004bb2f4,
    (s32)0x004afc22,
    (s32)0x004a4550,
    (s32)0x00498e7e,
    (s32)0x0048d7ac,
    (s32)0x004820da,
    (s32)0x00476a08,
    (s32)0x0046b336,
    (s32)0x0045fc64,
    (s32)0x00454592,
    (s32)0x00448ec0,
    (s32)0x0043d7ee,
    (s32)0x0043211c,
    (s32)0x00426a4a,
    (s32)0x0041b378,
    (s32)0x0040fca6,
    (s32)0x004045d4,
    (s32)0x003f8f02,
    (s32)0x003ed830,
    (s32)0x003e215e,
    (s32)0x003d6a8c,
    (s32)0x003cb3ba,
    (s32)0x003bfce8,
    (s32)0x003b4616,
    (s32)0x003a8f44,
    (s32)0x0039d872,
    (s32)0x003921a0,
    (s32)0x00386ace,
    (s32)0x0037b3fc,
    (s32)0x0036fd2a,
    (s32)0x00364658,
    (s32)0x00358f86,
    (s32)0x0034d8b4,
    (s32)0x003421e2,
    (s32)0x00336b10,
    (s32)0x0032b43e,
    (s32)0x0031fd6c,
    (s32)0x0031469a,
    (s32)0x00308fc8,
    (s32)0x002fd8f6,
    (s32)0x002f2224,
    (s32)0x002e6b52,
    (s32)0x002db480,
    (s32)0x002cfdae,
    (s32)0x002c46dc,
    (s32)0x002b900a,
    (s32)0x002ad938,
    (s32)0x002a2266,
    (s32)0x00296b94,
    (s32)0x0028b4c2,
    (s32)0x0027fdf0,
    (s32)0x0027471e,
    (s32)0x0026904c,
    (s32)0x0025d97a,
    (s32)0x002522a8,
    (s32)0x00246bd6,
    (s32)0x0023b504,
    (s32)0x0022fe32,
    (s32)0x00224760,
    (s32)0x0021908e,
    (s32)0x0020d9bc,
    (s32)0x002022ea,
    (s32)0x001f6c18,
    (s32)0x001eb546,
    (s32)0x001dfe74,
    (s32)0x001d47a2,
    (s32)0x001c90d0,
    (s32)0x001bd9fe,
    (s32)0x001b232c,
    (s32)0x001a6c5a,
    (s32)0x0019b588,
    (s32)0x0018feb6,
    (s32)0x001847e4,
    (s32)0x00179112,
    (s32)0x0016da40,
    (s32)0x0016236e,
    (s32)0x00156c9c,
    (s32)0x0014b5ca,
    (s32)0x0013fef8,
    (s32)0x00134826,
    (s32)0x00129154,
    (s32)0x0011da82,
    (s32)0x001123b0,
    (s32)0x00106cde,
    (s32)0x000fb60c,
    (s32)0x000eff3a,
    (s32)0x000e4868,
    (s32)0x000d9196,
    (s32)0x000cdac4,
    (s32)0x000c23f2,
    (s32)0x000b6d20,
    (s32)0x000ab64e,
    (s32)0x0009ff7c,
    (s32)0x000948aa,
    (s32)0x000891d8,
    (s32)0x0007db06,
    (s32)0x00072434,
    (s32)0x00066d62,
    (s32)0x0005b690,
    (s32)0x0004ffbe,
    (s32)0x000448ec,
    (s32)0x0003921a,
    (s32)0x0002db48,
    (s32)0x00022476,
    (s32)0x00016da4,
    (s32)0x0000b6d2,
    (s32)0x00000000,
    (s32)0xffff492e,
    (s32)0xfffe925c,
    (s32)0xfffddb8a,
    (s32)0xfffd24b8,
    (s32)0xfffc6de6,
    (s32)0xfffbb714,
    (s32)0xfffb0042,
    (s32)0xfffa4970,
    (s32)0xfff9929e,
    (s32)0xfff8dbcc,
    (s32)0xfff824fa,
    (s32)0xfff76e28,
    (s32)0xfff6b756,
    (s32)0xfff60084,
    (s32)0xfff549b2,
    (s32)0xfff492e0,
    (s32)0xfff3dc0e,
    (s32)0xfff3253c,
    (s32)0xfff26e6a,
    (s32)0xfff1b798,
    (s32)0xfff100c6,
    (s32)0xfff049f4,
    (s32)0xffef9322,
    (s32)0xffeedc50,
    (s32)0xffee257e,
    (s32)0xffed6eac,
    (s32)0xffecb7da,
    (s32)0xffec0108,
    (s32)0xffeb4a36,
    (s32)0xffea9364,
    (s32)0xffe9dc92,
    (s32)0xffe925c0,
    (s32)0xffe86eee,
    (s32)0xffe7b81c,
    (s32)0xffe7014a,
    (s32)0xffe64a78,
    (s32)0xffe593a6,
    (s32)0xffe4dcd4,
    (s32)0xffe42602,
    (s32)0xffe36f30,
    (s32)0xffe2b85e,
    (s32)0xffe2018c,
    (s32)0xffe14aba,
    (s32)0xffe093e8,
    (s32)0xffdfdd16,
    (s32)0xffdf2644,
    (s32)0xffde6f72,
    (s32)0xffddb8a0,
    (s32)0xffdd01ce,
    (s32)0xffdc4afc,
    (s32)0xffdb942a,
    (s32)0xffdadd58,
    (s32)0xffda2686,
    (s32)0xffd96fb4,
    (s32)0xffd8b8e2,
    (s32)0xffd80210,
    (s32)0xffd74b3e,
    (s32)0xffd6946c,
    (s32)0xffd5dd9a,
    (s32)0xffd526c8,
    (s32)0xffd46ff6,
    (s32)0xffd3b924,
    (s32)0xffd30252,
    (s32)0xffd24b80,
    (s32)0xffd194ae,
    (s32)0xffd0dddc,
    (s32)0xffd0270a,
    (s32)0xffcf7038,
    (s32)0xffceb966,
    (s32)0xffce0294,
    (s32)0xffcd4bc2,
    (s32)0xffcc94f0,
    (s32)0xffcbde1e,
    (s32)0xffcb274c,
    (s32)0xffca707a,
    (s32)0xffc9b9a8,
    (s32)0xffc902d6,
    (s32)0xffc84c04,
    (s32)0xffc79532,
    (s32)0xffc6de60,
    (s32)0xffc6278e,
    (s32)0xffc570bc,
    (s32)0xffc4b9ea,
    (s32)0xffc40318,
    (s32)0xffc34c46,
    (s32)0xffc29574,
    (s32)0xffc1dea2,
    (s32)0xffc127d0,
    (s32)0xffc070fe,
    (s32)0xffbfba2c,
    (s32)0xffbf035a,
    (s32)0xffbe4c88,
    (s32)0xffbd95b6,
    (s32)0xffbcdee4,
    (s32)0xffbc2812,
    (s32)0xffbb7140,
    (s32)0xffbaba6e,
    (s32)0xffba039c,
    (s32)0xffb94cca,
    (s32)0xffb895f8,
    (s32)0xffb7df26,
    (s32)0xffb72854,
    (s32)0xffb67182,
    (s32)0xffb5bab0,
    (s32)0xffb503de,
    (s32)0xffb44d0c,
    (s32)0xffb3963a,
    (s32)0xffb2df68,
    (s32)0xffb22896,
    (s32)0xffb171c4,
    (s32)0xffb0baf2,
    (s32)0xffb00420,
    (s32)0xffaf4d4e,
    (s32)0xffae967c,
    (s32)0xffaddfaa,
    (s32)0xffad28d8,
    (s32)0xffac7206,
    (s32)0xffabbb34,
    (s32)0xffab0462,
    (s32)0xffaa4d90,
    (s32)0xffa996be,
    (s32)0xffa8dfec,
    (s32)0xffa8291a,
    (s32)0xffa77248,
    (s32)0xffa6bb76,
    (s32)0xffa604a4,
    (s32)0xffa54dd2,

    // Blue
    (s16)0xff1d,
    (s16)0xff1f,
    (s16)0xff21,
    (s16)0xff22,
    (s16)0xff24,
    (s16)0xff26,
    (s16)0xff28,
    (s16)0xff2a,
    (s16)0xff2b,
    (s16)0xff2d,
    (s16)0xff2f,
    (s16)0xff31,
    (s16)0xff32,
    (s16)0xff34,
    (s16)0xff36,
    (s16)0xff38,
    (s16)0xff3a,
    (s16)0xff3b,
    (s16)0xff3d,
    (s16)0xff3f,
    (s16)0xff41,
    (s16)0xff42,
    (s16)0xff44,
    (s16)0xff46,
    (s16)0xff48,
    (s16)0xff49,
    (s16)0xff4b,
    (s16)0xff4d,
    (s16)0xff4f,
    (s16)0xff51,
    (s16)0xff52,
    (s16)0xff54,
    (s16)0xff56,
    (s16)0xff58,
    (s16)0xff59,
    (s16)0xff5b,
    (s16)0xff5d,
    (s16)0xff5f,
    (s16)0xff61,
    (s16)0xff62,
    (s16)0xff64,
    (s16)0xff66,
    (s16)0xff68,
    (s16)0xff69,
    (s16)0xff6b,
    (s16)0xff6d,
    (s16)0xff6f,
    (s16)0xff70,
    (s16)0xff72,
    (s16)0xff74,
    (s16)0xff76,
    (s16)0xff78,
    (s16)0xff79,
    (s16)0xff7b,
    (s16)0xff7d,
    (s16)0xff7f,
    (s16)0xff80,
    (s16)0xff82,
    (s16)0xff84,
    (s16)0xff86,
    (s16)0xff88,
    (s16)0xff89,
    (s16)0xff8b,
    (s16)0xff8d,
    (s16)0xff8f,
    (s16)0xff90,
    (s16)0xff92,
    (s16)0xff94,
    (s16)0xff96,
    (s16)0xff97,
    (s16)0xff99,
    (s16)0xff9b,
    (s16)0xff9d,
    (s16)0xff9f,
    (s16)0xffa0,
    (s16)0xffa2,
    (s16)0xffa4,
    (s16)0xffa6,
    (s16)0xffa7,
    (s16)0xffa9,
    (s16)0xffab,
    (s16)0xffad,
    (s16)0xffae,
    (s16)0xffb0,
    (s16)0xffb2,
    (s16)0xffb4,
    (s16)0xffb6,
    (s16)0xffb7,
    (s16)0xffb9,
    (s16)0xffbb,
    (s16)0xffbd,
    (s16)0xffbe,
    (s16)0xffc0,
    (s16)0xffc2,
    (s16)0xffc4,
    (s16)0xffc6,
    (s16)0xffc7,
    (s16)0xffc9,
    (s16)0xffcb,
    (s16)0xffcd,
    (s16)0xffce,
    (s16)0xffd0,
    (s16)0xffd2,
    (s16)0xffd4,
    (s16)0xffd5,
    (s16)0xffd7,
    (s16)0xffd9,
    (s16)0xffdb,
    (s16)0xffdd,
    (s16)0xffde,
    (s16)0xffe0,
    (s16)0xffe2,
    (s16)0xffe4,
    (s16)0xffe5,
    (s16)0xffe7,
    (s16)0xffe9,
    (s16)0xffeb,
    (s16)0xffed,
    (s16)0xffee,
    (s16)0xfff0,
    (s16)0xfff2,
    (s16)0xfff4,
    (s16)0xfff5,
    (s16)0xfff7,
    (s16)0xfff9,
    (s16)0xfffb,
    (s16)0xfffc,
    (s16)0xfffe,
    (s16)0x0000,
    (s16)0x0002,
    (s16)0x0004,
    (s16)0x0005,
    (s16)0x0007,
    (s16)0x0009,
    (s16)0x000b,
    (s16)0x000c,
    (s16)0x000e,
    (s16)0x0010,
    (s16)0x0012,
    (s16)0x0013,
    (s16)0x0015,
    (s16)0x0017,
    (s16)0x0019,
    (s16)0x001b,
    (s16)0x001c,
    (s16)0x001e,
    (s16)0x0020,
    (s16)0x0022,
    (s16)0x0023,
    (s16)0x0025,
    (s16)0x0027,
    (s16)0x0029,
    (s16)0x002b,
    (s16)0x002c,
    (s16)0x002e,
    (s16)0x0030,
    (s16)0x0032,
    (s16)0x0033,
    (s16)0x0035,
    (s16)0x0037,
    (s16)0x0039,
    (s16)0x003a,
    (s16)0x003c,
    (s16)0x003e,
    (s16)0x0040,
    (s16)0x0042,
    (s16)0x0043,
    (s16)0x0045,
    (s16)0x0047,
    (s16)0x0049,
    (s16)0x004a,
    (s16)0x004c,
    (s16)0x004e,
    (s16)0x0050,
    (s16)0x0052,
    (s16)0x0053,
    (s16)0x0055,
    (s16)0x0057,
    (s16)0x0059,
    (s16)0x005a,
    (s16)0x005c,
    (s16)0x005e,
    (s16)0x0060,
    (s16)0x0061,
    (s16)0x0063,
    (s16)0x0065,
    (s16)0x0067,
    (s16)0x0069,
    (s16)0x006a,
    (s16)0x006c,
    (s16)0x006e,
    (s16)0x0070,
    (s16)0x0071,
    (s16)0x0073,
    (s16)0x0075,
    (s16)0x0077,
    (s16)0x0078,
    (s16)0x007a,
    (s16)0x007c,
    (s16)0x007e,
    (s16)0x0080,
    (s16)0x0081,
    (s16)0x0083,
    (s16)0x0085,
    (s16)0x0087,
    (s16)0x0088,
    (s16)0x008a,
    (s16)0x008c,
    (s16)0x008e,
    (s16)0x0090,
    (s16)0x0091,
    (s16)0x0093,
    (s16)0x0095,
    (s16)0x0097,
    (s16)0x0098,
    (s16)0x009a,
    (s16)0x009c,
    (s16)0x009e,
    (s16)0x009f,
    (s16)0x00a1,
    (s16)0x00a3,
    (s16)0x00a5,
    (s16)0x00a7,
    (s16)0x00a8,
    (s16)0x00aa,
    (s16)0x00ac,
    (s16)0x00ae,
    (s16)0x00af,
    (s16)0x00b1,
    (s16)0x00b3,
    (s16)0x00b5,
    (s16)0x00b7,
    (s16)0x00b8,
    (s16)0x00ba,
    (s16)0x00bc,
    (s16)0x00be,
    (s16)0x00bf,
    (s16)0x00c1,
    (s16)0x00c3,
    (s16)0x00c5,
    (s16)0x00c6,
    (s16)0x00c8,
    (s16)0x00ca,
    (s16)0x00cc,
    (s16)0x00ce,
    (s16)0x00cf,
    (s16)0x00d1,
    (s16)0x00d3,
    (s16)0x00d5,
    (s16)0x00d6,
    (s16)0x00d8,
    (s16)0x00da,
    (s16)0x00dc,
    (s16)0x00de,
    (s16)0x00df,
    (s16)0x00e1,
};

// hufftree.h Const Data
const u16 gArDc_luminance_huffTable[24] = {
    0x0001,
    0x8002,
    0x0003,
    0x0000,
    0xc003,
    0xc004,
    0x8005,
    0x0001,
    0x0002,
    0x0003,
    0x0004,
    0x0005,
    0x8001,
    0x0006,
    0x8001,
    0x0007,
    0x8001,
    0x0008,
    0x8001,
    0x0009,
    0x8001,
    0x000a,
    0x8001,
    0x000b,
};

const u16 gArDc_chrominance_huffTable[24] = {
    0x0001,
    0xc002,
    0x8003,
    0x0000,
    0x0001,
    0x0002,
    0x8001,
    0x0003,
    0x8001,
    0x0004,
    0x8001,
    0x0005,
    0x8001,
    0x0006,
    0x8001,
    0x0007,
    0x8001,
    0x0008,
    0x8001,
    0x0009,
    0x8001,
    0x000a,
    0x8001,
    0x000b,
};

const u16 gArAc_luminance_huffTable[324] = {
    0x0001,
    0xc002,
    0x0003,
    0x0001,
    0x0002,
    0x8002,
    0x0003,
    0x0003,
    0xc003,
    0x8004,
    0x0005,
    0x0000,
    0x0004,
    0x0011,
    0xc003,
    0x8004,
    0x0005,
    0x0005,
    0x0012,
    0x0021,
    0xc003,
    0x0004,
    0x0005,
    0x0031,
    0x0041,
    0xc004,
    0xc005,
    0x0006,
    0x0007,
    0x0006,
    0x0013,
    0x0051,
    0x0061,
    0xc004,
    0x8005,
    0x0006,
    0x0007,
    0x0007,
    0x0022,
    0x0071,
    0xc005,
    0xc006,
    0x8007,
    0x0008,
    0x0009,
    0x0014,
    0x0032,
    0x0081,
    0x0091,
    0x00a1,
    0xc005,
    0xc006,
    0x8007,
    0x0008,
    0x0009,
    0x0008,
    0x0023,
    0x0042,
    0x00b1,
    0x00c1,
    0xc005,
    0xc006,
    0x0007,
    0x0008,
    0x0009,
    0x0015,
    0x0052,
    0x00d1,
    0x00f0,
    0xc006,
    0xc007,
    0x0008,
    0x0009,
    0x000a,
    0x000b,
    0x0024,
    0x0033,
    0x0062,
    0x0072,
    0x0008,
    0x0009,
    0x000a,
    0x000b,
    0x000c,
    0x000d,
    0x000e,
    0x000f,
    0x0010,
    0x0011,
    0x0012,
    0x0013,
    0x0014,
    0x0015,
    0x0016,
    0x0017,
    0x0018,
    0x0019,
    0x001a,
    0x001b,
    0x001c,
    0x001d,
    0x001e,
    0x001f,
    0x8020,
    0x0021,
    0x0022,
    0x0023,
    0x0024,
    0x0025,
    0x0026,
    0x0027,
    0x0028,
    0x0029,
    0x002a,
    0x002b,
    0x002c,
    0x002d,
    0x002e,
    0x002f,
    0x0030,
    0x0031,
    0x0032,
    0x0033,
    0x0034,
    0x0035,
    0x0036,
    0x0037,
    0x0038,
    0x0039,
    0x003a,
    0x003b,
    0x003c,
    0x003d,
    0x003e,
    0x003f,
    0x0082,
    0xc03f,
    0xc040,
    0xc041,
    0xc042,
    0xc043,
    0xc044,
    0xc045,
    0xc046,
    0xc047,
    0xc048,
    0xc049,
    0xc04a,
    0xc04b,
    0xc04c,
    0xc04d,
    0xc04e,
    0xc04f,
    0xc050,
    0xc051,
    0xc052,
    0xc053,
    0xc054,
    0xc055,
    0xc056,
    0xc057,
    0xc058,
    0xc059,
    0xc05a,
    0xc05b,
    0xc05c,
    0xc05d,
    0xc05e,
    0xc05f,
    0xc060,
    0xc061,
    0xc062,
    0xc063,
    0xc064,
    0xc065,
    0xc066,
    0xc067,
    0xc068,
    0xc069,
    0xc06a,
    0xc06b,
    0xc06c,
    0xc06d,
    0xc06e,
    0xc06f,
    0xc070,
    0xc071,
    0xc072,
    0xc073,
    0xc074,
    0xc075,
    0xc076,
    0xc077,
    0xc078,
    0xc079,
    0xc07a,
    0xc07b,
    0xc07c,
    0x807d,
    0x0009,
    0x000a,
    0x0016,
    0x0017,
    0x0018,
    0x0019,
    0x001a,
    0x0025,
    0x0026,
    0x0027,
    0x0028,
    0x0029,
    0x002a,
    0x0034,
    0x0035,
    0x0036,
    0x0037,
    0x0038,
    0x0039,
    0x003a,
    0x0043,
    0x0044,
    0x0045,
    0x0046,
    0x0047,
    0x0048,
    0x0049,
    0x004a,
    0x0053,
    0x0054,
    0x0055,
    0x0056,
    0x0057,
    0x0058,
    0x0059,
    0x005a,
    0x0063,
    0x0064,
    0x0065,
    0x0066,
    0x0067,
    0x0068,
    0x0069,
    0x006a,
    0x0073,
    0x0074,
    0x0075,
    0x0076,
    0x0077,
    0x0078,
    0x0079,
    0x007a,
    0x0083,
    0x0084,
    0x0085,
    0x0086,
    0x0087,
    0x0088,
    0x0089,
    0x008a,
    0x0092,
    0x0093,
    0x0094,
    0x0095,
    0x0096,
    0x0097,
    0x0098,
    0x0099,
    0x009a,
    0x00a2,
    0x00a3,
    0x00a4,
    0x00a5,
    0x00a6,
    0x00a7,
    0x00a8,
    0x00a9,
    0x00aa,
    0x00b2,
    0x00b3,
    0x00b4,
    0x00b5,
    0x00b6,
    0x00b7,
    0x00b8,
    0x00b9,
    0x00ba,
    0x00c2,
    0x00c3,
    0x00c4,
    0x00c5,
    0x00c6,
    0x00c7,
    0x00c8,
    0x00c9,
    0x00ca,
    0x00d2,
    0x00d3,
    0x00d4,
    0x00d5,
    0x00d6,
    0x00d7,
    0x00d8,
    0x00d9,
    0x00da,
    0x00e1,
    0x00e2,
    0x00e3,
    0x00e4,
    0x00e5,
    0x00e6,
    0x00e7,
    0x00e8,
    0x00e9,
    0x00ea,
    0x00f1,
    0x00f2,
    0x00f3,
    0x00f4,
    0x00f5,
    0x00f6,
    0x00f7,
    0x00f8,
    0x00f9,
    0x00fa,
};

const u16 gArAc_chrominance_huffTable[324] = {
    0x0001,
    0xc002,
    0x0003,
    0x0000,
    0x0001,
    0x8002,
    0x0003,
    0x0002,
    0xc003,
    0x0004,
    0x0005,
    0x0003,
    0x0011,
    0xc004,
    0xc005,
    0x0006,
    0x0007,
    0x0004,
    0x0005,
    0x0021,
    0x0031,
    0xc004,
    0xc005,
    0x0006,
    0x0007,
    0x0006,
    0x0012,
    0x0041,
    0x0051,
    0xc004,
    0x8005,
    0x0006,
    0x0007,
    0x0007,
    0x0061,
    0x0071,
    0xc005,
    0xc006,
    0x0007,
    0x0008,
    0x0009,
    0x0013,
    0x0022,
    0x0032,
    0x0081,
    0xc006,
    0xc007,
    0xc008,
    0x8009,
    0x000a,
    0x000b,
    0x0008,
    0x0014,
    0x0042,
    0x0091,
    0x00a1,
    0x00b1,
    0x00c1,
    0xc005,
    0xc006,
    0x8007,
    0x0008,
    0x0009,
    0x0009,
    0x0023,
    0x0033,
    0x0052,
    0x00f0,
    0xc005,
    0xc006,
    0x0007,
    0x0008,
    0x0009,
    0x0015,
    0x0062,
    0x0072,
    0x00d1,
    0xc006,
    0xc007,
    0x0008,
    0x0009,
    0x000a,
    0x000b,
    0x000a,
    0x0016,
    0x0024,
    0x0034,
    0x0008,
    0x0009,
    0x000a,
    0x000b,
    0x000c,
    0x000d,
    0x000e,
    0x000f,
    0x8010,
    0x0011,
    0x0012,
    0x0013,
    0x0014,
    0x0015,
    0x0016,
    0x0017,
    0x0018,
    0x0019,
    0x001a,
    0x001b,
    0x001c,
    0x001d,
    0x001e,
    0x001f,
    0x00e1,
    0xc01f,
    0x0020,
    0x0021,
    0x0022,
    0x0023,
    0x0024,
    0x0025,
    0x0026,
    0x0027,
    0x0028,
    0x0029,
    0x002a,
    0x002b,
    0x002c,
    0x002d,
    0x002e,
    0x002f,
    0x0030,
    0x0031,
    0x0032,
    0x0033,
    0x0034,
    0x0035,
    0x0036,
    0x0037,
    0x0038,
    0x0039,
    0x003a,
    0x003b,
    0x003c,
    0x003d,
    0x0025,
    0x00f1,
    0xc03c,
    0xc03d,
    0xc03e,
    0xc03f,
    0xc040,
    0xc041,
    0xc042,
    0xc043,
    0xc044,
    0xc045,
    0xc046,
    0xc047,
    0xc048,
    0xc049,
    0xc04a,
    0xc04b,
    0xc04c,
    0xc04d,
    0xc04e,
    0xc04f,
    0xc050,
    0xc051,
    0xc052,
    0xc053,
    0xc054,
    0xc055,
    0xc056,
    0xc057,
    0xc058,
    0xc059,
    0xc05a,
    0xc05b,
    0xc05c,
    0xc05d,
    0xc05e,
    0xc05f,
    0xc060,
    0xc061,
    0xc062,
    0xc063,
    0xc064,
    0xc065,
    0xc066,
    0xc067,
    0xc068,
    0xc069,
    0xc06a,
    0xc06b,
    0xc06c,
    0xc06d,
    0xc06e,
    0xc06f,
    0xc070,
    0xc071,
    0xc072,
    0xc073,
    0xc074,
    0xc075,
    0xc076,
    0x8077,
    0x0017,
    0x0018,
    0x0019,
    0x001a,
    0x0026,
    0x0027,
    0x0028,
    0x0029,
    0x002a,
    0x0035,
    0x0036,
    0x0037,
    0x0038,
    0x0039,
    0x003a,
    0x0043,
    0x0044,
    0x0045,
    0x0046,
    0x0047,
    0x0048,
    0x0049,
    0x004a,
    0x0053,
    0x0054,
    0x0055,
    0x0056,
    0x0057,
    0x0058,
    0x0059,
    0x005a,
    0x0063,
    0x0064,
    0x0065,
    0x0066,
    0x0067,
    0x0068,
    0x0069,
    0x006a,
    0x0073,
    0x0074,
    0x0075,
    0x0076,
    0x0077,
    0x0078,
    0x0079,
    0x007a,
    0x0082,
    0x0083,
    0x0084,
    0x0085,
    0x0086,
    0x0087,
    0x0088,
    0x0089,
    0x008a,
    0x0092,
    0x0093,
    0x0094,
    0x0095,
    0x0096,
    0x0097,
    0x0098,
    0x0099,
    0x009a,
    0x00a2,
    0x00a3,
    0x00a4,
    0x00a5,
    0x00a6,
    0x00a7,
    0x00a8,
    0x00a9,
    0x00aa,
    0x00b2,
    0x00b3,
    0x00b4,
    0x00b5,
    0x00b6,
    0x00b7,
    0x00b8,
    0x00b9,
    0x00ba,
    0x00c2,
    0x00c3,
    0x00c4,
    0x00c5,
    0x00c6,
    0x00c7,
    0x00c8,
    0x00c9,
    0x00ca,
    0x00d2,
    0x00d3,
    0x00d4,
    0x00d5,
    0x00d6,
    0x00d7,
    0x00d8,
    0x00d9,
    0x00da,
    0x00e2,
    0x00e3,
    0x00e4,
    0x00e5,
    0x00e6,
    0x00e7,
    0x00e8,
    0x00e9,
    0x00ea,
    0x00f2,
    0x00f3,
    0x00f4,
    0x00f5,
    0x00f6,
    0x00f7,
    0x00f8,
    0x00f9,
    0x00fa,
};

const u16 *hufftreePtr[4] = {
    gArDc_luminance_huffTable, gArDc_luminance_huffTable,
    gArDc_chrominance_huffTable, gArDc_luminance_huffTable};

/*------------------------------------------------------------------*/

/********************************************************************
    ODHEncodeRGB565 - RGB565?`????ODH????k????

    ?????F
        src : RGB565?f?[?^??o?b?t?@(width*heigh*2)
        dst : ODH?f?[?^??????o?b?t?@
        limit: ODH?f?[?^??o?b?t?@?T?C?Y
        quality: ??(0-100)
        work: ???[?N?G???A(width*height*3)?K?v
    ???l?F
        ???k(ODH)?T?C?Y 0????s
/********************************************************************/
int ODHEncodeRGB565(unsigned char *src, unsigned char *dst, int width, int height, int limit, int quality, unsigned char *work)
{
    CArGBAOdh jpg;
    int result;

    result = jpg.compressGbaOdh(src, dst, width, height, quality, (u32)limit, work, RGB565);
    return (result);
}

/********************************************************************
    ODHEncodeRGBA8 - RGBA8?`????ODH????k????

    ?????F
        src : RGBA8?f?[?^??o?b?t?@(width*heigh*4)
        dst : ODH?f?[?^??????o?b?t?@
        limit: ODH?f?[?^??o?b?t?@?T?C?Y
        quality: ??(0-100)
        work: ???[?N?G???A(width*height*3)?K?v
    ???l?F
        ???k(ODH)?T?C?Y 0????s
/********************************************************************/
int ODHEncodeRGBA8(unsigned char *src, unsigned char *dst, int width, int height, int limit, int quality, unsigned char *work)
{
    CArGBAOdh jpg;
    int result;

    result = jpg.compressGbaOdh(src, dst, width, height, quality, (u32)limit, work, RGBA8);
    return (result);
}

/********************************************************************
    ODHEncodeY8U8V8 - Y8U8V8?`????ODH????k????

    ?????F
        src : Y8U8V8?f?[?^??o?b?t?@(width*height*3)
        dst : ODH?f?[?^??????o?b?t?@
        limit: ODH?f?[?^??o?b?t?@?T?C?Y
        quality: ??(0-100)
        work: ???[?N?G???A(width*height*3)?K?v
    ???l?F
        ???k(ODH)?T?C?Y 0????s
/********************************************************************/
int ODHEncodeY8U8V8(unsigned char *src, unsigned char *dst, int width, int height, int limit, int quality, unsigned char *work)
{
    CArGBAOdh jpg;
    int result;

    result = jpg.compressGbaOdh(src, dst, width, height, quality, (u32)limit, work, Y8U8V8);
    return (result);
}

/********************************************************************
    ODHDecodeRGB565 - ODH?f?[?^??RGB565?`????W?J????

    ?????F
        src : ODH?f?[?^????????o?b?t?@
        dst : ?W?J?????????????o?b?t?@(width*height*2)?K?v
        work: ???[?N?G???A(width*height*3)?K?v
    ???l?F
        ????c???????	(height<<16)|width 0????s
/********************************************************************/
int ODHDecodeRGB565(unsigned char *src, unsigned char *dst, unsigned char *work)
{
    CArGBAOdh jpg;
    int result;

    result = jpg.decompressGbaOdh(src, dst, work, RGB565);
    return (result);
}

/********************************************************************
    ODHDecodeRGBA8 - ODH?f?[?^??RGBA8?`????W?J????

    ?????F
        src : ODH?f?[?^????????o?b?t?@
        dst : ?W?J?????????????o?b?t?@(width*height*4)?K?v
        work: ???[?N?G???A(width*height*3)?K?v
    ???l?F
        ????c???????	(height<<16)|width 0????s
/********************************************************************/
int ODHDecodeRGBA8(unsigned char *src, unsigned char *dst, unsigned char *work)
{
    CArGBAOdh jpg;
    int result;

    result = jpg.decompressGbaOdh(src, dst, work, RGBA8);
    return (result);
}

/********************************************************************
    ODHDecodeY8U8V8 - ODH?f?[?^??Y8U8V8?`????W?J????

    ?????F
        src : ODH?f?[?^????????o?b?t?@
        dst : ?W?J?????????????o?b?t?@(width*height*3)?K?v
        work: ???[?N?G???A(width*height*3)?K?v
    ???l?F
        ????c???????	(height<<16)|width 0????s
/********************************************************************/
int ODHDecodeY8U8V8(unsigned char *src, unsigned char *dst, unsigned char *work)
{
    CArGBAOdh jpg;
    int result;

    result = jpg.decompressGbaOdh(src, dst, work, Y8U8V8);
    return (result);
}

/********************************************************************/
/*          ODH?f?[?^????RGB565???f?[?^?????????            */
/********************************************************************/
// ??????o?b?t?@?i240?~160?????112.5KByte?j???w??????????
// ????*work??n???????
int CArGBAOdh::decompressGbaOdh(u8 *src, u8 *dest, u8 *work, int format)
{
    u32 result;
    int width, height;
    SArCDJ_OdhMaster cdjMasterCtrl; // ODH???C?u????????\????

    // ODH???f?[?^????E????????????W??{???????O??
    width = getOdhWidth(src);
    height = getOdhHeight(src);
    width = (width + 7) & 0x7F8;
    height = (height + 7) & 0x7F8;

    /*** ODH?L?? ***/
    result = cdj_d_initializeDecompressOdh(&cdjMasterCtrl, work, src);
    if (result)
    {
        printf("decompressGbaOdh : INITIALIZE ERROR %08x\n", result);
        return 0;
    }

    result = cdj_d_decompressLoop(&cdjMasterCtrl);
    if (result)
    {
        printf("decompressGbaOdh : DECOMPRESSING ERROR %08x\n", result);
        return 0;
    }

    result = cdj_d_colorDeconv(&cdjMasterCtrl, dest, format);
    if (result)
    {
        printf("decompressGbaOdh : COLOR DECONVERSION ERROR %08x\n", result);
        return 0;
    }

    return (int)((height << 16) | width);
}

/********************************************************************/
/*          RGB565???f?[?^????ODH?f?[?^?????????            */
/********************************************************************/
// ????sizeLimit??f?t?H???g?l???????????A?????w?????????0????B
// ??????A???k????????T?C?Y??????A?????l????????B
// (?????T?C?Y)?|(?v???t?B?[???f?[?^?T?C?Y)
// ??????o?b?t?@?i240?~160?????112.5KByte?j???w??????????
// ????work??n???????
int CArGBAOdh::compressGbaOdh(u8 *src, u8 *dest, int width, int height, int quality, u32 sizeLimit, u8 *work, int format)
{
    u32 result;
    u16 imgSize[2];
    int i, trueWidth, trueHeight;
    SArCDJ_OdhMaster cdjMasterCtrl; // ODH???C?u????????\????

    // ????sizeLimit???^?????????????
    // ?????iRGB565?j??e?????e??????B?v???t?B?[????e???????
    // ?i??^??v???t?B?[???f?[?^??ODH?N???X?????O????j
    if (sizeLimit == 0)
    {
        sizeLimit = (u32)(width * height * 2);
    }

    // ODH???f?[?^????E????????????W??{???????O??
    trueWidth = (width + 7) & 0x7F8;
    trueHeight = (height + 7) & 0x7F8;

    imgSize[0] = (u16)width;
    imgSize[1] = (u16)height;
    // CodeBufferSize???A?w?????l?????????
    result = cdj_c_initializeCompressOdh(
        &cdjMasterCtrl, imgSize, (u8)quality, work, dest, sizeLimit);
    if (result)
    {
        printf("compressGbaOdh : INITIALIZE ERROR %08x\n", result);
        return 0;
    }

    result = cdj_c_colorConv(&cdjMasterCtrl, src, format);
    if (result)
    {
        printf("compressGbaOdh : COLOR CONVERSION ERROR %08x\n", result);
        return 0;
    }

    result = cdj_c_compressLoop(&cdjMasterCtrl);
    i = 1;
    while (result == DArCDJRESULT_ERR_CODE_SIZE)
    {
        // ???k?f?[?^??f?[?^????o?b?t?@?e????z????????
        // ???????A???k?i????????????k????????
        printf("compressGbaOdh : COMPRESS OVER AND RETRY %d q=%d, %08x\n", i++, cdjMasterCtrl.mQuality, result);

        quality -= 5;
        if (quality > 0)
        {
            result = cdj_c_initializeCompressOdh(
                &cdjMasterCtrl, imgSize, (u8)quality, work, dest, sizeLimit);
            if (result)
            {
                printf("compressGbaOdh : INITIALIZE ERROR2 %08x\n", result);
                return 0;
            }

            result = cdj_c_compressLoop(&cdjMasterCtrl);
        }
        else
        {
            printf("compressGbaOdh : COMPRESSING ERROR %08x\n", result);
            return 0;
        }
    }

    return (int)result;
}

/********************************************************************/
/*                     ?G???R?[?_?????????                         */
/********************************************************************/
u32 CArGBAOdh::cdj_c_initializeCompressOdh(
    SArCDJ_OdhMaster *cdj_ctrl, u16 *imageSize, u8 quality,
    u8 *imgYCbCrBufPtr, u8 *CodeBufPtr, u32 CodeBufSize)
{
    u32 qualityEx;

    /*** ODH?G???R?[?h?O???x??K?v??????????? ***/
    /* ???T?C?Y????? */
    if ((!imageSize[DArCDJ_AXIS_X]) || (imageSize[DArCDJ_AXIS_X] > DArCDJ_PIXEL_SIZE_MAX_X) ||
        (!imageSize[DArCDJ_AXIS_Y]) || (imageSize[DArCDJ_AXIS_Y] > DArCDJ_PIXEL_SIZE_MAX_Y))
    {
        return DArCDJRESULT_ERR_SIZE;
    }

    /* ?i??????? */
    if (quality > 100)
    {
        return DArCDJRESULT_ERR_QUALITY;
    }

    /* YCbCr???o?b?t?@?o?^ */
    cdj_ctrl->mImgYCbCrBufferPtr = imgYCbCrBufPtr;
    /* ?????o?b?t?@?o?^ */
    cdj_ctrl->mCodeBufferPtr = CodeBufPtr;

    /* ????\?????????????s?? */
    cdj_ctrl->mImageSize[DArCDJ_AXIS_X] = imageSize[DArCDJ_AXIS_X];
    cdj_ctrl->mImageSize[DArCDJ_AXIS_Y] = imageSize[DArCDJ_AXIS_Y];
    cdj_ctrl->mQuality = quality;

    /* ?K?v??MCU???????v?Z???? */
    cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] =
        (u16)((imageSize[DArCDJ_AXIS_X] - 1) / DArCDJ_C_MCU_SIZE_X + 1);
    cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y] =
        (u16)((imageSize[DArCDJ_AXIS_Y] - 1) / DArCDJ_C_MCU_SIZE_Y + 1);

    // ?n?t?}???????p?\???????????
    cdj_ctrl->mHuffmanRequest[0].mPreviousDC[0] = cdj_ctrl->mPreviousDC;
    cdj_ctrl->mHuffmanRequest[0].mPreviousDC[1] = cdj_ctrl->mPreviousDC;
    cdj_ctrl->mHuffmanRequest[0].mDCTable = gArDC_L_Table;
    cdj_ctrl->mHuffmanRequest[0].mACTable = gArDC_L_Table; // gArAC_L_Table;
    cdj_ctrl->mHuffmanRequest[0].mCodeBuffer = cdj_ctrl->mCodeBufferPtr;
    cdj_ctrl->mHuffmanRequest[0].mCodeBufferRemain = &cdj_ctrl->mRemainCodeBuffer;
    cdj_ctrl->mHuffmanRequest[0].mHuffmanBuffer = &cdj_ctrl->mHuffmanBuffer;
    cdj_ctrl->mHuffmanRequest[0].mHuffmanBufferRemain = &cdj_ctrl->mHuffmanBits;
    cdj_ctrl->mHuffmanRequest[0].mCodeBufferSize = CodeBufSize;
    cdj_ctrl->mHuffmanRequest[1].mPreviousDC[0] = cdj_ctrl->mPreviousDC + 1;
    cdj_ctrl->mHuffmanRequest[1].mPreviousDC[1] = cdj_ctrl->mPreviousDC + 2;
    cdj_ctrl->mHuffmanRequest[1].mDCTable = gArDC_C_Table;
    cdj_ctrl->mHuffmanRequest[1].mACTable = gArDC_L_Table; // gArAC_C_Table;
    cdj_ctrl->mHuffmanRequest[1].mCodeBuffer = cdj_ctrl->mCodeBufferPtr;
    cdj_ctrl->mHuffmanRequest[1].mCodeBufferRemain = &cdj_ctrl->mRemainCodeBuffer;
    cdj_ctrl->mHuffmanRequest[1].mHuffmanBuffer = &cdj_ctrl->mHuffmanBuffer;
    cdj_ctrl->mHuffmanRequest[1].mHuffmanBufferRemain = &cdj_ctrl->mHuffmanBits;
    cdj_ctrl->mHuffmanRequest[1].mCodeBufferSize = CodeBufSize;

    // ??q???e?[?u????????s??
    qualityEx = (u32)quality;
    if (qualityEx <= 0)
        qualityEx = 1;
    if (qualityEx < 50)
        qualityEx = (u32)(5000 / qualityEx);
    else
        qualityEx = (u32)(200 - qualityEx * 2);

    cdj_c_setQuantizationTable(cdj_ctrl, qualityEx);

    /*** ODH?G???R?[?h?O?????K?v??????????? ***/
    /* ???????????MCU???u???n?_(0,0)????? */
    cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] = 0;
    cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] = 0;

    /* ?n?t?}???o??v?[???o?b?t?@???N???A???? */
    cdj_ctrl->mHuffmanBuffer = 0;
    /* ?n?t?}???o??v?[???o?b?t?@??c??r?b?g????32??????? */
    cdj_ctrl->mHuffmanBits = 32;
    /* ?????o?b?t?@?T?C?Y???Z?b?g */
    cdj_ctrl->mCodeBufferSize = CodeBufSize;
    /* ?????o?b?t?@??c?????o?b?t?@???????????? */
    /* ?w?b?_?????????????????????                   */
    cdj_ctrl->mRemainCodeBuffer = CodeBufSize - DArCDJ_HEADER_SIZE;

    // DC????????O??l???N???A
    cdj_ctrl->mPreviousDC[DArCDJ_COLOR_Y] = 0;
    cdj_ctrl->mPreviousDC[DArCDJ_COLOR_Cb] = 0;
    cdj_ctrl->mPreviousDC[DArCDJ_COLOR_Cr] = 0;

    return DArCDJRESULT_SUCCESS;
}

/********************************************************************/
/*                         ODH???k???[?v                           */
/********************************************************************/
u32 CArGBAOdh::cdj_c_compressLoop(SArCDJ_OdhMaster *cdj_ctrl)
{
    u8 *pixBlockPtr;
    int idx, i;
    u32 result;

    /*** ?S?u???b?N??n?????????????s?? ***/
    do
    {

#if (DArCDJ_C_Y_RATE_H == 1) && (DArCDJ_C_Y_RATE_V == 1)
        /*** Y????1?u???b?N??????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        idx = cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] * DArCDJ_DCT_SIZE_1D + cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X];
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]),
                  cdj_ctrl->mQuantizationTable[0]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] =
                cdj_ctrl->mDCTBuffer[64 + i] << 16;
        }
        cdj_ctrl->mDCTBuffer[64] = 0x40004000;

        // ?n?t?}????????
        // HuffmanCoderWram(((u16 *)cdj_ctrl->mDCTBuffer)-1,
        //                 cdj_ctrl->mHuffmanRequest);
        result = huffmanCoder(((u16 *)cdj_ctrl->mDCTBuffer) - 1, cdj_ctrl->mHuffmanRequest);

        // ???k???f?[?^????w????e????z??????????k??f????
        if (result == DArCDJRESULT_ERR_CODE_SIZE)
        {
            return DArCDJRESULT_ERR_CODE_SIZE;
        }

        /*** Cb????????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        idx += DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y];
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]),
                  cdj_ctrl->mQuantizationTable[1]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] =
                cdj_ctrl->mDCTBuffer[64 + i] & 0xffff;
        }

        /*** Cr????????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        idx += DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y];
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]),
                  cdj_ctrl->mQuantizationTable[1]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            // (Cr << 16) | Cb ??????
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] |=
                cdj_ctrl->mDCTBuffer[64 + i] << 16;
        }
        cdj_ctrl->mDCTBuffer[64] = 0x40004000;

        // ?n?t?}????????
        // HuffmanCoderWram((u16 *)cdj_ctrl->mDCTBuffer,
        //                 cdj_ctrl->mHuffmanRequest+1);
        result = huffmanCoder((u16 *)cdj_ctrl->mDCTBuffer, cdj_ctrl->mHuffmanRequest + 1);

        // ???k???f?[?^????w????e????z??????????k??f????
        if (result == DArCDJRESULT_ERR_CODE_SIZE)
        {
            return DArCDJRESULT_ERR_CODE_SIZE;
        }

#elif (DArCDJ_C_Y_RATE_H == 2) && (DArCDJ_C_Y_RATE_V == 1)
        /*** Y??????2?u???b?N??????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        idx = cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] * DArCDJ_DCT_SIZE_1D * 2 + cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2;
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2),
                  cdj_ctrl->mQuantizationTable[0]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] =
                cdj_ctrl->mDCTBuffer[64 + i] & 0xffff;
        }

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr + DArCDJ_DCT_SIZE_1D,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2),
                  cdj_ctrl->mQuantizationTable[0]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            // (Y1 << 16) | Y0 ??????
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] |=
                cdj_ctrl->mDCTBuffer[64 + i] << 16;
        }
        cdj_ctrl->mDCTBuffer[64] = 0x40004000;

        // ?n?t?}????????
        // HuffmanCoderWram((u16 *)cdj_ctrl->mDCTBuffer,
        //                 cdj_ctrl->mHuffmanRequest);
        result = huffmanCoder((u16 *)cdj_ctrl->mDCTBuffer, cdj_ctrl->mHuffmanRequest);

        // ???k???f?[?^????w????e????z??????????k??f????
        if (result == DArCDJRESULT_ERR_CODE_SIZE)
        {
            return DArCDJRESULT_ERR_CODE_SIZE;
        }

        /*** Cb????????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        idx >>= 1;
        idx += DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2 * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y];
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]),
                  cdj_ctrl->mQuantizationTable[1]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] =
                cdj_ctrl->mDCTBuffer[64 + i] & 0xffff;
        }

        /*** Cr????????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        idx += DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2 * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y];
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]),
                  cdj_ctrl->mQuantizationTable[1]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            // (Cr << 16) | Cb ??????
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] |=
                cdj_ctrl->mDCTBuffer[64 + i] << 16;
        }
        cdj_ctrl->mDCTBuffer[64] = 0x40004000;

        // ?n?t?}????????
        // HuffmanCoderWram((u16 *)cdj_ctrl->mDCTBuffer,
        //                 cdj_ctrl->mHuffmanRequest+1);
        result = huffmanCoder((u16 *)cdj_ctrl->mDCTBuffer, cdj_ctrl->mHuffmanRequest + 1);

        // ???k???f?[?^????w????e????z??????????k??f????
        if (result == DArCDJRESULT_ERR_CODE_SIZE)
        {
            return DArCDJRESULT_ERR_CODE_SIZE;
        }

#elif (DArCDJ_C_Y_RATE_H == 1) && (DArCDJ_C_Y_RATE_V == 2)
        /*** Y?????c2?u???b?N??????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        idx = cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] * DArCDJ_DCT_SIZE_1D + cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2;
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]),
                  cdj_ctrl->mQuantizationTable[0]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] =
                cdj_ctrl->mDCTBuffer[64 + i] & 0xffff;
        }

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X],
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]),
                  cdj_ctrl->mQuantizationTable[0]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            // (Y1 << 16) | Y0 ??????
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] |=
                cdj_ctrl->mDCTBuffer[64 + i] << 16;
        }
        cdj_ctrl->mDCTBuffer[64] = 0x40004000;

        // ?n?t?}????????
        // HuffmanCoderWram((u16 *)cdj_ctrl->mDCTBuffer,
        //                 cdj_ctrl->mHuffmanRequest);
        result = huffmanCoder((u16 *)cdj_ctrl->mDCTBuffer, cdj_ctrl->mHuffmanRequest);

        // ???k???f?[?^????w????e????z??????????k??f????
        if (result == DArCDJRESULT_ERR_CODE_SIZE)
        {
            return DArCDJRESULT_ERR_CODE_SIZE;
        }

        /*** Cb????????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        idx = cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] * DArCDJ_DCT_SIZE_1D + cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y] * 2;
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]),
                  cdj_ctrl->mQuantizationTable[1]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] =
                cdj_ctrl->mDCTBuffer[64 + i] & 0xffff;
        }

        /*** Cr????????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        idx += DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y] * 2;
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]),
                  cdj_ctrl->mQuantizationTable[1]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            // (Cr << 16) | Cb ??????
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] |=
                cdj_ctrl->mDCTBuffer[64 + i] << 16;
        }
        cdj_ctrl->mDCTBuffer[64] = 0x40004000;

        // ?n?t?}????????
        // HuffmanCoderWram((u16 *)cdj_ctrl->mDCTBuffer,
        //                 cdj_ctrl->mHuffmanRequest+1);
        result = huffmanCoder((u16 *)cdj_ctrl->mDCTBuffer, cdj_ctrl->mHuffmanRequest + 1);

        // ???k???f?[?^????w????e????z??????????k??f????
        if (result == DArCDJRESULT_ERR_CODE_SIZE)
        {
            return DArCDJRESULT_ERR_CODE_SIZE;
        }

#elif (DArCDJ_C_Y_RATE_H == 2) && (DArCDJ_C_Y_RATE_V == 2)
        /*** Y??????2?u???b?N???i???j????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        idx = cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] * DArCDJ_DCT_SIZE_1D * 2 + cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 4;
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2),
                  cdj_ctrl->mQuantizationTable[0]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] =
                cdj_ctrl->mDCTBuffer[64 + i] & 0xffff;
        }

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr + DArCDJ_DCT_SIZE_1D,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2),
                  cdj_ctrl->mQuantizationTable[0]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            // (Y1 << 16) | Y0 ??????
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] |=
                cdj_ctrl->mDCTBuffer[64 + i] << 16;
        }
        cdj_ctrl->mDCTBuffer[64] = 0x40004000;

        // ?n?t?}????????
        // HuffmanCoderWram((u16 *)cdj_ctrl->mDCTBuffer,
        //                 cdj_ctrl->mHuffmanRequest);
        result = huffmanCoder((u16 *)cdj_ctrl->mDCTBuffer, cdj_ctrl->mHuffmanRequest);

        // ???k???f?[?^????w????e????z??????????k??f????
        if (result == DArCDJRESULT_ERR_CODE_SIZE)
        {
            return DArCDJRESULT_ERR_CODE_SIZE;
        }

        /*** Y??????2?u???b?N???i?????j????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        // idx = cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X]*DArCDJ_DCT_SIZE_1D*2+cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y]*DArCDJ_DCT_SIZE_2D*cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]*4+DArCDJ_DCT_SIZE_2D*cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]*2;
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2),
                  cdj_ctrl->mQuantizationTable[0]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] =
                cdj_ctrl->mDCTBuffer[64 + i] & 0xffff;
        }

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr + DArCDJ_DCT_SIZE_1D,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2),
                  cdj_ctrl->mQuantizationTable[0]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            // (Y3 << 16) | Y2 ??????
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] |=
                cdj_ctrl->mDCTBuffer[64 + i] << 16;
        }
        cdj_ctrl->mDCTBuffer[64] = 0x40004000;

        // ?n?t?}????????
        // HuffmanCoderWram((u16 *)cdj_ctrl->mDCTBuffer,
        //                 cdj_ctrl->mHuffmanRequest);
        result = huffmanCoder((u16 *)cdj_ctrl->mDCTBuffer, cdj_ctrl->mHuffmanRequest);

        // ???k???f?[?^????w????e????z??????????k??f????
        if (result == DArCDJRESULT_ERR_CODE_SIZE)
        {
            return DArCDJRESULT_ERR_CODE_SIZE;
        }

        /*** Cb????????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        idx >>= 1;
        idx = cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] * DArCDJ_DCT_SIZE_1D + cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y] * 4;
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]),
                  cdj_ctrl->mQuantizationTable[1]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] =
                cdj_ctrl->mDCTBuffer[64 + i] & 0xffff;
        }

        /*** Cr????????k ***/
        // YCbCr???o?b?t?@????????u???b?N???????
        idx += DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y] * 4;
        pixBlockPtr = &(cdj_ctrl->mImgYCbCrBufferPtr[idx]);

        // DCT
        fdct_fast(&cdj_ctrl->mDCTBuffer[64], pixBlockPtr,
                  (u32)(DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]),
                  cdj_ctrl->mQuantizationTable[1]);

        // ?W?O?U?N?????????s???
        for (i = 0; i < DArCDJ_DCT_SIZE_2D; i++)
        {
            // (Cr << 16) | Cb ??????
            cdj_ctrl->mDCTBuffer[odh_zigzag_order[i]] |=
                cdj_ctrl->mDCTBuffer[64 + i] << 16;
        }
        cdj_ctrl->mDCTBuffer[64] = 0x40004000;

        // ?n?t?}????????
        // HuffmanCoderWram((u16 *)cdj_ctrl->mDCTBuffer,
        //                 cdj_ctrl->mHuffmanRequest+1);
        result = huffmanCoder((u16 *)cdj_ctrl->mDCTBuffer, cdj_ctrl->mHuffmanRequest + 1);

        // ???k???f?[?^????w????e????z??????????k??f????
        if (result == DArCDJRESULT_ERR_CODE_SIZE)
        {
            return DArCDJRESULT_ERR_CODE_SIZE;
        }
#endif

        // ????MCU???????i???
        cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X]++;
        if (cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] == cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X])
        {
            cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y]++;
            cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] = 0;
        }
    } while (cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] < cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y]);

    // ?o?b?t?@??c?????f?[?^??????o?????s??
    result = cdj_c_flashBuffer(cdj_ctrl);
    // ???k???f?[?^????w????e????z??????????k??f????
    if (result == DArCDJRESULT_ERR_CODE_SIZE)
    {
        return DArCDJRESULT_ERR_CODE_SIZE;
    }

    // GBA?pODH???w?b?_????
    cdj_c_makeHeader(cdj_ctrl, cdj_ctrl->mCodeBufferSize - cdj_ctrl->mRemainCodeBuffer);

    // ???k???f?[?^?T?C?Y????
    return (cdj_ctrl->mCodeBufferSize - cdj_ctrl->mRemainCodeBuffer);
}

/********************************************************************/
/*                ?n?t?}???o?b?t?@?t???b?V?????                    */
/********************************************************************/
u32 CArGBAOdh::cdj_c_flashBuffer(SArCDJ_OdhMaster *cdj_ctrl)
{

    /* ?n?t?}???o?b?t?@????e???????f???o?????? */
    /* 1?o?C?g?????????f?[?^??f???o?????????'1'???l??? */
    cdj_ctrl->mHuffmanBuffer |= (0x7F << (cdj_ctrl->mHuffmanBits - 7));
    cdj_ctrl->mHuffmanBits -= 7;
    /* ?n?t?}???o?b?t?@????X?g???[???o?b?t?@???f?[?^?????o?? */
    while (cdj_ctrl->mHuffmanBits <= 24)
    {
        if (cdj_ctrl->mRemainCodeBuffer == 0)
        {
            // ?w??w?k????e??I?[?o?[
            return DArCDJRESULT_ERR_CODE_SIZE;
        }

        (cdj_ctrl->mCodeBufferPtr)[cdj_ctrl->mCodeBufferSize - cdj_ctrl->mRemainCodeBuffer] =
            (u8)(cdj_ctrl->mHuffmanBuffer >> 24);
        cdj_ctrl->mRemainCodeBuffer--;
        cdj_ctrl->mHuffmanBits += 8;
        cdj_ctrl->mHuffmanBuffer <<= 8;
    }

    // ?t?@?C???T?C?Y??4?o?C?g??{???????????????
    while ((cdj_ctrl->mCodeBufferSize - cdj_ctrl->mRemainCodeBuffer) & 0x3)
    {
        (cdj_ctrl->mCodeBufferPtr)[cdj_ctrl->mCodeBufferSize - cdj_ctrl->mRemainCodeBuffer] = 0xff;
        cdj_ctrl->mRemainCodeBuffer--;
    }

    return 0;
}

/********************************************************************/
/*                    ??q???e?[?u???????                        */
/********************************************************************/
// ?w???????k?i???l???????q???e?[?u??????????B
// ?E????
// *cdj_ctrl?FODH???C?u????????\??????|?C???^
// quality  ?F???k?i??
void CArGBAOdh::cdj_c_setQuantizationTable(SArCDJ_OdhMaster *cdj_ctrl, u32 quality)
{
    u32 cnt, temp, value, currentTable;
    u8 tempQuantBuffer[DArCDJ_QUANTIZE_TABLE_NUM * DArCDJ_DCT_SIZE_2D];

    /* ??q???e?[?u?????w?????i????X?P?[?????O???? */
    for (currentTable = 0; currentTable < DArCDJ_QUANTIZE_TABLE_NUM; currentTable++)
    {
        for (cnt = 0; cnt < DArCDJ_DCT_SIZE_2D; cnt++)
        {
            temp = (u32)((gArCdj_std_quant_tbl[currentTable][cnt] * quality + 50L) / 100L);
            if (temp <= 0L)
                temp = 1L;
            if (temp > 255L)
                temp = 255L;
            /* ???o?b?t?@??l???? */
            tempQuantBuffer[cnt + currentTable * 64] = (u8)temp;
        }
    }

    /* ARM????DCT???C?u?????p???q???e?[?u?????????? */
    for (currentTable = 0; currentTable < DArCDJ_QUANTIZE_TABLE_NUM; currentTable++)
    {
        for (cnt = 0; cnt < DArCDJ_DCT_SIZE_2D; cnt++)
        {
            value = gArAANScales[cnt];
            value *= tempQuantBuffer[cnt + currentTable * 64];
            value = (1 << (14 + 14 - 2)) / (value);
            cdj_ctrl->mQuantizationTable[currentTable][cnt] = value;
        }
    }
}

/********************************************************************/
/*                  GBA?pODH???w?b?_???????                     */
/********************************************************************/
// 8?o?C?g??ODH?w?b?_???????A?????o?b?t?@???????????B
// ?E????
// *cdj_ctrl?FODH???C?u????????\??????|?C???^
// fileSize ?F???k?????f?[?^??
void CArGBAOdh::cdj_c_makeHeader(SArCDJ_OdhMaster *cdj_ctrl, u32 fileSize)
{
    // GBA?pODH???w?b?_????????????o?b?t?@?????i?[????
    // ?t?@?C??????R?[?h
    cdj_ctrl->mCodeBufferPtr[0] = 'A';
    cdj_ctrl->mCodeBufferPtr[1] = 'J';
    cdj_ctrl->mCodeBufferPtr[2] = 'P';
    cdj_ctrl->mCodeBufferPtr[3] = 'G';
    // ?????A???????A?T???v?????O???[?g?A???k?i??
    *(u32 *)(&cdj_ctrl->mCodeBufferPtr[4]) =
        (u32)(cdj_ctrl->mImageSize[DArCDJ_AXIS_X] | (cdj_ctrl->mImageSize[DArCDJ_AXIS_Y] << 11) | (DArCDJ_C_Y_RATE_H - 1 << 22) | (DArCDJ_C_Y_RATE_V - 1 << 23) | (cdj_ctrl->mQuality << 24));
    // ???k???f?[?^?T?C?Y
    *(u32 *)(&cdj_ctrl->mCodeBufferPtr[8]) = fileSize;
    // ?\?????N???A
    *(u32 *)(&cdj_ctrl->mCodeBufferPtr[12]) = 0;
}

/********************************************************************/
/*     RGB?????????YCbCr????A?_?E???T???v?????O???s?????     */
/********************************************************************/
u32 CArGBAOdh::cdj_c_colorConv(SArCDJ_OdhMaster *cdj_ctrl, u8 *imgRGBPtr, int format)
{
    u8 *imgYCbCrPtr;                            // ??????iYCbCr?f?[?^?j?o?b?t?@???|?C???^
    u8 *CbDataPtr, *CrDataPtr;                  // YCbCr???f?[?^????e???????|?C???^
    u16 alignedImgSize[DArCDJ_IMAGE_DIMENSION]; // 8??{???????O??????T?C?Y
    u16 loopImgSize[DArCDJ_IMAGE_DIMENSION];    // ???????[?v??????????T?C?Y
    u16 tmpMod;
    int cntLine; // ?????????????C?????
    int i, imgStep, adrs;

    if ((cdj_ctrl->mImageSize[DArCDJ_AXIS_X] & 1) || (cdj_ctrl->mImageSize[DArCDJ_AXIS_Y] & 1))
    {
        // ????T?C?Y????????????????
        return DArJCOL_RES_ERROR;
    }

    // 8??{???????O?????T?C?Y???????
    for (i = 0; i < DArCDJ_IMAGE_DIMENSION; i++)
    {
        if ((tmpMod = (u16)(cdj_ctrl->mImageSize[i] & 0x7)) != 0)
        {
            // 8??{???T?C?Y??????????
            alignedImgSize[i] = (u16)(cdj_ctrl->mImageSize[i] + 8 - tmpMod);
        }
        else
        {
            // ???X8??{???T?C?Y???
            alignedImgSize[i] = cdj_ctrl->mImageSize[i];
        }
    }

    // Y,Cb,Cr?f?[?^?i?[?J?n??u???|?C???^??????
    imgYCbCrPtr = cdj_ctrl->mImgYCbCrBufferPtr;
    CbDataPtr = imgYCbCrPtr + alignedImgSize[DArCDJ_AXIS_X] * alignedImgSize[DArCDJ_AXIS_Y];
    CrDataPtr = CbDataPtr + alignedImgSize[DArCDJ_AXIS_X] * alignedImgSize[DArCDJ_AXIS_Y];

    // ??????????[?v????T?C?Y??????
    // ??????[?v??????T?C?Y????A??????[?v????????
    // ?p?f?B???O?????????????s???
    loopImgSize[DArCDJ_AXIS_X] = cdj_ctrl->mImageSize[DArCDJ_AXIS_X];
    loopImgSize[DArCDJ_AXIS_Y] = cdj_ctrl->mImageSize[DArCDJ_AXIS_Y];

    if (format == RGB565)
    {
        imgStep = (loopImgSize[DArCDJ_AXIS_X] / 4) * 32;
    }
    else if (format == RGBA8)
    {
        imgStep = (loopImgSize[DArCDJ_AXIS_X] / 4) * 64;
    }
    else
    {
        imgStep = (loopImgSize[DArCDJ_AXIS_X] / 8) * 32;
    }

    /*** RGB???f?[?^???????AYCbCr?`?????????? ***/
    for (cntLine = 0; cntLine < loopImgSize[DArCDJ_AXIS_Y]; cntLine += DArCDJ_C_Y_RATE_V)
    {
        adrs = (cntLine / 4) * imgStep + (cntLine & 3) * 8;

#if (DArCDJ_C_Y_RATE_H == 1) && (DArCDJ_C_Y_RATE_V == 1)
        // 1???C???f?[?^??????A???
        LineConv11(&imgRGBPtr[adrs], imgYCbCrPtr, CbDataPtr, CrDataPtr, cdj_ctrl->mImageSize[DArCDJ_AXIS_X], cdj_ctrl->mImageSize[DArCDJ_AXIS_Y], (s32 *)gArConvPlttTbl, format);

        // YCbCr?o?b?t?@???|?C???^??????i???
        imgYCbCrPtr += alignedImgSize[DArCDJ_AXIS_X];
        CbDataPtr += alignedImgSize[DArCDJ_AXIS_X];
        CrDataPtr += alignedImgSize[DArCDJ_AXIS_X];

#elif (DArCDJ_C_Y_RATE_H == 2) && (DArCDJ_C_Y_RATE_V == 1)
        // 1???C???f?[?^??????A???
        LineConv21(&imgRGBPtr[adrs], imgYCbCrPtr, CbDataPtr, CrDataPtr, cdj_ctrl->mImageSize[DArCDJ_AXIS_X], (s32 *)gArConvPlttTbl, format);
        // YCbCr?o?b?t?@???|?C???^??????i???
        imgYCbCrPtr += alignedImgSize[DArCDJ_AXIS_X];
        CbDataPtr += alignedImgSize[DArCDJ_AXIS_X] >> 1;
        CrDataPtr += alignedImgSize[DArCDJ_AXIS_X] >> 1;

#elif (DArCDJ_C_Y_RATE_H == 1) && (DArCDJ_C_Y_RATE_V == 2)
        // 2???C???f?[?^??????A???
        LineConv12(&imgRGBPtr[adrs], imgYCbCrPtr, CbDataPtr, CrDataPtr, cdj_ctrl->mImageSize[DArCDJ_AXIS_X], (s32 *)gArConvPlttTbl, format);
        // YCbCr?o?b?t?@???|?C???^??????i???
        imgYCbCrPtr += alignedImgSize[DArCDJ_AXIS_X] << 1;
        CbDataPtr += alignedImgSize[DArCDJ_AXIS_X];
        CrDataPtr += alignedImgSize[DArCDJ_AXIS_X];

#elif (DArCDJ_C_Y_RATE_H == 2) && (DArCDJ_C_Y_RATE_V == 2)
        // 2???C???f?[?^??????A???
        LineConv22(&imgRGBPtr[adrs], imgYCbCrPtr, CbDataPtr, CrDataPtr, cdj_ctrl->mImageSize[DArCDJ_AXIS_X], (s32 *)gArConvPlttTbl, format);
        // YCbCr?o?b?t?@???|?C???^??????i???
        imgYCbCrPtr += alignedImgSize[DArCDJ_AXIS_X] << 1;
        CbDataPtr += alignedImgSize[DArCDJ_AXIS_X] >> 1;
        CrDataPtr += alignedImgSize[DArCDJ_AXIS_X] >> 1;
#endif
    }
    return DArJCOL_RES_SUCCESS;
}

#if (DArCDJ_C_Y_RATE_H == 1) && (DArCDJ_C_Y_RATE_V == 1)
void CArGBAOdh::LineConv11(u8 *imgRGBPtr, u8 *YDataPtr, u8 *CbDataPtr, u8 *CrDataPtr, u16 sizeX, u16 sizeY, const s32 *convPlttTbl, int format)
{
    s32 r, g, b, y, cb, cr;
    int i, z, tmp, sizeF;
    float yy, uu, vv;

    sizeF = sizeX * sizeY;
    for (i = 0; i < sizeX; i++)
    {
        // RGB?f?[?^???o
        if (format == RGB565)
        {
            z = (i >> 2) * 32 + (i & 3) * 2;
            tmp = (imgRGBPtr[z] << 8) | imgRGBPtr[z + 1];
            r = (s32)((tmp & 0xF800) >> 11);
            g = (s32)((tmp & 0x07C0) >> 6);
            b = (s32)(tmp & 0x001F);
        }
        else if (format == RGBA8)
        {
            z = (i >> 2) * 64 + (i & 3) * 2;
            r = (s32)(imgRGBPtr[z + 1] >> 3);
            g = (s32)(imgRGBPtr[z + 32] >> 3);
            b = (s32)(imgRGBPtr[z + 33] >> 3);
        }
        else
        {
            z = (i >> 3) * 32 + (i & 7);
            yy = (float)imgRGBPtr[z] - 16.0F;
            uu = (float)imgRGBPtr[z + sizeF] - 128.0F;
            vv = (float)imgRGBPtr[z + sizeF * 2] - 128.0F;
            r = (s32)(1.16438355F * yy + 1.59602715F * vv);
            g = (s32)(1.16438355F * yy - 0.3917616F * uu - 0.81296805F * vv);
            b = (s32)(1.16438355F * yy + 2.01723105 * uu);
            if (r < 0)
                r = 0;
            if (r > 255)
                r = 255;
            if (g < 0)
                g = 0;
            if (g > 255)
                g = 255;
            if (b < 0)
                b = 0;
            if (b > 255)
                b = 255;
            r >>= 3;
            g >>= 3;
            b >>= 3;
            //			y=imgRGBPtr[z];
            //			cb=imgRGBPtr[z+sizeF];
            //			cr=imgRGBPtr[z+sizeF*2];
        }

        // YCbCr?`??????
        y = (convPlttTbl[r] + convPlttTbl[32 + g] + convPlttTbl[64 + b]) >> DArSCALEBITS;
        cb = (convPlttTbl[DArCONV_TBL_IDX_Cb_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cb_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cb_B + b]) >> DArSCALEBITS;
        cr = (convPlttTbl[DArCONV_TBL_IDX_Cr_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cr_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cr_B + b]) >> DArSCALEBITS;

        // ?????YCbCr?l???o?b?t?@??i?[???A?o?b?t?@??|?C???^??i???
        *YDataPtr = (u8)y;
        *CbDataPtr = (u8)cb;
        *CrDataPtr = (u8)cr;
        YDataPtr++;
        CbDataPtr++;
        CrDataPtr++;
    }
}

#elif (DArCDJ_C_Y_RATE_H == 2) && (DArCDJ_C_Y_RATE_V == 1)
void CArGBAOdh::LineConv21(u8 *imgRGBPtr, u8 *YDataPtr, u8 *CbDataPtr, u8 *CrDataPtr, u16 imageSize, const s32 *convPlttTbl, int format)
{
    s32 r, g, b, y, cb, cr;
    int i, z, tmp;

    for (i = 0; i < imageSize; i += 2)
    {
        /*** 1?s?N?Z???????? ***/
        // RGB?f?[?^???o
        if (format == RGB565)
        {
            z = (i >> 2) * 32 + (i & 3) * 2;
            tmp = (imgRGBPtr[z] << 8) | imgRGBPtr[z + 1];
            r = (s32)((tmp & 0xF800) >> 11);
            g = (s32)((tmp & 0x07C0) >> 6);
            b = (s32)(tmp & 0x001F);
        }
        else
        {
            z = (i >> 2) * 64 + (i & 3) * 2;
            r = (s32)(imgRGBPtr[z + 1] >> 3);
            g = (s32)(imgRGBPtr[z + 32] >> 3);
            b = (s32)(imgRGBPtr[z + 33] >> 3);
        }

        // YCbCr?`??????
        y = (convPlttTbl[r] + convPlttTbl[32 + g] + convPlttTbl[64 + b]) >> DArSCALEBITS;
        cb = convPlttTbl[DArCONV_TBL_IDX_Cb_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cb_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cb_B + b];
        cr = convPlttTbl[DArCONV_TBL_IDX_Cr_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cr_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cr_B + b];

        // Y?????l?????o?b?t?@??i?[???A?o?b?t?@??|?C???^??i???
        *YDataPtr = (u8)y;
        YDataPtr++;

        /*** 2?s?N?Z???????? ***/
        // RGB?f?[?^???o
        if (format == RGB565)
        {
            tmp = (imgRGBPtr[z + 2] << 8) | imgRGBPtr[z + 3];
            r = (s32)((tmp & 0xF800) >> 11);
            g = (s32)((tmp & 0x07C0) >> 6);
            b = (s32)(tmp & 0x001F);
        }
        else
        {
            r = (s32)(imgRGBPtr[z + 3] >> 3);
            g = (s32)(imgRGBPtr[z + 34] >> 3);
            b = (s32)(imgRGBPtr[z + 35] >> 3);
        }

        // YCbCr?`??????
        y = (convPlttTbl[r] + convPlttTbl[32 + g] + convPlttTbl[64 + b]) >> DArSCALEBITS;
        cb += convPlttTbl[DArCONV_TBL_IDX_Cb_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cb_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cb_B + b];
        cr += convPlttTbl[DArCONV_TBL_IDX_Cr_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cr_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cr_B + b];

        // ?????YCbCr?l???o?b?t?@??i?[???A?o?b?t?@??|?C???^??i???
        // CbCr??????2?s?N?Z?????????l?????
        *YDataPtr = (u8)y;
        *CbDataPtr = (u8)(cb >> (DArSCALEBITS + 1));
        *CrDataPtr = (u8)(cr >> (DArSCALEBITS + 1));
        YDataPtr++;
        CbDataPtr++;
        CrDataPtr++;
    }
}

#elif (DArCDJ_C_Y_RATE_H == 1) && (DArCDJ_C_Y_RATE_V == 2)
void CArGBAOdh::LineConv12(u8 *imgRGBPtr, u8 *YDataPtr, u8 *CbDataPtr, u8 *CrDataPtr, u16 imageSize, const s32 *convPlttTbl, int format)
{
    u8 *YDataPtr2; // ?????????_?E???T???v?????O????YCbCr?o?b?t?@2???C?????|?C???^
    s32 r, g, b, y, cb, cr;
    int i, z, tmp;

    // ?e?o?b?t?@2???C??????|?C???^????
    if ((tmp = (imageSize & 0x7)) != 0)
    {
        // ??????8??{????????????C?????I?t?Z?b?g??8??{???????O??
        YDataPtr2 = YDataPtr + imageSize + (8 - tmp);
    }
    else
    {
        YDataPtr2 = YDataPtr + imageSize;
    }

    // 2???C???f?[?^??????A???
    for (i = 0; i < imageSize; i++)
    {
        /*** 1???C???????? ***/
        // RGB?f?[?^???o
        if (format == RGB565)
        {
            z = (i >> 2) * 32 + (i & 3) * 2;
            tmp = (imgRGBPtr[z] << 8) | imgRGBPtr[z + 1];
            r = (s32)((tmp & 0xF800) >> 11);
            g = (s32)((tmp & 0x07C0) >> 6);
            b = (s32)(tmp & 0x001F);
        }
        else
        {
            z = (i >> 2) * 64 + (i & 3) * 2;
            r = (s32)(imgRGBPtr[z + 1] >> 3);
            g = (s32)(imgRGBPtr[z + 32] >> 3);
            b = (s32)(imgRGBPtr[z + 33] >> 3);
        }

        // YCbCr?`??????
        y = (convPlttTbl[r] + convPlttTbl[32 + g] + convPlttTbl[64 + b]) >> DArSCALEBITS;
        cb = convPlttTbl[DArCONV_TBL_IDX_Cb_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cb_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cb_B + b];
        cr = convPlttTbl[DArCONV_TBL_IDX_Cr_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cr_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cr_B + b];

        // Y?????l?????o?b?t?@??i?[???A?o?b?t?@??|?C???^??i????
        *YDataPtr = (u8)y;

        /*** 2???C???????? ***/
        // ???????????????
        // RGB?f?[?^???o
        if (format == RGB565)
        {
            tmp = (imgRGBPtr[z + 8] << 8) | imgRGBPtr[z + 9];
            r = (s32)((tmp & 0xF800) >> 11);
            g = (s32)((tmp & 0x07C0) >> 6);
            b = (s32)(tmp & 0x001F);
        }
        else
        {
            r = (s32)(imgRGBPtr[z + 9] >> 3);
            g = (s32)(imgRGBPtr[z + 40] >> 3);
            b = (s32)(imgRGBPtr[z + 41] >> 3);
        }

        // YCbCr?`??????
        y = (convPlttTbl[r] + convPlttTbl[32 + g] + convPlttTbl[64 + b]) >> DArSCALEBITS;
        cb += convPlttTbl[DArCONV_TBL_IDX_Cb_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cb_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cb_B + b];
        cr += convPlttTbl[DArCONV_TBL_IDX_Cr_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cr_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cr_B + b];

        // ?????YCbCr?l???o?b?t?@??i?[???A?o?b?t?@??|?C???^??i???
        // CbCr??????2?s?N?Z?????????l?????
        *YDataPtr2 = (u8)y;
        *CbDataPtr = (u8)(cb >> (DArSCALEBITS + 1));
        *CrDataPtr = (u8)(cr >> (DArSCALEBITS + 1));
        YDataPtr++;
        YDataPtr2++;
        CbDataPtr++;
        CrDataPtr++;
    }
}

#elif (DArCDJ_C_Y_RATE_H == 2) && (DArCDJ_C_Y_RATE_V == 2)
void CArGBAOdh::LineConv22(u8 *imgRGBPtr, u8 *YDataPtr, u8 *CbDataPtr, u8 *CrDataPtr, u16 imageSize, const s32 *convPlttTbl, int format)
{
    u8 *YDataPtr2; // ?????????_?E???T???v?????O????YCbCr?o?b?t?@2???C?????|?C???^
    s32 r, g, b, y, cb, cr;
    int i, z, tmp;

    // ?e?o?b?t?@2???C??????|?C???^????
    if ((tmp = (imageSize & 0x7)) != 0)
    {
        // ??????8??{????????????C?????I?t?Z?b?g??8??{???????O??
        YDataPtr2 = YDataPtr + imageSize + (8 - tmp);
    }
    else
    {
        YDataPtr2 = YDataPtr + imageSize;
    }

    // 2???C???f?[?^??????A???
    for (i = 0; i < imageSize; i += 2)
    {
        /*** 1-1?s?N?Z??????? ***/
        // RGB?f?[?^???o
        if (format == RGB565)
        {
            z = (i >> 2) * 32 + (i & 3) * 2;
            tmp = (imgRGBPtr[z] << 8) | imgRGBPtr[z + 1];
            r = (s32)((tmp & 0xF800) >> 11);
            g = (s32)((tmp & 0x07C0) >> 6);
            b = (s32)(tmp & 0x001F);
        }
        else
        {
            z = (i >> 2) * 64 + (i & 3) * 2;
            r = (s32)(imgRGBPtr[z + 1] >> 3);
            g = (s32)(imgRGBPtr[z + 32] >> 3);
            b = (s32)(imgRGBPtr[z + 33] >> 3);
        }

        // YCbCr?`??????
        y = (convPlttTbl[r] + convPlttTbl[32 + g] + convPlttTbl[64 + b]) >> DArSCALEBITS;
        cb = convPlttTbl[DArCONV_TBL_IDX_Cb_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cb_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cb_B + b];
        cr = convPlttTbl[DArCONV_TBL_IDX_Cr_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cr_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cr_B + b];

        // Y?????l?????o?b?t?@??i?[???A?o?b?t?@??|?C???^??i????
        *YDataPtr = (u8)y;

        /*** 1-2?s?N?Z??????? ***/
        // ?????????????
        // RGB?f?[?^???o
        if (format == RGB565)
        {
            tmp = (imgRGBPtr[z + 2] << 8) | imgRGBPtr[z + 3];
            r = (s32)((tmp & 0xF800) >> 11);
            g = (s32)((tmp & 0x07C0) >> 6);
            b = (s32)(tmp & 0x001F);
        }
        else
        {
            r = (s32)(imgRGBPtr[z + 3] >> 3);
            g = (s32)(imgRGBPtr[z + 34] >> 3);
            b = (s32)(imgRGBPtr[z + 35] >> 3);
        }

        // YCbCr?`??????
        y = (convPlttTbl[r] + convPlttTbl[32 + g] + convPlttTbl[64 + b]) >> DArSCALEBITS;
        cb += convPlttTbl[DArCONV_TBL_IDX_Cb_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cb_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cb_B + b];
        cr += convPlttTbl[DArCONV_TBL_IDX_Cr_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cr_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cr_B + b];

        // Y?????l?????o?b?t?@??i?[???A?o?b?t?@??|?C???^??i????
        *(YDataPtr + 1) = (u8)y;

        /*** 2-1?s?N?Z??????? ***/
        // RGB?f?[?^???o
        if (format == RGB565)
        {
            tmp = (imgRGBPtr[z + 8] << 8) | imgRGBPtr[z + 9];
            r = (s32)((tmp & 0xF800) >> 11);
            g = (s32)((tmp & 0x07C0) >> 6);
            b = (s32)(tmp & 0x001F);
        }
        else
        {
            r = (s32)(imgRGBPtr[z + 9] >> 3);
            g = (s32)(imgRGBPtr[z + 40] >> 3);
            b = (s32)(imgRGBPtr[z + 41] >> 3);
        }

        // YCbCr?`??????
        y = (convPlttTbl[r] + convPlttTbl[32 + g] + convPlttTbl[64 + b]) >> DArSCALEBITS;
        cb += convPlttTbl[DArCONV_TBL_IDX_Cb_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cb_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cb_B + b];
        cr += convPlttTbl[DArCONV_TBL_IDX_Cr_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cr_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cr_B + b];

        // Y?????l?????o?b?t?@??i?[???A?o?b?t?@??|?C???^??i????
        *YDataPtr2 = (u8)y;

        /*** 2-2?s?N?Z??????? ***/
        // RGB?f?[?^???o
        if (format == RGB565)
        {
            tmp = (imgRGBPtr[z + 10] << 8) | imgRGBPtr[z + 11];
            r = (s32)((tmp & 0xF800) >> 11);
            g = (s32)((tmp & 0x07C0) >> 6);
            b = (s32)(tmp & 0x001F);
        }
        else
        {
            r = (s32)(imgRGBPtr[z + 11] >> 3);
            g = (s32)(imgRGBPtr[z + 42] >> 3);
            b = (s32)(imgRGBPtr[z + 43] >> 3);
        }

        // YCbCr?`??????
        y = (convPlttTbl[r] + convPlttTbl[32 + g] + convPlttTbl[64 + b]) >> DArSCALEBITS;
        cb += convPlttTbl[DArCONV_TBL_IDX_Cb_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cb_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cb_B + b];
        cr += convPlttTbl[DArCONV_TBL_IDX_Cr_R + r] + convPlttTbl[DArCONV_TBL_IDX_Cr_G + g] + convPlttTbl[DArCONV_TBL_IDX_Cr_B + b];

        // ?????YCbCr?l???o?b?t?@??i?[???A?o?b?t?@??|?C???^??i???
        // CbCr??????4?s?N?Z?????????l?????
        *(YDataPtr2 + 1) = (u8)y;
        *CbDataPtr = (u8)(cb >> (DArSCALEBITS + 2));
        *CrDataPtr = (u8)(cr >> (DArSCALEBITS + 2));
        YDataPtr += 2;
        YDataPtr2 += 2;
        CbDataPtr++;
        CrDataPtr++;
    }
}
#endif

/********************************************************************/
/*                          ????DCT???                             */
/********************************************************************/
void CArGBAOdh::fdct_fast(u32 *dctBlock, u8 *pixelBuffer, u32 pixelOffset, u32 *quantTable)
{
    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int tmp10, tmp11, tmp12, tmp13;
    int z1, z2, z3, z4, z5, z11, z13;
    int ctr;
    int *dataptr;
    int i, j;

    // ???DCT?u???b?N??K?v??f?[?^???R?s?[????
    for (i = 0; i < DArCDJ_DCT_SIZE_1D; i++)
    {
        for (j = 0; j < DArCDJ_DCT_SIZE_1D; j++)
        {
            dctBlock[i + j * DArCDJ_DCT_SIZE_1D] = pixelBuffer[i + j * pixelOffset];
            dctBlock[i + j * DArCDJ_DCT_SIZE_1D] -= 128;
        }
    }

    /* Pass 1: process rows. */

    dataptr = (int *)dctBlock;
    for (ctr = DArCDJ_DCT_SIZE_1D; ctr > 0; ctr--)
    {
        tmp0 = dataptr[0] + dataptr[7];
        tmp7 = dataptr[0] - dataptr[7];
        tmp1 = dataptr[1] + dataptr[6];
        tmp6 = dataptr[1] - dataptr[6];
        tmp2 = dataptr[2] + dataptr[5];
        tmp5 = dataptr[2] - dataptr[5];
        tmp3 = dataptr[3] + dataptr[4];
        tmp4 = dataptr[3] - dataptr[4];

        /* Even part */

        tmp10 = tmp0 + tmp3; /* phase 2 */
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        dataptr[0] = tmp10 + tmp11; /* phase 3 */
        dataptr[4] = tmp10 - tmp11;

        // dataptr[0] -= 2;  // bias ?i?V???[?v?\?[?X??????B?e??????j

        z1 = DArMULTIPLY(tmp12 + tmp13, DAr_FIX_0_707106781); /* c4 */
        dataptr[2] = tmp13 + z1;                              /* phase 5 */
        dataptr[6] = tmp13 - z1;

        /* Odd part */

        tmp10 = tmp4 + tmp5; /* phase 2 */
        tmp11 = tmp5 + tmp6;
        tmp12 = tmp6 + tmp7;

        /* The rotator is modified from fig 4-8 to avoid extra negations. */
        z5 = DArMULTIPLY(tmp10 - tmp12, DAr_FIX_0_382683433); /* c6 */
        z2 = DArMULTIPLY(tmp10, DAr_FIX_0_541196100) + z5;    /* c2-c6 */
        z4 = DArMULTIPLY(tmp12, DAr_FIX_1_306562965) + z5;    /* c2+c6 */
        z3 = DArMULTIPLY(tmp11, DAr_FIX_0_707106781);         /* c4 */

        z11 = tmp7 + z3; /* phase 5 */
        z13 = tmp7 - z3;

        dataptr[5] = z13 + z2; /* phase 6 */
        dataptr[3] = z13 - z2;
        dataptr[1] = z11 + z4;
        dataptr[7] = z11 - z4;

        dataptr += DArCDJ_DCT_SIZE_1D; /* advance pointer to next row */
    }

    /* Pass 2: process columns. */

    dataptr = (int *)dctBlock;
    for (ctr = DArCDJ_DCT_SIZE_1D; ctr > 0; ctr--)
    {
        tmp0 = dataptr[DArCDJ_DCT_SIZE_1D * 0] + dataptr[DArCDJ_DCT_SIZE_1D * 7];
        tmp7 = dataptr[DArCDJ_DCT_SIZE_1D * 0] - dataptr[DArCDJ_DCT_SIZE_1D * 7];
        tmp1 = dataptr[DArCDJ_DCT_SIZE_1D * 1] + dataptr[DArCDJ_DCT_SIZE_1D * 6];
        tmp6 = dataptr[DArCDJ_DCT_SIZE_1D * 1] - dataptr[DArCDJ_DCT_SIZE_1D * 6];
        tmp2 = dataptr[DArCDJ_DCT_SIZE_1D * 2] + dataptr[DArCDJ_DCT_SIZE_1D * 5];
        tmp5 = dataptr[DArCDJ_DCT_SIZE_1D * 2] - dataptr[DArCDJ_DCT_SIZE_1D * 5];
        tmp3 = dataptr[DArCDJ_DCT_SIZE_1D * 3] + dataptr[DArCDJ_DCT_SIZE_1D * 4];
        tmp4 = dataptr[DArCDJ_DCT_SIZE_1D * 3] - dataptr[DArCDJ_DCT_SIZE_1D * 4];

        /* Even part */

        tmp10 = tmp0 + tmp3; /* phase 2 */
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        dataptr[DArCDJ_DCT_SIZE_1D * 0] = tmp10 + tmp11; /* phase 3 */
        dataptr[DArCDJ_DCT_SIZE_1D * 4] = tmp10 - tmp11;

        z1 = DArMULTIPLY(tmp12 + tmp13, DAr_FIX_0_707106781); /* c4 */
        dataptr[DArCDJ_DCT_SIZE_1D * 2] = tmp13 + z1;         /* phase 5 */
        dataptr[DArCDJ_DCT_SIZE_1D * 6] = tmp13 - z1;

        /* Odd part */

        tmp10 = tmp4 + tmp5; /* phase 2 */
        tmp11 = tmp5 + tmp6;
        tmp12 = tmp6 + tmp7;

        /* The rotator is modified from fig 4-8 to avoid extra negations. */
        z5 = DArMULTIPLY(tmp10 - tmp12, DAr_FIX_0_382683433); /* c6 */
        z2 = DArMULTIPLY(tmp10, DAr_FIX_0_541196100) + z5;    /* c2-c6 */
        z4 = DArMULTIPLY(tmp12, DAr_FIX_1_306562965) + z5;    /* c2+c6 */
        z3 = DArMULTIPLY(tmp11, DAr_FIX_0_707106781);         /* c4 */

        z11 = tmp7 + z3; /* phase 5 */
        z13 = tmp7 - z3;

        dataptr[DArCDJ_DCT_SIZE_1D * 5] = z13 + z2; /* phase 6 */
        dataptr[DArCDJ_DCT_SIZE_1D * 3] = z13 - z2;
        dataptr[DArCDJ_DCT_SIZE_1D * 1] = z11 + z4;
        dataptr[DArCDJ_DCT_SIZE_1D * 7] = z11 - z4;

        dataptr++; /* advance pointer to next column */
    }

    // ?????????q?????s???
    for (i = 0; i < DArCDJ_DCT_SIZE_1D; i++)
    {
        for (j = 0; j < DArCDJ_DCT_SIZE_1D; j++)
        {
            // ????????????????????A???????DCT?W?????Q?{?????
            // AGB???DCT?v???O????????l??DCT?W???????????

            // ???AGB????t?????????????????????????R?????g?A?E?g
            // ???????g????????p??l??0x8000??A?E?V?t?g?l??16?????
            // dctBlock[i+j*DArCDJ_DCT_SIZE_1D] <<= 1;
            // if (dctBlock[i+j*DArCDJ_DCT_SIZE_1D] < 0){
            //    dctBlock[i+j*DArCDJ_DCT_SIZE_1D] += 1;
            //}

            // ???p??0x4000??1.16.15????????0.5
            dctBlock[i + j * DArCDJ_DCT_SIZE_1D] =
                (u32)(((int)(dctBlock[i + j * DArCDJ_DCT_SIZE_1D] * quantTable[i + j * DArCDJ_DCT_SIZE_1D] + 0x4000)) >> 15);
        }
    }
}

/********************************************************************/
/*                   ?n?t?}???o?b?t?@?}?????                       */
/********************************************************************/
inline u32 CArGBAOdh::EmitBit(s32 code, s32 len, SArCDJ_HuffmanRequest *huffRequest)
{
    s32 shift;

    shift = (s32)(*huffRequest->mHuffmanBufferRemain - len);
    *huffRequest->mHuffmanBuffer |= (code << shift); // ?n?t?}???o?b?t?@??R?[?h?}??
    *huffRequest->mHuffmanBufferRemain -= len;       // ?c??t???[?r?b?g????????

    while (*huffRequest->mHuffmanBufferRemain <= 24)
    {
        // ?c??t???[?r?b?g????24??????
        // ?n?t?}???o?b?t?@????8Bit??CodeBuffer??o?????
        if (*huffRequest->mCodeBufferRemain == 0)
        {
            // ???k???w????e??I?[?o?[
            return DArCDJRESULT_ERR_CODE_SIZE;
        }

        *pCodeBuf = (u8)(*huffRequest->mHuffmanBuffer >> 24);
        pCodeBuf++;
        *huffRequest->mCodeBufferRemain -= 1; // CodeBuffer??c??e???????

        *huffRequest->mHuffmanBuffer <<= 8;
        *huffRequest->mHuffmanBufferRemain += 8;
    }

    return 0;
}

/********************************************************************/
/*                      ?n?t?}???????????                          */
/********************************************************************/
u32 CArGBAOdh::huffmanCoder(u16 *DCTBuffer, SArCDJ_HuffmanRequest *huffRequest)
{
    u16 *DctBufBak;
    s32 dctVal; // DCT?W??
    s32 diffDc; // DC??????DCT?W???????l
    s32 tmpVal;
    s32 idt_bits; // ?????????r?b?g??
    s32 add_bits; // ?t???r?b?g??
    s32 adds;     // ?t???r?b?g
    s32 huffCode; // ?n?t?}???????l
    int nb;       // 1?u???b?N??(=0)??2?u???b?N??(=1)??
    int run;      // ?????????O?X
    u32 result;

    // ?R?[?h?o?b?t?@?T?C?Y????R?[?h?o?b?t?@??o?????|?C???^???????
    pCodeBuf = huffRequest->mCodeBuffer + (huffRequest->mCodeBufferSize - *huffRequest->mCodeBufferRemain);

    DctBufBak = DCTBuffer;
    DCTBuffer++; // DCT?W???????i???
    nb = 0;

    while (1)
    {
        /*** DC??????????? ***/
        // DCT?o?b?t?@????DCT?W?????
        dctVal = (s32)(*DCTBuffer << 16);
        dctVal >>= 16;  // ?????g??
        DCTBuffer += 2; // ?|?C???^??1?i??????u???b?N??l????2?i???

        // DC??????O????????l???????
        diffDc = (s32)(dctVal - (*huffRequest->mPreviousDC[nb]));
        *huffRequest->mPreviousDC[nb] = (u32)dctVal; // ?????DCT?W??????
        dctVal = diffDc;

        if (dctVal < 0)
        {
            // DCT?W?????????V?????]??2?????
            tmpVal = -1 * dctVal;
            dctVal -= 1;
        }
        else
        {
            tmpVal = dctVal;
        }

        // DCT?W????r?b?g?????????
        if (tmpVal == 0)
        {
            // ?l??0?????????I??r?b?g??0
            add_bits = 0;
        }
        else
        {
            for (add_bits = 1; (tmpVal >>= 1) != 0; add_bits++)
                ;
        }

        // ?t???r?b?g??????HuffmanCode?????[?h
        huffCode = (s32)huffRequest->mDCTable[add_bits];
        idt_bits = huffCode >> 24; // ??????????????
        huffCode &= 0x00ffffff;    // ??????????Z?b?g

        adds = dctVal & ((1 << add_bits) - 1); // ?t???r?b?g???????
        // ?????????t???r?b?g????????n?t?}??????????
        huffCode = (huffCode << add_bits) | adds;

        result = EmitBit(huffCode, idt_bits + add_bits, huffRequest);
        if (result == DArCDJRESULT_ERR_CODE_SIZE)
        {
            // ???k???w????e??I?[?o?[
            return DArCDJRESULT_ERR_CODE_SIZE;
        }

        /*** AC??????????? ***/
        while (1)
        {
            run = 0; // ?????????O?X??????

            while (1)
            {
                // ?[?????????J?E???g
                // DCT?o?b?t?@????DCT?W?????
                dctVal = (s32)(*DCTBuffer << 16);
                dctVal >>= 16;  // ?????g??
                DCTBuffer += 2; // ?|?C???^??1?i??????u???b?N??l????2?i???
                if (dctVal == 0)
                {
                    run++;
                }
                else
                {
                    break;
                }
            }

            // ???????????AC?W?????????I??
            if (dctVal == 0x4000)
            {
                break;
            }

            // ?[???????????O?X??????????s???
            tmpVal = run;

            // ?[???????????O?X??r?b?g?????????
            if (tmpVal == 0)
            {
                // ?l??0?????????I??r?b?g??0
                add_bits = 0;
            }
            else
            {
                for (add_bits = 1; (tmpVal >>= 1) != 0; add_bits++)
                    ;
            }

            // ?[???????????O?X??????HuffmanCode?????[?h
            huffCode = (s32)huffRequest->mACTable[add_bits];
            idt_bits = huffCode >> 24; // ??????????????
            huffCode &= 0x00ffffff;    // ??????????Z?b?g

            adds = run & ((1 << add_bits) - 1); // ?t???r?b?g???????
            // ?????????t???r?b?g????????n?t?}??????????
            huffCode = (huffCode << add_bits) | adds;

            result = EmitBit(huffCode, idt_bits + add_bits, huffRequest);
            if (result == DArCDJRESULT_ERR_CODE_SIZE)
            {
                // ???k???w????e??I?[?o?[
                return DArCDJRESULT_ERR_CODE_SIZE;
            }

            // ??[???W????????????s???
            if (dctVal < 0)
            {
                // DCT?W?????????V?????]??2?????
                tmpVal = -1 * dctVal;
                dctVal -= 1;
            }
            else
            {
                tmpVal = dctVal;
            }

            // DCT?W????r?b?g?????????
            if (tmpVal == 0)
            {
                // ?l??0?????????I??r?b?g??0
                add_bits = 0;
            }
            else
            {
                for (add_bits = 1; (tmpVal >>= 1) != 0; add_bits++)
                    ;
            }

            // ?t???r?b?g??????HuffmanCode?????[?h
            huffCode = (s32)huffRequest->mDCTable[add_bits];
            idt_bits = huffCode >> 24; // ??????????????
            huffCode &= 0x00ffffff;    // ??????????Z?b?g

            adds = dctVal & ((1 << add_bits) - 1); // ?t???r?b?g???????
            // ?????????t???r?b?g????????n?t?}??????????
            huffCode = (huffCode << add_bits) | adds;

            result = EmitBit(huffCode, idt_bits + add_bits, huffRequest);
            if (result == DArCDJRESULT_ERR_CODE_SIZE)
            {
                // ???k???w????e??I?[?o?[
                return DArCDJRESULT_ERR_CODE_SIZE;
            }
        }

        if (run != 0)
        {
            // EOB???n?t?}???e?[?u???????[?h????o?b?t?@??i?[????
            huffCode = (s32)huffRequest->mACTable[7];
            result = EmitBit(huffCode & 0x00ffffff, huffCode >> 24, huffRequest);
            if (result == DArCDJRESULT_ERR_CODE_SIZE)
            {
                // ???k???w????e??I?[?o?[
                return DArCDJRESULT_ERR_CODE_SIZE;
            }
        }

        if (((u32)DCTBuffer) & 0x2)
        {
            // 2bit???1???1????????????
            DCTBuffer = DctBufBak; // DCT?W??????o?????2?o?C?g????
            nb = 1;                // 2????p??PreviousDC?????????J?E???g?A?b?v
        }
        else
        {
            // 2bit???0???2???????????????n?t?}?????????I??
            break;
        }
    }

    return 0;
}

/********************************************************************/
/*                       ?f?R?[?_?????????                         */
/********************************************************************/
u32 CArGBAOdh::cdj_d_initializeDecompressOdh(
    SArCDJ_OdhMaster *cdj_ctrl, u8 *imgYCbCrBufPtr, u8 *CodeBufPtr)
{
    u32 quality;

    // GBA?pODH???w?b?_??????
    // ?t?@?C??????R?[?h??GBA?p???ODH???????????G???[
    if ((CodeBufPtr[0] != 'A') || (CodeBufPtr[1] != 'J') || (CodeBufPtr[2] != 'P') || (CodeBufPtr[3] != 'G'))
    {
        return DArCDJRESULT_ERR_INVALID_DATA;
    }
    // ????
    cdj_ctrl->mImageSize[DArCDJ_AXIS_X] =
        (u16)(((u32 *)CodeBufPtr)[1] & 0x7ff);
    // ??????
    cdj_ctrl->mImageSize[DArCDJ_AXIS_Y] =
        (u16)((((u32 *)CodeBufPtr)[1] >> 11) & 0x7ff);
    // ?????T???v?????O???[?g
    cdj_ctrl->mSmpRate[DArCDJ_AXIS_X] =
        (u8)(((((u32 *)CodeBufPtr)[1] >> 22) & 1) + 1);
    // ?????T???v?????O???[?g
    cdj_ctrl->mSmpRate[DArCDJ_AXIS_Y] =
        (u8)(((((u32 *)CodeBufPtr)[1] >> 23) & 1) + 1);
    // ???k?i??
    cdj_ctrl->mQuality = (u8)((((u32 *)CodeBufPtr)[1] >> 24) & 0xff);

    /* ???T?C?Y????? */
    if ((!cdj_ctrl->mImageSize[DArCDJ_AXIS_X]) || (cdj_ctrl->mImageSize[DArCDJ_AXIS_X] > DArCDJ_PIXEL_SIZE_MAX_X) || (!cdj_ctrl->mImageSize[DArCDJ_AXIS_Y]) || (cdj_ctrl->mImageSize[DArCDJ_AXIS_Y] > DArCDJ_PIXEL_SIZE_MAX_Y))
    {
        return DArCDJRESULT_ERR_SIZE;
    }

    /* ?i??????? */
    if (cdj_ctrl->mQuality > 100)
    {
        return DArCDJRESULT_ERR_QUALITY;
    }

    /* YCbCr???o?b?t?@?o?^ */
    cdj_ctrl->mImgYCbCrBufferPtr = imgYCbCrBufPtr;
    /* ?????o?b?t?@?o?^ */
    cdj_ctrl->mCodeBufferPtr = CodeBufPtr + DArCDJ_HEADER_SIZE;

    /* ?K?v??MCU???????v?Z???? */
    cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] =
        (u16)((cdj_ctrl->mImageSize[DArCDJ_AXIS_X] - 1) / (DArCDJ_DCT_SIZE_1D * cdj_ctrl->mSmpRate[DArCDJ_AXIS_X]) + 1);
    cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y] =
        (u16)((cdj_ctrl->mImageSize[DArCDJ_AXIS_Y] - 1) / (DArCDJ_DCT_SIZE_1D * cdj_ctrl->mSmpRate[DArCDJ_AXIS_Y]) + 1);

    /* ???????????MCU???u???n?_(0,0)????? */
    cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] = 0;
    cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] = 0;

    /* ?????o?b?t?@???????[?r?b?g?????????????? */
    cdj_ctrl->mHuffmanBits = 0;

    // DC????????O??l
    cdj_ctrl->mPreviousDC[DArCDJ_COLOR_Y] = 0;
    cdj_ctrl->mPreviousDC[DArCDJ_COLOR_Cb] = 0;
    cdj_ctrl->mPreviousDC[DArCDJ_COLOR_Cr] = 0;

    // ?n?t?}???????p?\???????????
    cdj_ctrl->mHuffmanRequest[0].mPreviousDC[0] = cdj_ctrl->mPreviousDC;
    cdj_ctrl->mHuffmanRequest[0].mPreviousDC[1] = cdj_ctrl->mPreviousDC;
    cdj_ctrl->mHuffmanRequest[0].mCodeBuffer = cdj_ctrl->mCodeBufferPtr;
    cdj_ctrl->mHuffmanRequest[0].mHuffmanBufferRemain = &cdj_ctrl->mHuffmanBits;
    cdj_ctrl->mHuffmanRequest[1].mPreviousDC[0] = cdj_ctrl->mPreviousDC + 1;
    cdj_ctrl->mHuffmanRequest[1].mPreviousDC[1] = cdj_ctrl->mPreviousDC + 2;
    cdj_ctrl->mHuffmanRequest[1].mCodeBuffer = cdj_ctrl->mCodeBufferPtr;
    cdj_ctrl->mHuffmanRequest[1].mHuffmanBufferRemain = &cdj_ctrl->mHuffmanBits;

    // ??q???e?[?u????????s??
    quality = (u32)cdj_ctrl->mQuality;
    if (quality <= 0)
        quality = 1;
    if (quality < 50)
        quality = (u32)(5000 / quality);
    else
        quality = (u32)(200 - quality * 2);

    cdj_d_setDequantizationTable(cdj_ctrl, quality);

    return DArCDJRESULT_SUCCESS;
}

/********************************************************************/
/*                         ODH?L?????[?v                           */
/********************************************************************/
u32 CArGBAOdh::cdj_d_decompressLoop(SArCDJ_OdhMaster *cdj_ctrl)
{
    u32 result;
    int idx, i;

    do
    {
        if ((cdj_ctrl->mSmpRate[DArCDJ_AXIS_X] == 1) && (cdj_ctrl->mSmpRate[DArCDJ_AXIS_Y] == 1))
        {
            /*** Y?????W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest,
                                    (u16 **)hufftreePtr, DArCDJ_COLOR_Y);

            // ?????o?b?t?@????????????????
            // ?????????n?t?}???????p?\??????????????????
            (cdj_ctrl->mHuffmanRequest[1]).mCodeBuffer =
                (cdj_ctrl->mHuffmanRequest[0]).mCodeBuffer;

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx = DArCDJ_DCT_SIZE_1D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X];

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[0],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]);
            /*** Cb?????W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest + 1,
                                    (u16 **)&hufftreePtr[2], DArCDJ_COLOR_Cb);

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx = DArCDJ_DCT_SIZE_1D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y];

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[1],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]);

            /*** Cr?????W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest + 1,
                                    (u16 **)&hufftreePtr[2], DArCDJ_COLOR_Cr);

            // ?????o?b?t?@????????????????
            // ?????????n?t?}???????p?\??????????????????
            (cdj_ctrl->mHuffmanRequest[0]).mCodeBuffer =
                (cdj_ctrl->mHuffmanRequest[1]).mCodeBuffer;

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx = DArCDJ_DCT_SIZE_1D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y] * 2;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[1],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]);
        }
        else if ((cdj_ctrl->mSmpRate[DArCDJ_AXIS_X] == 2) && (cdj_ctrl->mSmpRate[DArCDJ_AXIS_Y] == 1))
        {
            /*** Y?????i1?u???b?N??j?W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest,
                                    (u16 **)hufftreePtr, DArCDJ_COLOR_Y);

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx = DArCDJ_DCT_SIZE_1D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] * 2 + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[0],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2);

            /*** Y?????i2?u???b?N??j?W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest,
                                    (u16 **)hufftreePtr, DArCDJ_COLOR_Y);

            // ?????o?b?t?@????????????????
            // ?????????n?t?}???????p?\??????????????????
            (cdj_ctrl->mHuffmanRequest[1]).mCodeBuffer =
                (cdj_ctrl->mHuffmanRequest[0]).mCodeBuffer;

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx += DArCDJ_DCT_SIZE_1D;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[0],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2);

            /*** Cb?????W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest + 1,
                                    (u16 **)&hufftreePtr[2], DArCDJ_COLOR_Cb);

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx = DArCDJ_DCT_SIZE_1D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2 * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y];

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[1],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]);

            /*** Cr?????W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest + 1,
                                    (u16 **)&hufftreePtr[2], DArCDJ_COLOR_Cr);

            // ?????o?b?t?@????????????????
            // ?????????n?t?}???????p?\??????????????????
            (cdj_ctrl->mHuffmanRequest[0]).mCodeBuffer =
                (cdj_ctrl->mHuffmanRequest[1]).mCodeBuffer;

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx = DArCDJ_DCT_SIZE_1D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2 * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y] * 2;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[1],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]);
        }
        else if ((cdj_ctrl->mSmpRate[DArCDJ_AXIS_X] == 1) && (cdj_ctrl->mSmpRate[DArCDJ_AXIS_Y] == 2))
        {
            /*** Y?????i1?u???b?N??j?W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest,
                                    (u16 **)hufftreePtr, DArCDJ_COLOR_Y);

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx = DArCDJ_DCT_SIZE_1D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[0],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]);

            /*** Y?????i2?u???b?N??j?W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest,
                                    (u16 **)hufftreePtr, DArCDJ_COLOR_Y);

            // ?????o?b?t?@????????????????
            // ?????????n?t?}???????p?\??????????????????
            (cdj_ctrl->mHuffmanRequest[1]).mCodeBuffer =
                (cdj_ctrl->mHuffmanRequest[0]).mCodeBuffer;

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx += DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X];

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[0],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]);

            /*** Cb?????W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest + 1,
                                    (u16 **)&hufftreePtr[2], DArCDJ_COLOR_Cb);

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx = DArCDJ_DCT_SIZE_1D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y] * 2;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[1],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]);

            /*** Cr?????W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest + 1,
                                    (u16 **)&hufftreePtr[2], DArCDJ_COLOR_Cr);

            // ?????o?b?t?@????????????????
            // ?????????n?t?}???????p?\??????????????????
            (cdj_ctrl->mHuffmanRequest[0]).mCodeBuffer =
                (cdj_ctrl->mHuffmanRequest[1]).mCodeBuffer;

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx = DArCDJ_DCT_SIZE_1D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y] * 4;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[1],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]);
        }
        else if ((cdj_ctrl->mSmpRate[DArCDJ_AXIS_X] == 2) && (cdj_ctrl->mSmpRate[DArCDJ_AXIS_Y] == 2))
        {
            /*** Y?????i????u???b?N?j?W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest,
                                    (u16 **)hufftreePtr, DArCDJ_COLOR_Y);

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx = DArCDJ_DCT_SIZE_1D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] * 2 + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 4;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[0],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2);

            /*** Y?????i?E??u???b?N?j?W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest,
                                    (u16 **)hufftreePtr, DArCDJ_COLOR_Y);

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx += DArCDJ_DCT_SIZE_1D;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[0],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2);

            /*** Y?????i?????u???b?N?j?W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest,
                                    (u16 **)hufftreePtr, DArCDJ_COLOR_Y);

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx += DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2 - DArCDJ_DCT_SIZE_1D;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[0],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2);

            /*** Y?????i?E???u???b?N?j?W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest,
                                    (u16 **)hufftreePtr, DArCDJ_COLOR_Y);

            // ?????o?b?t?@????????????????
            // ?????????n?t?}???????p?\??????????????????
            (cdj_ctrl->mHuffmanRequest[1]).mCodeBuffer =
                (cdj_ctrl->mHuffmanRequest[0]).mCodeBuffer;

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx += DArCDJ_DCT_SIZE_1D;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[0],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * 2);

            /*** Cb?????W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest + 1,
                                    (u16 **)&hufftreePtr[2], DArCDJ_COLOR_Cb);

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx = DArCDJ_DCT_SIZE_1D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y] * 4;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[1],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]);

            /*** Cr?????W?J ***/
            // ?n?t?}????????
            result = huffmanDecoder(cdj_ctrl->mDCTBuffer,
                                    cdj_ctrl->mHuffmanRequest + 1,
                                    (u16 **)&hufftreePtr[2], DArCDJ_COLOR_Cr);

            // ?????o?b?t?@????????????????
            // ?????????n?t?}???????p?\??????????????????
            (cdj_ctrl->mHuffmanRequest[0]).mCodeBuffer =
                (cdj_ctrl->mHuffmanRequest[1]).mCodeBuffer;

            if (result)
                return result; // ?n?t?}?????????G???[

            // ?t?W?O?U?O????v?????s??
            for (i = 0; i < 64; i++)
            {
                cdj_ctrl->mDCTBuffer[odh_natural_order[i] + 64] =
                    cdj_ctrl->mDCTBuffer[i];
            }

            // ?L?????l??o?????????
            idx = DArCDJ_DCT_SIZE_1D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] + DArCDJ_DCT_SIZE_2D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X] * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y] * 8;

            // ?t??q????tDCT
            idct_fast(range_limit, cdj_ctrl->mQuantizationTable[1],
                      &(cdj_ctrl->mDCTBuffer[64]),
                      &(cdj_ctrl->mImgYCbCrBufferPtr[idx]),
                      (u32)DArCDJ_DCT_SIZE_1D * cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X]);
        }
        else
        {
            // ?????G???[??????????
        }

        // ????MCU???????i???
        cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X]++;
        if (cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] == cdj_ctrl->mMCUinImage[DArCDJ_AXIS_X])
        {
            cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y]++;
            cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_X] = 0;
        }
    } while (cdj_ctrl->mCurrentMCU[DArCDJ_AXIS_Y] < cdj_ctrl->mMCUinImage[DArCDJ_AXIS_Y]);

    return DArCDJRESULT_SUCCESS;
}

/********************************************************************/
/*                    ??q???e?[?u???????                        */
/********************************************************************/
// ?w???????k?i???l???????q???e?[?u??????????B
// ???k????L????????q???e?[?u????t?H?[?}?b?g????????A
// ???k??L????A??????s?????????A??q???e?[?u????
// ?????????????????B
// ?E????
// *cdj_ctrl?FODH???C?u????????\??????|?C???^
// quality  ?F???k?i??
void CArGBAOdh::cdj_d_setDequantizationTable(SArCDJ_OdhMaster *cdj_ctrl, u32 quality)
{
    u32 cnt, temp, currentTable;

    /* ??q???e?[?u?????w?????i????X?P?[?????O???? */
    for (currentTable = 0; currentTable < DArCDJ_QUANTIZE_TABLE_NUM; currentTable++)
    {
        for (cnt = 0; cnt < DArCDJ_DCT_SIZE_2D; cnt++)
        {
            temp = (gArCdj_std_quant_tbl[currentTable][cnt] * quality + 50L) / 100L;
            if (temp <= 0L)
                temp = 1L;
            if (temp > 255L)
                temp = 255L;

            cdj_ctrl->mQuantizationTable[currentTable][cnt] =
                (u32)((temp * gArAANScales[cnt] + 2048) >> 12);
        }
    }
}

/********************************************************************/
/*               YCbCr?????????RGB??????s?????               */
/********************************************************************/
u32 CArGBAOdh::cdj_d_colorDeconv(SArCDJ_OdhMaster *cdj_ctrl, u8 *imgRGBPtr, int format)
{
    u8 *imgYCbCrPtr;           // ??????iYCbCr?f?[?^?j?o?b?t?@???|?C???^
    u8 *CbDataPtr, *CrDataPtr; // YCbCr???f?[?^????e???????|?C???^
    int cntLine;               // ?????????????C?????
    int imgStep, adrs;

    if ((cdj_ctrl->mImageSize[DArCDJ_AXIS_X] & 1) || (cdj_ctrl->mImageSize[DArCDJ_AXIS_Y] & 1))
    {
        // ??^??T?C?Y????????????????
        return DArJCOL_RES_ERROR;
    }

    imgYCbCrPtr = cdj_ctrl->mImgYCbCrBufferPtr;
    // Cb,Cr?f?[?^??????J?n??u???|?C???^??????
    CbDataPtr =
        imgYCbCrPtr + cdj_ctrl->mImageSize[DArCDJ_AXIS_X] * cdj_ctrl->mImageSize[DArCDJ_AXIS_Y];
    CrDataPtr =
        CbDataPtr + cdj_ctrl->mImageSize[DArCDJ_AXIS_X] * cdj_ctrl->mImageSize[DArCDJ_AXIS_Y];

    if (format == RGB565)
    {
        imgStep = (cdj_ctrl->mImageSize[DArCDJ_AXIS_X] / 4) * 32;
    }
    else if (format == RGBA8)
    {
        imgStep = (cdj_ctrl->mImageSize[DArCDJ_AXIS_X] / 4) * 64;
    }
    else
    {
        imgStep = (cdj_ctrl->mImageSize[DArCDJ_AXIS_X] / 8) * 32;
    }

    /*** YCbCr???f?[?^???????ARGB?`?????????? ***/
    if ((cdj_ctrl->mSmpRate[DArCDJ_AXIS_X] == 1) && (cdj_ctrl->mSmpRate[DArCDJ_AXIS_Y] == 1))
    {
        for (cntLine = 0; cntLine < cdj_ctrl->mImageSize[DArCDJ_AXIS_Y]; cntLine++)
        {
            adrs = (cntLine / 4) * imgStep + (cntLine & 3) * 8;

            // 1???C??????YCbCr??RGB??????s???
            LineDeconv11(&imgRGBPtr[adrs], imgYCbCrPtr, CbDataPtr, CrDataPtr, cdj_ctrl->mImageSize[DArCDJ_AXIS_X], cdj_ctrl->mImageSize[DArCDJ_AXIS_Y], &gArDeconvPlttTbl, format);

            // ?C???[?W???|?C???^??????i???
            imgYCbCrPtr += cdj_ctrl->mImageSize[DArCDJ_AXIS_X];
            CbDataPtr += cdj_ctrl->mImageSize[DArCDJ_AXIS_X];
            CrDataPtr += cdj_ctrl->mImageSize[DArCDJ_AXIS_X];
        }
    }

    else if ((cdj_ctrl->mSmpRate[DArCDJ_AXIS_X] == 2) && (cdj_ctrl->mSmpRate[DArCDJ_AXIS_Y] == 1))
    {
        for (cntLine = 0; cntLine < cdj_ctrl->mImageSize[DArCDJ_AXIS_Y]; cntLine++)
        {
            adrs = (cntLine / 4) * imgStep + (cntLine & 3) * 8;

            // 1???C??????YCbCr??RGB??????s???
            LineDeconv21(&imgRGBPtr[adrs], imgYCbCrPtr, CbDataPtr, CrDataPtr, cdj_ctrl->mImageSize[DArCDJ_AXIS_X], cdj_ctrl->mImageSize[DArCDJ_AXIS_Y], &gArDeconvPlttTbl, format);

            // ?C???[?W???|?C???^??????i???
            imgYCbCrPtr += cdj_ctrl->mImageSize[DArCDJ_AXIS_X];
            CbDataPtr += (cdj_ctrl->mImageSize[DArCDJ_AXIS_X] >> 1);
            CrDataPtr += (cdj_ctrl->mImageSize[DArCDJ_AXIS_X] >> 1);
        }
    }

    else if ((cdj_ctrl->mSmpRate[DArCDJ_AXIS_X] == 1) && (cdj_ctrl->mSmpRate[DArCDJ_AXIS_Y] == 2))
    {
        for (cntLine = 0; cntLine < cdj_ctrl->mImageSize[DArCDJ_AXIS_Y]; cntLine += 2)
        {
            adrs = (cntLine / 4) * imgStep + (cntLine & 3) * 8;

            // 2???C??????YCbCr??RGB??????s???
            LineDeconv12(&imgRGBPtr[adrs], imgYCbCrPtr, CbDataPtr, CrDataPtr, cdj_ctrl->mImageSize[DArCDJ_AXIS_X], cdj_ctrl->mImageSize[DArCDJ_AXIS_Y], &gArDeconvPlttTbl, format);

            // ?C???[?W???|?C???^??????i???
            imgYCbCrPtr += (cdj_ctrl->mImageSize[DArCDJ_AXIS_X] << 1);
            CbDataPtr += cdj_ctrl->mImageSize[DArCDJ_AXIS_X];
            CrDataPtr += cdj_ctrl->mImageSize[DArCDJ_AXIS_X];
        }
    }

    else if ((cdj_ctrl->mSmpRate[DArCDJ_AXIS_X] == 2) && (cdj_ctrl->mSmpRate[DArCDJ_AXIS_Y] == 2))
    {
        for (cntLine = 0; cntLine < cdj_ctrl->mImageSize[DArCDJ_AXIS_Y]; cntLine += 2)
        {
            adrs = (cntLine / 4) * imgStep + (cntLine & 3) * 8;

            // 2???C??????YCbCr??RGB??????s???
            LineDeconv22(&imgRGBPtr[adrs], imgYCbCrPtr, CbDataPtr, CrDataPtr, cdj_ctrl->mImageSize[DArCDJ_AXIS_X], cdj_ctrl->mImageSize[DArCDJ_AXIS_Y], &gArDeconvPlttTbl, format);

            // ?C???[?W???|?C???^??????i???
            imgYCbCrPtr += (cdj_ctrl->mImageSize[DArCDJ_AXIS_X] << 1);
            CbDataPtr += (cdj_ctrl->mImageSize[DArCDJ_AXIS_X] >> 1);
            CrDataPtr += (cdj_ctrl->mImageSize[DArCDJ_AXIS_X] >> 1);
        }
    }

    return DArJCOL_RES_SUCCESS;
}

/********************************************************************/
/*      1???C??YCbCr??RGB???????i?_?E???T???v?????O????j         */
/********************************************************************/
void CArGBAOdh::LineDeconv11(u8 *rgbPtr, u8 *yPtr, u8 *cbPtr, u8 *crPtr, u16 sizeX, u16 sizeY, const SArDeconvTbl *tbl, int format)
{
    u8 y, cb, cr;
    int i, r, g, b, z, tmp, sizeF;

    sizeF = sizeX * sizeY;

    for (i = 0; i < sizeX; i++)
    {
        // YCbCr?f?[?^???o
        y = *yPtr;
        cb = *cbPtr;
        cr = *crPtr;
        // RGB?`??????
        r = ScaleLimit(y + tbl->R_Cr[cr]);
        g = ScaleLimit(y + ((tbl->G_Cb[cb] + tbl->G_Cr[cr]) >> DArSCALEBITS));
        b = ScaleLimit(y + tbl->B_Cb[cb]);

        // ?????RGB???f?[?^???o?b?t?@??i?[
        // GC??RGB565?t?H?[?}?b?g???????

        if (format == RGB565)
        {
            z = (i >> 2) * 32 + (i & 3) * 2;
            tmp = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            rgbPtr[z] = (u8)(tmp >> 8);
            rgbPtr[z + 1] = (u8)(tmp & 255);
        }
        else if (format == RGBA8)
        {
            z = (i >> 2) * 64 + (i & 3) * 2;
            rgbPtr[z] = 255;
            rgbPtr[z + 1] = (u8)r;
            rgbPtr[z + 32] = (u8)g;
            rgbPtr[z + 33] = (u8)b;
        }
        else
        { /* yuv */
            z = (i >> 3) * 32 + (i & 7);
#if 0
			yy =  0.257F*r + 0.504F*g + 0.098F*b + 16.0F;
			uu = -0.148F*r - 0.291F*g + 0.439F*b + 128.0F;
			vv =  0.439F*r - 0.368F*g - 0.071F*b + 128.0F;
			rgbPtr[z]=(u8)Mel_round(yy);
			rgbPtr[z+sizeF]=(u8)Mel_round(uu);
			rgbPtr[z+sizeF*2]=(u8)Mel_round(vv);
#else
            rgbPtr[z] = y;
            rgbPtr[z + sizeF] = cb;
            rgbPtr[z + sizeF * 2] = cr;
#endif
        }

        // ?C???[?W???|?C???^??????i???
        yPtr++;
        cbPtr++;
        crPtr++;
    }
}

/********************************************************************/
/*      1???C??YCbCr??RGB???????i?????_?E???T???v?????O?j         */
/********************************************************************/
void CArGBAOdh::LineDeconv21(u8 *rgbPtr, u8 *yPtr, u8 *cbPtr, u8 *crPtr, u16 sizeX, u16 sizeY, const SArDeconvTbl *tbl, int format)
{
    u8 y, cb, cr;
    int i, r, g, b, z, tmp, sizeF;

    sizeF = sizeX * sizeY;

    for (i = 0; i < sizeX; i += 2)
    {
        /*** 1?s?N?Z???? ***/
        // YCbCr?f?[?^???o
        y = *yPtr;
        cb = *cbPtr;
        cr = *crPtr;

        // RGB?`??????
        r = ScaleLimit(y + tbl->R_Cr[cr]);
        g = ScaleLimit(y + ((tbl->G_Cb[cb] + tbl->G_Cr[cr]) >> DArSCALEBITS));
        b = ScaleLimit(y + tbl->B_Cb[cb]);

        // ?????RGB???f?[?^???o?b?t?@??i?[
        // GC??RGB565?t?H?[?}?b?g???????

        if (format == RGB565)
        {
            z = (i >> 2) * 32 + (i & 3) * 2;
            tmp = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            rgbPtr[z] = (u8)(tmp >> 8);
            rgbPtr[z + 1] = (u8)(tmp & 255);
        }
        else if (format == RGBA8)
        {
            z = (i >> 2) * 64 + (i & 3) * 2;
            rgbPtr[z] = 255;
            rgbPtr[z + 1] = (u8)r;
            rgbPtr[z + 32] = (u8)g;
            rgbPtr[z + 33] = (u8)b;
        }
        else
        {
            z = (i >> 3) * 32 + (i & 7);
            rgbPtr[z] = y;
            rgbPtr[z + sizeF] = cb;
            rgbPtr[z + sizeF * 2] = cr;
        }

        // ?C???[?W???|?C???^??????i???
        yPtr++;

        /*** 2?s?N?Z???? ***/
        // Y?f?[?^?????o
        y = *yPtr;

        // RGB?`??????
        r = ScaleLimit(y + tbl->R_Cr[cr]);
        g = ScaleLimit(y + ((tbl->G_Cb[cb] + tbl->G_Cr[cr]) >> DArSCALEBITS));
        b = ScaleLimit(y + tbl->B_Cb[cb]);

        // ?????RGB???f?[?^???o?b?t?@??i?[
        // GC??RGB565?t?H?[?}?b?g???????

        if (format == RGB565)
        {
            tmp = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            rgbPtr[z + 2] = (u8)(tmp >> 8);
            rgbPtr[z + 3] = (u8)(tmp & 255);
        }
        else if (format == RGBA8)
        {
            rgbPtr[z + 2] = 255;
            rgbPtr[z + 3] = (u8)r;
            rgbPtr[z + 34] = (u8)g;
            rgbPtr[z + 35] = (u8)b;
        }
        else
        {
            rgbPtr[z + 1] = y;
            rgbPtr[z + sizeF + 1] = cb;
            rgbPtr[z + sizeF * 2 + 1] = cr;
        }

        // ?C???[?W???|?C???^??????i???
        yPtr++;
        cbPtr++;
        crPtr++;
    }
}

/********************************************************************/
/*      1???C??YCbCr??RGB???????i?????_?E???T???v?????O?j         */
/********************************************************************/
void CArGBAOdh::LineDeconv12(u8 *rgbPtr, u8 *yPtr, u8 *cbPtr, u8 *crPtr, u16 sizeX, u16 sizeY, const SArDeconvTbl *tbl, int format)
{
    u8 *yPtr2;
    u8 y, cb, cr;
    int i, r, g, b, z, tmp, sizeF;

    sizeF = sizeX * sizeY;

    for (i = 0; i < sizeX; i++)
    {
        /*** 1???C???? ***/
        // YCbCr?f?[?^???o
        y = *yPtr;
        cb = *cbPtr;
        cr = *crPtr;

        // RGB?`??????
        r = ScaleLimit(y + tbl->R_Cr[cr]);
        g = ScaleLimit(y + ((tbl->G_Cb[cb] + tbl->G_Cr[cr]) >> DArSCALEBITS));
        b = ScaleLimit(y + tbl->B_Cb[cb]);

        // ?????RGB???f?[?^???o?b?t?@??i?[
        // GC??RGB565?t?H?[?}?b?g???????

        if (format == RGB565)
        {
            z = (i >> 2) * 32 + (i & 3) * 2;
            tmp = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            rgbPtr[z] = (u8)(tmp >> 8);
            rgbPtr[z + 1] = (u8)(tmp & 255);
        }
        else if (format == RGBA8)
        {
            z = (i >> 2) * 64 + (i & 3) * 2;
            rgbPtr[z] = 255;
            rgbPtr[z + 1] = (u8)r;
            rgbPtr[z + 32] = (u8)g;
            rgbPtr[z + 33] = (u8)b;
        }
        else
        {
            z = (i >> 3) * 32 + (i & 7);
            rgbPtr[z] = y;
            rgbPtr[z + sizeF] = cb;
            rgbPtr[z + sizeF * 2] = cr;
        }

        /*** 2???C???? ***/
        // 2???C?????|?C???^???i?[
        yPtr2 = yPtr + sizeX;

        // Y?f?[?^?????o
        y = *yPtr2;

        // RGB?`??????
        r = ScaleLimit(y + tbl->R_Cr[cr]);
        g = ScaleLimit(y + ((tbl->G_Cb[cb] + tbl->G_Cr[cr]) >> DArSCALEBITS));
        b = ScaleLimit(y + tbl->B_Cb[cb]);

        // ?????RGB???f?[?^???o?b?t?@??i?[
        // GC??RGB565?t?H?[?}?b?g???????

        if (format == RGB565)
        {
            tmp = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            rgbPtr[z + 8] = (u8)(tmp >> 8);
            rgbPtr[z + 9] = (u8)(tmp & 255);
        }
        else if (format == RGBA8)
        {
            rgbPtr[z + 8] = 255;
            rgbPtr[z + 9] = (u8)r;
            rgbPtr[z + 40] = (u8)g;
            rgbPtr[z + 41] = (u8)b;
        }
        else
        {
            z = (i >> 3) * 32 + (i & 7);
            rgbPtr[z + 8] = y;
            rgbPtr[z + sizeF + 8] = cb;
            rgbPtr[z + sizeF * 2 + 8] = cr;
        }

        // ?C???[?W???|?C???^??????i???
        yPtr++;
        cbPtr++;
        crPtr++;
    }
}

/********************************************************************/
/*   1???C??YCbCr??RGB???????i?????E?????_?E???T???v?????O?j      */
/********************************************************************/
void CArGBAOdh::LineDeconv22(u8 *rgbPtr, u8 *yPtr, u8 *cbPtr, u8 *crPtr, u16 sizeX, u16 sizeY, const SArDeconvTbl *tbl, int format)
{
    u8 *yPtr2;
    u8 y, cb, cr;
    int i, r, g, b, z, tmp, sizeF;

    sizeF = sizeX * sizeY;

    for (i = 0; i < sizeX; i += 2)
    {
        // 2???C?????|?C???^???i?[
        yPtr2 = yPtr + sizeX;

        /*** 1???C??-1?s?N?Z???? ***/
        // YCbCr?f?[?^???o
        y = *yPtr;
        cb = *cbPtr;
        cr = *crPtr;

        // RGB?`??????
        r = ScaleLimit(y + tbl->R_Cr[cr]);
        g = ScaleLimit(y + ((tbl->G_Cb[cb] + tbl->G_Cr[cr]) >> DArSCALEBITS));
        b = ScaleLimit(y + tbl->B_Cb[cb]);

        // ?????RGB???f?[?^???o?b?t?@??i?[
        // GC??RGB565?t?H?[?}?b?g???????

        if (format == RGB565)
        {
            z = (i >> 2) * 32 + (i & 3) * 2;
            tmp = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            rgbPtr[z] = (u8)(tmp >> 8);
            rgbPtr[z + 1] = (u8)(tmp & 255);
        }
        else if (format == RGBA8)
        {
            z = (i >> 2) * 64 + (i & 3) * 2;
            rgbPtr[z] = 255;
            rgbPtr[z + 1] = (u8)r;
            rgbPtr[z + 32] = (u8)g;
            rgbPtr[z + 33] = (u8)b;
        }
        else
        {
            z = (i >> 3) * 32 + (i & 7);
            rgbPtr[z] = y;
            rgbPtr[z + sizeF] = cb;
            rgbPtr[z + sizeF * 2] = cr;
        }

        // ?C???[?W???|?C???^??????i???
        yPtr++;

        /*** 1???C??-2?s?N?Z???? ***/
        // Y?f?[?^?????o
        y = *yPtr;

        // RGB?`??????
        r = ScaleLimit(y + tbl->R_Cr[cr]);
        g = ScaleLimit(y + ((tbl->G_Cb[cb] + tbl->G_Cr[cr]) >> DArSCALEBITS));
        b = ScaleLimit(y + tbl->B_Cb[cb]);

        // ?????RGB???f?[?^???o?b?t?@??i?[
        // GC??RGB565?t?H?[?}?b?g???????

        if (format == RGB565)
        {
            tmp = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            rgbPtr[z + 2] = (u8)(tmp >> 8);
            rgbPtr[z + 3] = (u8)(tmp & 255);
        }
        else if (format == RGBA8)
        {
            rgbPtr[z + 2] = 255;
            rgbPtr[z + 3] = (u8)r;
            rgbPtr[z + 34] = (u8)g;
            rgbPtr[z + 35] = (u8)b;
        }
        else
        {
            z = (i >> 3) * 32 + (i & 7);
            rgbPtr[z + 1] = y;
            rgbPtr[z + sizeF + 1] = cb;
            rgbPtr[z + sizeF * 2 + 1] = cr;
        }

        // ?C???[?W???|?C???^??????i???
        yPtr++;

        /*** 2???C??-1?s?N?Z???? ***/
        // Y?f?[?^?????o
        y = *yPtr2;

        // RGB?`??????
        r = ScaleLimit(y + tbl->R_Cr[cr]);
        g = ScaleLimit(y + ((tbl->G_Cb[cb] + tbl->G_Cr[cr]) >> DArSCALEBITS));
        b = ScaleLimit(y + tbl->B_Cb[cb]);

        // ?????RGB???f?[?^???o?b?t?@??i?[
        // GC??RGB565?t?H?[?}?b?g???????

        if (format == RGB565)
        {
            tmp = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            rgbPtr[z + 8] = (u8)(tmp >> 8);
            rgbPtr[z + 9] = (u8)(tmp & 255);
        }
        else if (format == RGBA8)
        {
            rgbPtr[z + 8] = 255;
            rgbPtr[z + 9] = (u8)r;
            rgbPtr[z + 40] = (u8)g;
            rgbPtr[z + 41] = (u8)b;
        }
        else
        {
            z = (i >> 3) * 32 + (i & 7);
            rgbPtr[z + 8] = y;
            rgbPtr[z + sizeF + 8] = cb;
            rgbPtr[z + sizeF * 2 + 8] = cr;
        }

        // ?C???[?W???|?C???^??????i???
        yPtr2++;

        /*** 2???C??-2?s?N?Z???? ***/
        // Y?f?[?^?????o
        y = *yPtr2;

        // RGB?`??????
        r = ScaleLimit(y + tbl->R_Cr[cr]);
        g = ScaleLimit(y + ((tbl->G_Cb[cb] + tbl->G_Cr[cr]) >> DArSCALEBITS));
        b = ScaleLimit(y + tbl->B_Cb[cb]);

        // ?????RGB???f?[?^???o?b?t?@??i?[
        // GC??RGB565?t?H?[?}?b?g???????

        if (format == RGB565)
        {
            tmp = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            rgbPtr[z + 10] = (u8)(tmp >> 8);
            rgbPtr[z + 11] = (u8)(tmp & 255);
        }
        else if (format == RGBA8)
        {
            rgbPtr[z + 10] = 255;
            rgbPtr[z + 11] = (u8)r;
            rgbPtr[z + 42] = (u8)g;
            rgbPtr[z + 43] = (u8)b;
        }
        else
        {
            z = (i >> 3) * 32 + (i & 7);
            rgbPtr[z + 9] = y;
            rgbPtr[z + sizeF + 9] = cb;
            rgbPtr[z + sizeF * 2 + 9] = cr;
        }

        // ?C???[?W???|?C???^??????i???
        yPtr2++;
        cbPtr++;
        crPtr++;
    }
}

/********************************************************************/
/*                      ?n?t?}???????????                          */
/********************************************************************/
u32 CArGBAOdh::huffmanDecoder(u32 *DCTBlock,
                              SArCDJ_HuffmanRequest *huffmanRequest,
                              u16 **hufftree, int col)
{
    u8 *bufPtr;   // ???????f?[?^???i?[???????o?b?t?@???|?C???^
    u32 buf_ofs;  // ??????????????f?[?^??1?o?C?g???E????????????????
                  // ?I?t?Z?b?g?r?b?g???i??????E???r?b?g???j
    u32 code;     // ???????f?[?^
    u32 bit_mask; // ?t???r?b?g???}?X?N???????}?X?N?l
    // int run;       // ?????????O?X
    int blk_idx;         // DCT?u???b?N??C???f?b?N?X
    int idt_bits;        // ?????????r?b?g??
    int add_bits;        // ?t???r?b?g??
    int treeIdx;         // ?n?t?}???c???[??C???f?b?N?X
    int nextOfs;         // ????c???[?m?[?h?f?[?^???I?t?Z?b?g
    int idt_bits_max_DC; // DC???????????r?b?g??
    int i, colBak;

    blk_idx = 0;

    // ?????f?[?^???????|?C???^??A?I?t?Z?b?g?r?b?g?????Z?b?g????
    bufPtr = huffmanRequest->mCodeBuffer;
    buf_ofs = *huffmanRequest->mHuffmanBufferRemain;

    /*** DC????????? ***/
    // ???????f?[?^??4?o?C?g???o???A?I?t?Z?b?g???V?t?g????i????l???j
    code = (u32)(((*bufPtr << 24) | (*(bufPtr + 1) << 16) | (*(bufPtr + 2) << 8) | *(bufPtr + 3)) << buf_ofs);

    if (col == 0)
    {
        // Y????????????????9?r?b?g
        idt_bits_max_DC = 9;
    }
    else
    {
        // Cb,Cr????????????????11?r?b?g
        idt_bits_max_DC = 11;
    }

    // ?n?t?}???c???[?e?[?u????H?????????????????
    for (idt_bits = 1, treeIdx = 0; idt_bits <= idt_bits_max_DC; idt_bits++)
    {
        nextOfs = ((hufftree[0])[treeIdx] & 0x3fff);
        if (((code >> (32 - idt_bits)) & 1) == 0)
        {
            // 0????A???q?m?[?h????
            if ((hufftree[0])[treeIdx] & 0x8000)
            {
                // ???q?m?[?h???t???
                add_bits = (hufftree[0])[treeIdx + nextOfs];
                break;
            }
            else
            {
                treeIdx += nextOfs;
            }
        }
        else
        {
            // 1????A?E?q?m?[?h????
            if ((hufftree[0])[treeIdx] & 0x4000)
            {
                // ?E?q?m?[?h???t???
                add_bits = (hufftree[0])[treeIdx + nextOfs + 1];
                break;
            }
            else
            {
                treeIdx += nextOfs + 1;
            }
        }
    }

    // ????????f?[?^??ODH?t?H?[?}?b?g??]????????????
    // ?s????e?[?u????????????
    if (idt_bits > idt_bits_max_DC)
        return DArCDJRESULT_ERR_HUFFCODE;

    if (add_bits > 0)
    {
        // ????t???r?b?g???????
        bit_mask = (u32)((1 << add_bits) - 1); // ?t???r?b?g???????}?X?N?l???????
        code = (code >> (32 - idt_bits - add_bits)) & bit_mask;

        // ????r?b?g??0?????????????????
        if ((code & (1 << (add_bits - 1))) == 0)
        {
            code = (code + 1) | ~bit_mask;
        }
    }
    else
    {
        // ?l??0?????t???r?b?g???????
        code = 0;
    }

    // ???????????f?[?^??DCT?o?b?t?@??i?[
    // ??????f?[?^??????l???????????A?O???DC?l????????l????
    colBak = col >> 1; // Y,Cb ?? colBak = 0?ACr ?? colBak = 1 ???????
    DCTBlock[blk_idx] = code + (*huffmanRequest->mPreviousDC[colBak]);
    *huffmanRequest->mPreviousDC[colBak] = DCTBlock[blk_idx];
    blk_idx++;

    // DC???????f?[?^??r?b?g?????I?t?Z?b?g??Z?b?g
    // ????f?[?^??A*bufPtr??????buf_ofs?r?b?g?V?t?g????
    // ???????r?b?g????n???
    buf_ofs += idt_bits + add_bits;
    // ????f?[?^??J?n??u???????
    while (buf_ofs >= 8)
    {
        bufPtr++;
        buf_ofs -= 8;
    }

    /*** AC????????? ***/
    while (1)
    {
        // ?[???????????O?X?????
        // ???????f?[?^??4?o?C?g???o???A?I?t?Z?b?g???V?t?g????i????l???j
        code = (u32)(((*bufPtr << 24) | (*(bufPtr + 1) << 16) | (*(bufPtr + 2) << 8) | *(bufPtr + 3)) << buf_ofs);

        // ?n?t?}???c???[?e?[?u????H?????????????????
        // ?[???????????O?X??J?e?S???[6????????????????6?r?b?g???
        for (idt_bits = 1, treeIdx = 0; idt_bits <= 6; idt_bits++)
        {
            nextOfs = ((hufftree[1])[treeIdx] & 0x3fff);
            if (((code >> (32 - idt_bits)) & 1) == 0)
            {
                // 0????A???q?m?[?h????
                if ((hufftree[1])[treeIdx] & 0x8000)
                {
                    // ???q?m?[?h???t???
                    add_bits = (hufftree[1])[treeIdx + nextOfs];
                    break;
                }
                else
                {
                    treeIdx += nextOfs;
                }
            }
            else
            {
                // 1????A?E?q?m?[?h????
                if ((hufftree[1])[treeIdx] & 0x4000)
                {
                    // ?E?q?m?[?h???t???
                    add_bits = (hufftree[1])[treeIdx + nextOfs + 1];
                    break;
                }
                else
                {
                    treeIdx += nextOfs + 1;
                }
            }
        }

        // ????????f?[?^??ODH?t?H?[?}?b?g??]????????????
        // ?s????e?[?u????????????
        if (idt_bits > 6)
            return DArCDJRESULT_ERR_HUFFCODE;

        if (add_bits == 7)
        {
            // ?????????EOB????????
            // DC???????f?[?^??r?b?g?????I?t?Z?b?g??Z?b?g
            buf_ofs += idt_bits;
            // ????f?[?^??J?n??u???????
            while (buf_ofs >= 8)
            {
                bufPtr++;
                buf_ofs -= 8;
            }
            break;
        }
        else
        {
            if (add_bits > 0)
            {
                // ?????????f?[?^??????
                // ?????????O?X??????A?????????O?X????0??DCT?o?b?t?@?????????
                // ????t???r?b?g???????
                bit_mask = (u32)((1 << add_bits) - 1); // ?t???r?b?g???????}?X?N?l???????
                code = (code >> (32 - idt_bits - add_bits)) & bit_mask;

                // ????r?b?g??0?????????????????
                // ?????????O?X??K???????????G???[????
                if ((code & (1 << (add_bits - 1))) == 0)
                {
                    // code = (code+1) | ~bit_mask;
                    return DArCDJRESULT_ERR_HUFFCODE;
                }
            }
            else
            {
                // ?l??0?????t???r?b?g???????
                code = 0;
            }

            // ?[???????????O?X????0??DCT?o?b?t?@??i?[
            for (i = 0; i < code; i++)
            {
                DCTBlock[blk_idx] = 0;
                blk_idx++;
            }

            // DC???????f?[?^??r?b?g?????I?t?Z?b?g??Z?b?g
            // ????f?[?^??A*bufPtr??????buf_ofs?r?b?g?V?t?g????
            // ???????r?b?g????n???
            buf_ofs += idt_bits + add_bits;
            // ????f?[?^??J?n??u???????
            while (buf_ofs >= 8)
            {
                bufPtr++;
                buf_ofs -= 8;
            }

            // ??[???W???????
            // ???????f?[?^??4?o?C?g???o???A?I?t?Z?b?g???V?t?g????i????l???j
            code = (u32)(((*bufPtr << 24) | (*(bufPtr + 1) << 16) | (*(bufPtr + 2) << 8) | *(bufPtr + 3)) << buf_ofs);

            if (col == 0)
            {
                // Y????????????????9?r?b?g
                idt_bits_max_DC = 9;
            }
            else
            {
                // Cb,Cr????????????????11?r?b?g
                idt_bits_max_DC = 11;
            }

            // ?n?t?}???c???[?e?[?u????H?????????????????
            for (idt_bits = 1, treeIdx = 0; idt_bits <= idt_bits_max_DC; idt_bits++)
            {
                nextOfs = ((hufftree[0])[treeIdx] & 0x3fff);
                if (((code >> (32 - idt_bits)) & 1) == 0)
                {
                    // 0????A???q?m?[?h????
                    if ((hufftree[0])[treeIdx] & 0x8000)
                    {
                        // ???q?m?[?h???t???
                        add_bits = (hufftree[0])[treeIdx + nextOfs];
                        break;
                    }
                    else
                    {
                        treeIdx += nextOfs;
                    }
                }
                else
                {
                    // 1????A?E?q?m?[?h????
                    if ((hufftree[0])[treeIdx] & 0x4000)
                    {
                        // ?E?q?m?[?h???t???
                        add_bits = (hufftree[0])[treeIdx + nextOfs + 1];
                        break;
                    }
                    else
                    {
                        treeIdx += nextOfs + 1;
                    }
                }
            }

            // ????????f?[?^??ODH?t?H?[?}?b?g??]????????????
            // ?s????e?[?u????????????
            if (idt_bits > idt_bits_max_DC)
                return DArCDJRESULT_ERR_HUFFCODE;

            if (add_bits > 0)
            {
                // ????t???r?b?g???????
                bit_mask = (u32)((1 << add_bits) - 1); // ?t???r?b?g???????}?X?N?l???????
                code = (code >> (32 - idt_bits - add_bits)) & bit_mask;

                // ????r?b?g??0?????????????????
                if ((code & (1 << (add_bits - 1))) == 0)
                {
                    code = (code + 1) | ~bit_mask;
                }
            }
            else
            {
                // ?l??0?????t???r?b?g???????
                code = 0;
            }

            // ???????????f?[?^??DCT?o?b?t?@??i?[
            DCTBlock[blk_idx] = code;
            blk_idx++;

            // DC???????f?[?^??r?b?g?????I?t?Z?b?g??Z?b?g
            // ????f?[?^??A*bufPtr??????buf_ofs?r?b?g?V?t?g????
            // ???????r?b?g????n???
            buf_ofs += idt_bits + add_bits;
            // ????f?[?^??J?n??u???????
            while (buf_ofs >= 8)
            {
                bufPtr++;
                buf_ofs -= 8;
            }

            if (blk_idx >= (DArCDJ_DCT_BUFFER_SIZE >> 3))
            {
                // EOB????????????AC?????l??????????
                break;
            }
        }
    }

    /*while (1){
        // ???????f?[?^??4?o?C?g???o???A?I?t?Z?b?g???V?t?g????i????l???j
        code = (u32)(((*bufPtr << 24) | (*(bufPtr+1) << 16)
            | (*(bufPtr+2) << 8) | *(bufPtr+3)) << buf_ofs);

        // ?n?t?}???c???[?e?[?u????H?????????????????
        for (idt_bits = 1, treeIdx = 0; idt_bits < 17; idt_bits++){
            nextOfs = ((hufftree[1])[treeIdx] & 0x3fff);
            if (((code >> (32-idt_bits)) & 1) == 0){
                // 0????A???q?m?[?h????
                if ((hufftree[1])[treeIdx] & 0x8000){
                    // ???q?m?[?h???t???
                    add_bits = (hufftree[1])[treeIdx+nextOfs];
                    break;
                }
                else {
                    treeIdx += nextOfs;
                }
            }
            else {
                // 1????A?E?q?m?[?h????
                if ((hufftree[1])[treeIdx] & 0x4000){
                    // ?E?q?m?[?h???t???
                    add_bits = (hufftree[1])[treeIdx+nextOfs+1];
                    break;
                }
                else {
                    treeIdx += nextOfs+1;
                }
            }
        }

        // ????????f?[?^??ODH?t?H?[?}?b?g??]????????????
        // ?s????e?[?u????????????
        if (idt_bits == 17) return DArCDJRESULT_ERR_HUFFCODE;

        if (add_bits == 0){
            // ?????????EOB????????
            // DC???????f?[?^??r?b?g?????I?t?Z?b?g??Z?b?g
            buf_ofs += idt_bits;
            // ????f?[?^??J?n??u???????
            while (buf_ofs >= 8){
                bufPtr++;
                buf_ofs -= 8;
            }
            break;
        }
        else if (add_bits == 240){
            // ?????????RUN*16?????????A0??16??DCT?o?b?t?@??i?[
            for (i = 0; i < 16; i++){
                DCTBlock[blk_idx] = 0;
                blk_idx++;
            }
            add_bits = 0;  // ???I?t?Z?b?g?????????????N???A???????
        }
        else {
            // ?????????f?[?^??????
            // ?????????O?X??????A?????????O?X????0??DCT?o?b?t?@?????????
            // AC?e?[?u????\????Aadd_bits??16??????????????O?X????
            run = add_bits >> 4;
            for (i = 0; i < run; i++){
                DCTBlock[blk_idx] = 0;
                blk_idx++;
            }

            // ????t???r?b?g???????
            // AC?e?[?u????\????Aadd_bits??16????????]??i????4?r?b?g?j??
            // ?t???r?b?g????????A??????????
            add_bits &= 0xf;

            if ((buf_ofs+idt_bits+add_bits) > 32){
                // ?I?t?Z?b?g?{??????????{?t???r?b?g????32?r?b?g???z??????
                // ?I?t?Z?b?g????l??7?A?????f?[?^??????l??26????
                // ?????33?r?b?g??????????

                // ????1?o?C?g??f?[?^????????t??????
                // ?????f?[?^??32?r?b?g???z???????????O??????????
                code |= *(bufPtr+4) >> (8-buf_ofs);
            }

            bit_mask = (u32)((1 << add_bits)-1);  // ?t???r?b?g???????}?X?N?l???????
            code     = (code >> (32-idt_bits-add_bits)) & bit_mask;

            // ????r?b?g??0?????????????????
            if ((code & (1 << (add_bits-1))) == 0){
                code = (code+1) | ~bit_mask;
            }

            // ???????????f?[?^??DCT?o?b?t?@??i?[
            DCTBlock[blk_idx] = code;
            blk_idx++;
        }

        // DC???????f?[?^??r?b?g?????I?t?Z?b?g??Z?b?g
        // ????f?[?^??AbufPtr??????buf_ofs?r?b?g?V?t?g????
        // ???????r?b?g????n???
        buf_ofs += idt_bits+add_bits;
        // ????f?[?^??J?n??u???????
        while (buf_ofs >= 8){
            bufPtr++;
            buf_ofs -= 8;
        }

        if (blk_idx >= (DArCDJ_DCT_BUFFER_SIZE >> 3)){
            // EOB????????????AC?????l??????????
            break;
        }
    }*/

    // EOB??????????ADCT?u???b?N??c??S????0??????
    for (; blk_idx < (DArCDJ_DCT_BUFFER_SIZE >> 3); blk_idx++)
    {
        DCTBlock[blk_idx] = 0;
    }

    // ?????f?[?^???????|?C???^??A?I?t?Z?b?g?r?b?g???????????
    huffmanRequest->mCodeBuffer = bufPtr;
    *(huffmanRequest->mHuffmanBufferRemain) = buf_ofs;

    return DArCDJRESULT_SUCCESS;
}

/********************************************************************/
/*                          ?tDCT???                               */
/********************************************************************/
void CArGBAOdh::idct_fast(const u8 *range_limit, u32 *quant_table,
                          u32 *DCTBuffer, u8 *output_buf, u32 pixelOffset)
{
    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int tmp10, tmp11, tmp12, tmp13;
    int z5, z10, z11, z12, z13;
    s32 *inptr;
    s32 *quantptr;
    int *wsptr;
    u8 *outptr;
    u8 dcval;
    int dcval_tmp;
    int ctr;
    int workspace[DArCDJ_DCT_SIZE_2D]; /* buffers data between passes */

    /* Pass 1: process columns from input, store into work array. */

    inptr = (s32 *)DCTBuffer;
    quantptr = (s32 *)quant_table;
    wsptr = workspace;
    for (ctr = DArCDJ_DCT_SIZE_1D; ctr > 0; ctr--)
    {

        if ((inptr[DArCDJ_DCT_SIZE_1D * 1] == 0) && (inptr[DArCDJ_DCT_SIZE_1D * 2] == 0) && (inptr[DArCDJ_DCT_SIZE_1D * 3] == 0) && (inptr[DArCDJ_DCT_SIZE_1D * 4] == 0) && (inptr[DArCDJ_DCT_SIZE_1D * 5] == 0) && (inptr[DArCDJ_DCT_SIZE_1D * 6] == 0) && (inptr[DArCDJ_DCT_SIZE_1D * 7] == 0))
        {
            /* AC terms all zero */
            dcval_tmp = inptr[DArCDJ_DCT_SIZE_1D * 0] * quantptr[DArCDJ_DCT_SIZE_1D * 0];

            wsptr[DArCDJ_DCT_SIZE_1D * 0] = dcval_tmp;
            wsptr[DArCDJ_DCT_SIZE_1D * 1] = dcval_tmp;
            wsptr[DArCDJ_DCT_SIZE_1D * 2] = dcval_tmp;
            wsptr[DArCDJ_DCT_SIZE_1D * 3] = dcval_tmp;
            wsptr[DArCDJ_DCT_SIZE_1D * 4] = dcval_tmp;
            wsptr[DArCDJ_DCT_SIZE_1D * 5] = dcval_tmp;
            wsptr[DArCDJ_DCT_SIZE_1D * 6] = dcval_tmp;
            wsptr[DArCDJ_DCT_SIZE_1D * 7] = dcval_tmp;

            inptr++; /* advance pointers to next column */
            quantptr++;
            wsptr++;
            continue;
        }

        /* Even part */

        tmp0 = inptr[DArCDJ_DCT_SIZE_1D * 0] * quantptr[DArCDJ_DCT_SIZE_1D * 0];
        tmp1 = inptr[DArCDJ_DCT_SIZE_1D * 2] * quantptr[DArCDJ_DCT_SIZE_1D * 2];
        tmp2 = inptr[DArCDJ_DCT_SIZE_1D * 4] * quantptr[DArCDJ_DCT_SIZE_1D * 4];
        tmp3 = inptr[DArCDJ_DCT_SIZE_1D * 6] * quantptr[DArCDJ_DCT_SIZE_1D * 6];

        tmp10 = tmp0 + tmp2; /* phase 3 */
        tmp11 = tmp0 - tmp2;

        tmp13 = tmp1 + tmp3;                                          /* phases 5-3 */
        tmp12 = DArMULTIPLY(tmp1 - tmp3, DArFIX_1_414213562) - tmp13; /* 2*c4 */

        tmp0 = tmp10 + tmp13; /* phase 2 */
        tmp3 = tmp10 - tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;

        /* Odd part */

        tmp4 = inptr[DArCDJ_DCT_SIZE_1D * 1] * quantptr[DArCDJ_DCT_SIZE_1D * 1];
        tmp5 = inptr[DArCDJ_DCT_SIZE_1D * 3] * quantptr[DArCDJ_DCT_SIZE_1D * 3];
        tmp6 = inptr[DArCDJ_DCT_SIZE_1D * 5] * quantptr[DArCDJ_DCT_SIZE_1D * 5];
        tmp7 = inptr[DArCDJ_DCT_SIZE_1D * 7] * quantptr[DArCDJ_DCT_SIZE_1D * 7];

        z13 = tmp6 + tmp5; /* phase 6 */
        z10 = tmp6 - tmp5;
        z11 = tmp4 + tmp7;
        z12 = tmp4 - tmp7;

        tmp7 = z11 + z13;                                   /* phase 5 */
        tmp11 = DArMULTIPLY(z11 - z13, DArFIX_1_414213562); /* 2*c4 */

        z5 = DArMULTIPLY(z10 + z12, DArFIX_1_847759065);    /* 2*c2 */
        tmp10 = DArMULTIPLY(z12, DArFIX_1_082392200) - z5;  /* 2*(c2-c6) */
        tmp12 = DArMULTIPLY(z10, -DArFIX_2_613125930) + z5; /* -2*(c2+c6) */

        tmp6 = tmp12 - tmp7; /* phase 2 */
        tmp5 = tmp11 - tmp6;
        tmp4 = tmp10 + tmp5;

        wsptr[DArCDJ_DCT_SIZE_1D * 0] = (int)(tmp0 + tmp7);
        wsptr[DArCDJ_DCT_SIZE_1D * 7] = (int)(tmp0 - tmp7);
        wsptr[DArCDJ_DCT_SIZE_1D * 1] = (int)(tmp1 + tmp6);
        wsptr[DArCDJ_DCT_SIZE_1D * 6] = (int)(tmp1 - tmp6);
        wsptr[DArCDJ_DCT_SIZE_1D * 2] = (int)(tmp2 + tmp5);
        wsptr[DArCDJ_DCT_SIZE_1D * 5] = (int)(tmp2 - tmp5);
        wsptr[DArCDJ_DCT_SIZE_1D * 4] = (int)(tmp3 + tmp4);
        wsptr[DArCDJ_DCT_SIZE_1D * 3] = (int)(tmp3 - tmp4);

        inptr++; /* advance pointers to next column */
        quantptr++;
        wsptr++;
    }

    /* Pass 2: process rows from work array, store into output array. */
    /* Note that we must descale the results by a factor of 8 == 2**3, */
    /* and also undo the PASS1_BITS scaling. */

    wsptr = workspace;
    for (ctr = 0; ctr < DArCDJ_DCT_SIZE_1D; ctr++)
    {
        outptr = output_buf + ctr * pixelOffset;

        if ((wsptr[1] == 0) && (wsptr[2] == 0) && (wsptr[3] == 0) && (wsptr[4] == 0) && (wsptr[5] == 0) && (wsptr[6] == 0) && (wsptr[7] == 0))
        {
            /* AC terms all zero */
            dcval = range_limit[DArDESCALE(wsptr[0], 5) & DArRANGE_MASK];
            outptr[0] = dcval;
            outptr[1] = dcval;
            outptr[2] = dcval;
            outptr[3] = dcval;
            outptr[4] = dcval;
            outptr[5] = dcval;
            outptr[6] = dcval;
            outptr[7] = dcval;

            wsptr += DArCDJ_DCT_SIZE_1D; /* advance pointer to next row */
            continue;
        }

        /* Even part */

        tmp10 = ((int)wsptr[0] + (int)wsptr[4]);
        tmp11 = ((int)wsptr[0] - (int)wsptr[4]);

        tmp13 = ((int)wsptr[2] + (int)wsptr[6]);
        tmp12 = DArMULTIPLY((int)wsptr[2] - (int)wsptr[6], DArFIX_1_414213562) - tmp13;

        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;

        /* Odd part */

        z13 = (int)wsptr[5] + (int)wsptr[3];
        z10 = (int)wsptr[5] - (int)wsptr[3];
        z11 = (int)wsptr[1] + (int)wsptr[7];
        z12 = (int)wsptr[1] - (int)wsptr[7];

        tmp7 = z11 + z13;                                   /* phase 5 */
        tmp11 = DArMULTIPLY(z11 - z13, DArFIX_1_414213562); /* 2*c4 */

        z5 = DArMULTIPLY(z10 + z12, DArFIX_1_847759065);    /* 2*c2 */
        tmp10 = DArMULTIPLY(z12, DArFIX_1_082392200) - z5;  /* 2*(c2-c6) */
        tmp12 = DArMULTIPLY(z10, -DArFIX_2_613125930) + z5; /* -2*(c2+c6) */

        tmp6 = tmp12 - tmp7; /* phase 2 */
        tmp5 = tmp11 - tmp6;
        tmp4 = tmp10 + tmp5;

        /* Final output stage: scale down by a factor of 8 and range-limit */
        outptr[0] = range_limit[DArDESCALE(tmp0 + tmp7, 5) & DArRANGE_MASK];
        outptr[7] = range_limit[DArDESCALE(tmp0 - tmp7, 5) & DArRANGE_MASK];
        outptr[1] = range_limit[DArDESCALE(tmp1 + tmp6, 5) & DArRANGE_MASK];
        outptr[6] = range_limit[DArDESCALE(tmp1 - tmp6, 5) & DArRANGE_MASK];
        outptr[2] = range_limit[DArDESCALE(tmp2 + tmp5, 5) & DArRANGE_MASK];
        outptr[5] = range_limit[DArDESCALE(tmp2 - tmp5, 5) & DArRANGE_MASK];
        outptr[4] = range_limit[DArDESCALE(tmp3 + tmp4, 5) & DArRANGE_MASK];
        outptr[3] = range_limit[DArDESCALE(tmp3 - tmp4, 5) & DArRANGE_MASK];

        wsptr += DArCDJ_DCT_SIZE_1D; /* advance pointer to next row */
    }
}

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

int decode(uint8_t *cdbfile, uint32_t attachment_size, uint32_t attachment_offset, char *outpath)
{
    uint32_t fileSize = sizeof(cdbfile); // Retrieve the total file size

    // Adjust the pointer to the start of the data
    uint8_t *src = (uint8_t *)malloc(attachment_size);

    // Copy the data from cdbfile to src
    memcpy(src, cdbfile + attachment_offset, attachment_size);

    CArGBAOdh GBAOdh;

    std::vector<u8>
        dst = std::vector<u8>(GBAOdh.getOdhWidth(src) * GBAOdh.getOdhHeight(src) * 4);

    std::vector<u8> work = std::vector<u8>(GBAOdh.getOdhWidth(src) * GBAOdh.getOdhHeight(src) * 3);

    // int abc = ODHDecodeY8U8V8(src.data(), dst.data(), (u8 *)work);
    // abc = ODHDecodeRGBA8(src.data(), (u8 *)dst, (u8 *)work);
    int abc = ODHDecodeRGBA8(src, dst.data(), work.data());

    void *pngbuffer = malloc(dst.size());

    unsigned char *ctx = PNGU_EncodeFromGXTexture(PNGU_SelectImageFromBuffer(pngbuffer), GBAOdh.getOdhWidth(src), GBAOdh.getOdhHeight(src), (void *)dst.data(), 0);

    FILE *outputFilez = fopen(outpath, "wb");

    fwrite(ctx, (dst.size() / 2), 1, outputFilez);

    fclose(outputFilez);

    free(pngbuffer);

    free(src);

    return 0;
}