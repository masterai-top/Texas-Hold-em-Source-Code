#ifndef _ROUTER_HELPER_H_
#define _ROUTER_HELPER_H_

#include "servant/Application.h"
#include "MyTimeoutQueue.h"
#include "globe.h"
#include "JFGame.h"
#include "JFGameCommProto.h"
#include "XGameComm.pb.h"
#include "XGameService.pb.h"
#include "CommonStruct.pb.h"
#include "CommonCode.pb.h"
#include "LogComm.h"
#include "push.pb.h"

using namespace tars;
using namespace JFGame;
using namespace JFGamecomm;
using namespace XGameProto;

//基本结构，将组合消息分解后封装成一个个基本结构
struct MsgStruct
{
    //消息头部
    XGameComm::TMsgHead msgHead;
    //消息内容
    string sMsgData;
    //操作结果
    int iResultID;

    //根据传入参数，填充请求态的基本结构
    void pad(const XGameComm::TMsgHead &mh, const string &vecMsgData)
    {
        msgHead = mh;
        sMsgData.assign(vecMsgData.begin(), vecMsgData.end());
    }

    //根据传入参数，填充返回态的基本结构
    template<typename T>
    void makeResp(int iMsgId, const T &rsp)
    {
        msgHead.set_nmsgid(iMsgId);
        msgHead.set_nmsgtype(XGameComm::MSGTYPE::MSGTYPE_RESPONSE);
        iResultID = rsp.nResultID;
        sMsgData  = pbToString(rsp);
    }

    //根据传入参数，填充返回态的基本结构
    template<typename T>
    void makeResp(int iMsgId, const T &rsp, int iResult)
    {
        msgHead.set_nmsgid(iMsgId);
        msgHead.set_nmsgtype(XGameComm::MSGTYPE::MSGTYPE_RESPONSE);
        iResultID = iResult;
        sMsgData  = pbToString(rsp);
    }

    //根据传入参数，填充请求包体的基本原始结构
    template<typename T>
    void makeReq(int iMsgId, const T &req)
    {
        vector<char> _vecBuff;
        msgHead.set_nmsgid(iMsgId);
        msgHead.set_nmsgtype(XGameComm::MSGTYPE::MSGTYPE_REQUEST);
        sMsgData = pbToString(req);
    }
};

/**
 * 用户连接对象
*/
struct ConnStruct : public TC_HandleBase
{
    ConnStruct()
    {
        tUpdateTime = TNOW;
        tLastActiveTime = tUpdateTime;
        tLastSyncTime = tUpdateTime;
        current = nullptr;
        iLoginTime = 0;
        lUserId = 0L;
        sRoomID = "";
        sHallID = "";
        iCloseType = 0;
        iActiveCount = 0;
    }

    int initRsaKeys(std::vector<string> &vStrKeys)
    {
        if(vStrKeys.size() != 2)
        {
            return -1;
        }
        sPubKey = vStrKeys[0];
        sPrivateKey = vStrKeys[1];
        return 0;
    }

    std::string getPubKey()
    {
        return sPubKey;
    }

    std::string getPrivateKey()
    {
        return sPrivateKey;
    }

    //对接连接对象
    tars::TarsCurrentPtr current;
    //更新时间，标识有消息过来
    time_t tUpdateTime;
    //上次连接激活时间
    time_t tLastActiveTime;
    //上次更新状态时间
    time_t tLastSyncTime;
    //鉴权时间
    time_t iLoginTime;
    //用户标识
    int64_t lUserId;
    //关闭类型
    int32_t iCloseType;
    //房间服务标识
    string sRoomID;
    //广场服务标识
    string sHallID;
    //rsa 公钥
    std::string sPubKey;
    //rsa 私钥
    std::string sPrivateKey; 
    //收发包次数
    int32_t iActiveCount;
    //单位时间处理的消息列表
    vector<int> vecMsgIdList;
    //用户基础信息
    UserBaseInfoExt stUser;
    //用户连接使用的线程锁，保证在处理线程和回调线程中的数据同步
    TC_ThreadMutex tMutex;
};

typedef TC_AutoPtr<ConnStruct> ConnStructPtr;

/**
 * 解包/打包
 */
class CRouterHelper
{
public:
    //
    static int Encode(const JFGamecomm::TPackage &tPkg, vector<char> &vecOutBuffer)
    {
        tobuffer(tPkg, vecOutBuffer);
        char tmp[2];
        US2N(tmp, (uint16_t)(vecOutBuffer.size() + 2));
        vecOutBuffer.insert(vecOutBuffer.begin(), tmp, tmp + 2);
        return 0;
    }
    //
    static int Encode(const XGameComm::TPackage &tPkg, string &sMsgPack)
    {
        sMsgPack = pbToString(tPkg);
        char tmp[2];
        US2N(tmp, (uint16_t)(sMsgPack.length() + 2));
        sMsgPack.insert(0, tmp, 2);
        return 0;
    }
    //
    static int Encode(const string &buf, string &sMsgPack)
    {
        sMsgPack = buf;
        char tmp[2];
        US2N(tmp, (uint16_t)(sMsgPack.length() + 2));
        sMsgPack.insert(0, tmp, 2);
        return 0;
    }
    //
    static void getClientParam(TClientParam &clientparam, const ConnStructPtr &cs)
    {
        clientparam.sAddr = cs->current->getIp();
        clientparam.nPort = cs->current->getPort();
    }
};

/**
 * 用户连接池管理类
 */
class CConnMap : public tars::MyTimeoutQueue<ConnStructPtr>
{
public:
    //默认10分钟超时
    CConnMap() : tars::MyTimeoutQueue<ConnStructPtr>(10 * 60 * 1000), _iContinueInterval(900), _iKeepAliveTime(600)
    {

    }

public:
    //获取连接标识
    int64_t getConnId(long Uin)
    {
        return Uin;
    }
    //更新连接信息，如果没有则插入
    void update(int64_t connid, const ConnStructPtr &cs)
    {
        TC_LockT<TC_ThreadMutex> lock(*this);
        time_t now = TC_TimeProvider::getInstance()->getNow();
        PtrInfo pi;
        pi.ptr = cs;
        auto result = _data.insert(make_pair(connid, pi));
        auto it = result.first;
        if (result.second == false)
        {
            _time.erase(it->second.timeIter);
            it->second.ptr = cs;
        }

        NodeInfo ni;
        ni.createTime = now;
        ni.dataIter = it;
        ni.hasPoped = false;
        _time.push_back(ni);
        auto tmp = _time.end();
        --tmp;
        it->second.timeIter = tmp;
        if (_firstNoPopIter == _time.end())
        {
            _firstNoPopIter = tmp;
        }
    }
    //更新连接信息，如果没有则插入
    void update(uint32_t Uin, const ConnStructPtr &cs)
    {
        TC_LockT<TC_ThreadMutex> lock(*this);
        int64_t connid = getConnId(Uin);
        time_t now = TC_TimeProvider::getInstance()->getNow();

        PtrInfo pi;
        pi.ptr = cs;
        auto result = _data.insert(make_pair(connid, pi));
        auto it = result.first;
        if (result.second == false)
        {
            //先删除时间链上节点
            _time.erase(it->second.timeIter);
            //修改连接信息
            it->second.ptr = cs;
        }

        NodeInfo ni;
        ni.createTime = now;
        ni.dataIter = it;
        ni.hasPoped = false;
        _time.push_back(ni);
        auto tmp = _time.end();
        --tmp;
        it->second.timeIter = tmp;
        if (_firstNoPopIter == _time.end())
        {
            _firstNoPopIter = tmp;
        }
    }
    //杀死所有连接
    void kill()
    {
        TC_LockT<TC_ThreadMutex> lock(*this);
        for (auto iter = _data.begin(); iter != _data.end(); iter++)
        {
            auto gid = iter->first;
            auto cs = iter->second.ptr;
            if (!cs || !(cs->current))
                continue;

            sendData(cs, PushProto::SERVICE_MAINTAIN, true);
            cs->current->close();
            ROLLLOG_DEBUG << "Shutdown connection: gid:" << gid <<  ", uid: " << cs->lUserId << endl;
        }
    }
    //获取当前连接信息
    ConnStructPtr getCurrent(int64_t connid)
    {
        return MyTimeoutQueue<ConnStructPtr>::get(connid, false);
    }
    //清除连接信息
    ConnStructPtr erase(int64_t connid)
    {
        return MyTimeoutQueue<ConnStructPtr>::erase(connid);
    }
    //清除连接信息
    ConnStructPtr erase2(int64_t Uin)
    {
        int64_t connid = getConnId(Uin);
        return MyTimeoutQueue<ConnStructPtr>::erase(connid);
    }
    //设置超时时间
    void setTimeOut(int timeout)
    {
        if (timeout <= 0 || timeout * 1000 <= 0)
            return;

        MyTimeoutQueue<ConnStructPtr>::setTimeout(timeout * 1000);
    }
    //设置更新连接信息间隔
    void setContinueInterval(int interval)
    {
        if (interval >= _timeout / 1000)
        {
            _iContinueInterval = _timeout / 1000 / 2;
        }
        else if (interval > 0)
        {
            _iContinueInterval = interval;
        }
    }
    //设置活跃时间间隔
    void setKeepAliveTime(int activeTime)
    {
        _iKeepAliveTime = activeTime;
    }
    //淘汰超时连接信息
    void timeout()
    {
        time_t now = TC_TimeProvider::getInstance()->getNow();
        while (true)
        {
            ConnStructPtr ptr = nullptr;
            {
                TC_LockT<TC_ThreadMutex> lock(*this);
                auto it = _time.begin();
                if (it == _time.end())
                    break;

                if ((now - it->createTime) <= (_timeout / 1000))
                    break;

                ptr = (*it->dataIter).second.ptr;
                _data.erase(it->dataIter);
                _time.erase(it);
            }

            if (ptr)
            {
                ptr->current->close();
                LOG_DEBUG << "timeout close connection, uid: " << ptr->lUserId << endl;
            }
        }
    }
    //闲时连接处理
    void idle()
    {
        TC_LockT<TC_ThreadMutex> lock(*this);

        std::vector<ConnStructPtr> v;
        auto iter = _time.begin();
        while (iter != _time.end())
        {
            auto ptr = (*iter->dataIter).second.ptr;
            auto diff = TNOW - ptr->tLastActiveTime;
            //LOG_DEBUG << "user idle..., uid: " << ptr->lUserId << ", diff: " << diff  << ", interval: " << _iKeepAliveTime << endl;
            if (diff >= _iKeepAliveTime)
            {
                if (ptr && ptr->current)
                {
                    LOG_DEBUG << "user kickout successed, uid: " << ptr->lUserId << ", diff: " << diff << endl;
                    sendData(ptr, PushProto::CONNECTION_IDLE, true);
                    v.push_back(ptr);
                }
                else
                {
                    LOG_DEBUG << "user kickout failed, diff: " << diff << endl;
                }
            }

            iter++;
        }

        if (!v.empty())
        {
            for (auto iter = v.begin(); iter != v.end(); iter++)
            {
                auto ptr = *iter;
                if (ptr && ptr->current)
                {
                    ptr->current->close();
                }
            }
            v.clear();
        }
    }
    //获取在线用户
    const std::vector<int64_t> getUsers()
    {
        std::vector<int64_t> users;
        {
            TC_LockT<TC_ThreadMutex> lock(*this);
            auto it = _time.begin();
            while (it != _time.end())
            {
                auto ptr = (*it->dataIter).second.ptr;
                users.push_back(ptr->lUserId);
                it++;
            }
        }

        return users;
    }
    //判断连接信息是否续期
    void checkTimeOut(uint32_t Uin, const ConnStructPtr &cs)
    {
        time_t now = TC_TimeProvider::getInstance()->getNow();
        if (cs && (now - cs->tUpdateTime >= _iContinueInterval))
        {
            cs->tUpdateTime = now;
            update(Uin, cs);
        }
    }
    //发送数据
    int sendData(const ConnStructPtr ptr, const int iNotifyType, const bool bReturnLoginUI)
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

        string sMsgPack = pbToString(t);
        char tmp[2];
        US2N(tmp, (uint16_t)(sMsgPack.length() + 2));
        sMsgPack.insert(0, tmp, 2);
        ptr->current->sendResponse(sMsgPack.c_str(), sMsgPack.length());
        return 0;
    }
    //连接有效检查周期
    int getKeepAliveTime()
    {
        return _iKeepAliveTime;
    }
private:
    //更新连接信息间隔
    int _iContinueInterval;
    //用户多久没有上行数据时，并且连接未断, 自动推送SPEED测速消息
    int _iKeepAliveTime;
};

#endif

