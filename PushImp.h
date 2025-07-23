#ifndef _PushImp_H_
#define _PushImp_H_

#include "servant/Application.h"
#include "Push.h"

/**
 *
 * 推送服务接口
 */
class PushImp : public JFGame::Push
{
public:
    /**
     *
     */
    virtual ~PushImp() {}

    /**
     *
     */
    virtual void initialize();

    /**
     *
     */
    virtual void destroy();

public:
    //推送pb协议数据给客户端
    virtual tars::Int32 doPushBuf(tars::Int64 uin, const std::string &buf, tars::TarsCurrentPtr current);
    //推送pb协议数据给客户端
    virtual tars::Int32 doPushUserState(tars::Int64 uin, tars::Int64 cid, tars::Int32 state, const std::string &addr, const std::string &buf, tars::TarsCurrentPtr current);
    //推送pb协议数据给客户端(roomid相关)
    virtual tars::Int32 doPushBufByRoomID(tars::Int64 uin, const std::string &buf, const std::string &sRoomID, int msgID, tars::TarsCurrentPtr current);
};
/////////////////////////////////////////////////////

#endif


