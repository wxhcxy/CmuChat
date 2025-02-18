#ifndef SINGLETON_H
#define SINGLETON_H

#include "global.h"

//单例类
template <typename T>
class Singleton{
protected:
    Singleton() = default;//不允许构造函数被外面访问，但是我希望子类去继承它，所以设置为protected
    Singleton(const Singleton<T>&) = delete;//拷贝构造
    Singleton& operator = (const Singleton<T>& st) = delete;//拷贝赋值

    static std::shared_ptr<T> _instance;//返回实例，用智能指针管理

public:
    static std::shared_ptr<T> GetInstance(){
        static std::once_flag s_flag;//这个函数如果被多次调用,s_flag也只会初始化一次
        std::call_once(s_flag, [&](){//只有第一次调用GetInstance()，然后初始化s_flag后，才会执行这个函数
            _instance = std::shared_ptr<T>(new T);
        });
        return _instance;
    }

    void printAddress(){
        std::cout<<_instance.get()<<std::endl;
    }

    ~Singleton(){
        std::cout<<"this is singleton destruct"<<std::endl;
    }

};


template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

#endif // SINGLETON_H
