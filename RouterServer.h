#ifndef _RouterServer_H_
#define _RouterServer_H_

//
#include <iostream>

//
#include "globe.h"
#include "Functor.h"
#include "RouterHelper.h"
#include "servant/Application.h"
#include "OuterFactoryImp.h"

//
using namespace std;
using namespace tars;

/**
 * tcp长连接路由分发
 **/
class RouterServer : public Application
{
public:
    /**
    *
    **/
    virtual ~RouterServer() {};

    /**
    *
    **/
    virtual void initialize();

    /**
    *
    **/
    virtual void destroyApp();

public:
    /*
    * 配置变更，重新加载配置
    */
    bool reloadSvrConfig(const string &command, const string &params, string &result);

    /*
    * 远程加载房间列表配置
    */
    bool loadRemoteRoomList(const string &command, const string &params, string &result);

    /**
     * 显示已有连接账户
    */
    bool showConnUsers(const string &command, const string &params, string &result);

    /**
    * 主动关闭用户连接
    */
    bool killUsers(const string &command, const string &params, string &result);

    /**
     * 设置白单开关
    */
    bool OpenWhiteList(const string &command, const string &params, string &result);

public:
    /**
     * 初始化所有线程
     */
    void initialThread();

    /**
     * 结束所有线程
     */
    void destroyThread();

    /**
     * 设置连接池超时
     */
    void setConnTimeout();

public:
    /**
     * 获取连接映射
     */
    tars::Int64 GetUidFromClientIDMap(const tars::Int64 clientId);

    /**
    * 清除连接映射
    */
    tars::Int64 PopUidFromClientIDMap(const tars::Int64 clientId);

    /**
     * 更新连接映射
     */
    int SetUidToClientIDMap(const tars::Int64 clientId, const tars::Int64 userId);

    /**
     * 获取映射信息<uid, cid>
     */
    int GetClientInfoList(std::map<long, long> &users);

public:
    /**
     * 获取外部对象
     */
    OuterFactoryImp *getOutPtr();

private:
    //定时处理对象
    Functor _functor;
    //线程池
    TC_ThreadPool _tpool;
    //线程锁
    TC_ThreadMutex m_ThreadMutex;
    //<cid, uid>
    map<tars::Int64, tars::Int64> m_cid2UidMap;
    //<cid, ptr>
    map<tars::Int64, ConnStructPtr> m_cid2PtrMap;
    //外部对象工厂
    OuterFactoryImp *m_pOuter;
};

//application
extern RouterServer g_app;

//用户连接池
extern CConnMap g_connMap;

//本地PushObj的具体地址
extern string g_sLocalPushObj;

////////////////////////////////////////////
#endif



