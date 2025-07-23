#include "PushImp.h"
#include "servant/Application.h"
#include "OuterFactoryImp.h"
#include "RouterServer.h"
#include "LogComm.h"
#include "globe.h"
#include "XGameMTT.pb.h"

//
using namespace std;

//////////////////////////////////////////////////////
void PushImp::initialize()
{
    //initialize servant here:
    //...
}

//////////////////////////////////////////////////////
void PushImp::destroy()
{
    //destroy servant here:
    //...
}

//推送pb协议数据给客户端
tars::Int32 PushImp::doPushBuf(tars::Int64 uin, const std::string &buf, tars::TarsCurrentPtr current)
{
    __TRY__

    current->setResponse(false);
    async_response_doPushBuf(current, 0);

    auto cs = g_connMap.getCurrent(uin);
    if (cs && cs->current)
    {
        g_app.getOutPtr()->sendResponse(buf, cs);
        return 0;
    }

    ROLLLOG_ERROR << "send message to client failed. uin: " << uin << endl;
    return 1;
    __CATCH__
    return -1;
}

//推送pb协议数据给客户端
tars::Int32 PushImp::doPushUserState(tars::Int64 uin, tars::Int64 cid, tars::Int32 state, const std::string &addr, const std::string &buf, tars::TarsCurrentPtr current)
{
    __TRY__

    current->setResponse(false);
    async_response_doPushBuf(current, 0);

    auto cs = g_connMap.getCurrent(uin);
    if (cs && cs->current)
    {
        if ((cs->current->getUId() != cid) && (addr != g_sLocalPushObj) && (state == 1))
        {
            g_app.getOutPtr()->sendResponse(buf, cs);
            cs->current->close();
        }

        return 0;
    }

    ROLLLOG_ERROR << "send message(1601) to client failed. uin: " << uin << ", cid: " << cid << endl;
    return 1;
    __CATCH__
    return -1;
}

//推送pb协议数据给客户端(roomid相关)
tars::Int32 PushImp::doPushBufByRoomID(tars::Int64 uin, const std::string &buf, const std::string &sRoomID, int msgID, tars::TarsCurrentPtr current)
{
    __TRY__

    current->setResponse(false);
    async_response_doPushBuf(current, 0);

    auto cs = g_connMap.getCurrent(uin);
    if (cs && cs->current && (cs->sRoomID == sRoomID || sRoomID.empty() || msgID == XGameMTT::E_MTT_MSGID_GAME_BEGIN_2C/* 开赛通知*/))
    {
        g_app.getOutPtr()->sendResponse(buf, cs);
        return 0;
    }

    if (cs)
    {
        ROLLLOG_DEBUG << "send message to client failed(1), uin: " << uin
                      << ", msgID:"       << msgID
                      << ", cs->sRoomID:" << cs->sRoomID
                      << ", sRoomID: "    << sRoomID
                      << ", current: "    << cs->current << endl;
    }
    else
    {
        ROLLLOG_DEBUG << "send message to client failed(2), uin: " << uin
                      << ", msgID:" << msgID << endl;
    }

    return 1;
    __CATCH__
    return -1;
}


