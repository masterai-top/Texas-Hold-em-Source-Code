#ifndef _RSA_H_
#define _RSA_H_

#include <string>
//
#include <util/tc_singleton.h>
#include "LogComm.h"

//
using namespace tars;

class CRSAEnCrypt 
{
public:

	std::string base64Encode(uint8_t const* src, uint32_t len);
	std::string base64Decode(std::string const& src);

	void generateRSAKey(std::vector<string> &vStrKeys);

	//公钥加密, 分段处理 
	//1024位，明文最大长度为1024/8 - 11 = 117
	std::string rsaPublicKeyEncryptSplit(const std::string &clearText, const string &m_pubKey);

	//私钥加密，分段处理
	std::string rsaPrivateKeyEncryptSplit(const std::string &clearText, const string &m_privateKey);

	//公钥解密，分段处理
	std::string rsaPublicKeyDecryptSplit(const std::string &clearText, const string &m_pubKey);

    //私钥解密，分段处理
    std::string rsaPrivateKeyDecryptSplit(const std::string &clearText, const string &m_privateKey);

    void test();

private:
    //私钥解密
    std::string rsaPrivateKeyDecrypt(const std::string &clearText, const string &m_privateKey);

    //私钥加密
    std::string rsaPrivateKeyEncrypt(const std::string &clearText, const string &m_privateKey);

	//公钥加密
	std::string rsaPublicKeyEncrypt(const std::string &clearText, const string &m_pubKey);

	//公钥解密
	std::string rsaPublicKeyDecrypt(const std::string &clearText, const string &m_pubKey);
};

//singleton
typedef TC_Singleton<CRSAEnCrypt, CreateStatic, DefaultLifetime> CRSAEnCryptSingleton;

#endif