#ifndef _TARS_MY_TIMEOUT_QUEUE_H_
#define _TARS_MY_TIMEOUT_QUEUE_H_

#include <deque>
#include <map>
#include <ext/hash_map>
#include <list>
#include <iostream>
#include <cassert>
#include <functional>

#include "util/tc_autoptr.h"
#include "util/tc_monitor.h"
#include "util/tc_timeprovider.h"
#include "util/tc_timeout_queue.h"

//
using namespace std;
using namespace __gnu_cxx;

//
namespace tars
{
    /////////////////////////////////////////////////
    /**
     * @file tc_timeout_queue.h
     * @brief 超时队列(模板元素只能是智能指针).
     * @author
     */
    /////////////////////////////////////////////////
    template<class T>
    class MyTimeoutQueue : public TC_ThreadMutex, public TC_HandleBase
    {
    public:
        //
        struct PtrInfo;
        //
        struct NodeInfo;
        //
        typedef hash_map<int64_t, PtrInfo> data_type;
        //
        typedef std::list<NodeInfo> time_type;
        //
        typedef std::function<void(T &)> data_functor;
        //
        struct PtrInfo
        {
            //
            T ptr;
            //
            typename time_type::iterator timeIter;
        };

        struct NodeInfo
        {
            //
            bool hasPoped;
            //
            int64_t createTime;
            //
            typename data_type::iterator dataIter;
        };

        /**
        * @brief 超时队列，缺省5s超时.
        *
        * @param timeout 超时设定时间
        * @param size
        */
        MyTimeoutQueue(int timeout = 5 * 1000, size_t size = 100): _uniqId(0), _timeout(timeout)
        {
            _firstNoPopIter = _time.end();
            _data.resize(size);
        }

        /**
        * @brief  产生该队列的下一个ID
        */
        int64_t generateId();

        /**
        * @brief 获取指定id的数据.
        *
        * @param id 指定的数据的id
        * @return T 指定id的数据
        */
        T get(int64_t uniqId, bool bErase = true);

        /**
        * @brief 删除.
        *
        * @param uniqId 要删除的数据的id
        * @return T     被删除的数据
        */
        T erase(int64_t uniqId);

        /**
        * @brief 设置消息到队列尾端.
        *
        * @param ptr        要插入到队列尾端的消息
        * @return uint32_t id号
        */
        bool push(T &ptr, int64_t uniqId);

        /**
        * @brief 超时删除数据
        */
        void timeout();

        /**
        * @brief 删除超时的数据，并用df对数据做处理
        */
        void timeout(data_functor &df);

        /**
        * @brief 取出队列头部的消息.
        *
        * @return T 队列头部的消息
        */
        T pop();

        /**
        * @brief 取出队列头部的消息(减少一次copy).
        *
        * @param t
        */
        bool pop(T &t);

        /**
        * @brief 交换数据.
        *
        * @param q
        * @return bool
        */
        bool swap(deque<T> &q);

        /**
        * @brief 设置超时时间(毫秒).
        *
        * @param timeout
        */
        void setTimeout(int timeout)
        {
            _timeout = timeout;
        }

        /**
        * @brief 队列中的数据.
        *
        * @return size_t
        */
        size_t size() const
        {
            TC_LockT<TC_ThreadMutex> lock(*this);
            return _data.size();
        }

    protected:
        //
        int64_t _uniqId;
        //
        time_t _timeout;
        //
        data_type _data;
        //
        time_type _time;
        //
        typename time_type::iterator _firstNoPopIter;
    };

    template<typename T>
    T MyTimeoutQueue<T>::get(int64_t uniqId, bool bErase)
    {
        TC_LockT<TC_ThreadMutex> lock(*this);

        auto it = _data.find(uniqId);
        if (it == _data.end())
            return NULL;

        T ptr = it->second.ptr;
        if (bErase)
        {
            if (_firstNoPopIter == it->second.timeIter)
                ++_firstNoPopIter;

            _time.erase(it->second.timeIter);
            _data.erase(it);
        }

        return ptr;
    }

    template<typename T>
    int64_t MyTimeoutQueue<T>::generateId()
    {
        TC_LockT<TC_ThreadMutex> lock(*this);
        while (++_uniqId == 0);
        return _uniqId;
    }

    template<typename T>
    bool MyTimeoutQueue<T>::push(T &ptr, int64_t uniqId)
    {
        TC_LockT<TC_ThreadMutex> lock(*this);

        PtrInfo pi;
        pi.ptr = ptr;
        auto result = _data.insert(make_pair(uniqId, pi));
        if (result.second == false)
            return false;

        auto it = result.first;
        struct timeval tv;
        TC_TimeProvider::getInstance()->getNow(&tv);

        NodeInfo ni;
        ni.createTime = tv.tv_sec * (int64_t)1000 + tv.tv_usec / 1000;
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

        return true;
    }

    template<typename T> void
    MyTimeoutQueue<T>::timeout()
    {
        struct timeval tv;
        TC_TimeProvider::getInstance()->getNow(&tv);

        while (true)
        {
            TC_LockT<TC_ThreadMutex> lock(*this);
            auto it = _time.begin();
            if (it != _time.end() && (tv.tv_sec * (int64_t)1000 + tv.tv_usec / 1000 - it->createTime > _timeout))
            {
                _data.erase(it->dataIter);
                if (_firstNoPopIter == it)
                {
                    ++_firstNoPopIter;
                }
                _time.erase(it);
            }
            else
            {
                break;
            }
        }
    }

    template<typename T>
    void MyTimeoutQueue<T>::timeout(data_functor &df)
    {
        struct timeval tv;
        TC_TimeProvider::getInstance()->getNow(&tv);

        while (true)
        {
            T ptr;
            {
                TC_LockT<TC_ThreadMutex> lock(*this);
                auto it = _time.begin();
                if (it != _time.end() && (tv.tv_sec * (int64_t)1000 + tv.tv_usec / 1000 - it->createTime > _timeout))
                {
                    ptr = (*it->dataIter).second.ptr;
                    _data.erase(it->dataIter);
                    if (_firstNoPopIter == it)
                    {
                        _firstNoPopIter++;
                    }

                    _time.erase(it);
                }
                else
                {
                    break;
                }
            }

            try
            {
                df(ptr);
            }
            catch(...)
            {
                ///
            }
        }
    }

    template<typename T>
    T MyTimeoutQueue<T>::erase(int64_t uniqId)
    {
        TC_LockT<TC_ThreadMutex> lock(*this);

        auto it = _data.find(uniqId);
        if (it == _data.end())
        {
            return NULL;
        }

        T ptr = it->second.ptr;
        if (_firstNoPopIter == it->second.timeIter)
        {
            _firstNoPopIter++;
        }

        _time.erase(it->second.timeIter);
        _data.erase(it);
        return ptr;
    }

    template<typename T>
    T MyTimeoutQueue<T>::pop()
    {
        T ptr;
        return pop(ptr) ? ptr : NULL;
    }

    template<typename T>
    bool MyTimeoutQueue<T>::pop(T &ptr)
    {
        TC_LockT<TC_ThreadMutex> lock(*this);

        if (_time.empty())
            return false;

        auto it = _time.begin();
        if (it->hasPoped == true)
            it = _firstNoPopIter;

        if (it == _time.end())
            return false;

        assert(it->hasPoped == false);

        ptr = it->dataIter->second.ptr;
        it->hasPoped = true;
        _firstNoPopIter = it;
        ++_firstNoPopIter;
        return true;
    }

    template<typename T>
    bool MyTimeoutQueue<T>::swap(deque<T> &q)
    {
        TC_LockT<TC_ThreadMutex> lock(*this);

        if (_time.empty())
        {
            return false;
        }

        auto it = _time.begin();
        while (it != _time.end())
        {
            if (it->hasPoped == true)
                it = _firstNoPopIter;

            if (it == _time.end())
                break;

            assert(it->hasPoped == false);

            T ptr = it->dataIter->second.ptr;
            it->hasPoped = true;
            _firstNoPopIter = it;
            ++_firstNoPopIter;
            q.push_back(ptr);
            ++it;
        }

        if (q.empty())
        {
            return false;
        }

        return true;
    }

    /////////////////////////////////////////////////////////////////
}

#endif


