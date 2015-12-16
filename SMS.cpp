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
    if (size > 0)
        wcsncpy(temp, str + start, size);
    else if (size < 0)
        wcscpy(temp, str + start);

    return temp;
}

SMS::SMS() {

    this->mCSMMR = 0;
    this->mRD = false;
    this->mSRR = false;
    this->mSCA = L"";
    this->mVP = L"A7";
    this->mCSMIEI = BIT8MIEI;
}

SMS::~SMS() {

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

struct UDHS *SMS::UDHDecoding(const wchar_t *data, int index) {

    int len;
    struct UDHS *result;

    len = wcstol(sub_str(data, index, 2), NULL, 16);
    index += 2;
    int i = 0;

    result = (struct UDHS *) malloc(sizeof(struct UDHS));
    result->UDH = (struct PDUUDH *) malloc(sizeof(struct PDUUDH) * len);
    result->count = 0;
    memset(result->UDH, 0, sizeof(struct PDUUDH) * len);

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
        result->UDH[result->count].IEI = IEI;
        result->UDH[result->count].IED = IED;
        result->count++;
        i += IEDL + 2;
    }

    return result;
}

wchar_t *SMS::UserDataDecoding(const wchar_t *data, int index, bool UDHI, enum EnumDCS dcs) {
    wchar_t *result = NULL;
    wchar_t *buf;



    // 用户数据区长度
    int UDL = wcstol(sub_str(data, index, 2), NULL, 16);
    index += 2;


    // 跳过用户数据头
    int UDHL = 0;
    if (UDHI) {
        // 用户数据头长度
        UDHL = wcstol(sub_str(data, index, 2), NULL, 16);
        UDHL++;
        index += UDHL << 1;

    }

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

    return result;
}

wchar_t *SMS::BIT7Decoding(wchar_t *BIT7Data, unsigned int size) {
    wchar_t *result, *buf;

    result = (wchar_t *) malloc(sizeof(wchar_t) * (size + 1));
    buf = result;
    for (int i = 0; i < size; i++) {
        u_int16_t key = BIT7Data[i];
        if (isBIT7Same(key)) {
            swprintf(buf++, size, L"%c", key);
        }
        else if (map_get_value(BIT7ToUCS2, map_size(BIT7ToUCS2), key) >= 0) {
            u_int16_t value;
            if (key == 0x1B) { // 转义字符
                value = map_get_value(BIT7EToUCS2, map_size(BIT7EToUCS2), BIT7Data[i + 1]);
                if (i < size - 1 && value > 0) {
                    swprintf(buf++, size, L"%c", value);
                    i++;
                }
                else {
                    value = map_get_value(BIT7ToUCS2, map_size(BIT7ToUCS2), key);
                    swprintf(buf++, size, L"%c", value);
                }
            }
            else {
                //printf("go b\n");
                value = map_get_value(BIT7ToUCS2, map_size(BIT7ToUCS2), key);
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

int SMS::isBIT7Same(u_int16_t UCS2) {
    if (UCS2 >= 0x61 && UCS2 <= 0x7A ||
        UCS2 >= 0x41 && UCS2 <= 0x5A ||
        UCS2 >= 0x25 && UCS2 <= 0x3F ||
        UCS2 >= 0x20 && UCS2 <= 0x23 ||
        UCS2 == 0x0A || UCS2 == 0x0D) {
        return 1;
    }
    return 0;
}

struct PDUS *SMS::PDUEncoding(wchar_t *DA, wchar_t *UDC, struct UDHS *udhs) {
    enum EnumDCS DCS;

    if (isGSMString(UDC))
        DCS = BIT7;
    else
        DCS = UCS2;

    return PDUDoEncoding(L"", DA, UDC, udhs, DCS);
}

struct PDUS *SMS::PDUDoEncoding(wchar_t *SCA, wchar_t *DA, wchar_t *UDC, struct UDHS *udhs, enum EnumDCS DCS) {
    // 短信拆分
    struct UDS *uds = UDCSplit(UDC, udhs, DCS);
    struct PDUS *pdus;

    if (uds == NULL)
        return NULL;
    pdus = (struct PDUS *) malloc(sizeof(struct PDUS));
    pdus->count = 0;
    pdus->PDU = (wchar_t **) malloc(sizeof(wchar_t *) * uds->total);

    if (uds->total > 1) {
        // 长短信
        int CSMMR = this->mCSMMR;
        if (++this->mCSMMR > 0xFFFF)
            this->mCSMMR = 0;
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

int SMS::isGSMString(wchar_t *Data) {
    if (Data == NULL || wcscmp(Data, L"") == 0)
        return 1;

    while (*Data) {
        u_int16_t code = (u_int16_t) *Data;
        if (!(isBIT7Same(code) || map_get_value(UCS2ToBIT7, map_size(UCS2ToBIT7), code) >= 0))
            return 0;
        Data++;
    }

    return 1;
}

struct UDS *SMS::UDCSplit(wchar_t *UDC, struct UDHS *udhs, enum EnumDCS DCS) {
    int UDHL = getUDHL(udhs);
    UDS *result;

    if (DCS == BIT7) {
        // 7-Bit编码
        // 计算剩余房间数
        int room = BIT7UDL - (UDHL * 8 + 6) / 7;
        if (room < 1) {
            if (UDC == NULL || wcscmp(UDC, L"") == 0) {
                result = (struct UDS *) malloc(sizeof(struct UDS));
                result->total = 1;
                result->Data = &UDC;
                return result;
            }
            else
                return NULL;
        }

        // 不需要拆分
        if (SeptetsLength(UDC) <= room) {
            result = (struct UDS *) malloc(sizeof(struct UDS));
            result->total = 1;
            result->Data = &UDC;
            return result;
        }
        else // 拆分短信
        {
            if (UDHL == 0)
                UDHL++;
            if (this->mCSMIEI == BIT8MIEI)
                UDHL += 5;  // 1字节消息参考号
            else
                UDHL += 6;  // 2字节消息参考号
            // 更新剩余房间数
            room = BIT7UDL - (UDHL * 8 + 6) / 7;
            if (room < 1)
                return NULL;

            int i = 0;
            int len = wcslen(UDC);

            result = (struct UDS *) malloc(sizeof(struct UDS));
            result->total = 0;
            result->Data = (wchar_t **) malloc(MAX_SMS_NR * sizeof(wchar_t *));

            while (i < len) {
                int step = SeptetsToChars(UDC, i, room);
                if (i + step < len) {
                    result->Data[result->total] = (wchar_t *) malloc(sizeof(wchar_t) * (step + 1));
                    wcscpy(result->Data[result->total++], sub_str(UDC, i, step));
                }
                else {
                    result->Data[result->total] = (wchar_t *) malloc(sizeof(wchar_t) * (len - i + 1));
                    wcscpy(result->Data[result->total++], sub_str(UDC, i, -1));
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
            if (UDC == NULL || wcscmp(UDC, L"") == 0) {
                result = (struct UDS *) malloc(sizeof(struct UDS));
                result->total = 1;
                result->Data = &UDC;
                return result;
            }
            else
                return NULL;
        }
        if (UDC == NULL || wcslen(UDC) <= room) {
            result = (struct UDS *) malloc(sizeof(struct UDS));
            result->total = 1;
            result->Data = &UDC;
            return result;
        }
        else // 需要拆分成多条短信
        {
            if (UDHL == 0)
                UDHL++;
            if (this->mCSMIEI == BIT8MIEI)
                UDHL += 5;  // 1字节消息参考号
            else
                UDHL += 6;  // 2字节消息参考号

            // 更新剩余房间数
            room = (BIT8UDL - UDHL) >> 1;
            if (room < 1)
                return NULL;

            int len = wcslen(UDC);
            result = (struct UDS *) malloc(sizeof(struct UDS));
            result->total = 0;
            result->Data = (wchar_t **) malloc(MAX_SMS_NR * sizeof(wchar_t *));
            for (int i = 0; i < len; i += room) {
                if (i + room < len) {
                    result->Data[result->total] = (wchar_t*)malloc(sizeof(wchar_t) * (room + 1));
                    wcscpy(result->Data[result->total++],sub_str(UDC, i, room));
                }
                else {
                    result->Data[result->total] = (wchar_t*)malloc(sizeof(wchar_t) * (len - i + 1));
                    wcscpy(result->Data[result->total++], sub_str(UDC, i, -1));
                }
            }
            return result;
        }

    }
}

int SMS::getUDHL(struct UDHS *udhs) {
    if (udhs == NULL)
        return 0;

    // 加上1字节的用户数据头长度
    int UDHL = 1;
    for (int i = 0; i < udhs->count; i++) {
        UDHL += wcslen(udhs->UDH[i].IED) + 2;
    }
    return UDHL;
}

int SMS::SeptetsLength(wchar_t *source) {
    if (source == NULL || wcscmp(source, L"") == 0) {
        return 0;
    }
    int len = wcslen(source);
    while (*source) {
        u_int16_t code = (u_int16_t) *source;
        if (map_get_value(UCS2ToBIT7, map_size(UCS2ToBIT7), code) > 0xFF) {
            len++;
        }
        source++;
    }
    return len;
}

int SMS::SeptetsToChars(wchar_t *source, int index, int septets) {
    if (source == NULL || wcscmp(source, L"") == 0)
        return 0;
    int count = 0;
    int i;

    for (i = index; i < wcslen(source); i++) {
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

struct UDHS *SMS::UpdateUDH(struct UDHS *udhs, int CSMMR, int total, int index) {
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
    if (this->mCSMIEI == BIT8MIEI) {
        result->UDH[0].IED = (wchar_t *) malloc(sizeof(wchar_t) * 3);
        result->UDH[0].count = 3;
        result->UDH[0].IED[0] = CSMMR & 0xFF;
        result->UDH[0].IED[1] = total;
        result->UDH[0].IED[2] = index + 1;
        result->UDH[0].IEI = 0;
    }
    else {
        result->UDH[0].IED = (wchar_t *) malloc(sizeof(wchar_t) * 4);
        result->UDH[0].count = 4;
        result->UDH[0].IED[0] = (CSMMR >> 8) & 0xFF;
        result->UDH[0].IED[1] = CSMMR & 0xFF;
        result->UDH[0].IED[2] = total;
        result->UDH[0].IED[3] = index + 1;
        result->UDH[0].IEI = 8;
    }

    return result;
}

wchar_t *SMS::SoloPDUEncoding(wchar_t *SCA, wchar_t *DA, wchar_t *UC, struct UDHS *udhs, enum EnumDCS DCS) {
    wchar_t *result;
    wchar_t *buf, *ret, *end;
    int index;

    result = (wchar_t *) malloc(sizeof(wchar_t) * 400);
    buf = result;
    end = buf + 400;
    //  短信中心
    ret = SCAEncoding(SCA);
    index = wcslen(ret);
    swprintf(buf, end - buf, L"%ls", ret);
    buf += index;
    // 协议数据单元类型
    if (udhs == NULL || udhs->count == 0) {
        ret = PDUTypeEncoding(false);
        swprintf(buf, end - buf, L"%ls", ret);
        buf += wcslen(ret);
    }
    else {
        ret = PDUTypeEncoding(true);
        swprintf(buf, end - buf, L"%ls", ret);
        buf += wcslen(ret);
    }
    // 消息参考值
    ret = MREncoding();
    swprintf(buf, end - buf, L"%ls", ret);
    buf += wcslen(ret);
    // 接收方SME地址
    ret = DAEncoding(DA);
    swprintf(buf, end - buf, L"%ls", ret);
    buf += wcslen(ret);
    // 协议标识
    ret = PIDEncoding();
    swprintf(buf, end - buf, L"%ls", ret);
    buf += wcslen(ret);
    // 编码方案
    ret = DCSEncoding(UC, DCS);
    swprintf(buf, end - buf, L"%ls", ret);
    buf += wcslen(ret);
    // 有效期
    swprintf(buf, end - buf, L"%ls", this->mVP);
    buf += wcslen(this->mVP);

    // 用户数据长度及内容
    ret = UDEncoding(UC, udhs, DCS);
    swprintf(buf, end - buf, L"%ls", ret);

    return result;
}

wchar_t *SMS::SCAEncoding(wchar_t *SCA) {

    if (SCA == NULL || wcscmp(SCA, L"") == 0) {
        // 表示使用SIM卡内部的设置值，该值通过AT+CSCA指令设置
        return L"00";
    }

    wchar_t *result;
    wchar_t *buf, *end;
    int len;
    len = wcslen(SCA);
    result = (wchar_t *) malloc((len + 5) * sizeof(wchar_t));
    buf = result;
    end = buf + len + 5;
    int index = 0;
    if (SCA[0] == L'+') {
        // 国际号码
        swprintf(buf, end - buf, L"%02X", len / 2 + 1);
        buf += 2;
        swprintf(buf, end - buf, L"91");
        buf += 2;
        index = 1;
    }
    else {
        // 国内号码
        swprintf(buf, end - buf, L"%02X", len / 2 + 1);
        buf += 2;
        swprintf(buf, end - buf, L"81");
        buf += 2;
    }
    // SCA地址编码
    for (; index < len; index += 2) {
        if (index == len - 1) {
            // 补“F”凑成偶数个
            swprintf(buf++, end - buf, L"F");
            swprintf(buf++, end - buf, L"%l c", SCA[index]);

        }
        else {
            swprintf(buf++, end - buf, L"%lc", SCA[index + 1]);
            swprintf(buf++, end - buf, L"%lc", SCA[index]);
        }
    }

    return result;
}

wchar_t *SMS::PDUTypeEncoding(bool UDH) {
    // 信息类型指示（Message Type Indicator）
    // 01 SMS-SUBMIT（MS -> SMSC）
    int PDUType = 0x01;
    wchar_t *result;
    result = (wchar_t *) malloc(3 * sizeof(wchar_t));

    // 用户数据头标识（User Data Header Indicator）
    if (UDH) {
        PDUType |= 0x40;
    }
    // 有效期格式（Validity Period Format）
    if (wcslen(this->mVP) == 2) {
        // VP段以整型形式提供（相对的）
        PDUType |= 0x10;
    }
    else if (wcslen(this->mVP) == 14) {
        // VP段以8位组的一半(semi-octet)形式提供（绝对的）
        PDUType |= 0x18;
    }

    // 请求状态报告（Status Report Request）
    if (this->mSRR) {
        // 请求状态报告
        PDUType |= 0x20;
    }

    // 拒绝复本（Reject Duplicate）
    if (this->mRD) {
        PDUType |= 0x04;
    }
    swprintf(result, 3, L"%02X", PDUType);
    return result;
}

wchar_t *SMS::MREncoding() {
    // 由手机设置
    return L"00";
}

wchar_t *SMS::DAEncoding(wchar_t *DA) {
    if (DA == NULL || wcscmp(DA, L"") == 0) {
        // 地址长度0，地址类型未知
        return L"0080";
    }
    wchar_t *result, *buf, *end;
    int len = wcslen(DA);
    int index;

    result = (wchar_t *) malloc(sizeof(wchar_t) * (len + 5));
    buf = result;
    end = buf + len + 5;
    if (DA[0] == L'+') {
        // 国际号码
        // 地址长度编码
        swprintf(buf, end - buf, L"%02X", len - 1);
        buf += 2;
        // 地址类型
        swprintf(buf, end - buf, L"91");
        buf += 2;
        index = 1;
    }
    else {
        // 国内号码
        // 地址长度编码
        swprintf(buf, end - buf, L"%02X", len);
        buf += 2;
        // 地址类型
        swprintf(buf, end - buf, L"81");
        buf += 2;
    }

    for (; index < len; index += 2) {
        // 号码部分奇偶位对调
        if (index == len - 1) {
            swprintf(buf++, end - buf, L"F");
            swprintf(buf++, end - buf, L"%lc", DA[index]);
        }
        else {
            swprintf(buf++, end - buf, L"%lc", DA[index + 1]);
            swprintf(buf++, end - buf, L"%lc", DA[index]);
        }
    }
    return result;

}

wchar_t *SMS::PIDEncoding() {
    return L"00";
}

wchar_t *SMS::DCSEncoding(wchar_t *UD, enum EnumDCS DCS) {
    if (DCS == BIT7) {
        // 7-Bit编码
        return L"00";
    }
    else {
        // UCS2编码
        return L"08";
    }
}

wchar_t *SMS::UDEncoding(wchar_t *UD, struct UDHS *udhs, enum EnumDCS DCS) {
    int UDHL;

    wchar_t *result;

    // 用户数据头编码
    wchar_t *header = UDHEncoding(udhs, UDHL);



    // 用户数据内容编码
    int UDCL;
    wchar_t *body;

    body = UDCEncoding(UD, UDCL, UDHL, DCS);

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
    int len = wcslen(header) + wcslen(body) + 2;
    result = (wchar_t *) malloc(sizeof(wchar_t) * (len + 1));
    swprintf(result, len + 1, L"%02X%ls%ls", UDL, header, body);

    return result;

}

wchar_t *SMS::UDHEncoding(struct UDHS *udhs, int &UDHL) {

    UDHL = 0;

    if (udhs == NULL || udhs->count == 0)
        return L"";
    for (int i = 0; i < udhs->count; i++) {
        UDHL += udhs->UDH[i].count + 2;
    }

    wchar_t *result;
    wchar_t *buf, *end;
    result = (wchar_t *) malloc(sizeof(wchar_t) * ((UDHL + 1) * 2 + 1));
    buf = result;
    end = buf + (UDHL + 1) * 2 + 1;
    swprintf(buf, end - buf, L"%02X", UDHL);
    buf += 2;
    for (int i = 0; i < udhs->count; i++) {
        // 信息元素标识1字节
        swprintf(buf, end - buf, L"%02X", udhs->UDH[i].IEI);
        buf += 2;
        // 信息元素长度1字节
        swprintf(buf, end - buf, L"%02X", udhs->UDH[i].count);
        buf += 2;
        // 信息元素数据
        for (int j = 0; j < udhs->UDH[i].count; j++) {
            swprintf(buf, end - buf, L"%02X", udhs->UDH[i].IED[j]);
            buf += 2;
        }

    }
    // 加上1字节的用户数据头长度
    UDHL++;
    return result;

}

wchar_t *SMS::UDCEncoding(wchar_t *UDC, int &UDCL, int UDHL, enum EnumDCS DCS) {
    if (UDC == NULL || wcscmp(UDC, L"") == 0) {
        UDCL = 0;
        return L"";
    }

    if (DCS == BIT7) {
        // 7-Bit编码，需要参考用户数据头长度，已保证7-Bit边界对齐
        return BIT7Pack(BIT7Encoding(UDC, UDCL), UDHL);
    }
    else {
        // UCS2编码

        int len = wcslen(UDC);
        UDCL = len * 2;
        wchar_t *result = (wchar_t *) malloc(sizeof(wchar_t) * (UDCL * 2 + 1));
        wchar_t *buf = result;
        wchar_t *end = buf + len * 4 + 1;
        for (int i = 0; i < len; i++) {

            swprintf(buf, end - buf, L"%04X", UDC[i]);
            buf += 4;
        }
        return result;
    }
}

struct ByteArray *SMS::BIT7Encoding(wchar_t *UDC, int &Septets) {
    struct ByteArray *result;
    wchar_t *buf, *end;
    int len = wcslen(UDC);

    result = (struct ByteArray *) malloc(sizeof(struct ByteArray));
    result->len = 0;
    result->array = (wchar_t *) malloc(sizeof(wchar_t) * (len * 2 + 1));
    Septets = 0;

    for (int i = 0; i < len; i++) {
        u_int16_t code = (u_int16_t) UDC[i];
        if (isBIT7Same(code)) {
            //  编码不变
            result->array[Septets++] = code;
        }
        else {
            u_int16_t value = map_get_value(UCS2ToBIT7, map_size(UCS2ToBIT7), code);
            if (value >= 0) {
                if (value > 0xFF) {
                    // 转义序列
                    result->array[Septets++] = value >> 8;
                    result->array[Septets++] = value & 0xFF;
                }
                else {
                    result->array[Septets++] = value;
                }
            }
            else {
                // 未知字符
                result->array[Septets++] = (u_int16_t) L'?';
            }
        }
    }
    // 重新调整大小
    result->len = Septets;

    return result;
}

wchar_t *SMS::BIT7Pack(struct ByteArray *Bit7Array, int UDHL) {
    // 7Bit对齐需要的填充位
    int fillBits = (UDHL * 8 + 6) / 7 * 7 - (UDHL * 8);

    // 压缩字节数
    int len = Bit7Array->len;
    int packLen = (len * 7 + fillBits + 7) / 8;
    wchar_t *result;
    wchar_t *buf, *end;

    result = (wchar_t *) malloc(sizeof(wchar_t) * (packLen * 2 + 1));
    buf = result;
    end = buf + packLen * 2 + 1;
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
            swprintf(buf, end - buf, L"%02X", n);
            buf += 2;
            left = Value >> index;
        }
    }


    if ((len * 7 + fillBits) % 8 != 0) {
        // 写入剩余数据
        swprintf(buf, end - buf, L"%02X", left);
        buf += 2;
    }
    buf[0] = L'\0';
    return result;
}
