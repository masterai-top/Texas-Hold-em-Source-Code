#ifndef _FUNCTOR_H_
#define _FUNCTOR_H_

#include <sys/time.h>
#include "util/tc_thread.h"
#include "globe.h"
#include "OuterFactoryImp.h"

//
using namespace tars;

// 线程停止标记
extern volatile bool g_bStop;

/***
**
* 定时接口
**/
class Functor
{
public:
    typedef enum
    {
        eRouteStatus = 0,  //更新路由状态
        eDoOthers, //其他
    } DoType;

public:
    Functor(): iLastUpdateRouteTime(0), iClearTimeOutConnTime(0), iKeepAliveTime(0)
    {

    }
    ~Functor()
    {

    }

public:
    //重载
    void operator()(int t)
    {
        if (t == eRouteStatus)
        {
            updateRouteStatus();
        }
        else if (t == eDoOthers)
        {
            doOthers();
        }
    }

    void stop()
    {
        g_bStop = true;
    }

private:
    // 更新路由状态
    void updateRouteStatus();
    // 实时性要求不高的合在一起
    void doOthers();

private:
    // 更新路由最近时间
    time_t iLastUpdateRouteTime;
    // 清除超时连接
    time_t iClearTimeOutConnTime;
    //心跳检测时间
    time_t iKeepAliveTime;
};

#endif



