//
// Created by hitmoon on 15-12-9.
//

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "SMS.h"
#include "alphabet.h"

wchar_t temp[256];

wchar_t *sub_str(const wchar_t *str, int start, int size) {
    wmemset(temp, L'\0', 256);
    wcsncpy(temp, str + start, size);
    return temp;
}

SMS_Struct SMS::PDUDecoding(const wchar_t *data) {

    SMS_Struct sms;
    int end_index;
    int PDUType;
    // 短信中心
    sms.SCA = SCADecoding(data, end_index);

    // 协议数据单元类型
    PDUType = wcstol(sub_str(data, end_index, 2), NULL, 16);
    end_index += 2;

    sms.RP = PDUType & (1 << 7) ? true : false;   // 应答路径
    sms.UDHI = PDUType & (1 << 6) ? true : false;  // 用户数据头标识
    sms.SRI = PDUType & (1 << 5) ? true : false;  // 状态报告指示
    sms.MMS = PDUType & (1 << 2) ? false : true;  // 更多信息发送
    sms.MTI = PDUType & 3;                        // 信息类型指示

    // 发送方SME的地址
    sms.OA = OADecoding(data, end_index, end_index);

    // 协议标识
    sms.PID = wcstol(sub_str(data, end_index, 2), NULL, 16);
    end_index += 2;

    // 数据编码方案
    int DCSType = wcstol(sub_str(data, end_index, 2), NULL, 16);
    end_index += 2;

    // 文本压缩指示
    sms.TC = DCSType & (1 << 5);
    // 编码字符集
    sms.DCS = (EnumDCS) ((DCSType >> 2) & 3);

    if (DCSType & (1 << 4)) {
        // 信息类型信息 0：立即显示 1：移动设备特定类型 2：SIM特定类型 3：终端设备特定类型
        sms.MC = DCSType & 3;
    }
    else {
        // 不含信息类型信息
        sms.MC = -1;
    }
    // 服务中心时间戳（BCD编码）
    sms.SCTS = SCTSDecoding(data, end_index);
    end_index += 14;

    // 用户数据头
    if (sms.UDHI) {
        sms.UDH = UDHDecoding(data, end_index + 2);
    }
    else {
        sms.UDH = NULL;
    }

    // 用户数据
    sms.UD = UserDataDecoding(data, end_index, sms.UDHI, sms.DCS);

    return sms;
}


wchar_t *SMS::SCADecoding(const wchar_t *data, int &EndIndex) {
    int len;

    wchar_t *result;
    wchar_t *buf;

    len = wcstol(sub_str(data, 0, 2), NULL, 16);
    if (len == 0) {
        EndIndex = 2;
        return NULL;
    }

    EndIndex = (len + 1) * 2;

    result = (wchar_t *) malloc(sizeof(wchar_t) * len * 2);
    //wmemset(result, '0', sizeof(wchar_t) * (len * 2 + 1));


    buf = result;
    len *= 2;

    // 服务中心地址类型
    if (wcsncmp(data + 2, L"91", 2) == 0) {
        swprintf(buf++, len--, L"+");
    }

    // 服务中心地址

    for (int i = 4; i < EndIndex; i += 2) {
        swprintf(buf++, len--, L"%lc", data[i + 1]);
        swprintf(buf++, len--, L"%lc", data[i]);
    }

    //  去掉填充的 'F'
    if (result[wcslen(result) - 1] == L'F') {
        result[wcslen(result) - 1] = L'\0';
    }

    return result;
}

wchar_t *SMS::OADecoding(const wchar_t *data, int index, int &EndIndex) {
    int len;
    wchar_t *result, *buf;

    len = wcstol(sub_str(data, index, 2), NULL, 16);

    if (len == 0) {
        EndIndex = index + 2;
        return NULL;
    }

    EndIndex = index + 4 + len;

    result = (wchar_t *) malloc(sizeof(wchar_t) * (len + 2));
    //wmemset(result, 0, sizeof(wchar_t) * (len + 1));
    buf = result;
    if (wcsncmp(data + index + 2, L"91", 2) == 0) {
        swprintf(buf++, len, L"+");
    }

    // 电话号码
    for (int i = 0; i < len; i += 2) {
        swprintf(buf++, len, L"%lc", data[index + i + 5]);
        swprintf(buf++, len, L"%lc", data[index + i + 4]);

    }

    if (len % 2 != 0) {
        result[wcslen(result) - 1] = L'\0';
        EndIndex++;
    }
    return result;
}

wchar_t *SMS::SCTSDecoding(const wchar_t *data, int index) {

    wchar_t *result;

    result = (wchar_t *) malloc(sizeof(wchar_t) * 32);
    swprintf(result, 32, L"20%02d-%02d-%02d %02d:%02d:%02d",
             BCDDecoding(data, index, 0),                // 年
             BCDDecoding(data, index + 2, 0),            // 月
             BCDDecoding(data, index + 4, 0),            // 日
             BCDDecoding(data, index + 6, 0),            // 时
             BCDDecoding(data, index + 8, 0),            // 分
             BCDDecoding(data, index + 10, 0)            // 秒

    );
    return result;
}

int SMS::BCDDecoding(const wchar_t *data, int index, bool isMSB) {

    int n1, n10;

    n1 = wcstol(sub_str(data, index, 1), NULL, 10);
    n10 = wcstol(sub_str(data, index + 1, 1), NULL, 10);

    if (isMSB) {
        if (n10 >= 8)
            return -((n10 - 8) * 10 + n1); // 负值
        else
            return n10 * 10 + n1;
    }
    else {
        return n10 * 10 + n1;
    }
}

struct PDUUDH *SMS::UDHDecoding(const wchar_t *data, int index) {

    int len;
    struct PDUUDH *result;

    len = wcstol(sub_str(data, index, 2), NULL, 16);
    index += 2;
    int i = 0;
    int count = 0;

    result = (struct PDUUDH *) malloc(sizeof(struct PDUUDH) * len);
    memset(result, 0, sizeof(struct PDUUDH) * len);

    while (i < len) {
        // 信息元素标识（Information Element Identifier
        wchar_t IEI = wcstol(sub_str(data, index, 2), NULL, 16);
        index += 2;
        // 信息元素数据长度（Length of Information Element）
        int IEDL = wcstol(sub_str(data, index, 2), NULL, 16);
        index += 2;
        // 信息元素数据（Information Element Data）
        wchar_t *IED = (wchar_t *) malloc(sizeof(wchar_t) * (IEDL + 1));
        for (int j = 0; j < IEDL; j++) {
            IED[j] = wcstol(sub_str(data, index, 2), NULL, 16);
            index += 2;
        }
        result[count].IEI = IEI;
        result[count].IED = IED;
        i += IEDL + 2;
    }

    return result;
}

wchar_t *SMS::UserDataDecoding(const wchar_t *data, int index, bool UDHI, EnumDCS dcs) {
    wchar_t *result = NULL;
    wchar_t *buf;

    //wprintf(L"userdata data start= %ls\n", data + index);

    // 用户数据区长度
    int UDL = wcstol(sub_str(data, index, 2), NULL, 16);
    index += 2;

    //wprintf(L"UDL = %d\n", UDL);
    // 跳过用户数据头
    int UDHL = 0;
    if (UDHI) {
        // 用户数据头长度
        UDHL = wcstol(sub_str(data, index, 2), NULL, 16);
        UDHL++;
        index += UDHL << 1;

    }
    //wprintf(L"UDHL = %d\n", UDHL);
    //wprintf(L"userdata data = %ls\n", data + index);
    // 获取用户数据
    if (dcs == UCS2) {
        int len = (UDL - UDHL) >> 1;

        result = (wchar_t *) malloc(sizeof(wchar_t) * (len + 1));
        buf = result;
        for (int i = 0; i < len; i++) {
            wchar_t d = wcstol(sub_str(data, (i << 2) + index, 4), NULL, 16);
            swprintf(buf++, len, L"%lc", d);
        }

        result[wcslen(result)] = L'\0';
        return result;
    }
    else if (dcs == BIT7) {
        int Septets = UDL - (UDHL * 8 + 6) / 7;           // 7-Bit编码字符数

        int FillBits = (UDHL * 8 + 6) / 7 * 7 - UDHL * 8; //  填充位数
        return BIT7Decoding(BIT7Unpack(data, index, Septets, FillBits), Septets);
    }
    else {// 8Bit编码
        // 获取数据长度
        UDL -= UDHL;
        result = (wchar_t *) malloc(sizeof(wchar_t) * (UDL + 1));
        for (int i = 0; i < UDL; i++) {
            result[i] = wcstol(sub_str(data, (i << 1) + index, 2), NULL, 16);
        }
        return result;
    }

    return L"Error!";
}

wchar_t *SMS::BIT7Unpack(const wchar_t *data, int index, int Septets, int FillBits) {
    wchar_t *result;

    result = (wchar_t *) malloc(sizeof(wchar_t) * (Septets + 1));
    // 每8个7-Bit编码字符存放到7个字节
    int PackLen = (Septets * 7 + FillBits + 7) / 8;
    int n = 0;
    int left = 0;


//    printf("Unapck data = %s\n", data + index);
//    printf("Septets = %d\n", Septets);
//    printf("pack len = %d\n", PackLen);
//    printf("fillbits = %d\n", FillBits);

    for (int i = 0; i < PackLen; i++) {

        int Order = (i + (7 - FillBits)) % 7;
        int Value = wcstol(sub_str(data, (i << 1) + index, 2), NULL, 16);
        if (i != 0 || FillBits == 0) {
            result[n++] = ((Value << Order) + left) & 0x7F;
        }
//        printf("left = %d, i = %d, order = %d, value = %d\n", left, i, Order, Value);
//        printf("result[%d] = %d\n", n - 1, result[n - 1]);
        left = Value >> (7 - Order);
        if (Order == 6) {
            if (n == Septets)
                break;
            result[n++] = left;
            //printf("result[%d] = %d\n", n - 1, result[n - 1]);
            left = 0;
        }
    }
    //printf("BIT7 Unpack = %s\n", result);
    return result;
}

wchar_t *SMS::BIT7Decoding(wchar_t *BIT7Data, unsigned int size) {
    wchar_t *result, *buf;

    result = (wchar_t *) malloc(sizeof(wchar_t) * (size + 1));
    buf = result;
    for (int i = 0; i < size; i++) {
        unsigned short key = BIT7Data[i];
        if (isBIT7Same(key)) {
            swprintf(buf++, size, L"%c", key);
        }
        else if (map_get_key(BIT7ToUCS2, map_size(BIT7ToUCS2), key) > 0) {
            unsigned short value;
            if (key == 0x1B) { // 转义字符
                value = map_get_key(BIT7EToUCS2, map_size(BIT7EToUCS2), BIT7Data[i + 1]);
                if (i < size - 1 && value > 0) {
                    swprintf(buf++, size, L"%c", value);
                    i++;
                }
                else {
                    value = map_get_key(BIT7ToUCS2, map_size(BIT7ToUCS2), key);
                    swprintf(buf++, size, L"%c", value);
                }
            }
            else {
                //printf("go b\n");
                value = map_get_key(BIT7ToUCS2, map_size(BIT7ToUCS2), key);
                //printf("value = %u\n", value);
                swprintf(buf++, size, L"%c", value);

            }
        }
        else {// 异常数据
            swprintf(buf++, size, L"?");
        }

    }
    return result;

}

int SMS::isBIT7Same(unsigned short UCS2) {
    if (UCS2 >= 0x61 && UCS2 <= 0x7A ||
        UCS2 >= 0x41 && UCS2 <= 0x5A ||
        UCS2 >= 0x25 && UCS2 <= 0x3F ||
        UCS2 >= 0x20 && UCS2 <= 0x23 ||
        UCS2 == 0x0A || UCS2 == 0x0D) {
        return 1;
    }
    return 0;
}
