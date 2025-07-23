#ifndef _RouterServantImp_H_
#define _RouterServantImp_H_

//
#include "servant/Application.h"
#include "servant/ServantProxy.h"
#include "servant/Servant.h"
#include "OuterFactoryImp.h"

/**
 **业务处理类
 *
 */
class RouterServantImp : public Servant
{
public:
    /**
     * 对象初始化
     */
    virtual void initialize();

    /**
     * 对象销毁
     */
    virtual void destroy();

    /**
     * 处理客户端的主动请求
     * @param current
     * @param response
     * @return int
     */
    virtual int doRequest(tars::TarsCurrentPtr current, vector<char> &response);

    /**
     * @param resp
     * @return int
     */
    virtual int doResponse(ReqMessagePtr resp);

    /**
     * @param resp
     * @return int
     */
    virtual int doResponseException(ReqMessagePtr resp);

    /**
     * @param resp
     * @return ints
     */
    virtual int doResponseNoRequest(ReqMessagePtr resp);

    /**
     * @param resp
     * @return ints
     */
    virtual int doClose(tars::TarsCurrentPtr current);

private:
    /**
     * 服务请求处理
     * @param  t  [description]
     * @param  cs [description]
     * @return    [description]
     */
    int onService(const XGameComm::TPackage &t, const ConnStructPtr &cs);
};
/////////////////////////////////////////////////////
#endif


