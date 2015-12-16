//
// Created by hitmoon on 15-12-9.
//

#ifndef SMS_SMS_H
#define SMS_SMS_H

#include <string.h>
#include <wchar.h>

#define MAX_SMS_NR 32

enum EnumDCS {
    BIT7 = 0,            // GSM 字符集
    BIT8 = 1,            // ASCII字符集
    UCS2 = 2             // Unicode 字符集
};

enum EnumUDL {
    BIT7UDL = 160,
    BIT8UDL = 140,
    UCS2UDL = 70
};

enum EnumCSMIEI {
    BIT8MIEI = 0,
    BIT16MIEI = 8
};

struct PDUUDH {
    unsigned int count;    // 信息元素数据字节数
    wchar_t IEI;           // 信息元素标识
    wchar_t *IED;          // 信息元素数据
};

// 用户数据头
struct UDHS {
    int count;
    struct PDUUDH *UDH;
};

// 用户数据数组，用于拆分短信
struct UDS {
    unsigned int total;
    wchar_t **Data;
};

// 编码后短信
struct PDUS {
    unsigned int count;
    wchar_t **PDU;
};

struct ByteArray {
    wchar_t *array;
    unsigned int len;
};

struct SMS_Struct {
    wchar_t *SCA;         // 服务中心地址
    wchar_t *OA;          // 发送方地址
    wchar_t *SCTS;        // 服务中心时间戳
    struct UDHS *UDH;     // 用户数据头
    wchar_t *UD;          // 用户数据

    bool RP;              // 应答路径
    bool UDHI;            // 用户数据头标识
    bool SRI;             // 状态报告指示
    bool MMS;             // 更多信息发送
    int MTI;              // 信息类型指示

    wchar_t PID;          // PID 协议标识

    enum EnumDCS DCS;      // 数据编码方案
    bool TC;              // 文本压缩指示 0： 未压缩 1：压缩
    int MC;               // 消息类型 -1： 无 1：移动设备特定类型 2：SIM特定类型 3：终端设备特定类型

};

class SMS {

public:
    SMS();

    virtual ~SMS();

public:
    // 短信解码
    struct SMS_Struct PDUDecoding(const wchar_t *data);

    // 短信编码, 自动确定编码方案
    struct PDUS *PDUEncoding(wchar_t *DA, wchar_t *UDC, struct UDHS *udhs);

    // 短信编码真正的工作
    /// 发送方PDU格式（SMS-SUBMIT-PDU）
    /// SCA（Service Center Adress）：短信中心，长度1-12
    /// PDU-Type（Protocol Data Unit Type）：协议数据单元类型，长度1
    /// MR（Message Reference）：消息参考值，为0～255。长度1
    /// DA（Destination Adress）：接收方SME的地址，长度2-12
    /// PID（Protocol Identifier）：协议标识，长度1
    /// DCS（Data Coding Scheme）：编码方案，长度1
    /// VP（Validity Period）：有效期，长度为1（相对）或者7（绝对或增强）
    /// UDL（User Data Length）：用户数据段长度，长度1
    /// UD（User Data）：用户数据，长度0-140

    struct PDUS *PDUDoEncoding(wchar_t *SCA, wchar_t *DA, wchar_t *UDC, struct UDHS *udhs, enum EnumDCS DCS);

private:

    //长短信信息元素参考号
    enum EnumCSMIEI mCSMIEI;
    // 服务中心地址
    wchar_t *mSCA;
    // 请求状态报告
    bool mSRR;
    // 拒绝副本
    bool mRD;
    // 短信有效期
    wchar_t *mVP;
    // 长短信信息元素消息参考号
    int mCSMMR;

    // 服务中心地址解码
    wchar_t *SCADecoding(const wchar_t *data, int &EndIndex);

    // 原始地址解码
    wchar_t *OADecoding(const wchar_t *data, int index, int &EndIndex);

    // 服务中心时间戳解码
    wchar_t *SCTSDecoding(const wchar_t *data, int index);

    // BCD 解码
    int BCDDecoding(const wchar_t *data, int index, bool isMSB);

    // 用户数据头解码
    struct UDHS *UDHDecoding(const wchar_t *data, int index);

    // 用户数据解码
    wchar_t *UserDataDecoding(const wchar_t *data, int index, bool UDHI, enum EnumDCS dcs);

    // 7-Bit编码解压缩
    wchar_t *BIT7Unpack(const wchar_t *data, int index, int Septets, int FillBits);

    // 转换GSM字符编码到Unicode编码
    wchar_t *BIT7Decoding(wchar_t *BIT7Data, unsigned int size);

    // 7-Bit序列和Unicode编码是否相同
    int isBIT7Same(u_int16_t UCS2);

    // 判断是否是GSM字符串
    int isGSMString(wchar_t *Data);

    // 用户数据拆分
    struct UDS *UDCSplit(wchar_t *UDC, struct UDHS *uhds, enum EnumDCS DCS);

    // 获得用户数据头长度
    int getUDHL(struct UDHS *udhs);

    // 计算需要的7-Bit编码字节数
    int SeptetsLength(wchar_t *source);

    // 将7-Bit编码字节数换算成UCS2编码字符数
    int SeptetsToChars(wchar_t *source, int index, int septets);

    // 在用户数据头中增加长短信信息元素
    struct UDHS *UpdateUDH(struct UDHS *udhs, int CSMMR, int total, int index);

    //单条短信编码
    wchar_t *SoloPDUEncoding(wchar_t *SCA, wchar_t *DA, wchar_t *UC, struct UDHS *udhs, enum EnumDCS DCS);

    // SCA编码
    wchar_t *SCAEncoding(wchar_t *SCA);

    // PDUTYPE 编码
    wchar_t *PDUTypeEncoding(bool UDH);

    // MR,消息参考值
    wchar_t *MREncoding();

    //接收方SME地址
    wchar_t *DAEncoding(wchar_t *DA);

    // 协议标识
    wchar_t *PIDEncoding();

    // 编码方案
    wchar_t *DCSEncoding(wchar_t *UD, enum EnumDCS DCS);

    // 用户数据长度及内容
    wchar_t *UDEncoding(wchar_t *UD, struct UDHS *udhs, enum EnumDCS DCS);

    // 用户数据头编码
    wchar_t *UDHEncoding(struct UDHS *udhs, int &UDHL);

    // 用户数据内容编码
    wchar_t *UDCEncoding(wchar_t *UDC, int &UDCL, int UDHL, enum EnumDCS DCS);

    // 将UCS2编码字符串转换成7-Bit编码字节 序列
    struct ByteArray *BIT7Encoding(wchar_t *UDC, int &Septets);

    //  7-Bit编码压缩
    wchar_t *BIT7Pack(struct ByteArray *Bit7Array, int UDHL);

};


#endif //SMS_SMS_H
