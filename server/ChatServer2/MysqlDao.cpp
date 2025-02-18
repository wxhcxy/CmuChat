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
            pool_->returnConnection(std::move(con));
            return db_email == email;
        }

        pool_->returnConnection(std::move(con));
        return false;
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
            //pool_->returnConnection(std::move(con));
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
bool MysqlDao::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo)
{
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            return false;
        }

        Defer defer([this, &con]() {
            pool_->returnConnection(std::move(con));
        }); //超出作用域，析构函数会调用lambda表达式,自己定义的Defer类

        // 准备SQL语句
        mysqlpp::Query query = con->_con->query();
        query << "SELECT * FROM user WHERE name = " << mysqlpp::quote << name;

        // 执行查询
        mysqlpp::StoreQueryResult res = query.store();

        // 检查结果
        if (res && res.num_rows() > 0) {
            std::string origin_pwd = res[0]["pwd"].c_str();
            std::cout << "Password: " << origin_pwd << std::endl;

            if (pwd != origin_pwd) {
                return false;
            }
            if (pwd == origin_pwd) {
                userInfo.email = res[0]["email"].c_str();
                userInfo.name = name;
                userInfo.uid = res[0]["uid"];
                userInfo.pwd = origin_pwd;
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

std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return nullptr;
    }

    Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });

    try {
        // 准备SQL语句
        mysqlpp::Query query = con->_con->query();
        query << "SELECT * FROM user WHERE uid = " << mysqlpp::quote << uid;

        // 执行查询
        mysqlpp::StoreQueryResult res = query.store();

        std::shared_ptr<UserInfo> user_ptr = nullptr;
        if (res && res.num_rows() > 0) {
            user_ptr = std::make_shared<UserInfo>();
            user_ptr->pwd = std::string(res[0]["pwd"].c_str());
            user_ptr->email = std::string(res[0]["email"].c_str());
            user_ptr->name = std::string(res[0]["name"].c_str());
            user_ptr->nick = std::string(res[0]["nick"].c_str());
            user_ptr->desc = std::string(res[0]["desc"].c_str());
            user_ptr->sex = res[0]["sex"];
            user_ptr->icon = std::string(res[0]["icon"].c_str());
            user_ptr->uid = uid;
        }
        return user_ptr;
    } catch (mysqlpp::Exception& e) {
        std::cerr << "MySQL++ Exception: " << e.what() << std::endl;
        return nullptr;
    }
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(std::string name)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return nullptr;
    }

    Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });

    try {
        // 准备SQL语句
        mysqlpp::Query query = con->_con->query();
        query << "SELECT * FROM user WHERE name = " << mysqlpp::quote << name;

        // 执行查询
        mysqlpp::StoreQueryResult res = query.store();

        std::shared_ptr<UserInfo> user_ptr = nullptr;
        if (res && res.num_rows() > 0) {
            user_ptr = std::make_shared<UserInfo>();
            user_ptr->pwd = std::string(res[0]["pwd"].c_str());
            user_ptr->email = std::string(res[0]["email"].c_str());
            user_ptr->name = std::string(res[0]["name"].c_str());
            user_ptr->nick = std::string(res[0]["nick"].c_str());
            user_ptr->desc = std::string(res[0]["desc"].c_str());
            user_ptr->sex = res[0]["sex"];
            user_ptr->uid = res[0]["uid"];
        }
        return user_ptr;
    } catch (mysqlpp::Exception& e) {
        std::cerr << "MySQL++ Exception: " << e.what() << std::endl;
        return nullptr;
    }
}

bool MysqlDao::AddFriendApply(const int& from, const int& to)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });

    try {
        // 准备SQL语句
        mysqlpp::Query query = con->_con->query();
        query << "INSERT INTO friend_apply (from_uid, to_uid) VALUES (" << from << ", " << to
              << ") "
              << "ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid";

        // 执行更新
        query.execute();
        return true;
    } catch (mysqlpp::Exception& e) {
        std::cerr << "MySQL++ Exception: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlDao::GetApplyList(int touid,
                            std::vector<std::shared_ptr<ApplyInfo>>& applyList,
                            int begin,
                            int limit)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });

    try {
        // 准备SQL语句
        mysqlpp::Query query = con->_con->query();
        query << "SELECT apply.from_uid, apply.status, user.name, user.nick, user.sex "
              << "FROM friend_apply AS apply JOIN user ON apply.from_uid = user.uid "
              << "WHERE apply.to_uid = " << touid << " AND apply.id > " << begin << " "
              << "ORDER BY apply.id ASC LIMIT " << limit;

        // 执行查询
        mysqlpp::StoreQueryResult res = query.store();

        // 遍历结果集
        for (const auto& row : res) {
            std::string name = std::string(row["name"].c_str());
            int uid = row["from_uid"];
            int status = row["status"];
            std::string nick = std::string(row["nick"].c_str());
            int sex = row["sex"];
            auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
            applyList.push_back(apply_ptr);
        }
        return true;
    } catch (mysqlpp::Exception& e) {
        std::cerr << "MySQL++ Exception: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlDao::AuthFriendApply(const int& from, const int& to)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });

    try {
        // 准备SQL语句
        mysqlpp::Query query = con->_con->query();
        query << "UPDATE friend_apply SET status = 1 WHERE from_uid = " << to
              << " AND to_uid = " << from;

        // 执行更新
        query.execute();
        return true;
    } catch (mysqlpp::Exception& e) {
        std::cerr << "MySQL++ Exception: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlDao::AddFriend(const int& from, const int& to, std::string back_name)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });

    try {
        // 开始事务
        con->_con->query("START TRANSACTION");

        // 插入认证方好友数据
        mysqlpp::Query query1 = con->_con->query();
        query1 << "INSERT IGNORE INTO friend (self_id, friend_id, back) VALUES (" << from << ", "
               << to << ", " << mysqlpp::quote << back_name << ")";
        query1.execute();

        // 插入申请方好友数据
        mysqlpp::Query query2 = con->_con->query();
        query2 << "INSERT IGNORE INTO friend (self_id, friend_id, back) VALUES (" << to << ", "
               << from << ", '')";
        query2.execute();

        // 提交事务
        con->_con->query("COMMIT");
        return true;
    } catch (mysqlpp::Exception& e) {
        // 回滚事务
        con->_con->query("ROLLBACK");
        std::cerr << "MySQL++ Exception: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlDao::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info_list)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });

    try {
        // 准备SQL语句
        mysqlpp::Query query = con->_con->query();
        query << "SELECT * FROM friend WHERE self_id = " << self_id;

        // 执行查询
        mysqlpp::StoreQueryResult res = query.store();

        // 遍历结果集
        for (const auto& row : res) {
            int friend_id = row["friend_id"];
            std::string back = std::string(row["back"].c_str());

            // 查询好友信息
            auto user_info = GetUser(friend_id);
            if (user_info) {
                user_info->back = user_info->name;
                user_info_list.push_back(user_info);
            }
        }
        return true;
    } catch (mysqlpp::Exception& e) {
        std::cerr << "MySQL++ Exception: " << e.what() << std::endl;
        return false;
    }
}
