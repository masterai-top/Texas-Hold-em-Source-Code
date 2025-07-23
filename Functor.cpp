#include "Functor.h"
#include "LogComm.h"
#include "RouterServer.h"
#include "util/tc_timeprovider.h"

//线程停止标记
volatile bool g_bStop = false;

//检查事件调整毫秒数
#define SLEEP_INTERVAL 10

//更新路由状态, 0成功，其它失败
void Functor::updateRouteStatus()
{
    __TRY__

    while (!g_bStop)
    {
        time_t tNow = TC_TimeProvider::getInstance()->getNow();
        auto &cfg = g_app.getOutPtr()->getTimeIntervalConfig();
        if ((tNow - iLastUpdateRouteTime) >= cfg.iUpdateRouteInterval)
        {
            LOG_DEBUG << "Check router status...." << endl;
            iLastUpdateRouteTime = tNow;
        }

        TC_Common::msleep(SLEEP_INTERVAL);
    }

    __CATCH__
}

// 实时性要求不高的合在一起
void Functor::doOthers()
{
    __TRY__

    while (!g_bStop)
    {
        time_t tNow = TNOW;
        auto &cfg = g_app.getOutPtr()->getTimeIntervalConfig();

        //连接超时
        if ((tNow - iClearTimeOutConnTime) >= cfg.iClearTimeOutConnInterval)
        {
            LOG_DEBUG << "Check connection time out..." << endl;
            g_connMap.timeout();
            iClearTimeOutConnTime = tNow;
        }

        //操作空闲
        if ((tNow - iKeepAliveTime) >= cfg.iCheckKeepAliveInterval)
        {
            LOG_DEBUG << "check keep alive..." << endl;
            iKeepAliveTime = tNow;

            //连接超时检查
            g_connMap.idle();

            //上线在线用户
            std::map<long, long> users;
            if (-1 != g_app.GetClientInfoList(users))
                g_app.getOutPtr()->asyncReportOnlineUsers(users);
            else
                LOG_ERROR << "GetClientInfoList() failed." << endl;
        }

        TC_Common::msleep(SLEEP_INTERVAL);
    }

    __CATCH__
}


