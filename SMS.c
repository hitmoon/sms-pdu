//
// Created by hitmoon on 15-12-9.
//

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "SMS.h"
#include "alphabet.h"

#define SUB_STR_SIZE 512
char temp[SUB_STR_SIZE];

// some constant

//长短信信息元素参考号
enum EnumCSMIEI mCSMIEI;
// 服务中心地址
char *mSCA;
// 请求状态报告
bool mSRR;
// 拒绝副本
bool mRD;
// 短信有效期
char *mVP;
// 长短信信息元素消息参考号
int mCSMMR;

// initialize PDU constants
void sms_init()
{
    mCSMMR = 0;
    mRD = false;
    mSRR = false;
    mSCA = "";
    mVP = "";
    mCSMIEI = BIT8MIEI;
}


char *sub_str(const char *str, int start, int size) {
    memset(temp, '\0', SUB_STR_SIZE);
    if (size > 0)
        strncpy(temp, str + start, size);
    else if (size < 0)
        strcpy(temp, str + start);

    return temp;
}

struct SMS_Struct PDUDecoding(const char *data) {

    struct SMS_Struct sms;
    int end_index;
    int PDUType;
    // 短信中心
    sms.SCA = SCADecoding(data, &end_index);

    // 协议数据单元类型
    PDUType = strtol(sub_str(data, end_index, 2), NULL, 16);
    end_index += 2;

    sms.RP = PDUType & (1 << 7) ? true : false;   // 应答路径
    sms.UDHI = PDUType & (1 << 6) ? true : false;  // 用户数据头标识
    sms.SRI = PDUType & (1 << 5) ? true : false;  // 状态报告指示
    sms.MMS = PDUType & (1 << 2) ? false : true;  // 更多信息发送
    sms.MTI = PDUType & 3;                        // 信息类型指示

    // 发送方SME的地址
    sms.OA = OADecoding(data, end_index, &end_index);

    // 协议标识
    sms.PID = strtol(sub_str(data, end_index, 2), NULL, 16);
    end_index += 2;

    // 数据编码方案
    int DCSType = strtol(sub_str(data, end_index, 2), NULL, 16);
    end_index += 2;

    // 文本压缩指示
    sms.TC = DCSType & (1 << 5);
    // 编码字符集
    sms.DCS = (enum EnumDCS) ((DCSType >> 2) & 3);

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


char *SCADecoding(const char *data, int *EndIndex) {
    int len;

    char *result;
    char *buf;

    len = strtol(sub_str(data, 0, 2), NULL, 16);
    if (len == 0) {
        *EndIndex = 2;
        return NULL;
    }

    *EndIndex = (len + 1) * 2;

    result = (char *) malloc(sizeof(char) * len * 2);
    //wmemset(result, '0', sizeof(char) * (len * 2 + 1));

    buf = result;
    len *= 2;

    // 服务中心地址类型
    if (strncmp(data + 2, "91", 2) == 0) {
        sprintf(buf++, "+");
    }

    // 服务中心地址
    for (int i = 4; i < *EndIndex; i += 2) {
        sprintf(buf++, "%c", data[i + 1]);
        sprintf(buf++, "%c", data[i]);
    }

    //  去掉填充的 'F'
    if (result[strlen(result) - 1] == L'F') {
        result[strlen(result) - 1] = L'\0';
    }

    return result;
}

char *OADecoding(const char *data, int index, int *EndIndex) {
    int len;
    char *result, *buf;

    len = strtol(sub_str(data, index, 2), NULL, 16);

    if (len == 0) {
        *EndIndex = index + 2;
        return NULL;
    }

    *EndIndex = index + 4 + len;

    result = (char *) malloc(sizeof(char) * (len + 2));
    //wmemset(result, 0, sizeof(char) * (len + 1));
    buf = result;
    
    if (strncmp(data + index + 2, "91", 2) == 0) {
        sprintf(buf++, "+");
    }

    // 电话号码
    for (int i = 0; i < len; i += 2) {
        sprintf(buf++, "%c", data[index + i + 5]);
        sprintf(buf++, "%c", data[index + i + 4]);

    }

    if (len % 2 != 0) {
        result[strlen(result) - 1] = '\0';
        (*EndIndex)++;
    }
    return result;
}

char *SCTSDecoding(const char *data, int index) {

    char *result;

    result = (char *) malloc(sizeof(char) * 32);
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

int BCDDecoding(const char *data, int index, bool isMSB) {

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

struct UDHS *UDHDecoding(const char *data, int index) {

    int len;
    struct UDHS *result;

    len = strtol(sub_str(data, index, 2), NULL, 16);
    index += 2;
    int i = 0;

    result = (struct UDHS *) malloc(sizeof(struct UDHS));
    result->UDH = (struct PDUUDH *) malloc(sizeof(struct PDUUDH) * len);
    result->count = 0;
    memset(result->UDH, 0, sizeof(struct PDUUDH) * len);

    while (i < len) {
        // 信息元素标识（Information Element Identifier
        char IEI = strtol(sub_str(data, index, 2), NULL, 16);
        index += 2;
        // 信息元素数据长度（Length of Information Element）
        int IEDL = strtol(sub_str(data, index, 2), NULL, 16);
        index += 2;
        // 信息元素数据（Information Element Data）
        char *IED = (char *) malloc(sizeof(char) * (IEDL + 1));
        for (int j = 0; j < IEDL; j++) {
            IED[j] = strtol(sub_str(data, index, 2), NULL, 16);
            index += 2;
        }
        result->UDH[result->count].IEI = IEI;
        result->UDH[result->count].IED = IED;
        result->count++;
        i += IEDL + 2;
    }

    return result;
}

char *UserDataDecoding(const char *data, int index, bool UDHI, enum EnumDCS dcs) {
    char *result = NULL;
    char *buf;

    // 用户数据区长度
    int UDL = strtol(sub_str(data, index, 2), NULL, 16);
    index += 2;

    // 跳过用户数据头
    int UDHL = 0;
    if (UDHI) {
        // 用户数据头长度
        UDHL = strtol(sub_str(data, index, 2), NULL, 16);
        UDHL++;
        index += UDHL << 1;

    }

    // 获取用户数据
    if (dcs == UCS2) {
        int len = (UDL - UDHL) >> 1;
        int utf8_len;

        result = (char *) malloc(sizeof(char) * (len * 3));
        buf = result;
        u_int32_t code[2];

        for (int i = 0; i < len; i++) {
            code[0] = strtol(sub_str(data, (i << 2) + index, 4), NULL, 16);
            code[1] = 0;
            utf32toutf8((wchar_t*)code, (unsigned char*)buf, len * 3, &utf8_len);
            buf += utf8_len;
        }

        buf[0] = '\0';
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
        result = (char *) malloc(sizeof(char) * (UDL + 1));
        for (int i = 0; i < UDL; i++) {
            result[i] = strtol(sub_str(data, (i << 1) + index, 2), NULL, 16);
        }
        return result;
    }

    return "Error!";
}

char *BIT7Unpack(const char *data, int index, int Septets, int FillBits) {
    char *result;

    result = (char *) malloc(sizeof(char) * (Septets + 1));
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

    return result;
}

char *BIT7Decoding(char *BIT7Data, unsigned int size)
{
    char *result, *buf;

    result = (char *) malloc(sizeof(char) * (size + 1));
    buf = result;

    for (int i = 0; i < size; i++) {
        u_int16_t key = BIT7Data[i];
        if (isBIT7Same(key)) {
            sprintf(buf++, "%c", key);
        }
        else if (map_get_value(BIT7ToUCS2, map_size(BIT7ToUCS2), key) >= 0) {
            u_int16_t value;
            if (key == 0x1B) { // 转义字符
                value = map_get_value(BIT7EToUCS2, map_size(BIT7EToUCS2), BIT7Data[i + 1]);
                if (i < size - 1 && value > 0) {
                    sprintf(buf++, "%c", value);
                    i++;
                }
                else {
                    value = map_get_value(BIT7ToUCS2, map_size(BIT7ToUCS2), key);
                    sprintf(buf++, "%c", value);
                }
            }
            else {
                //printf("go b\n");
                value = map_get_value(BIT7ToUCS2, map_size(BIT7ToUCS2), key);
                //printf("value = %u\n", value);
                sprintf(buf++, "%c", value);

            }
        }
        else {// 异常数据
            sprintf(buf++, "?");
        }

    }
    return result;

}

int isBIT7Same(u_int16_t UCS2) {
	if ((UCS2 >= 0x61 && UCS2 <= 0x7A) ||
	    (UCS2 >= 0x41 && UCS2 <= 0x5A) ||
	    (UCS2 >= 0x25 && UCS2 <= 0x3F) ||
	    (UCS2 >= 0x20 && UCS2 <= 0x23) ||
	    UCS2 == 0x0A || UCS2 == 0x0D) {
        return 1;
    }
    return 0;
}

struct PDUS *PDUEncoding(char *DA, char *UDC, struct UDHS *udhs) {
    enum EnumDCS DCS;

    sms_init();

    if (isGSMString(UDC))
        DCS = BIT7;
    else
        DCS = UCS2;

    return PDUDoEncoding("", DA, UDC, udhs, DCS);
}

struct PDUS *PDUDoEncoding(char *SCA, char *DA, char *UDC, struct UDHS *udhs, enum EnumDCS DCS) {
    // 短信拆分
    struct UDS *uds = UDCSplit(UDC, udhs, DCS);
    struct PDUS *pdus;

    if (uds == NULL)
        return NULL;
    pdus = (struct PDUS *) malloc(sizeof(struct PDUS));
    pdus->count = 0;
    pdus->PDU = (char **) malloc(sizeof(char *) * uds->total);

    if (uds->total > 1) {
        // 长短信
        int CSMMR = mCSMMR;
        if (++mCSMMR > 0xFFFF)
            mCSMMR = 0;
        // 生成短信编码序列
        for (int i = 0; i < uds->total; i++) {
            // 更新用户数据头
            struct UDHS *CSMUDH = UpdateUDH(udhs, CSMMR, uds->total, i);
            pdus->PDU[i] = SoloPDUEncoding(SCA, DA, uds->Data[i], CSMUDH, DCS);
            pdus->count++;
        }

    }
    else {  // 单条短信
        pdus->PDU[0] = SoloPDUEncoding(SCA, DA, uds->Data[0], udhs, DCS);
        pdus->count = 1;
    }

    return pdus;
}

int isGSMString(char *Data) {

    if (Data == NULL || strcmp(Data, "") == 0)
        return 1;

    if (is_acsii((unsigned char*)Data) == 0) {
        int len;
        len = utf8len((unsigned char *) Data); 

        u_int16_t *code = (u_int16_t *) malloc(sizeof(u_int16_t) * len);
        utf8toutf16((unsigned char *) Data, code, len, &len);

        while (*code) {
            if (!(isBIT7Same(*code) || map_get_value(UCS2ToBIT7, map_size(UCS2ToBIT7), *code) >= 0))
                return 0;
            code++;
        }

        return 1;
    }
    else
        return 1;

}

struct UDS *UDCSplit(char *UDC, struct UDHS *udhs, enum EnumDCS DCS) {
    int UDHL = getUDHL(udhs);
    struct UDS *result;

    if (DCS == BIT7) {
        // 7-Bit编码
        // 计算剩余房间数
        int room = BIT7UDL - (UDHL * 8 + 6) / 7;
        if (room < 1) {
            if (UDC == NULL || strcmp(UDC, "") == 0) {
                result = (struct UDS *) malloc(sizeof(struct UDS));
                result->Data = (char **)malloc(sizeof(char *));
                result->total = 1;
                result->Data[0] = UDC;
                return result;
            }
            else
                return NULL;
        }

        // 不需要拆分
        if (SeptetsLength(UDC) <= room) {
            result = (struct UDS *) malloc(sizeof(struct UDS));
            result->Data = (char **)malloc(sizeof(char *));
            result->total = 1;
            result->Data[0] = UDC;
            return result;
        }
        else // 拆分短信
        {
            if (UDHL == 0)
                UDHL++;
            if (mCSMIEI == BIT8MIEI)
                UDHL += 5;  // 1字节消息参考号
            else
                UDHL += 6;  // 2字节消息参考号
            // 更新剩余房间数
            room = BIT7UDL - (UDHL * 8 + 6) / 7;
            if (room < 1)
                return NULL;

            int i = 0;
            int len = strlen(UDC);

            result = (struct UDS *) malloc(sizeof(struct UDS));
            result->total = 0;
            result->Data = (char **) malloc(MAX_SMS_NR * sizeof(char *));

            while (i < len) {
                int step = SeptetsToChars(UDC, i, room);
                if (i + step < len) {
                    result->Data[result->total] = (char *) malloc(sizeof(char) * (step + 1));
                    strcpy(result->Data[result->total++], sub_str(UDC, i, step));
                }
                else {
                    result->Data[result->total] = (char *) malloc(sizeof(char) * (len - i + 1));
                    strcpy(result->Data[result->total++], sub_str(UDC, i, -1));
                }

                i += step;

            }
            return result;

        }
    }
    else { // UCS2编码
        // 计算剩余房间数
        int room = (BIT8UDL - UDHL) >> 1;
        if (room < 1) {
            if (UDC == NULL || strcmp(UDC, "") == 0) {
                result = (struct UDS *) malloc(sizeof(struct UDS));
                result->Data = (char **)malloc(sizeof(char *));
                result->total = 1;
                result->Data[0] = UDC;
                return result;
            }
            else
                return NULL;
        }
        if (UDC == NULL || utf8len(UDC) <= room) {
            result = (struct UDS *) malloc(sizeof(struct UDS));
            result->Data = (char **)malloc(sizeof(char *));
            result->total = 1;
            result->Data[0] = UDC;
            return result;
        }
        else // 需要拆分成多条短信
        {
            if (UDHL == 0)
                UDHL++;
            if (mCSMIEI == BIT8MIEI)
                UDHL += 5;  // 1字节消息参考号
            else
                UDHL += 6;  // 2字节消息参考号

            // 更新剩余房间数
            room = (BIT8UDL - UDHL) >> 1;
            if (room < 1)
                return NULL;

            int len = utf8len(UDC);

            result = (struct UDS *) malloc(sizeof(struct UDS));
            result->total = 0;
            result->Data = (char **) malloc(MAX_SMS_NR * sizeof(char *));
	    int index = 0;
            for (int i = 0; i < len; i += room) {
		    int real_size;
                if (i + room < len) {
		    real_size = utf8_get_size(UDC + index, room);
                    result->Data[result->total] = (char*)malloc(sizeof(char) * (real_size + 1));
                    strcpy(result->Data[result->total++],sub_str(UDC, index, real_size));
                }
                else {
		    real_size = utf8_get_size(UDC + index, len - i);
                    result->Data[result->total] = (char*)malloc(sizeof(char) * (real_size + 1));
                    strcpy(result->Data[result->total++], sub_str(UDC, index, -1));
                }
		index += real_size;
            }
            return result;
        }

    }
}

int getUDHL(struct UDHS *udhs) {
    if (udhs == NULL)
        return 0;

    // 加上1字节的用户数据头长度
    int UDHL = 1;
    for (int i = 0; i < udhs->count; i++) {
        UDHL += strlen(udhs->UDH[i].IED) + 2;
    }
    return UDHL;
}

int SeptetsLength(char *source) {
    if (source == NULL || strcmp(source, "") == 0) {
        return 0;
    }
    int len = strlen(source);
    while (*source) {
        u_int16_t code = (u_int16_t) *source;
        if (map_get_value(UCS2ToBIT7, map_size(UCS2ToBIT7), code) > 0xFF) {
            len++;
        }
        source++;
    }
    return len;
}

int SeptetsToChars(char *source, int index, int septets) {
    if (source == NULL || strcmp(source, "") == 0)
        return 0;
    int count = 0;
    int i;

    for (i = index; i < strlen(source); i++) {
        u_int16_t code = (u_int16_t) source[i];
        if (map_get_value(UCS2ToBIT7, map_size(UCS2ToBIT7), code) > 0xFF)
            count++;

        if (++count >= septets) {
            if (count == septets)
                i++;
            break;
        }
    }
    return i - index;
}

struct UDHS *UpdateUDH(struct UDHS *udhs, int CSMMR, int total, int index) {
    struct UDHS *result;

    result = (struct UDHS *) malloc(sizeof(struct UDHS));

    if (udhs == NULL || udhs->count == 0) {
        result->UDH = (struct PDUUDH *) malloc(sizeof(struct PDUUDH));
        result->count = 1;

    }
    else {
        result->UDH = (struct PDUUDH *) malloc(sizeof(struct PDUUDH) * (udhs->count + 1));
        result->count = udhs->count + 1;
        // 复制 UDH
        memcpy(&result->UDH[1], udhs->UDH, sizeof(struct PDUUDH) * udhs->count);
    }
    // 插入第一个位置
    if (mCSMIEI == BIT8MIEI) {
        result->UDH[0].IED = (char *) malloc(sizeof(char) * 3);
        result->UDH[0].count = 3;
        result->UDH[0].IED[0] = CSMMR & 0xFF;
        result->UDH[0].IED[1] = total;
        result->UDH[0].IED[2] = index + 1;
        result->UDH[0].IEI = 0;
    }
    else {
        result->UDH[0].IED = (char *) malloc(sizeof(char) * 4);
        result->UDH[0].count = 4;
        result->UDH[0].IED[0] = (CSMMR >> 8) & 0xFF;
        result->UDH[0].IED[1] = CSMMR & 0xFF;
        result->UDH[0].IED[2] = total;
        result->UDH[0].IED[3] = index + 1;
        result->UDH[0].IEI = 8;
    }

    return result;
}

char *SoloPDUEncoding(char *SCA, char *DA, char *UC, struct UDHS *udhs, enum EnumDCS DCS) {
    char *result;
    char *buf, *ret;
    int index;

    result = (char *) malloc(sizeof(char) * 400);
    buf = result;

    //  短信中心
    ret = SCAEncoding(SCA);
    index = strlen(ret);
    sprintf(buf, "%s", ret);
    buf += index;
    // 协议数据单元类型
    if (udhs == NULL || udhs->count == 0) {
        ret = PDUTypeEncoding(false);
        sprintf(buf, "%s", ret);
        buf += strlen(ret);
    }
    else {
        ret = PDUTypeEncoding(true);
        sprintf(buf, "%s", ret);
        buf += strlen(ret);
    }
    // 消息参考值
    ret = MREncoding();
    sprintf(buf, "%s", ret);
    buf += strlen(ret);
    // 接收方SME地址
    ret = DAEncoding(DA);
    sprintf(buf, "%s", ret);
    buf += strlen(ret);
    // 协议标识
    ret = PIDEncoding();
    sprintf(buf, "%s", ret);
    buf += strlen(ret);
    // 编码方案
    ret = DCSEncoding(UC, DCS);
    sprintf(buf, "%s", ret);
    buf += strlen(ret);
    // 有效期
    sprintf(buf, "%s", mVP);
    buf += strlen(mVP);

    // 用户数据长度及内容
    ret = UDEncoding(UC, udhs, DCS);
    sprintf(buf, "%s", ret);

    return result;
}

char *SCAEncoding(char *SCA) {

    if (SCA == NULL || strcmp(SCA, "") == 0) {
        // 表示使用SIM卡内部的设置值，该值通过AT+CSCA指令设置
        return "00";
    }

    char *result;
    char *buf;
    int len;
    len = strlen(SCA);
    result = (char *) malloc((len + 5) * sizeof(char));
    buf = result;

    int index = 0;
    if (SCA[0] == '+') {
        // 国际号码
        sprintf(buf, "%02X", len / 2 + 1);
        buf += 2;
        sprintf(buf, "91");
        buf += 2;
        index = 1;
    }
    else {
        // 国内号码
        sprintf(buf, "%02X", len / 2 + 1);
        buf += 2;
        sprintf(buf, "81");
        buf += 2;
    }
    // SCA地址编码
    for (; index < len; index += 2) {
        if (index == len - 1) {
            // 补“F”凑成偶数个
            sprintf(buf++, "F");
            sprintf(buf++, "%c", SCA[index]);

        }
        else {
            sprintf(buf++, "%c", SCA[index + 1]);
            sprintf(buf++, "%c", SCA[index]);
        }
    }

    return result;
}

char *PDUTypeEncoding(bool UDH) {
    // 信息类型指示（Message Type Indicator）
    // 01 SMS-SUBMIT（MS -> SMSC）
    int PDUType = 0x01;
    char *result;
    result = (char *) malloc(3 * sizeof(char));

    // 用户数据头标识（User Data Header Indicator）
    if (UDH) {
        PDUType |= 0x40;
    }
    // 有效期格式（Validity Period Format）
    if (strlen(mVP) == 2) {
        // VP段以整型形式提供（相对的）
        PDUType |= 0x10;
    }
    else if (strlen(mVP) == 14) {
        // VP段以8位组的一半(semi-octet)形式提供（绝对的）
        PDUType |= 0x18;
    }

    // 请求状态报告（Status Report Request）
    if (mSRR) {
        // 请求状态报告
        PDUType |= 0x20;
    }

    // 拒绝复本（Reject Duplicate）
    if (mRD) {
        PDUType |= 0x04;
    }
    sprintf(result, "%02X", PDUType);
    return result;
}

char *MREncoding() {
    // 由手机设置
    return "00";
}

char *DAEncoding(char *DA) {
    if (DA == NULL || strcmp(DA, "") == 0) {
        // 地址长度0，地址类型未知
        return "0080";
    }
    char *result, *buf;
    int len = strlen(DA);
    int index;

    result = (char *) malloc(sizeof(char) * (len + 5));
    buf = result;

    if (DA[0] == '+') {
        // 国际号码
        // 地址长度编码
        sprintf(buf, "%02X", len - 1);
        buf += 2;
        // 地址类型
        sprintf(buf, "91");
        buf += 2;
        index = 1;
    }
    else {
        // 国内号码
        // 地址长度编码
        sprintf(buf, "%02X", len);
        buf += 2;
        // 地址类型
        sprintf(buf, "81");
        buf += 2;
    }

    for (; index < len; index += 2) {
        // 号码部分奇偶位对调
        if (index == len - 1) {
            sprintf(buf++, "F");
            sprintf(buf++, "%c", DA[index]);
        }
        else {
            sprintf(buf++, "%c", DA[index + 1]);
            sprintf(buf++, "%c", DA[index]);
        }
    }
    return result;

}

char *PIDEncoding() {
    return "00";
}

char *DCSEncoding(char *UD, enum EnumDCS DCS) {
    if (DCS == BIT7) {
        // 7-Bit编码
        return "00";
    }
    else {
        // UCS2编码
        return "08";
    }
}

char *UDEncoding(char *UD, struct UDHS *udhs, enum EnumDCS DCS) {
    int UDHL;

    char *result;

    // 用户数据头编码
    char *header = UDHEncoding(udhs, &UDHL);

    // 用户数据内容编码
    int UDCL;
    char *body;

    body = UDCEncoding(UD, &UDCL, UDHL, DCS);

    // 用户数据区长度
    int UDL;
    if (DCS == BIT7) {
        // 7-Bit编码
        UDL = (UDHL * 8 + 6) / 7 + UDCL;
    }
    else {
        // UCS2编码或者8-Bit编码
        UDL = UDHL + UDCL;
    }

    int len = strlen(header) + strlen(body) + 2;
    result = (char *) malloc(sizeof(char) * (len + 1));
    sprintf(result, "%02X%s%s", UDL, header, body);

    return result;

}

char *UDHEncoding(struct UDHS *udhs, int *UDHL) {

    *UDHL = 0;

    if (udhs == NULL || udhs->count == 0)
        return "";
    for (int i = 0; i < udhs->count; i++) {
        *UDHL += udhs->UDH[i].count + 2;
    }

    char *result;
    char *buf;
    result = (char *) malloc(sizeof(char) * ((*UDHL + 1) * 2 + 1));
    buf = result;

    sprintf(buf, "%02X", *UDHL);
    buf += 2;
    for (int i = 0; i < udhs->count; i++) {
        // 信息元素标识1字节
        sprintf(buf, "%02X", udhs->UDH[i].IEI);
        buf += 2;
        // 信息元素长度1字节
        sprintf(buf, "%02X", udhs->UDH[i].count);
        buf += 2;
        // 信息元素数据
        for (int j = 0; j < udhs->UDH[i].count; j++) {
            sprintf(buf, "%02X", udhs->UDH[i].IED[j]);
            buf += 2;
        }

    }
    // 加上1字节的用户数据头长度
    (*UDHL)++;
    return result;

}

char *UDCEncoding(char *UDC, int *UDCL, int UDHL, enum EnumDCS DCS) {
    if (UDC == NULL || strcmp(UDC, "") == 0) {
        *UDCL = 0;
        return "";
    }

    if (DCS == BIT7) {
        // 7-Bit编码，需要参考用户数据头长度，已保证7-Bit边界对齐
        return BIT7Pack(BIT7Encoding(UDC, UDCL), UDHL);
    }
    else {
        // UCS2编码

        int len = utf8len((unsigned char*)UDC);
        int len2;
        unsigned short *code;

        code = (unsigned short*)malloc(sizeof(unsigned short) * len);
        utf8toutf16((unsigned char*)UDC, code, len, &len2);
        *UDCL = len * 2;
        char *result = (char *) malloc(sizeof(char) * (*UDCL * 2 + 1));
        char *buf = result;

        for (int i = 0; i < len; i++) {
            sprintf(buf, "%04X", code[i]);
            buf += 4;
        }
        free(code);
        return result;
    }
}

struct ByteArray *BIT7Encoding(char *UDC, int *Septets) {
    struct ByteArray *result;

    int len = strlen(UDC);

    result = (struct ByteArray *) malloc(sizeof(struct ByteArray));
    result->len = 0;
    result->array = (char *) malloc(sizeof(char) * (len * 2 + 1));
    *Septets = 0;

    for (int i = 0; i < len; i++) {
        u_int16_t code = (u_int16_t) UDC[i];
        if (isBIT7Same(code)) {
            //  编码不变
	    result->array[(*Septets)++] = code;
        }
        else {
            u_int16_t value = map_get_value(UCS2ToBIT7, map_size(UCS2ToBIT7), code);
            if (value >= 0) {
                if (value > 0xFF) {
                    // 转义序列
			result->array[(*Septets)++] = value >> 8;
			result->array[(*Septets)++] = value & 0xFF;
                }
                else {
			result->array[(*Septets)++] = value;
                }
            }
            else {
                // 未知字符
		    result->array[(*Septets)++] = (u_int16_t) '?';
            }
        }
    }
    // 重新调整大小
    result->len = *Septets;

    return result;
}

char *BIT7Pack(struct ByteArray *Bit7Array, int UDHL) {
    // 7Bit对齐需要的填充位
    int fillBits = (UDHL * 8 + 6) / 7 * 7 - (UDHL * 8);

    // 压缩字节数
    int len = Bit7Array->len;
    int packLen = (len * 7 + fillBits + 7) / 8;
    char *result;
    char *buf;

    result = (char *) malloc(sizeof(char) * (packLen * 2 + 1));
    buf = result;

    int left = 0;
    for (int i = 0; i < len; i++) {
        // 每8个字节压缩成7个字节
        int32_t Value = Bit7Array->array[i];
        int32_t index = (i + 8 - fillBits) % 8;
        if (index == 0) {
            left = Value;
        }
        else {
            int32_t n = ((Value << (8 - index)) | left) & 0xFF;
            sprintf(buf, "%02X", n);
            buf += 2;
            left = Value >> index;
        }
    }


    if ((len * 7 + fillBits) % 8 != 0) {
        // 写入剩余数据
        sprintf(buf, "%02X", left);
        buf += 2;
    }
    buf[0] = '\0';
    return result;
}

