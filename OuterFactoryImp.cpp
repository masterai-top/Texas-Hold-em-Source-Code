#include <cstring>
#include "OuterFactoryImp.h"
#include "RouterServer.h"
#include "LogComm.h"
#include "AsyncLoginCallback.h"
#include "AsyncLogoutCallback.h"
#include "AsyncGetUserCallback.h"
#include "AsyncUserStateCallback.h"
#include "push.pb.h"
#include "UserInfo.pb.h"
#include "head.pb.h"

OuterFactoryImp::OuterFactoryImp()
{
    createAllObject();
}

OuterFactoryImp::~OuterFactoryImp()
{
    deleteAllObject();
}

void OuterFactoryImp::deleteAllObject()
{
    if (_pFileConf)
    {
        delete _pFileConf;
        _pFileConf = NULL;
    }
}

void OuterFactoryImp::createAllObject()
{
    try
    {
        //删除
        deleteAllObject();
        //本地配置文件
        _pFileConf = new tars::TC_Config();
        //加载配置
        load();
    }
    catch (TC_Exception &ex)
    {
        LOG->error() << ex.what() << endl;
        throw;
    }
    catch (exception &e)
    {
        LOG->error() << e.what() << endl;
        throw;
    }
    catch (...)
    {
        LOG->error() << "unknown exception." << endl;
        throw;
    }
}

void OuterFactoryImp::load()
{
    __TRY__

    //拉取远程配置
    g_app.addConfig(ServerConfig::ServerName + ".conf");

    wbl::WriteLocker lock(m_rwlock);

    //读配置
    _pFileConf->parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
    LOG_DEBUG << "init config file succ: " << ServerConfig::BasePath + ServerConfig::ServerName + ".conf" << endl;

    //代理配置
    readPrxConfig();
    printPrxConfig();

    //身份校验开关
    readAuthFlag();
    printAuthFlag();

    //目标处理对象
    readTransMitSvrIdObj();
    printTransMitSvrIdObj();

    //连接配置
    readConnConfig();
    printConnConfig();

    //时间间隔设置配置信息
    readTimeIntervalConfig();
    printTimeIntervalConfig();

    //读取房间配置信息
    readRoomServerList();
    printRoomServerList();

    //测试账号配置信息
    readTestUidConfig();
    printTestUidConfig();

    //
    readRsaOpenConfig();

    //
    readVersionConfig();

    readMessageCountConfig();

    CRSAEnCryptSingleton::getInstance()->test();

    __CATCH__
}

void OuterFactoryImp::loadRoomServerList()
{
    __TRY__

    wbl::WriteLocker lock(m_rwlock);
    readRoomServerList();
    printRoomServerList();

    __CATCH__
}

//代理配置
void OuterFactoryImp::readPrxConfig()
{
    _sLoginServerObj   = (*_pFileConf).get("/Main/LoginServer<ProxyObj>", "");
    _sConfigServerObj  = (*_pFileConf).get("/Main/ConfigServer<ProxyObj>", "");
    _sHallServerObj    = (*_pFileConf).get("/Main/HallServer<ProxyObj>", "");
    _sGlobalServerObj  = (*_pFileConf).get("/Main/GlobalServer<ProxyObj>", "");
    _sPushServantObj   = (*_pFileConf).get("/Main/PushServer<ProxyObj>", "");
}

void OuterFactoryImp::printPrxConfig()
{
    FDLOG_CONFIG_INFO << "_sLoginServerObj ProxyObj: "    << _sLoginServerObj << endl;
    FDLOG_CONFIG_INFO << "_sConfigServerObj ProxyObj: "   << _sConfigServerObj << endl;
    FDLOG_CONFIG_INFO << "_sHallServerObj ProxyObj: "     << _sHallServerObj << endl;
    FDLOG_CONFIG_INFO << "_sGlobalServerObj ProxyObj: "   << _sGlobalServerObj << endl;
    FDLOG_CONFIG_INFO << "_sPushServantObj ProxyObj: "    << _sPushServantObj << endl;
}

void OuterFactoryImp::readAuthFlag()
{
    authFlag = S2I((*_pFileConf).get("/Main<AuthFlag>", "1"));
}

void OuterFactoryImp::printAuthFlag()
{
    FDLOG_CONFIG_INFO << "authFlag: " << authFlag << endl;
}

int OuterFactoryImp::getAuthFlag()
{
    return authFlag;
}

void OuterFactoryImp::readTransMitSvrIdObj()
{
    vector<string> v = (*_pFileConf).getDomainKey("/Main/TransmitObj");
    if (v.empty())
        return;

    _mapTransmitObjects.clear();
    for (size_t i = 0; i < v.size(); ++i)
    {
        string sUrlKey = v[i];
        string strObj = (*_pFileConf).get(string("/Main/TransmitObj/<") + v[i] + ">", "");
        _mapTransmitObjects[S2I(sUrlKey)] = strObj;
    }
}

void OuterFactoryImp::printTransMitSvrIdObj()
{
    for (auto it = _mapTransmitObjects.begin(); it != _mapTransmitObjects.end(); ++it)
    {
        FDLOG_CONFIG_INFO << "UrlKey : " << it->first << ", strObj : " << it->second << endl;
    }
}

void OuterFactoryImp::readRoomServerList()
{
    int iRet = getConfigServantPrx()->listAllRoomAddress(mapRoomServerFromRemote);
    if (iRet != 0)
    {
        terminate();
    }
}

void OuterFactoryImp::printRoomServerList()
{
    ostringstream os;
    os << "_sRoomServerObj from remote, ProxyObj: ";

    for (auto it = mapRoomServerFromRemote.begin(); it != mapRoomServerFromRemote.end(); it++)
    {
        os << ", key: " << it->first << ", value: " << it->second << endl;
    }

    FDLOG_CONFIG_INFO << os.str() << endl;
}

int OuterFactoryImp::getRoomServerPrx(const string &id, string &prx)
{
    wbl::ReadLocker lock(m_rwlock);

    if (id.length() == 0)
        return -1;

    auto it = mapRoomServerFromRemote.find(id);
    if (it == mapRoomServerFromRemote.end())
        return -2;

    prx = it->second;
    return 0;
}

//连接配置
void OuterFactoryImp::readConnConfig()
{
    tConnConfig.SessionTimeOut = S2I((*_pFileConf).get("/Main<SessionTimeOut>", "1800"));
    tConnConfig.ConnContinueInterval = S2I((*_pFileConf).get("/Main<ConnContinueInterval>", "900"));
    tConnConfig.KeepAliveInterval = S2I((*_pFileConf).get("/Main<KeepAliveInterval>", "30"));
}

void OuterFactoryImp::printConnConfig()
{
    FDLOG_CONNECTION_CONFIG_INFO
            << "SessionTimeOut: " << tConnConfig.SessionTimeOut
            << ", ConnContinueInterval: " << tConnConfig.ConnContinueInterval
            << ", KeepAliveInterval: " << tConnConfig.KeepAliveInterval << endl;
}

const TConnConfig &OuterFactoryImp::getConnConfig()
{
    wbl::ReadLocker lock(m_rwlock);

    return tConnConfig;
}

//时间间隔设置配置信息
void OuterFactoryImp::readTimeIntervalConfig()
{
    tTimeIntervalConfig.iUpdateRouteInterval = S2I((*_pFileConf).get("/Main/<UpdateRouteInterval>", "1"));
    tTimeIntervalConfig.iDoOtherInterval = S2I((*_pFileConf).get("/Main/<DoOtherInterval>", "10"));
    tTimeIntervalConfig.iClearTimeOutConnInterval = S2I((*_pFileConf).get("/Main/<ClearTimeOutConnInterval>", "10"));
    tTimeIntervalConfig.iCheckKeepAliveInterval = S2I((*_pFileConf).get("/Main/<CheckKeepAliveInterval>", "20"));
}

void OuterFactoryImp::printTimeIntervalConfig()
{
    FDLOG_TIME_INTERVAL_CONFIG_INFO
            << "iUpdateRouteInterval: " << tTimeIntervalConfig.iUpdateRouteInterval
            << ", iDoOtherInterval: " << tTimeIntervalConfig.iDoOtherInterval
            << ", iClearTimeOutConnInterval: " << tTimeIntervalConfig.iClearTimeOutConnInterval
            << ", iCheckKeepAliveInterval: " << tTimeIntervalConfig.iCheckKeepAliveInterval << endl;
}

const TTimeIntervalConfig &OuterFactoryImp::getTimeIntervalConfig()
{
    wbl::ReadLocker lock(m_rwlock);

    return tTimeIntervalConfig;
}

//测试账号配置信息
void OuterFactoryImp::readTestUidConfig()
{
    string sTestOpen = (*_pFileConf).get("/Main/<testOpen>", "0");
    if (sTestOpen == "1")
        testOpen = 1;
    else
        testOpen = 0;

    string testUids = (*_pFileConf).get("/Main/<TestUin>", "");
    splitInt(testUids, vecTestUid);
}

void OuterFactoryImp::printTestUidConfig()
{
    ostringstream os;
    os << "testOpen: " << testOpen << ", ";
    os << "testUids: ";
    for (auto it = vecTestUid.begin(); it != vecTestUid.end(); it++)
    {
        os << ", " << *it << endl;
    }
    FDLOG_TIME_TEST_UID_CONFIG_INFO << os.str() << endl;
}

//测试账号配置信息
void OuterFactoryImp::readRsaOpenConfig()
{
    string sRsaOpen = (*_pFileConf).get("/Main/<RsaOpen>", "0");
    if (sRsaOpen == "1")
        RsaOpen = 1;
    else
        RsaOpen = 0;
}

int OuterFactoryImp::getRsaOpen()
{
    return RsaOpen;
}

void OuterFactoryImp::readVersionConfig()
{
    iVersion = S2I((*_pFileConf).get("/Main/<Version>", "0"));
}

int OuterFactoryImp::getVersionConfig()
{
    return iVersion;
}

void OuterFactoryImp::readMessageCountConfig()
{
    iMessageCountLimit = S2I((*_pFileConf).get("/Main/<MessageCountLimit>", "1000"));
}

int OuterFactoryImp::getMessageCountLimit()
{
    return iMessageCountLimit;
}

const vector<long> &OuterFactoryImp::getTestUidConfig()
{
    wbl::ReadLocker lock(m_rwlock);
    return vecTestUid;
}

//获取白名单开关
std::atomic<int> &OuterFactoryImp::getTestOpen()
{
    return testOpen;
}

//设置白名单开关
void OuterFactoryImp::setTestOpen(bool isOpen)
{
    if (isOpen)
        testOpen = 1;
    else
        testOpen = 0;
}

//检查是否白名单
bool OuterFactoryImp::isWhiteList(const long uid)
{
    wbl::ReadLocker lock(m_rwlock);
    auto iter = std::find(vecTestUid.begin(), vecTestUid.end(), uid);
    if (iter == vecTestUid.end())
        return false;

    return true;
}

const login::LoginServantPrx OuterFactoryImp::getLoginServantPrx()
{
    if (!_loginServerPrx)
    {
        _loginServerPrx = Application::getCommunicator()->stringToProxy<login::LoginServantPrx>(_sLoginServerObj);
        LOG_DEBUG << "Init _sLoginServerObj succ, _sLoginServerObj: " << _sLoginServerObj << endl;
    }

    return _loginServerPrx;
}

const config::ConfigServantPrx OuterFactoryImp::getConfigServantPrx()
{
    if (!_configServerPrx)
    {
        _configServerPrx = Application::getCommunicator()->stringToProxy<config::ConfigServantPrx>(_sConfigServerObj);
        LOG_DEBUG << "Init _sConfigServerObj succ, _sConfigServerObj: " << _sConfigServerObj << endl;
    }

    return _configServerPrx;
}

const hall::HallServantPrx OuterFactoryImp::getHallServerPrx()
{
    if (!_hallServantPrx)
    {
        _hallServantPrx = Application::getCommunicator()->stringToProxy<hall::HallServantPrx>(_sHallServerObj);
        LOG_DEBUG << "Init _sHallServerObj succ, _sHallServerObj: " << _sHallServerObj << endl;
    }

    return _hallServantPrx;
}

const push::PushServantPrx OuterFactoryImp::getPushServerPrx()
{
    if (!_pushServerPrx)
    {
        _pushServerPrx = Application::getCommunicator()->stringToProxy<push::PushServantPrx>(_sPushServantObj);
        LOG_DEBUG << "Init _sPushServantObj succ, _sPushServantObj: " << _sPushServantObj << endl;
    }

    return _pushServerPrx;
}

const global::GlobalServantPrx OuterFactoryImp::getGlobalServantPrx()
{
    if (!_globalServantPrx)
    {
        _globalServantPrx = Application::getCommunicator()->stringToProxy<global::GlobalServantPrx>(_sGlobalServerObj);
        LOG_DEBUG << "Init _sGlobalServerObj succ, _sGlobalServerObj: " << _sGlobalServerObj << endl;
    }

    return _globalServantPrx;
}

const tars::TC_Config &OuterFactoryImp::getConfig() const
{
    return *_pFileConf;
}

const XGame::GameTCPPrx OuterFactoryImp::getGameTCPPrx(int urlKey)
{
    auto iter = _mapTransmitObjects.find(urlKey);
    if (iter == _mapTransmitObjects.end())
        return NULL;

    return Application::getCommunicator()->stringToProxy<XGame::GameTCPPrx>(iter->second);
}

int OuterFactoryImp::splitInt(string szSrc, vector<long> &vecInt)
{
    split_int(szSrc, "[ \t]*\\|[ \t]*", vecInt);
    return 0;
}

vector<std::string> OuterFactoryImp::split(const string &str, const string &pattern)
{
    return SEPSTR(str, pattern);
}

// 异步调用RoomServer
void OuterFactoryImp::asyncRequest2Room(XGameComm::TPackage &t, const vector<MsgStruct> &vRm, const ConnStructPtr &cs)
{
    __TRY__

    LOG_DEBUG << "uid : " << t.stuid().luid() << ", to request to room." << endl;

    for (size_t j = 0; j < vRm.size(); ++j)
    {
        //vecData.assign(vRm[j].sMsgData.begin(), vRm[j].sMsgData.end());

        auto mh = t.add_vecmsghead();
        mh->set_nmsgid(vRm[j].msgHead.nmsgid());
        mh->set_nmsgtype(vRm[j].msgHead.nmsgtype());

        auto msg = t.add_vecmsgdata();
        msg->assign(vRm[j].sMsgData);
    }

    asyncSend2Room(t, cs);

    __CATCH__
}

// 异步调用RoomServer
void OuterFactoryImp::asyncSend2Room(const XGameComm::TPackage &t, const ConnStructPtr &cs)
{
    LOG_DEBUG << "uid: " << (unsigned)t.stuid().luid() << ", send to room server. " << "roomid: " << t.sroomid() << ", push obj: " << g_sLocalPushObj << endl;

    TClientParam clientParam;
    CRouterHelper::getClientParam(clientParam, cs);

    string sServantPrx;
    int ret = OuterFactorySingleton::getInstance()->getRoomServerPrx(t.sroomid(), sServantPrx);
    if (ret != 0)
    {
        ROLLLOG_ERROR << "get room prx err, uid: " << t.stuid().luid() << ", sRoomID: " << t.sroomid() << ", ret: " << ret << endl;
        return;
    }

    auto pServantObj = Application::getCommunicator()->stringToProxy<JFGame::RoomServantPrx>(sServantPrx);
    if (!pServantObj)
    {
        ROLLLOG_ERROR << "send msg to roomsvr error, uid: " << (unsigned)t.stuid().luid() << ", sServantPrx: " << sServantPrx << endl;
        return;
    }

    LOG_DEBUG << "send msg to roomsvr, uid: " << (unsigned)t.stuid().luid() << ", sServantPrx: " << sServantPrx << endl;
    pServantObj->tars_hash(t.stuid().luid())->async_onRequest(NULL, t.stuid().luid(), pbToString(t), g_sLocalPushObj, clientParam, cs->stUser);
}

// 异步调用RoomServer, offline
void OuterFactoryImp::asyncSend2RoomServerOffline(ConnStructPtr cs, bool standup)
{
    if (!cs)
    {
        ROLLLOG_DEBUG << "cs is null, roomid: " << endl;
        return;
    }

    if (cs->sRoomID.empty())
    {
        ROLLLOG_DEBUG << "room id empty, roomid: " << cs->sRoomID << ", uin: " << cs->lUserId << endl;
        return;
    }

    string sServantPrx;
    int ret = OuterFactorySingleton::getInstance()->getRoomServerPrx(cs->sRoomID, sServantPrx);
    if (ret != 0 || sServantPrx.empty())
    {
        ROLLLOG_ERROR << "get room prx err, uid: " << cs->lUserId << ", roomid: " << cs->sRoomID << ", sServantPrx: " << sServantPrx << ", ret: " << ret << endl;
        return;
    }

    auto pServantObj = Application::getCommunicator()->stringToProxy<JFGame::RoomServantPrx>(sServantPrx);
    if (!pServantObj)
    {
        ROLLLOG_ERROR << "send offline msg to roomsvr failed, uid: " << (unsigned)cs->lUserId << ", roomid: " << cs->sRoomID << ", prx: " << sServantPrx << endl;
        return;
    }

    pServantObj->tars_hash(cs->lUserId)->async_onOffline(NULL, cs->lUserId, cs->sRoomID, standup);
    ROLLLOG_DEBUG << "send offline msg to roomsvr succeed, uid: " << (unsigned)cs->lUserId << ", roomid: " << cs->sRoomID << ", prx: " << sServantPrx << endl;
}

//异步调用LoginServer(身份校验)
void OuterFactoryImp::asyncCheckToken(const XGameComm::TPackage &t, const ConnStructPtr cs)
{
    auto prx = g_app.getOutPtr()->getLoginServantPrx();
    if (!prx)
    {
        ROLLLOG_ERROR << "pLoginServantPrx is null" << endl;
        return;
    }

    CheckLoginTokenReq req;
    req.lUid = t.stuid().luid();
    req.sToken = t.stuid().stoken();
    req.sRemoteIP = cs->current->getIp();
    prx->tars_hash(t.stuid().luid())->async_checkLoginToken(new AsyncLoginCallback(t, req, cs), req);
}

//异步调用LoginServer(帐号退出)
void OuterFactoryImp::asyncLogoutNotify(const long uid, const string &sRemoteIP)
{
    auto prx = g_app.getOutPtr()->getLoginServantPrx();
    if (!prx)
    {
        ROLLLOG_ERROR << "pLoginServantPrx is null" << endl;
        return;
    }

    UserLogoutReq req;
    req.uid = uid;
    req.sRemoteIP = sRemoteIP;
    prx->tars_hash(uid)->async_Logout(new AsyncLogoutCallback(req), req);
}

// 异步调用LoginServer,传递请求数据
void OuterFactoryImp::asyncRequest2LoginServer(const XGameComm::TPackage &t, const ConnStructPtr &cs)
{
    auto prx = g_app.getOutPtr()->getLoginServantPrx();
    if (!prx)
    {
        ROLLLOG_ERROR << "LoginServantPrx is null" << endl;
        return;
    }

    TClientParam param;
    CRouterHelper::getClientParam(param, cs);
    prx->tars_hash(t.stuid().luid())->async_onRequest(NULL, t.stuid().luid(), pbToString(t), g_sLocalPushObj, param, cs->stUser);
}

// 异步调用HallServer,传递请求数据
void OuterFactoryImp::asyncRequest2UserInfoServer(const XGameComm::TPackage &t, const ConnStructPtr &cs)
{
    auto prx = g_app.getOutPtr()->getHallServerPrx();
    if (!prx)
    {
        ROLLLOG_ERROR << "getHallServerPrx is null" << endl;
        return;
    }

    TClientParam param;
    CRouterHelper::getClientParam(param, cs);
    prx->tars_hash(t.stuid().luid())->async_onRequest(NULL, t.stuid().luid(), pbToString(t), g_sLocalPushObj, param, cs->stUser);
}

// 异步调用UserStateServer,传递请求数据
void OuterFactoryImp::asyncRequest2UserStateServer(const XGameComm::TPackage &t, const ConnStructPtr &cs)
{
    auto prx = g_app.getOutPtr()->getPushServerPrx();
    if (!prx)
    {
        ROLLLOG_ERROR << "getPushServerPrx is null" << endl;
        return;
    }

    TClientParam param;
    CRouterHelper::getClientParam(param, cs);
    prx->tars_hash(t.stuid().luid())->async_onRequest(NULL, t.stuid().luid(), pbToString(t), g_sLocalPushObj, param, cs->stUser);
}

// 异步调用ConfigServer,传递请求数据
void OuterFactoryImp::asyncRequest2ConfigServer(const XGameComm::TPackage &t, const ConnStructPtr &cs)
{
    auto prx = g_app.getOutPtr()->getConfigServantPrx();
    if (!prx)
    {
        ROLLLOG_ERROR << "getConfigServantPrx is null" << endl;
        return;
    }

    TClientParam param;
    CRouterHelper::getClientParam(param, cs);;
    prx->tars_hash(t.stuid().luid())->async_onRequest(NULL, t.stuid().luid(), pbToString(t), g_sLocalPushObj, param, cs->stUser);
}

//异步调用PushServer更新用户状态
void OuterFactoryImp::asyncRequest2PushUserState(const long iUin, const long iCid, const bool isOnline)
{
    auto prx = g_app.getOutPtr()->getPushServerPrx();
    if (!prx)
    {
        ROLLLOG_ERROR << "getPushServerPrx is null" << endl;
        return;
    }

    if (isOnline)
    {
        userstate::ReportOnlineStateReq req;
        req.uid    = iUin;
        req.gwAddr = g_sLocalPushObj;
        req.gwCid  = iCid;
        req.state  = userstate::E_ONLINE_STATE_ONLINE;
        prx->tars_hash(iUin)->async_reportOnlineState(NULL, req);
    }
    else
    {
        userstate::ReportOnlineStateReq req;
        req.uid    = iUin;
        req.gwAddr = g_sLocalPushObj;
        req.gwCid  = iCid;
        req.state  = userstate::E_ONLINE_STATE_OFFLINE;
        prx->tars_hash(iUin)->async_reportOnlineState(NULL, req);
    }
}

// 异步调用HallServer(1-在线, 0-离线)
void OuterFactoryImp::asyncRequest2HallUserState(long iUin, int state)
{
    auto prx = g_app.getOutPtr()->getHallServerPrx();
    if (!prx)
    {
        ROLLLOG_ERROR << "getHallServerPrx is null" << endl;
        return;
    }

    hall::RouterUserStateReq req;
    req.uid = iUin;
    req.state = state;
    req.sRouterId = g_sLocalPushObj;
    prx->tars_hash(iUin)->async_RouterUserState(new AsyncUserStateCallback(iUin), req);
}

//异步调用推送服务
void OuterFactoryImp::asyncRequest2PushServer(const XGameComm::TPackage &t, const ConnStructPtr &cs)
{
    auto prx = g_app.getOutPtr()->getPushServerPrx();
    if (!prx)
    {
        ROLLLOG_ERROR << "getPushServerPrx is null" << endl;
        return;
    }

    TClientParam param;
    CRouterHelper::getClientParam(param, cs);
    prx->tars_hash(t.stuid().luid())->async_onRequest(NULL, t.stuid().luid(), pbToString(t), g_sLocalPushObj, param, cs->stUser);
}

//异步调用PushServer在线用户状态
void OuterFactoryImp::asyncReportOnlineUsers(const std::map<long, long> &users)
{
    auto prx = g_app.getOutPtr()->getPushServerPrx();
    if (!prx)
    {
        ROLLLOG_ERROR << "getPushServerPrx is null" << endl;
        return;
    }

    push::OnlineUserReportReq req;
    req.users = users;
    req.sRouterAddr = g_sLocalPushObj;
    prx->async_onlineUserReport(NULL, req);
    ROLLLOG_DEBUG << "@PushServer report online users, userNum: " << req.users.size() << endl;
}

//发送通知到客户端
int OuterFactoryImp::sendPushNotify(const ConnStructPtr ptr, const int iNotifyType, const bool bReturnLoginUI)
{
    if (!ptr || !(ptr->current))
    {
        LOG_DEBUG << "invalid ptr" << endl;
        return -1;
    }

    XGameComm::TPackage t;
    t.set_iversion(1);
    t.set_igameid(22000401);
    t.set_sroomid(ptr->sRoomID);
    t.set_iroomserverid(3);

    auto ptuid = t.mutable_stuid();
    ptuid->set_luid(ptr->lUserId);
    ptuid->set_stoken("");
    t.set_isequence(1);
    t.set_iflag(2);
    t.clear_vecmsghead();
    t.clear_vecmsgdata();

    auto mh = t.add_vecmsghead();
    mh->set_nmsgid(XGameProto::PUSH_SERVER_CHANGE_NOTIFY);
    mh->set_nmsgtype(XGameComm::MSGTYPE::MSGTYPE_NOTIFY);
    mh->set_servicetype(XGameComm::SERVICE_TYPE::SERVICE_TYPE_PUSH);

    PushProto::ServerChangeNotify msg;
    msg.set_inotifytype(iNotifyType);
    msg.set_ireturnlogingui(bReturnLoginUI);
    t.add_vecmsgdata(pbToString(msg));

    sendResponse(pbToString(t), ptr);
    return 0;
}

void OuterFactoryImp::sendResponse(const string &buff, const ConnStructPtr cs)
{
    int nMsgID = 0;
    int nSvrType = 0;
    XGameComm::TPackage rsp;
    if (rsp.ParseFromString(buff))
    {
        nMsgID = rsp.vecmsghead().at(0).nmsgid();
        nSvrType = rsp.vecmsghead().at(0).servicetype();
    }

    if(!cs)
    {
       ROLLLOG_ERROR << "cs is nullprt." << endl;
       return; 
    }

    ROLLLOG_DEBUG << "TNOWMS: " << TNOWMS << endl;

    cs->iActiveCount += 1;
    rsp.set_iactivecount(cs->iActiveCount);

    if(RsaOpen == 1 && nMsgID != XGameComm::Eum_Comm_Msgid::E_MSGID_LOGIN_HALL_RESP)
    {
        for(int i = 0; i < rsp.vecmsgdata_size(); i++)
        {
            if(rsp.vecmsgdata().at(i).empty())
            {
                continue;
            }
            string data = string(rsp.vecmsgdata().at(i).begin(), rsp.vecmsgdata().at(i).end());

            if(nMsgID == 3)
            {
                XGameSoProto::TSoMsg sTSoMsg;
                pbToObj(string(data.begin(), data.end()), sTSoMsg);
                
                ROLLLOG_DEBUG <<", uin: " << rsp.stuid().luid() <<", nMsgID: "<< nMsgID <<  ", cmd: "<<  sTSoMsg.ncmd() << ", data len: "<< data.length() << endl;
            }

            data = CRSAEnCryptSingleton::getInstance()->rsaPrivateKeyEncryptSplit(data, cs->getPrivateKey());//私钥加密
            //data = CRSAEnCryptSingleton::getInstance()->base64Encode((uint8_t*)data.c_str(), data.length());

            rotateStrLeft(data, cs->iActiveCount);
            rsp.set_vecmsgdata(i, data);

            /*data = CRSAEnCryptSingleton::getInstance()->rsaPublicKeyDecryptSplit(data, cs->getPubKey());
            LOG_DEBUG << "pub data: " << data << ", len: "<< data.length() << endl;*/

        }
    }

    ROLLLOG_DEBUG << "send message to client success"<< ", uin: " << rsp.stuid().luid() << ", msgID:"<< nMsgID<< ", svrType:" << nSvrType << ", TNOWMS: " << TNOWMS << endl;

    string sMsgPack;
    CRouterHelper::Encode(rsp, sMsgPack);
    cs->current->sendResponse(sMsgPack.c_str(), sMsgPack.length());
    return;
}

string OuterFactoryImp::decryptRequest(const string &buff, const ConnStructPtr cs)
{
    string data = buff;
    if(!cs)
    {
        ROLLLOG_ERROR << "cs is nullprt." << endl;
        return data; 
    }
    if(!RsaOpen)
    {
        return data;
    }
    rotateStrRight(data, cs->iActiveCount);
    //LOG_DEBUG << "data: "<< data << ", buff: "<< buff << endl;
    return CRSAEnCryptSingleton::getInstance()->rsaPrivateKeyDecryptSplit(data, cs->getPrivateKey());//私钥解密
    //return CRSAEnCryptSingleton::getInstance()->base64Decode(data);
}

void OuterFactoryImp::reverse(string &str,int start,int end)
{
    for( ;start < end; start++,end--)
    {
        char tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
    }
}

void OuterFactoryImp::rotateStrLeft(string &str, int offset)
{
    return;
    if(str.empty())
    {
        return;
    }
    offset = offset % str.length();
    // 三次翻转
    reverse(str, 0, offset-1);
    reverse(str, offset, str.length()-1);
    reverse(str, 0, str.length()-1);
}

void OuterFactoryImp::rotateStrRight(string &str, int offset)
{
    return ;
    if(str.empty())
    {
        return;
    }
    offset = offset % str.length();
    // 三次翻转
    reverse(str, 0, str.length()- offset -1);
    reverse(str, str.length()-offset, str.length()-1);
    reverse(str, 0, str.length()-1);
}
