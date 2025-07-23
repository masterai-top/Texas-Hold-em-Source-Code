
#include <openssl/pem.h>
#include <cstring>
#include <iostream>
#include "RsaEncrypt.h"

#define SPLIT_LEN 117
static const std::string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline bool isBase64(uint8_t c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}


std::string CRSAEnCrypt::base64Encode(uint8_t const* src, uint32_t len) {

    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char charArray3[3] = {0};
    unsigned char charArray4[4] = {0};

    while (len--) {
        charArray3[i++] = *(src++);
        if (i == 3) {
            charArray4[0] = (charArray3[0] & 0xfc) >> 2;
            charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
            charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
            charArray4[3] = charArray3[2] & 0x3f;

            for (i = 0; (i < 4); i++) {
                ret += base64Chars[charArray4[i]];
            }
                
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++) {
            charArray3[j] = '\0';    
        }

        charArray4[0] = (charArray3[0] & 0xfc) >> 2;
        charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
        charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);

        for (j = 0; (j < i + 1); j++) {
            ret += base64Chars[charArray4[j]];    
        }

        while ((i++ < 3)) {
            ret += '=';    
        }
    }

    return ret;
}


std::string CRSAEnCrypt::base64Decode(std::string const& src) {
    int32_t srcPosition = src.size();
    uint8_t charArray4[4] = {0};
    uint8_t charArray3[3] = {0};
    std::string ret;

    int32_t i = 0;
    int32_t j = 0;
    int32_t srcIndex = 0;
    while (srcPosition-- && (src[srcIndex] != '=') && isBase64(src[srcIndex])) {
        charArray4[i++] = src[srcIndex]; srcIndex++;
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                charArray4[i] = base64Chars.find(charArray4[i]);
            }

            charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
            charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
            charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];

            for (i = 0; i < 3; i++) {
                ret += charArray3[i];
            }

            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++) {
            charArray4[j] = base64Chars.find(charArray4[j]);
        }

        charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
        charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);

        for (j = 0; j < i - 1; j++) {
            ret += charArray3[j];
        }
    }

    return ret;
}

//公钥加密
std::string CRSAEnCrypt::rsaPublicKeyEncrypt(const std::string &clearText, const string &m_pubKey)
{
    std::string strRet;
    BIO *keybio = BIO_new_mem_buf((unsigned char *)m_pubKey.c_str(), -1);

    RSA* rsa = RSA_new();
    rsa = PEM_read_bio_RSA_PUBKEY(keybio, NULL, NULL, NULL);
    if (!rsa)
    {
        BIO_free_all(keybio);
        return std::string("");
    }

    int len = RSA_size(rsa);
    char *encryptedText = (char *)malloc(len + 1);
    memset(encryptedText, 0, len + 1);

    // 加密函数 
    int ret = RSA_public_encrypt(clearText.length(), (const unsigned char*)clearText.c_str(), (unsigned char*)encryptedText, rsa, RSA_PKCS1_PADDING);
    if (ret >= 0)
    {
        strRet = std::string(encryptedText, ret);
    }

    // 释放内存 
    free(encryptedText);
    BIO_free_all(keybio);
    RSA_free(rsa);

    return strRet;
}

//公钥加密，分段处理
std::string CRSAEnCrypt::rsaPublicKeyEncryptSplit(const std::string &clearText, const string &m_pubKey)
{
    std::string result;
    std::string input;
    result.clear();

    for(unsigned int i = 0 ; i< clearText.length() / SPLIT_LEN; i++)
    {  
        input.clear();
        input.assign(clearText.begin() + i * SPLIT_LEN, clearText.begin() + i * SPLIT_LEN + SPLIT_LEN);
        result = result + rsaPublicKeyEncrypt(input, m_pubKey);
    }

    if(clearText.length() % SPLIT_LEN != 0)
    {
        int32_t tmp = clearText.length() / SPLIT_LEN * SPLIT_LEN;
        input.clear();
        input.assign(clearText.begin() + tmp, clearText.end());
        result = result + rsaPublicKeyEncrypt(input, m_pubKey);
    }

    std::string encode_str = base64Encode((uint8_t*)result.c_str(), result.length());
    return encode_str;
}

//私钥加密
std::string CRSAEnCrypt::rsaPrivateKeyEncrypt(const std::string &clearText, const string &m_privateKey)
{
	std::string strRet;  
    BIO *keybio = BIO_new_mem_buf((unsigned char *)m_privateKey.c_str(), -1);  
    RSA* rsa = RSA_new();
    rsa = PEM_read_bio_RSAPrivateKey(keybio, NULL, NULL, NULL);
    if (!rsa)
    {
        BIO_free_all(keybio);
        return std::string("");
    }

    int len = RSA_size(rsa);  
    //int len = 1028;
    char *encryptedText = (char *)malloc(len + 1);  
    memset(encryptedText, 0, len + 1);  
  
    // 加密  
    int ret = RSA_private_encrypt(clearText.length(), (const unsigned char*)clearText.c_str(), (unsigned char*)encryptedText, rsa, RSA_PKCS1_PADDING);  
    if (ret >= 0)  
        strRet = std::string(encryptedText, ret);  
  
    // 释放内存  
    free(encryptedText);  
    BIO_free_all(keybio);  
    RSA_free(rsa);  
  
    return strRet;  
}

//私钥加密，分段处理
std::string CRSAEnCrypt::rsaPrivateKeyEncryptSplit(const std::string &clearText, const string &m_privateKey)
{
    std::string result;
    std::string input;
    result.clear();

    for(unsigned int i = 0 ; i< clearText.length() / SPLIT_LEN; i++)
    {  
        input.clear();
        input.assign(clearText.begin() + i * SPLIT_LEN, clearText.begin() + i * SPLIT_LEN + SPLIT_LEN);
        result = result + rsaPrivateKeyEncrypt(input, m_privateKey);
    }

    if(clearText.length() % SPLIT_LEN != 0)
    {
        int32_t tmp = clearText.length() / SPLIT_LEN * SPLIT_LEN;
        input.clear();
        input.assign(clearText.begin() + tmp, clearText.end());
        result = result + rsaPrivateKeyEncrypt(input, m_privateKey);
    }

    std::string encode_str = base64Encode((uint8_t*)result.c_str(), result.length());
    return encode_str;
}

//公钥解密
std::string CRSAEnCrypt::rsaPublicKeyDecrypt(const std::string &clearText, const string &m_pubKey)
{
    std::string strRet;
    BIO *keybio = BIO_new_mem_buf((unsigned char *)m_pubKey.c_str(), -1);
    RSA* rsa = RSA_new();
    rsa = PEM_read_bio_RSA_PUBKEY(keybio, NULL, NULL, NULL);
    if (!rsa)
    {
        BIO_free_all(keybio);
        return std::string("");
    }

    int len = RSA_size(rsa);
    char *encryptedText = (char *)malloc(len + 1);
    memset(encryptedText, 0, len + 1);

    //解密
    int ret = RSA_public_decrypt(clearText.length(), (const unsigned char*)clearText.c_str(), (unsigned char*)encryptedText, rsa, RSA_PKCS1_PADDING);
    if (ret >= 0)
    {
        strRet = std::string(encryptedText, ret);
    }

    // 释放内存
    free(encryptedText);
    BIO_free_all(keybio);
    RSA_free(rsa);
    return strRet;
}

//私钥解密，分段处理
std::string CRSAEnCrypt::rsaPrivateKeyDecrypt(const std::string &clearText, const string &m_privateKey)
{
    std::string strRet;
    BIO *keybio = BIO_new_mem_buf((unsigned char *)m_privateKey.c_str(), -1);
    RSA* rsa = RSA_new();
    rsa = PEM_read_bio_RSAPrivateKey(keybio, NULL, NULL, NULL);
    if (!rsa)
    {
        BIO_free_all(keybio);
        return std::string("");
    }

    int len = RSA_size(rsa);
    char *encryptedText = (char *)malloc(len + 1);
    memset(encryptedText, 0, len + 1);

    //解密
    int ret = RSA_private_decrypt(clearText.length(), (const unsigned char*)clearText.c_str(), (unsigned char*)encryptedText, rsa, RSA_PKCS1_PADDING);
    if (ret >= 0)
    {
       strRet = std::string(encryptedText, ret); 
    }
    
    // 释放内存
    free(encryptedText);
    BIO_free_all(keybio);
    RSA_free(rsa);

    return strRet;
}

//公钥解密
std::string CRSAEnCrypt::rsaPublicKeyDecryptSplit(const std::string &data, const string &m_pubKey)
{    
    std::string clearText = base64Decode(data);
    std::string result;
    std::string input;
    result.clear();
    for(unsigned int i = 0 ; i< clearText.length() / 128; i++)
    {
        input.clear();
        input.assign(clearText.begin() + i * 128, clearText.begin() + i * 128 + 128);
        result = result + rsaPublicKeyDecrypt(input, m_pubKey);
    }

    if(clearText.length() % 128 != 0)
    {
        int tem1 = clearText.length()/128 * 128;
        input.clear();
        input.assign(clearText.begin()+ tem1, clearText.end());
        result = result + rsaPublicKeyDecrypt(input, m_pubKey);
    }

    return result;
}

//私钥解密 + 分片
std::string CRSAEnCrypt::rsaPrivateKeyDecryptSplit(const std::string &Text, const string &m_privateKey)
{
    std::string clearText = base64Decode(Text);

    std::string result;
    std::string input;
    result.clear();
    for(unsigned int i = 0 ; i< clearText.length() / 128; i++)
    {
        input.clear();
        input.assign(clearText.begin() + i * 128, clearText.begin() + i * 128 + 128);
        result = result + rsaPrivateKeyDecrypt(input, m_privateKey);
    }

    if(clearText.length() % 128 != 0)
    {
        int tem1 = clearText.length()/128 * 128;
        input.clear();
        input.assign(clearText.begin()+ tem1, clearText.end());
        result = result + rsaPrivateKeyDecrypt(input, m_privateKey);
    }

    return result;    
}

void CRSAEnCrypt::generateRSAKey(std::vector<string> &vStrKeys)  
{  
    // 密钥对
    size_t bio_private_length;
    size_t bio_public_length;
    char *private_key = NULL;
    char *public_key = NULL;
  
    // 生成密钥对    
    RSA *keypair = RSA_generate_key(1024, RSA_F4, NULL, NULL);  
    if (NULL == keypair) {
        return ;
    }

    BIO* bio_public = NULL;
    bio_public= BIO_new(BIO_s_mem());
    if (NULL == bio_public)
    {
        return ;
    }

    BIO* bio_private = NULL;
    bio_private = BIO_new(BIO_s_mem());
    if (NULL == bio_private)
    {
        return ;
    }

    if (PEM_write_bio_RSAPrivateKey(bio_private, keypair, NULL, NULL, 0, NULL, NULL) != 1)
    {
        return ;
    }

    if (PEM_write_bio_RSA_PUBKEY(bio_public, keypair) != 1)
    {
        return;
    }
    
    // 获取长度
    bio_private_length = BIO_pending(bio_private);
    bio_public_length = BIO_pending(bio_public); 
  
    // 分配长度
    private_key = new char[bio_private_length + 1];
    public_key = new char[bio_public_length + 1]; 
  
    // 秘钥对读取到字符串
    BIO_read(bio_private, private_key, bio_private_length);
    BIO_read(bio_public, public_key, bio_public_length);
    private_key[bio_private_length] = '\0';
    public_key[bio_public_length] = '\0';

    BIO_free_all(bio_private);
    BIO_free_all(bio_public);
    RSA_free(keypair);

    ROLLLOG_DEBUG  << "private_key: "<< private_key << "\npublic_key: "<< public_key << endl;

  /*  string public_key1 = "-----BEGIN PUBLIC KEY-----\nMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCZHFgCmrguBaZ2prKYIkZm1OWO\nVrwCa3OWO/5puCEuDPdrS4cqX0N6FBvni6mXS9zuyI8wya7nWhvttn1tybMO4wv5\nm6LnuWmcfOn0lT6Mz9DS+Cy5YPn8OeFy0A6Ys/B5hBng1Sw3OclduP5X8QDM3Xza\nExfdGuqiVWWugtI4zwIDAQAB\n-----END PUBLIC KEY-----";
    string private_key1 = "-----BEGIN RSA PRIVATE KEY-----\nMIICdwIBADANBgkqhkiG9w0BAQEFAASCAmEwggJdAgEAAoGBAJkcWAKauC4Fpnam\nspgiRmbU5Y5WvAJrc5Y7/mm4IS4M92tLhypfQ3oUG+eLqZdL3O7IjzDJrudaG+22\nfW3Jsw7jC/mboue5aZx86fSVPozP0NL4LLlg+fw54XLQDpiz8HmEGeDVLDc5yV24\n/lfxAMzdfNoTF90a6qJVZa6C0jjPAgMBAAECgYAGZFiIQ01NHo9EhNEP6N5njJvI\nxXYz46h/rSGB6F36PjBWGmEaM7/taMmBcSMzXcdrcJQJxWG35tsjoWq7GqCO/f3l\nuYlSoewy3WprA+Chs3e4ouYQO7ctlSQdjPeolUYpv3q4WGr1xQoLBMdpdcZBiQ6P\nbX3CgvFvKjcipXyHAQJBANCnrvDcC4VKf1wg67Yqkkx/D1Co+8Z+XptJcPcADUhb\nyhTJ8swQJJmHW2sQgkiBbHd6FW8ZFASyDfLTzRxfrzkCQQC72jaCceKjA7fspFDj\n+gGdYpi+F6h6Wc9eD4V9P23EBo34oAovuW9x3bMR0Y4P0odjAfloYBhdtiMDqiCX\n7aBHAkEArQGcaFHLq6VtnLIfP1hlHdBsnnC+8oJtZ0ypwePlH44cLMiV7OWlszcs\nccWqgPvvN9GeXBPrKUmJj0JW26Pq4QJBAIQnQnvIVLFr10OCYWnQorwu9dedWygf\n8HNype1z5uul1NDY/fGPGejYF7bsXm2hJR+w7t3P5LRggwd78wwO3tcCQFVDg4Yf\nEf9lgFYflqiSAE9TOuyz7uOR+LgtJ1A+c8PnaqC2zbotuuYpnLFb26fZ3DQsZpWY\nME+8N7vjifjYXW0=\n-----END RSA PRIVATE KEY-----";
*/    vStrKeys.push_back(public_key);
    vStrKeys.push_back(private_key);

}

void CRSAEnCrypt::test(){

   /* string public_key = "-----BEGIN PUBLIC KEY-----\nMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCZHFgCmrguBaZ2prKYIkZm1OWO\nVrwCa3OWO/5puCEuDPdrS4cqX0N6FBvni6mXS9zuyI8wya7nWhvttn1tybMO4wv5\nm6LnuWmcfOn0lT6Mz9DS+Cy5YPn8OeFy0A6Ys/B5hBng1Sw3OclduP5X8QDM3Xza\nExfdGuqiVWWugtI4zwIDAQAB\n-----END PUBLIC KEY-----";
    string private_key = "-----BEGIN RSA PRIVATE KEY-----\nMIICdwIBADANBgkqhkiG9w0BAQEFAASCAmEwggJdAgEAAoGBAJkcWAKauC4Fpnam\nspgiRmbU5Y5WvAJrc5Y7/mm4IS4M92tLhypfQ3oUG+eLqZdL3O7IjzDJrudaG+22\nfW3Jsw7jC/mboue5aZx86fSVPozP0NL4LLlg+fw54XLQDpiz8HmEGeDVLDc5yV24\n/lfxAMzdfNoTF90a6qJVZa6C0jjPAgMBAAECgYAGZFiIQ01NHo9EhNEP6N5njJvI\nxXYz46h/rSGB6F36PjBWGmEaM7/taMmBcSMzXcdrcJQJxWG35tsjoWq7GqCO/f3l\nuYlSoewy3WprA+Chs3e4ouYQO7ctlSQdjPeolUYpv3q4WGr1xQoLBMdpdcZBiQ6P\nbX3CgvFvKjcipXyHAQJBANCnrvDcC4VKf1wg67Yqkkx/D1Co+8Z+XptJcPcADUhb\nyhTJ8swQJJmHW2sQgkiBbHd6FW8ZFASyDfLTzRxfrzkCQQC72jaCceKjA7fspFDj\n+gGdYpi+F6h6Wc9eD4V9P23EBo34oAovuW9x3bMR0Y4P0odjAfloYBhdtiMDqiCX\n7aBHAkEArQGcaFHLq6VtnLIfP1hlHdBsnnC+8oJtZ0ypwePlH44cLMiV7OWlszcs\nccWqgPvvN9GeXBPrKUmJj0JW26Pq4QJBAIQnQnvIVLFr10OCYWnQorwu9dedWygf\n8HNype1z5uul1NDY/fGPGejYF7bsXm2hJR+w7t3P5LRggwd78wwO3tcCQFVDg4Yf\nEf9lgFYflqiSAE9TOuyz7uOR+LgtJ1A+c8PnaqC2zbotuuYpnLFb26fZ3DQsZpWY\nME+8N7vjifjYXW0=\n-----END RSA PRIVATE KEY-----";

    string sourceStr= "abc123";

    //公钥加密
    std::string rsaEncryptStr = rsaPublicKeyEncryptSplit(sourceStr, public_key);
    ROLLLOG_DEBUG<<"公钥加密: "<< rsaEncryptStr << endl;

    //私钥解密
    std::string rsaDecryptStr = rsaPrivateKeyDecryptSplit(rsaEncryptStr, private_key);
    ROLLLOG_DEBUG<<"私钥解密: "<< rsaDecryptStr << endl;

    //私钥加密
    rsaEncryptStr = rsaPrivateKeyEncryptSplit(sourceStr, private_key);
    ROLLLOG_DEBUG<<"私钥加密: "<< rsaEncryptStr << endl;

    //公钥解密
    rsaDecryptStr = rsaPublicKeyDecryptSplit(rsaEncryptStr, public_key);
    ROLLLOG_DEBUG<<"公钥解密: "<< rsaDecryptStr << endl;*/


    return;
}
