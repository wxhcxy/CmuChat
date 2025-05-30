#pragma once

#include "const.h"

#include <thread>

#include <iostream>
#include <mariadb/conncpp.hpp>

class SqlConnection
{
public:
    SqlConnection(sql::Connection* con, int64_t lasttime)
        : _con(con)
        , _last_oper_time(lasttime)
    {}
    std::unique_ptr<sql::Connection> _con;
    int64_t _last_oper_time;
};

class MySqlPool
{
public:
    MySqlPool(const std::string& url,
              const std::string& user,
              const std::string& pass,
              const std::string& schema,
              int poolSize)
        : url_(url)
        , user_(user)
        , pass_(pass)
        , schema_(schema)
        , poolSize_(poolSize)
        , b_stop_(false)
    {
        try {
            std::cout << "MySQL 连接池大小为：" << poolSize_ << std::endl;
            for (int i = 0; i < poolSize_; ++i) {
                sql::Driver* driver = sql::mariadb::get_driver_instance();
                auto* con = driver->connect(url_, user_, pass_);
                std::cout << "MySQL 连接成功!" << std::endl;
                con->setSchema(schema_);

                // 获取当前时间戳
                auto currentTime = std::chrono::system_clock::now().time_since_epoch();
                // 将时间戳转换为秒
                long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime)
                                          .count();
                pool_.push(std::make_unique<SqlConnection>(con, timestamp));
            }

            _check_thread = std::thread([this]() {
                while (!b_stop_) {
                    checkConnection();
                    std::this_thread::sleep_for(std::chrono::seconds(60));
                }
            });

            _check_thread.detach();
        } catch (sql::SQLException& e) {
            // 处理异常
            std::cout << url_ << ", " << user_ << ", " << pass_ << ", " << schema_ << ", "
                      << poolSize_ << std::endl;
            std::cout << "mysql pool init failed, error is " << e.what() << std::endl;
        }
    }

    void checkConnection()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        int poolsize = pool_.size();
        // 获取当前时间戳
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        // 将时间戳转换为秒
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
        for (int i = 0; i < poolsize; i++) {
            auto con = std::move(pool_.front());
            pool_.pop();
            Defer defer([this, &con]() { pool_.push(std::move(con)); });
            if (timestamp - con->_last_oper_time < 5) {
                continue;
            }

            try {
                std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
                stmt->executeQuery("SELECT 1");
                con->_last_oper_time = timestamp;
                //std::cout << "execute timer alive query , cur is " << timestamp << std::endl;
            } catch (sql::SQLException& e) {
                std::cout << "Error keeping connection alive: " << e.what() << std::endl;
                // 重新创建连接并替换旧的连接
                sql::Driver* driver = sql::mariadb::get_driver_instance();
                auto* newcon = driver->connect(url_, user_, pass_);
                newcon->setSchema(schema_);
                con->_con.reset(newcon);
                con->_last_oper_time = timestamp;
            }
        }
    }

    //获取连接
    std::unique_ptr<SqlConnection> getConnection()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !pool_.empty();
        });
        if (b_stop_) {
            return nullptr;
        }
        std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
        pool_.pop();
        return con;
    }

    //归还连接
    void returnConnection(std::unique_ptr<SqlConnection> con)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        pool_.push(std::move(con));
        cond_.notify_one();
    }

    void Close()
    {
        b_stop_ = true;
        cond_.notify_all();
    }

    ~MySqlPool()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!pool_.empty()) {
            pool_.pop();
        }
    }

private:
    std::string url_;                                 //连接mysql的url
    std::string user_;                                //连接的用户名
    std::string pass_;                                //密码
    std::string schema_;                              //要使用哪个数据库
    int poolSize_;                                    //池子的大小
    std::queue<std::unique_ptr<SqlConnection>> pool_; //队列，每一个连接都要放到队列中
    std::mutex mutex_;                                //保证多线程访问队列的安全性
    std::condition_variable cond_; //队列为空时，这个变量让访问的线程挂起
    std::atomic<bool> b_stop_;     //当池子退出的时候，标记为true
    std::thread
        _check_thread; //检测线程，每隔一分钟去检测连接，发现连接有一分钟没有操作了(上一次操作时间跟
    //现在时间已经大于1分钟了)，就主动发一个简单的请求告诉mysql我还活着(心跳机制)
};

struct UserInfo
{
    std::string name;
    std::string pwd;
    int uid;
    std::string email;
};

class MysqlDao
{
public:
    MysqlDao();

    ~MysqlDao();

    int RegUser(const std::string& name, const std::string& email, const std::string& pwd);

    bool CheckEmail(const std::string& name, const std::string& email);

    bool UpdatePwd(const std::string& name, const std::string& newpwd);

    bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);

    bool TestProcedure(const std::string& email, int& uid, std::string& name);

private:
    std::unique_ptr<MySqlPool> pool_;
};
