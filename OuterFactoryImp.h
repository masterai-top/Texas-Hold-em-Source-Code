#ifndef _OUTER_FACTORY_IMP_H_
#define _OUTER_FACTORY_IMP_H_

#include <string>
#include "servant/Application.h"
#include "util/tc_singleton.h"
#include "globe.h"
#include "RouterHelper.h"
#include "RoomServant.h"
#include "LoginServant.h"
#include "HallServant.h"
#include "ConfigServant.h"
#include "PushServant.h"
#include "JFGameCommProto.h"
#include "XGameComm.pb.h"
#include "CommonStruct.pb.h"
#include "UserInfo.pb.h"

//wbl
#include <wbl/regex_util.h>
#include <wbl/stream_util.h>
#include "PushServant.h"
#include "GlobalServant.h"
#include "GameTcp.h"

#include "RsaEncrypt.h"

//
using namespace std;
using namespace tars;
using namespace wbl;
using namespace JFGame;
using namespace JFGamecomm;
using namespace hall;
using namespace push;
using namespace global;
using namespace userinfo;

//连接配置
typedef struct _TConnConfig
{
    int SessionTimeOut;       //会话超时时间
    int ConnContinueInterval; //保持连接间隔
    int KeepAliveInterval;   //keepalive,心跳检测时间间隔
} TConnConfig;

//时间间隔设置配置信息
typedef struct _TTimeIntervalConfig
{
    int iUpdateRouteInterval;      //更新路由信息间隔时间
    int iDoOtherInterval;          //频率低的业务
    int iClearTimeOutConnInterval; //清除超时连接
    int iCheckKeepAliveInterval;   //定时检测心跳间隔
} TTimeIntervalConfig;

/**
 * 外部工具接口对象工厂
 */
class OuterFactoryImp
{
public:
    /**
     *
    */
    OuterFactoryImp();

    /**
     *
    */
    ~OuterFactoryImp();

private:
    /**
     *
    */
    void createAllObject();

    /**
     *
    */
    void deleteAllObject();

public:
    //加载配置
    void load();
    //加载房间配置
    void loadRoomServerList();
    //加载代理配置
    void readPrxConfig();
    //打印代理配置
    void printPrxConfig();
    //加载身份校验开关
    void readAuthFlag();
    //打印身份校验开关
    void printAuthFlag();
    //获取身份校验开关
    int getAuthFlag();
    //目标处理对象
    void readTransMitSvrIdObj();
    //打印目标处理对象
    void printTransMitSvrIdObj();
    //加载RoomServer配置
    void readRoomServerList();
    //加载RoomServer配置
    void printRoomServerList();
    //获取RoomServer配置
    int getRoomServerPrx(const string &id, string &prx);
    //加载连接配置
    void readConnConfig();
    //打印连接配置
    void printConnConfig();
    //获取连接配置
    const TConnConfig &getConnConfig();
    //时间间隔设置配置信息
    void readTimeIntervalConfig();
    //打印时间间隔设置配置信息
    void printTimeIntervalConfig();
    //获取时间间隔设置配置信息
    const TTimeIntervalConfig &getTimeIntervalConfig();
    //测试账号配置信息
    void readTestUidConfig();
    //打印测试帐号信息
    void printTestUidConfig();
    //
    void readRsaOpenConfig();
    //
    int getRsaOpen();

    void readVersionConfig();

    int getVersionConfig();

    void readMessageCountConfig();

    int getMessageCountLimit();
public:
    //获取白名单帐号标识
    const vector<long> &getTestUidConfig();
    //获取白名单开关
    std::atomic<int> &getTestOpen();
    //设置白名单开关
    void setTestOpen(bool isOpen);
    //检查是否白名单
    bool isWhiteList(const long uid);

public:
    //异步调用RoomServer
    void asyncRequest2Room(XGameComm::TPackage &t, const vector<MsgStruct> &vRm, const ConnStructPtr &cs);
    //异步调用RoomServer
    void asyncSend2Room(const XGameComm::TPackage &t, const ConnStructPtr &cs);
    //异步调用RoomServer, offline
    void asyncSend2RoomServerOffline(ConnStructPtr cs, bool standup = false);
    //异步调用LoginServer(身份校验)
    void asyncCheckToken(const XGameComm::TPackage &t, const ConnStructPtr cs);
    //异步调用LoginServer(帐号退出)
    void asyncLogoutNotify(const long uid, const string &sRemoteIP);
    //异步调用LoginServer,传递请求数据
    void asyncRequest2LoginServer(const XGameComm::TPackage &t, const ConnStructPtr &cs);
    //异步调用UserInfoServer,传递请求数据
    void asyncRequest2UserInfoServer(const XGameComm::TPackage &t, const ConnStructPtr &cs);
    //异步调用UserStateServer,传递请求数据
    void asyncRequest2UserStateServer(const XGameComm::TPackage &t, const ConnStructPtr &cs);
    //异步调用ConfigServer,传递请求数据
    void asyncRequest2ConfigServer(const XGameComm::TPackage &t, const ConnStructPtr &cs);
    //异步调用PushServer更新用户状态
    void asyncRequest2PushUserState(const long iUin, const long iCid, const bool isOnline);
    //异步调用HallServer(1-在线, 0-离线)
    void asyncRequest2HallUserState(long iUin, int state);
    //异步调用推送服务
    void asyncRequest2PushServer(const XGameComm::TPackage &t, const ConnStructPtr &cs);
    //异步调用PushServer在线用户状态
    void asyncReportOnlineUsers(const std::map<long, long> &users);

public:
    //获取服务配置
    const tars::TC_Config &getConfig() const;
    // 获取服务代理
    const XGame::GameTCPPrx getGameTCPPrx(int urlKey);
    //LoginServer的对象名称
    const login::LoginServantPrx getLoginServantPrx();
    //ConfigServer的对象名称
    const config::ConfigServantPrx getConfigServantPrx();
    //HallServer的对象名称
    const hall::HallServantPrx getHallServerPrx();
    //GlobalServer的对象名称
    const global::GlobalServantPrx getGlobalServantPrx();
    //PushServer的对象名称
    const push::PushServantPrx getPushServerPrx();

public:
    //拆分字符串成整形
    int splitInt(string szSrc, vector<long> &vecInt);
    //拆分字符串
    vector<string> split(const string &str, const string &pattern);

    void reverse(string &str,int start,int end);

    void rotateStrLeft(string &str, int offset);

    void rotateStrRight(string &str, int offset);

public:
    //发送通知到客户端
    int sendPushNotify(const ConnStructPtr ptr, const int iNotifyType, const bool bReturnLoginUI);

    void sendResponse(const string &buff, const ConnStructPtr cs);

    string decryptRequest(const string &buff, const ConnStructPtr cs);

private:
    //业务配置
    tars::TC_Config *_pFileConf;
    //RoomServer配置列表
    map<string, string> mapRoomServerFromRemote;
    //LoginServer的对象名称
    string _sLoginServerObj;
    //LoginServer的对象
    login::LoginServantPrx _loginServerPrx;
    //ConfigServer的对象名称
    string _sConfigServerObj;
    //ConfigServer的对象
    config::ConfigServantPrx _configServerPrx;
    //HallServer的对象名称
    string _sHallServerObj;
    //HallServer的对象
    hall::HallServantPrx _hallServantPrx;
    //GlobalServer的对象名称
    string _sGlobalServerObj;
    //GlobalServer的对象
    global::GlobalServantPrx _globalServantPrx;
    //PushServer的对象名称
    string _sPushServantObj;
    //PushServer的对象
    push::PushServantPrx _pushServerPrx;

private:
    //连接配置
    TConnConfig tConnConfig;
    //时间间隔设置配置信息
    TTimeIntervalConfig tTimeIntervalConfig;

private:
    //测试账号
    std::vector<long> vecTestUid;
    //测试开关
    std::atomic<int> testOpen;
    //身份校验开关
    int authFlag;
    //rsa 开关
    int RsaOpen; 
    //版本号
    int iVersion;
    //
    int iMessageCountLimit;
    //服务代理对象列表
    std::map<int, string> _mapTransmitObjects;
    //防止数据读写不一致
    wbl::ReadWriteLocker m_rwlock;
};

//单例
typedef TC_Singleton<OuterFactoryImp, CreateStatic, DefaultLifetime> OuterFactorySingleton;

/////////////////////////////////////////////////////////////////////
#endif



