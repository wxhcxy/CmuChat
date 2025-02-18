#include "MysqlDao.h"
#include "ConfigMgr.h"

MysqlDao::MysqlDao()
{
    auto& cfg = ConfigMgr::Inst(); // 读取配置
    const auto& host = cfg["Mysql"]["Host"];
    const auto& port = cfg["Mysql"]["Port"];
    const auto& pwd = cfg["Mysql"]["Passwd"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user = cfg["Mysql"]["User"];
    pool_.reset(new MySqlPool(host + ":" + port, user, pwd, schema, 5));
    // new了一个池子,生成一个mysql连接池，给智能指针pool_管理
}

MysqlDao::~MysqlDao()
{
    pool_->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            return -1;
        }

        // 准备调用存储过程
        mysqlpp::Query query = con->_con->query();
        query << "CALL reg_user(" << mysqlpp::quote << name << ", " << mysqlpp::quote << email
              << ", " << mysqlpp::quote << pwd << ", @result)";

        // 执行存储过程
        query.execute();

        // 获取存储过程的输出参数
        mysqlpp::StoreQueryResult res = query.store();
        if (res && res.num_rows() > 0) {
            int result = res[0]["result"];
            std::cout << "Result: " << result << std::endl;
            pool_->returnConnection(std::move(con));
            return result;
        }

        pool_->returnConnection(std::move(con));
        return -1;
    } catch (mysqlpp::Exception& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "MySQL++ Exception: " << e.what() << std::endl;
        return -1;
    }
}

// 检测邮箱
bool MysqlDao::CheckEmail(const std::string& name, const std::string& email)
{
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            //pool_->returnConnection(std::move(con));
            return false;
        }

        // 准备查询语句
        mysqlpp::Query query = con->_con->query();
        query << "SELECT email FROM user WHERE name = " << mysqlpp::quote << name;

        // 执行查询
        mysqlpp::StoreQueryResult res = query.store();

        // 遍历结果集
        if (res && res.num_rows() > 0) {
            std::string db_email = res[0]["email"].c_str();
            std::cout << "Check Email: " << db_email << std::endl;
            if (email != db_email) {
                pool_->returnConnection(std::move(con));
                return false;
            }
            pool_->returnConnection(std::move(con));
            return true;
        }
    } catch (mysqlpp::Exception& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "MySQL++ Exception: " << e.what() << std::endl;
        return false;
    }
}

// 更新密码
bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd)
{
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            pool_->returnConnection(std::move(con));
            return false;
        }

        // 准备查询语句
        mysqlpp::Query query = con->_con->query();
        query << "UPDATE user SET pwd = " << mysqlpp::quote << newpwd
              << " WHERE name = " << mysqlpp::quote << name;

        // 执行更新
        query.execute();

        std::cout << "Password updated for user: " << name << std::endl;
        pool_->returnConnection(std::move(con));
        return true;
    } catch (mysqlpp::Exception& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "MySQL++ Exception: " << e.what() << std::endl;
        return false;
    }
}

// 检查密码
bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            return false;
        }

        Defer defer([this, &con]() {
            pool_->returnConnection(std::move(con));
        }); //超出作用域，析构函数会调用lambda表达式,自己定义的Defer类

        std::cout << con->_con << std::endl;
        // 准备SQL语句
        mysqlpp::Query query = con->_con->query();
        query << "SELECT * FROM user WHERE email = " << mysqlpp::quote << email;
        std::cout << "select email : " << email << std::endl;

        std::cout << "query: " << query << std::endl;

        // 执行查询
        mysqlpp::StoreQueryResult res = query.store();

        std::cout << "res: " << res << std::endl;

        // 检查结果
        if (res && res.num_rows() > 0) {
            std::string origin_pwd = res[0]["pwd"].c_str();
            std::cout << "Password origin_pwd MysqlDao::CheckPwd: " << origin_pwd << std::endl;

            if (pwd != origin_pwd) {
                std::cout << "pwd != origin_pwd MysqlDao::CheckPwd 密码错误" << std::endl;
                return false;
            }
            if (pwd == origin_pwd) {
                std::cout << "pwd == origin_pwd MysqlDao::CheckPwd 密码正确" << std::endl;
                userInfo.name = res[0]["name"].c_str();
                userInfo.email = email;
                userInfo.uid = res[0]["uid"];
                userInfo.pwd = origin_pwd;
                std::cout << "userInfo.name: " << userInfo.name << std::endl;
                std::cout << "userInfo.email: " << userInfo.email << std::endl;
                std::cout << "userInfo.uid: " << userInfo.uid << std::endl;
                std::cout << "userInfo.pwd: " << userInfo.pwd << std::endl;
                //pool_->returnConnection(std::move(con));
                return true;
            }
        }

        //pool_->returnConnection(std::move(con));
        return false;
    } catch (mysqlpp::Exception& e) {
        //pool_->returnConnection(std::move(con));
        std::cerr << "MySQL++ Exception: " << e.what() << std::endl;
        return false;
    }
}

// 测试存储过程
bool MysqlDao::TestProcedure(const std::string& email, int& uid, std::string& name)
{
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            return false;
        }

        Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });

        // 准备调用存储过程
        mysqlpp::Query query = con->_con->query();
        query << "CALL test_procedure(" << mysqlpp::quote << email << ", @userId, @userName)";

        // 执行存储过程
        query.execute();

        // 获取输出参数
        mysqlpp::StoreQueryResult res = query.store();
        if (res && res.num_rows() > 0) {
            uid = res[0]["userId"];
            name = res[0]["userName"].c_str();
            std::cout << "uid: " << uid << ", name: " << name << std::endl;
            //pool_->returnConnection(std::move(con));
            return true;
        }

        //pool_->returnConnection(std::move(con));
        return false;
    } catch (mysqlpp::Exception& e) {
        //pool_->returnConnection(std::move(con));
        std::cerr << "MySQL++ Exception: " << e.what() << std::endl;
        return false;
    }
}
