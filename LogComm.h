#ifndef __LOGCOMM_H__
#define __LOGCOMM_H__

#include <util/tc_logger.h>
#include "servant/RemoteLogger.h"

//
using namespace tars;

//
#define ROLLLOG(level) (LOG->level() << "[" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "] ")
#define ROLLLOG_DEBUG  (ROLLLOG(debug))
#define ROLLLOG_INFO   (ROLLLOG(info))
#define ROLLLOG_WARN   (ROLLLOG(warn))
#define ROLLLOG_ERROR  (ROLLLOG(error))

#define FUNC_ENTRY(in) (ROLLLOG(debug) << ">>>> Enter " << __FUNCTION__ << "() in(" << in << ")" << endl)
#define FUNC_EXIT(out, ret) (ROLLLOG(debug) << "<<<< Exit " << __FUNCTION__ << "() out[" << out << "], ret = " << ret << endl)

#define FDLOG_ERROR (FDLOG("error") << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "|")
#define FDLOG_EXCEPT (FDLOG("except") << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "|")

//配置信息
#define FDLOG_CONFIG_INFO (FDLOG("config_info") << "|")

//时间间隔设置配置信息
#define FDLOG_TIME_INTERVAL_CONFIG_INFO (FDLOG("time_interval_config_info") << "|") 

//测试账号配置信息
#define FDLOG_TIME_TEST_UID_CONFIG_INFO (FDLOG("time_test_uid_config_info") << "|") 

//连接配置信息
#define FDLOG_CONNECTION_CONFIG_INFO (FDLOG("connection_config_info") << "|") 

//身份验证日志
#define FDLOG_LOGIN_INFO (FDLOG("login_info") << "|")
#define FDLOG_LOGIN_EXCEPTION_INFO (FDLOG("login_exception_info") << "|")

//黑白名单日志
#define FDLOG_CHECK_BW_LIST_INFO (FDLOG("check_bw_list_info") << "|")
#define FDLOG_CHECK_BW_LSIT_EXCEPTION_INFO (FDLOG("check_bw_list_exception_info") << "|")

//红点服务
//用户上线通知日志
#define FDLOG_ONLINE_NOTIFY_INFO (FDLOG("online_notify_info") << "|")
//用户上线通知异常日志
#define FDLOG_ONLINE_NOTIFY_EXCEPTION_INFO (FDLOG("online_notify_exception_info") << "|")
//用户下线通知日志
#define FDLOG_OFFLINE_NOTIFY_INFO (FDLOG("offline_notify_info") << "|")
//用户下线通知异常日志
#define FDLOG_OFFLINE_NOTIFY_EXCEPTION_INFO (FDLOG("offline_notify_exception_info") << "|")

//防沉迷
#define FDLOG_ASSISTANT_INFO (FDLOG("assistant") << "|")
#define FDLOG_ASSISTANT_EXCEPTION_INFO (FDLOG("assistant_exception_info") << "|")

//UserServerMap
#define FDLOG_USERSERVERMAP_ONLINE_INFO (FDLOG("userservermap_online_info") << "|")
#define FDLOG_USERSERVERMAP_ONLINE_EXCEPTION_INFO (FDLOG("userservermap_online_exception_info") << "|")

#define FDLOG_USERSERVERMAP_OFFLINE_INFO (FDLOG("userservermap_offline_info") << "|")
#define FDLOG_USERSERVERMAP_OFFLINE_EXCEPTION_INFO (FDLOG("userservermap_offline_exception_info") << "|")

#define FDLOG_USERSERVERMAP_GETROOMSTATUSOFUSER_INFO (FDLOG("userservermap_getroomstatusofuser_info") << "|")
#define FDLOG_USERSERVERMAP_GETROOMSTATUSOFUSER_EXCEPTION_INFO (FDLOG("userservermap_getroomstatusofUser_exception_info") << "|")

//
#define FDLOG_CONFIG_OFFLINE (FDLOG("offline_info") << "[" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "] ")

//
#define COST_MS 100

#endif


