//
// Created by hitmoon on 15-12-9.
//

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "SMS.h"
#include "alphabet.h"


char temp[256];

char *sub_str(const char *str, int start, int size) {
    memset(temp, 0, 256);
    strncpy(temp, str + start, size);
    return temp;
}

SMS_Struct SMS::PDUDecoding(const char *data) {

    SMS_Struct sms;
    int end_index;
    int PDUType;
    // 短信中心
    sms.SCA = SCADecoding(data, end_index);

    // 协议数据单元类型
    PDUType = strtol(sub_str(data, end_index, 2), NULL, 16);
    end_index += 2;

    sms.RP = PDUType & (1 << 7) ? true : false;   // 应答路径
    sms.UDHI = PDUType & (1 << 6) ? true : false;  // 用户数据头标识
    sms.SRI = PDUType & (1 << 5) ? true : false;  // 状态报告指示
    sms.MMS = PDUType & (1 << 2) ? false : true;  // 更多信息发送
    sms.MTI = PDUType & 3;                        // 信息类型指示

    // 发送方SME的地址
    sms.OA = OADecoding(data, end_index, end_index);

    // 协议标识
    sms.PID = strtol(sub_str(data, end_index, 2), NULL, 16);
    end_index += 2;

    // 数据编码方案
    int DCSType = strtol(sub_str(data, end_index, 2), NULL, 16);
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


char *SMS::SCADecoding(const char *data, int &EndIndex) {
    int len;

    char *result;
    char *buf;

    len = strtol(sub_str(data, 0, 2), NULL, 16);
    if (len == 0) {
        EndIndex = 2;
        return NULL;
    }

    result = (char *) malloc(sizeof(char) * len * 2 + 1);
    memset(result, 0, sizeof(char) * len * 2);

    buf = result;

    // 服务中心地址类型
    if (strncmp(data + 2, "91", 2) == 0) {
        sprintf(buf++, "+");
    }

    // 服务中心地址
    EndIndex = (len + 1) * 2;
    for (int i = 4; i < EndIndex; i += 2) {
        sprintf(buf++, "%c", data[i + 1]);
        sprintf(buf++, "%c", data[i]);
    }

    //  去掉填充的 'F'
    if (result[strlen(result) - 1] == 'F') {
        result[strlen(result) - 1] = '\0';
    }

    return result;
}

char *SMS::OADecoding(const char *data, int index, int &EndIndex) {
    int len;
    char *result, *buf;

    len = strtol(sub_str(data, index, 2), NULL, 16);

    if (len == 0) {
        EndIndex = index + 2;
        return NULL;
    }

    result = (char *) malloc(sizeof(char) * len + 2);
    memset(result, 0, sizeof(char) * len + 2);
    buf = result;
    if (strncmp(data + index + 2, "91", 2) == 0) {
        sprintf(buf++, "+");
    }

    // 电话号码
    for (int i = 0; i < len; i += 2) {
        sprintf(buf++, "%c", data[index + i + 5]);
        sprintf(buf++, "%c", data[index + i + 4]);

    }

    EndIndex = index + 4 + len;
    if (len % 2 != 0) {
        result[strlen(result) - 1] = '\0';
        EndIndex++;
    }
    return result;
}

char *SMS::SCTSDecoding(const char *data, int index) {

    char *result;

    result = (char *) malloc(64);
    sprintf(result, "20%02d-%02d-%02d %02d:%02d:%02d",
            BCDDecoding(data, index, 0),                // 年
            BCDDecoding(data, index + 2, 0),            // 月
            BCDDecoding(data, index + 4, 0),            // 日
            BCDDecoding(data, index + 6, 0),            // 时
            BCDDecoding(data, index + 8, 0),            // 分
            BCDDecoding(data, index + 10, 0)            // 秒

    );
    return result;
}

int SMS::BCDDecoding(const char *data, int index, bool isMSB) {

    int n1, n10;

    n1 = strtol(sub_str(data, index, 1), NULL, 10);
    n10 = strtol(sub_str(data, index + 1, 1), NULL, 10);

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

struct PDUUDH *SMS::UDHDecoding(const char *data, int index) {

    int len;
    struct PDUUDH *result;

    len = strtol(sub_str(data, index, 2), NULL, 16);
    index += 2;
    int i = 0;

    result = (struct PDUUDH *) malloc(sizeof(struct PDUUDH) * len + 1);

    while (i < len) {
        // 信息元素标识（Information Element Identifier
        char IEI = strtol(sub_str(data, index, 2), NULL, 16);
        index += 2;
        // 信息元素数据长度（Length of Information Element）
        int IEDL = strtol(sub_str(data, index, 2), NULL, 16);
        index += 2;
        // 信息元素数据（Information Element Data）
        char *IED = (char *) malloc(sizeof(char) * IEDL);
        for (int j = 0; j < IEDL; j++) {
            IED[j] = *(data + index);
            index += 2;
        }

        i += IEDL + 2;
    }

    return result;
}

char *SMS::UserDataDecoding(const char *data, int index, bool UDHI, EnumDCS dcs) {
    char *result;

    // 用户数据区长度
    int UDL = strtol(sub_str(data, index, 2), NULL, 16);
    index += 2;

    //printf("UDL = %d\n", UDL);
    // 跳过用户数据头
    int UDHL = 0;
    if (UDHI) {
        // 用户数据头长度
        UDHL = strtol(sub_str(data, index, 2), NULL, 16);
        UDHL++;
        index += UDHL << 1;

    }
    //printf("userdata data = %s\n", data + index);
    // 获取用户数据
    if (dcs == UCS2) {
        int len = (UDL - UDHL) >> 1;

        result = (char *) malloc(sizeof(char) * len + 1);
        for (int i = 0; i < len; i++) {
            unsigned int d2 = strtol(sub_str(data, (i << 2) + index, 4), NULL, 16);
        }
        // TODO
    }
    else if (dcs == BIT7) {
        int Septets = UDL - (UDHL * 8 + 6) / 7;           // 7-Bit编码字符数

        int FillBits = (UDHL * 8 + 6) / 7 * 7 - UDHL * 8; //  填充位数
        return BIT7Decoding(BIT7Unpack(data, index, Septets, FillBits), Septets);
    }
    else {// 8Bit编码
        // 获取数据长度
        UDL -= UDHL;
        result = (char *) malloc(UDL + 1);
        for (int i = 0; i < UDL; i++) {
            result[i] = strtol(sub_str(data, (i << 1) + index, 2), NULL, 16);
        }

        return result;
    }
}

char *SMS::BIT7Unpack(const char *data, int index, int Septets, int FillBits) {
    char *result;

    result = (char *) malloc(sizeof(char) * Septets + 1);
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
        int Value = strtol(sub_str(data, (i << 1) + index, 2), NULL, 16);
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

char *SMS::BIT7Decoding(char *BIT7Data, unsigned int size) {
    char *result, *buf;

    result = (char *) malloc(size + 1);
    buf = result;
    for (int i = 0; i < size; i++) {
        unsigned short key = BIT7Data[i];
        if (isBIT7Same(key)) {
            // TODO 转换为utf
            //printf("key[%d] = %d\n", i, key);
            sprintf(buf++, "%c", key);
            //printf("go a\n");

        }
        else if (map_get_key(BIT7ToUCS2, map_size(BIT7ToUCS2), key) > 0) {
            unsigned short value;
            if (key == 0x1B) { // 转义字符
                value = map_get_key(BIT7EToUCS2, map_size(BIT7EToUCS2), BIT7Data[i + 1]);
                if (i < size - 1 && value > 0) {
                    // TODO 转换为utf
                    sprintf(buf++, "%c", value);
                    i++;
                }
                else {
                    value = map_get_key(BIT7ToUCS2, map_size(BIT7ToUCS2), key);
                    // TODO 转换为utf
                    sprintf(buf++, "%c", value);
                }
            }
            else {
                //printf("go b\n");
                value = map_get_key(BIT7ToUCS2, map_size(BIT7ToUCS2), key);
                //printf("value = %u\n", value);
                // TODO 转换为utf
                sprintf(buf++, "%c", value);

            }
        }
        else {// 异常数据
            sprintf(buf++, "?");
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
