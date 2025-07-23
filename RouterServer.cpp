#include "RouterServer.h"
#include "RouterServantImp.h"
#include "PushImp.h"
#include "LogComm.h"

#include "push.pb.h"

//application
RouterServer g_app;
//用户连接映射
CConnMap g_connMap;
//PushObj的具体地址
string g_sLocalPushObj;

/////////////////////////////////////////////////////////////////
void RouterServer::initialize()
{
    // 注册推送处理对象
    string sPushObj = ServerConfig::Application + "." + ServerConfig::ServerName + ".PushObj";
    addServant<PushImp>(sPushObj);
    ROLLLOG_DEBUG << "push obj: " << sPushObj << endl;

    // 自定义协议监听端口,绑定Servant
    string sRouterObj = ServerConfig::Application + "." + ServerConfig::ServerName + ".RouterServantObj";
    addServant<RouterServantImp>(sRouterObj);
    ROLLLOG_DEBUG << "router obj: " << sRouterObj << endl;

    // 设置adaptor的协议解析器
    addServantProtocol(sRouterObj, AppProtocol::parseStream<0, uint16_t, true>);

    // 获取本地PushObj具体地址
    TC_Endpoint ep = Application::getEpollServer()->getBindAdapter(ServerConfig::Application + "." + ServerConfig::ServerName + ".PushObjAdapter")->getEndpoint();
    g_sLocalPushObj = ServerConfig::Application + "." + ServerConfig::ServerName + ".PushObj" + "@" + ep.toString();

    // 外部对象
    m_pOuter = getOutPtr();

    // 设置连接池超时
    setConnTimeout();
    // 初始化所有线程
    initialThread();

    // 注册动态加载命令
    TARS_ADD_ADMIN_CMD_NORMAL("reload", RouterServer::reloadSvrConfig);
    // 注册远程加载路由表命令
    TARS_ADD_ADMIN_CMD_NORMAL("loadremoteroomlist", RouterServer::loadRemoteRoomList);
    // 注册显示已有连接账户
    TARS_ADD_ADMIN_CMD_NORMAL("showconnusers", RouterServer::showConnUsers);
    // 注册关闭所用用户连接
    TARS_ADD_ADMIN_CMD_NORMAL("killuser", RouterServer::killUsers);
    // 注册设置白单开关
    TARS_ADD_ADMIN_CMD_NORMAL("openwhilelist", RouterServer::OpenWhiteList);

    ROLLLOG_DEBUG << "init RouterServer Succ." << endl;
}

/////////////////////////////////////////////////////////////////
void RouterServer::destroyApp()
{
    LOG_ERROR << "shtudonw all users" << endl;
    g_connMap.kill();
}

/*
* 配置变更，重新加载配置
*/
bool RouterServer::reloadSvrConfig(const string &command, const string &params, string &result)
{
    try
    {
        getOutPtr()->load();
        result = "success";
        LOG_ERROR << "reloadSvrConfig: " << result << endl;
        return true;
    }
    catch (TC_Exception const &e)
    {
        result = string("catch tc exception: ") + e.what();
    }
    catch (std::exception const &e)
    {
        result = string("catch std exception: ") + e.what();
    }
    catch (...)
    {
        result = "catch unknown exception";
    }

    result += "\n fail, please check it.";
    LOG_ERROR << "reloadSvrConfig: " << result << endl;
    return true;
}

/*
* 远程加载房间列表配置
*/
bool RouterServer::loadRemoteRoomList(const string &command, const string &params, string &result)
{
    try
    {
        getOutPtr()->loadRoomServerList();
        result = "success";
        LOG_ERROR << "loadremoteroomlist: " << result << endl;
        return true;
    }
    catch (TC_Exception const &e)
    {
        result = string("catch tc exception: ") + e.what();
    }
    catch (std::exception const &e)
    {
        result = string("catch std exception: ") + e.what();
    }
    catch (...)
    {
        result = "catch unknown exception";
    }

    result += "\n fail, please check it.";
    LOG_ERROR << "load remote room list: " << result << endl;
    return true;
}

/**
 * 显示已有连接账户
*/
bool RouterServer::showConnUsers(const string &command, const string &params, string &result)
{
    try
    {
        std::vector<int64_t> users = g_connMap.getUsers();

        ostringstream os;
        for(auto it = users.begin(); it != users.end(); ++it)
        {
            os << *it << " ";
        }

        result = "success";
        LOG_ERROR << "showConnUsers: " << result << ", users: " << os.str() << endl;
        return true;
    }
    catch (TC_Exception const &e)
    {
        result = string("catch tc exception: ") + e.what();
    }
    catch (std::exception const &e)
    {
        result = string("catch std exception: ") + e.what();
    }
    catch (...)
    {
        result = "catch unknown exception.";
    }

    result += "\n fail, please check it.";
    LOG_ERROR << "show conn users: " << result << endl;
    return true;
}

/**
* 主动关闭用户连接
*/
bool RouterServer::killUsers(const string &command, const string &params, string &result)
{
    try
    {
        long uid = 0;
        if (params != "")
        {
            uid = S2L(params);
        }

        if (uid == 0)
        {
            g_connMap.kill();
        }
        else
        {
            auto cs = g_connMap.getCurrent(uid);
            if (cs)
            {
                g_app.getOutPtr()->sendPushNotify(cs, PushProto::SERVICE_MAINTAIN, true);
                cs->current->close();
            }
        }

        LOG_ERROR << "shutdown users, result: " << params << endl;
        result = "success";
        return true;
    }
    catch (TC_Exception const &e)
    {
        result = string("catch tc exception: ") + e.what();
    }
    catch (std::exception const &e)
    {
        result = string("catch std exception: ") + e.what();
    }
    catch (...)
    {
        result = "catch unknown exception.";
    }

    result += "\n fail, please check it.";
    LOG_ERROR << "shutdown users result: " << result << endl;
    return true;
}

/**
 * 设置白单开关
*/
bool RouterServer::OpenWhiteList(const string &command, const string &params, string &result)
{
    try
    {
        if (params != "1")
            g_app.getOutPtr()->setTestOpen(false);
        else
            g_app.getOutPtr()->setTestOpen(true);

        LOG_ERROR << "set while list, result: " << params << endl;
        result = "success, testOpen: " + params;
        return true;
    }
    catch (TC_Exception const &e)
    {
        result = string("catch tc exception: ") + e.what();
    }
    catch (std::exception const &e)
    {
        result = string("catch std exception: ") + e.what();
    }
    catch (...)
    {
        result = "catch unknown exception.";
    }

    result += "\n fail, please check it.";
    LOG_ERROR << "set while list result: " << result << endl;
    return true;
}

/**
 * 初始化所有线程
 */
void RouterServer::initialThread()
{
    std::function<void(int)> cmd(_functor);

    // 定时启动线程
    //_tpool.init(2);
    _tpool.init(1);
    _tpool.start();
    //线程1
    _tpool.exec(cmd, Functor::eDoOthers);
    //线程2
    //_tpool.exec(cmd, Functor::eRouteStatus);
}

/**
 * 结束所有线程
 */
void RouterServer::destroyThread()
{
    _functor.stop();

    LOG_DEBUG << "wait for all done ... " << endl;
    bool b = _tpool.waitForAllDone(1000);
    if (!b)
    {
        LOG_DEBUG << "wait for all done again, but forever..." << endl;
        b = _tpool.waitForAllDone(-1);
        LOG_DEBUG <<  "wait for all done again..." << b << ":" << _tpool.getJobNum() << endl;
    }

    _tpool.stop();
}

/**
 * 设置连接池超时
 */
void RouterServer::setConnTimeout()
{
    const TConnConfig &tConnConfig = g_app.getOutPtr()->getConnConfig();

    ROLLLOG_DEBUG << "session time out: " << tConnConfig.SessionTimeOut << ", conn continue interval: " << tConnConfig.ConnContinueInterval << endl;

    //设置超时
    g_connMap.setTimeOut(tConnConfig.SessionTimeOut);
    g_connMap.setContinueInterval(tConnConfig.ConnContinueInterval);
    g_connMap.setKeepAliveTime(tConnConfig.KeepAliveInterval);
}

/**
 * 获取外部对象
 */
OuterFactoryImp *RouterServer::getOutPtr()
{
    return OuterFactorySingleton::getInstance();
}

/**
* 获取连接映射
*/
tars::Int64 RouterServer::GetUidFromClientIDMap(const tars::Int64 clientId)
{
    try
    {
        TC_LockT<TC_ThreadMutex> lock(m_ThreadMutex);
        auto iter = m_cid2UidMap.find(clientId);
        if (iter != m_cid2UidMap.end())
        {
            tars::Int64 userId = (*iter).second;
            return userId;
        }

        return -1;
    }
    catch (...)
    {
        ROLLLOG_ERROR << "GetUidFromClientIDMap() exception, clientId: " << clientId << endl;
        return -1;
    }
}

/**
* 清除连接映射
*/
tars::Int64 RouterServer::PopUidFromClientIDMap(const tars::Int64 clientId)
{
    try
    {
        TC_LockT<TC_ThreadMutex> lock(m_ThreadMutex);
        auto iter = m_cid2UidMap.find(clientId);
        if (iter != m_cid2UidMap.end())
        {
            tars::Int64 userId = (*iter).second;
            m_cid2UidMap.erase(iter);
            return userId;
        }

        return -1;
    }
    catch (...)
    {
        ROLLLOG_ERROR << "PopUidFromClientIDMap() exception, clientId: " << clientId << endl;
        return -1;
    }
}

/**
 * 设置连接映射
 */
int RouterServer::SetUidToClientIDMap(const tars::Int64 clientId, const tars::Int64 userId)
{
    try
    {
        TC_LockT<TC_ThreadMutex> lock(m_ThreadMutex);
        auto iter = m_cid2UidMap.find(clientId);
        if (iter != m_cid2UidMap.end())
            (*iter).second = userId;
        else
            m_cid2UidMap.insert(make_pair(clientId, userId));

        return 0;
    }
    catch (...)
    {
        ROLLLOG_ERROR << "SetUidToClientIDMap() exception, userId: " << userId << endl;
        return -1;
    }
}

/**
 * 获取映射信息<uid, cid>
 */
int RouterServer::GetClientInfoList(std::map<long, long> &users)
{
    try
    {
        TC_LockT<TC_ThreadMutex> lock(m_ThreadMutex);
        for (auto iter = m_cid2UidMap.begin(); iter != m_cid2UidMap.end(); iter++)
        {
            long cid = (*iter).first;
            long uid = (*iter).second;
            users.insert(std::pair<long, long>(uid, cid));
        }

        return 0;
    }
    catch (...)
    {
        ROLLLOG_ERROR << "GetClientInfoList() exception" << endl;
        return -1;
    }
}

/////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    try
    {
        g_app.main(argc, argv);
        g_app.waitForShutdown();
    }
    catch (std::exception &e)
    {
        cerr << "std::exception:" << e.what() << std::endl;
    }
    catch (...)
    {
        cerr << "unknown exception." << std::endl;
    }

    return -1;
}
/////////////////////////////////////////////////////////////////


