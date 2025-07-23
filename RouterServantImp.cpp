#include "RouterServantImp.h"

#include "globe.h"
#include "LogComm.h"
#include "JFGameCommProto.h"
#include "XGameComm.pb.h"
#include "OuterFactoryImp.h"
#include "RouterHelper.h"
#include "RouterServer.h"
#include "push.pb.h"
#include "head.pb.h"

//
using namespace std;
using namespace tars;
using namespace JFGamecomm;
using namespace login;

//////////////////////////////////////////////////////
void RouterServantImp::initialize()
{

}

//////////////////////////////////////////////////////
void RouterServantImp::destroy()
{
    //destroy servant here:
    //...
}

// 将请求进行分发
int RouterServantImp::doRequest(tars::TarsCurrentPtr current, vector<char> &response)
{
    __TRY__

    XGameComm::TPackage tPkg;
    pbToObj(string(&current->getRequestBuffer()[2], current->getRequestBuffer().size() - 2), tPkg);

    auto uid = tPkg.stuid().luid();
    auto cs = g_connMap.getCurrent(uid);
    if (!cs) //first login
    {
        cs = new ConnStruct();
        cs->current = current;
        cs->lUserId = tPkg.stuid().luid();
        cs->iLoginTime = TNOW;

        std::vector<string> vStrKeys;
        CRSAEnCryptSingleton::getInstance()->generateRSAKey(vStrKeys);
        cs->initRsaKeys(vStrKeys);

        ROLLLOG_DEBUG << "First connection info, uid: " << uid
                      << ", guid1: " << current->getUId()
                      << ", addr: "  << current->getIp()
                      << ", port: "  << current->getPort() << endl;
    }
    else if (cs->current->getUId() != current->getUId())//multi login
    {
        ROLLLOG_DEBUG << "Again connection info, uid: " << uid
                      << ", guid1: " << current->getUId()
                      << ", guid2: " << cs->current->getUId()
                      << ", addr: "  << current->getIp()
                      << ", port: "  << current->getPort() << endl;

        //设备切换通知
        XGameComm::TPackage rsp;
        rsp.set_iversion(tPkg.iversion());
        rsp.mutable_stuid()->set_luid(tPkg.stuid().luid());
        rsp.set_igameid(tPkg.igameid());
        rsp.set_sroomid(tPkg.sroomid());
        rsp.set_iroomserverid(tPkg.iroomserverid());
        rsp.set_isequence(tPkg.isequence());
        rsp.set_iflag(tPkg.iflag());

        auto mh = rsp.add_vecmsghead();
        mh->set_nmsgid(XGameProto::ActionName::PUSH_MULTIPLE_LOGIN);
        mh->set_nmsgtype(XGameComm::MSGTYPE::MSGTYPE_NOTIFY);
        mh->set_servicetype(XGameComm::SERVICE_TYPE::SERVICE_TYPE_PUSH);

        PushProto::DefaultNotify notify;
        notify.set_resultcode(0);
        rsp.add_vecmsgdata(pbToString(notify));

        g_app.getOutPtr()->sendResponse(pbToString(rsp), cs);
        cs->current->close();

        //新建连接
        cs = new ConnStruct();
        cs->current = current;
        cs->lUserId = tPkg.stuid().luid();
        cs->iLoginTime = TNOW;

        std::vector<string> vStrKeys;
        CRSAEnCryptSingleton::getInstance()->generateRSAKey(vStrKeys);
        cs->initRsaKeys(vStrKeys);
    }
    else //Is connected
    {
        g_connMap.checkTimeOut(tPkg.stuid().luid(), cs);
    }

    if(tPkg.iversion() < g_app.getOutPtr()->getVersionConfig())
    {
        XGameComm::TPackage rsp;
        rsp.set_iversion(tPkg.iversion());
        rsp.mutable_stuid()->set_luid(tPkg.stuid().luid());
        rsp.set_igameid(tPkg.igameid());
        rsp.set_sroomid(tPkg.sroomid());
        rsp.set_iroomserverid(tPkg.iroomserverid());
        rsp.set_isequence(tPkg.isequence());
        rsp.set_iflag(tPkg.iflag());

        auto mh = rsp.add_vecmsghead();
        mh->set_nmsgid(XGameProto::ActionName::PUSH_VERSION_NOT_EQUAL);
        mh->set_nmsgtype(XGameComm::MSGTYPE::MSGTYPE_NOTIFY);
        mh->set_servicetype(XGameComm::SERVICE_TYPE::SERVICE_TYPE_PUSH);

        PushProto::DefaultNotify notify;
        notify.set_resultcode(0);
        rsp.add_vecmsgdata(pbToString(notify));
        g_app.getOutPtr()->sendResponse(pbToString(rsp), cs);

        ROLLLOG_ERROR << "version:"<< tPkg.iversion() << " < "<< g_app.getOutPtr()->getVersionConfig() << endl;
    }

    //单个链接单位时间内检查消息个数
    if(cs->tLastActiveTime == TNOW)
    {
        if(cs->iActiveCount >= g_app.getOutPtr()->getMessageCountLimit())
        {
            ROLLLOG_ERROR << "message count out limit. cs active count:"<< cs->iActiveCount << ", limit: "<< g_app.getOutPtr()->getMessageCountLimit()<< endl;
            return 0;
        }
    }
    else
    {
        cs->iActiveCount = 0;
    }
    cs->iActiveCount += 1;

    ROLLLOG_DEBUG << "cs active count:"<< cs->iActiveCount << "limit: "<< g_app.getOutPtr()->getMessageCountLimit()<< endl;

    g_app.getOutPtr()->asyncRequest2PushUserState(uid, cs->current->getUId(), true);

    //消息
    vector<MsgStruct> vRm;
    for (int i = 0; i < tPkg.vecmsghead_size(); ++i)
    {
        int64_t ms1 = TNOWMS;

        XGameComm::TPackage t;
        t.set_iversion(tPkg.iversion());

        auto ptuid = t.mutable_stuid();
        ptuid->set_luid(tPkg.stuid().luid());

        t.set_igameid(tPkg.igameid());
        t.set_sroomid(tPkg.sroomid());
        t.set_iroomserverid(tPkg.iroomserverid());
        t.set_isequence(tPkg.isequence());
        t.set_iflag(tPkg.iflag());

        auto mh = t.add_vecmsghead();
        mh->set_nmsgid(tPkg.vecmsghead(i).nmsgid());
        mh->set_nmsgtype(XGameComm::MSGTYPE::MSGTYPE_REQUEST);
        mh->set_servicetype(tPkg.vecmsghead(i).servicetype());

        if (tPkg.vecmsgdata_size() > i)
        {
            string data = g_app.getOutPtr()->decryptRequest(tPkg.vecmsgdata(i), cs);
            t.add_vecmsgdata(data);
        }
        else
        {
            t.add_vecmsgdata("");
        }

        ROLLLOG_DEBUG << "msgId: "<< tPkg.vecmsghead(i).nmsgid() << ", i: "<< i << ", data size:"<< tPkg.vecmsgdata_size() << "TNOWMS：" << TNOWMS << endl;

        int iRet = onService(t, cs);
        if (iRet == 0)
            continue;

        MsgStruct ms;
        if (tPkg.vecmsgdata_size() > i)
        {
            string data = g_app.getOutPtr()->decryptRequest(tPkg.vecmsgdata(i), cs);
            ms.pad(tPkg.vecmsghead(i), data);
        }
        else
        {
            ms.pad(tPkg.vecmsghead(i), "");
        }

        int msgId = tPkg.vecmsghead(i).nmsgid();

        //单位时间， 游戏消息，一条消息 只处理一次，存在bug， 产品提的需要要求这样改
       /* if(msgId == 1)
        {
            XGameSoProto::TSoMsg sTSoMsg;
            pbToObj(string(tPkg.vecmsgdata(i).begin(), tPkg.vecmsgdata(i).end()), sTSoMsg);

            if(cs->tLastActiveTime == TNOW)
            {
                auto it = std::find(cs->vecMsgIdList.begin(), cs->vecMsgIdList.end(), sTSoMsg.ncmd());
                if(it != cs->vecMsgIdList.end())
                {
                    ROLLLOG_DEBUG << "multi deal msg. cmd: "<< sTSoMsg.ncmd() << endl;
                    continue;
                }
                else
                {
                    cs->vecMsgIdList.push_back(sTSoMsg.ncmd());
                }
            }
            else
            {
                cs->vecMsgIdList.clear();
            }

            ROLLLOG_DEBUG <<", uin: " << tPkg.stuid().luid() <<", msgId: "<< msgId <<  ", cmd: "<<  sTSoMsg.ncmd() << ", data len: "<< tPkg.vecmsgdata(i).length() << endl;
        }*/

        if (XGameComm::Eum_Comm_Msgid::E_MSGID_KEEP_ALIVE_REQ != msgId)
        {
            ROLLLOG_DEBUG << "recv msg, uid: " << tPkg.stuid().luid() << ", msgid : " << msgId << endl;
        }

        switch (msgId)
        {
        case XGameComm::Eum_Comm_Msgid::E_MSGID_LOGIN_HALL_REQ:
        {
            XGameComm::TPackage t;
            t.set_iversion(tPkg.iversion());

            auto ptuid = t.mutable_stuid();
            ptuid->set_luid(tPkg.stuid().luid());
            ptuid->set_stoken(tPkg.stuid().stoken());

            t.set_igameid(tPkg.igameid());
            t.set_sroomid(tPkg.sroomid());
            t.set_iroomserverid(tPkg.iroomserverid());
            t.set_isequence(tPkg.isequence());
            t.set_iflag(tPkg.iflag());

            auto mh = t.add_vecmsghead();
            mh->set_nmsgid(tPkg.vecmsghead(i).nmsgid());
            mh->set_nmsgtype(XGameComm::MSGTYPE::MSGTYPE_REQUEST);
            mh->set_servicetype(XGameComm::SERVICE_TYPE::SERVICE_TYPE_DEFAULT);

            if (tPkg.vecmsgdata_size() > i)
                t.add_vecmsgdata(tPkg.vecmsgdata(i));
            else
                t.add_vecmsgdata("");

            cs->tLastActiveTime = TNOW;
            g_app.getOutPtr()->asyncCheckToken(t, cs);
            return 0;
        }
        case XGameComm::Eum_Comm_Msgid::E_MSGID_KEEP_ALIVE_REQ:
        {
            XGameComm::TPackage t;
            t.set_iversion(tPkg.iversion());
            t.set_igameid(tPkg.igameid());
            t.set_sroomid(tPkg.sroomid());
            t.set_iroomserverid(tPkg.iroomserverid());

            auto ptuid = t.mutable_stuid();
            ptuid->set_luid(tPkg.stuid().luid());
            ptuid->set_stoken(tPkg.stuid().stoken());

            t.set_isequence(tPkg.isequence());
            t.set_iflag(tPkg.iflag());
            t.clear_vecmsghead();
            t.clear_vecmsgdata();

            auto mh = t.add_vecmsghead();
            mh->set_nmsgid(XGameComm::Eum_Comm_Msgid::E_MSGID_KEEP_ALIVE_RESP);
            mh->set_nmsgtype(XGameComm::MSGTYPE::MSGTYPE_RESPONSE);
            t.add_vecmsgdata(L2S(time(NULL)));

            g_app.getOutPtr()->sendResponse(pbToString(t), cs);

            //通知@世界服用户状态
           /* int now = TNOW;
            int interval = 1800;
            if (now >= (cs->tLastSyncTime + interval))
            {
                cs->tLastSyncTime = TNOW;
                g_app.getOutPtr()->asyncRequest2PushUserState(uid, cs->current->getUId(), true);
            }*/

            //ROLLLOG_DEBUG << "send keep alive response msg, uid: " << tPkg.stuid().luid() << ", msgid: " << tPkg.vecmsghead(i).nmsgid() << ", len: " << sMsgPack.length() /*<< ", t: " << pbToString(t) */<< endl;
            return 0;
        }
        //心跳消息响应
        case XGameComm::Eum_Comm_Msgid::E_MSGID_KEEP_ALIVE_RESP:
        {
            ROLLLOG_DEBUG << "recv keep alive response msg, uid: " << tPkg.stuid().luid() << ", msgid: " << tPkg.vecmsghead(i).nmsgid() << endl;
            return 0;
        }
        //异常操作
        default:
        {
            cs->tLastActiveTime = TNOW;
            vRm.push_back(ms);
            break;
        }
        }

        //房间号切换. 做下线处理
        if (!tPkg.sroomid().empty())
        {
            if (tPkg.sroomid().compare(cs->sRoomID) != 0)
            {
                if (!cs->sRoomID.empty())
                {
                    g_app.getOutPtr()->asyncSend2RoomServerOffline(cs, true);
                }

                cs->sRoomID = tPkg.sroomid();
            }
        }

        int64_t ms2 = TNOWMS;
        if ((ms2 - ms1) > COST_MS)
        {
            ROLLLOG_WARN << "@Performance, msgid:" << tPkg.vecmsghead(i).nmsgid() << ", costTime:" << (ms2 - ms1) << endl;
        }
    }

    //异步操作
    current->setResponse(false);

    //判断是否同时透传到RoomServer
    if (vRm.size() > 0)
    {
        if (cs->iLoginTime > 0)
        {
            XGameComm::TPackage t;
            t.set_iversion(tPkg.iversion());
            t.mutable_stuid()->set_luid(tPkg.stuid().luid());
            t.set_igameid(tPkg.igameid());
            t.set_sroomid(tPkg.sroomid());
            t.set_iroomserverid(tPkg.iroomserverid());
            t.set_isequence(tPkg.isequence());
            t.set_iflag(tPkg.iflag());
            OuterFactorySingleton::getInstance()->asyncRequest2Room(t, vRm, cs);
            // ROLLLOG_DEBUG << "uid : " << t.stuid().luid() << ", package flag : " << t.iflag() << endl;
        }
        else
        {
            // ROLLLOG_DEBUG << "authentication fail, close the connection, uid: " << (unsigned)t.stuid().luid() << endl;
            cs->current->close();
        }
    }

    cs->tLastActiveTime = TNOW;

    return 0;
    __CATCH__
    return 0;
}

//服务请求处理
int RouterServantImp::onService(const XGameComm::TPackage &t, const ConnStructPtr &cs)
{
    if (t.vecmsghead_size() == 0)
    {
        ROLLLOG_ERROR << "no request data, uid: " << (unsigned)t.stuid().luid() << endl;
        return -1;
    }

    int iServiceType = t.vecmsghead(0).servicetype();
    auto pServantPrx = g_app.getOutPtr()->getGameTCPPrx(iServiceType);
    if (!pServantPrx)
    {
        if (iServiceType > 0)
        {
            ROLLLOG_ERROR << "pServantPrx is null, uid: " << (unsigned)t.stuid().luid() << ", serviceType: " << iServiceType << endl;
        }

        return -2;
    }

    if (cs->iLoginTime <= 0)
    {
        ROLLLOG_DEBUG << "no authentication , close the connection, uid : " << t.stuid().luid() << ", serviceType: " << iServiceType << endl;
        cs->current->close();
        return 0;
    }

    TClientParam clientParam;
    CRouterHelper::getClientParam(clientParam, cs);
    pServantPrx->tars_hash(t.stuid().luid())->async_onRequest(NULL, t.stuid().luid(), pbToString(t), g_sLocalPushObj, clientParam, cs->stUser);
    ROLLLOG_DEBUG << "onService, uid: " << t.stuid().luid() << ", serviceType: " << iServiceType << endl;
    return 0;
}

int RouterServantImp::doResponse(ReqMessagePtr resp)
{
    ROLLLOG_DEBUG << "get a response, size: " << resp->response->sBuffer.size() << endl;
    return 0;
}

int RouterServantImp::doResponseException(ReqMessagePtr resp)
{
    ROLLLOG_DEBUG << "get a exception response, size: " << resp->response->sBuffer.size() << endl;
    return 0;
}

int RouterServantImp::doResponseNoRequest(ReqMessagePtr resp)
{
    ROLLLOG_DEBUG << "get a no request response, size: " << resp->response->sBuffer.size() << endl;
    return 0;
}

int RouterServantImp::doClose(tars::TarsCurrentPtr current)
{
    //用户索引
    auto uid = g_app.GetUidFromClientIDMap(current->getUId());
    if (uid != -1)
    {
        //获取连接对象
        auto cs = g_connMap.getCurrent(uid);
        if (cs && !(cs->sRoomID.empty()))
        {
            //通知@房间服用户状态
            g_app.getOutPtr()->asyncSend2RoomServerOffline(cs);
        }

        //////////////////////////////////////////////////////////////////////////

        //通知@登录服用户状态
        g_app.getOutPtr()->asyncLogoutNotify(uid, current->getIp());
        //通知@广场服用户状态
        g_app.getOutPtr()->asyncRequest2HallUserState(uid, false);
        //通知@世界服用户状态
        g_app.getOutPtr()->asyncRequest2PushUserState(uid, current->getUId(), false);
        //清除当前连接映射
        g_connMap.erase(uid);
    }

    //清除映射关系
    g_app.PopUidFromClientIDMap(current->getUId());

    ROLLLOG_DEBUG << "User close, uid: " << uid << ", clientId: "  << current->getUId() << ", closeType: " << current->getCloseType() << endl;
    return 0;
}

