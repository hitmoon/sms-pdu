//
// Created by hitmoon on 15-12-9.
//

#ifndef SMS_SMS_H
#define SMS_SMS_H

#include <string.h>
#include <wchar.h>


enum EnumDCS {
    BIT7 = 0,            // GSM 字符集
    BIT8 = 1,            // ASCII字符集
    UCS2 = 2             // Unicode 字符集
};


struct PDUUDH {
    wchar_t IEI;           // 信息元素标识
    wchar_t *IED;           // 信息元素数据
};

struct SMS_Struct {
    wchar_t *SCA;         // 服务中心地址
    wchar_t *OA;          // 发送方地址
    wchar_t *SCTS;        // 服务中心时间戳
    struct PDUUDH *UDH;  // 用户数据头
    wchar_t *UD;             // 用户数据

    bool RP;              // 应答路径
    bool UDHI;            // 用户数据头标识
    bool SRI;             // 状态报告指示
    bool MMS;             // 更多信息发送
    int MTI;              // 信息类型指示

    wchar_t PID;             // PID 协议标识

    EnumDCS DCS;          // 数据编码方案
    bool TC;              // 文本压缩指示 0： 未压缩 1：压缩
    int MC;               // 消息类型 -1： 无 1：移动设备特定类型 2：SIM特定类型 3：终端设备特定类型

};

class SMS {

public:
    struct SMS_Struct PDUDecoding(const wchar_t *data);

    // 服务中心地址解码
    wchar_t *SCADecoding(const wchar_t *data, int &EndIndex);

    // 原始地址解码
    wchar_t *OADecoding(const wchar_t *data, int index, int &EndIndex);

    // 服务中心时间戳解码
    wchar_t *SCTSDecoding(const wchar_t *data, int index);

    // BCD 解码
    int BCDDecoding(const wchar_t *data, int index, bool isMSB);

    // 用户数据头解码
    struct PDUUDH *UDHDecoding(const wchar_t *data, int index);

    // 用户数据解码
    wchar_t *UserDataDecoding(const wchar_t *data, int index, bool UDHI, EnumDCS dcs);

    // 7-Bit编码解压缩
    wchar_t *BIT7Unpack(const wchar_t *data, int index, int Septets, int FillBits);

    // 转换GSM字符编码到Unicode编码
    wchar_t *BIT7Decoding(wchar_t *BIT7Data, unsigned int size);

    // 7-Bit序列和Unicode编码是否相同
    int isBIT7Same(unsigned short UCS2);

};


#endif //SMS_SMS_H
