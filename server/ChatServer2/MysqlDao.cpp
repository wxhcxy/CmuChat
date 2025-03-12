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

        std::unique_ptr<sql::PreparedStatement> stmt(
            con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));

        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, pwd);

        stmt->execute();

        std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
        if (res->next()) {
            int result = res->getInt("result");
            std::cout << "Result: " << result << std::endl;
            pool_->returnConnection(std::move(con));
            return result;
        }
        pool_->returnConnection(std::move(con));
        return -1;
    } catch (sql::SQLException& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return -1;
    }
}

// 检测邮箱
bool MysqlDao::CheckEmail(const std::string& name, const std::string& email)
{
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));

        pstmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next()) {
            std::cout << "Check Email: " << res->getString("email") << std::endl;
            if (email != res->getString("email")) {
                pool_->returnConnection(std::move(con));
                return false;
            }
            pool_->returnConnection(std::move(con));
            return true;
        }
        return true;
    } catch (sql::SQLException& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

// 更新密码
bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd)
{
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

        pstmt->setString(2, name);
        pstmt->setString(1, newpwd);

        int updateCount = pstmt->executeUpdate();

        std::cout << "Updated rows: " << updateCount << std::endl;
        pool_->returnConnection(std::move(con));
        return true;
    } catch (sql::SQLException& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

// 检查密码
bool MysqlDao::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo)
{
    auto con = pool_->getConnection();

    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    }); //超出作用域，析构函数会调用lambda表达式,自己定义的Defer类

    try {
        // 准备SQL语句
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::string origin_pwd = "";

        while (res->next()) {
            origin_pwd = res->getString("pwd");

            std::cout << "Password: " << origin_pwd << std::endl;
            break;
        }

        if (pwd != origin_pwd) {
            return false;
        }
        userInfo.name = name;
        userInfo.email = res->getString("email");
        userInfo.uid = res->getInt("uid");
        userInfo.pwd = origin_pwd;
        return true;
    } catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
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
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
        pstmt->setInt(1, uid);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;

        while (res->next()) {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->nick = res->getString("nick");
            user_ptr->desc = res->getString("desc");
            user_ptr->sex = res->getInt("sex");
            user_ptr->icon = res->getString("icon");
            user_ptr->uid = uid;
            break;
        }
        return user_ptr;
    } catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
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
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;

        while (res->next()) {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->nick = res->getString("nick");
            user_ptr->desc = res->getString("desc");
            user_ptr->sex = res->getInt("sex");
            user_ptr->uid = res->getInt("uid");
            break;
        }
        return user_ptr;
    } catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
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
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "INSERT INTO friend_apply (from_uid, to_uid) values (?,?) "
            "ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid"));
        pstmt->setInt(1, from); // from id
        pstmt->setInt(2, to);

        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0) {
            return false;
        }
        return true;
    } catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
    return true;
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
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("select apply.from_uid, apply.status, user.name, "
                                        "user.nick, user.sex from friend_apply as apply join user "
                                        "on apply.from_uid = user.uid where apply.to_uid = ? "
                                        "and apply.id > ? order by apply.id ASC LIMIT ? "));

        pstmt->setInt(1, touid);
        pstmt->setInt(2, begin);
        pstmt->setInt(3, limit);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next()) {
            std::string name = res->getString("name").c_str();
            int uid = res->getInt("from_uid");
            int status = res->getInt("status");
            std::string nick = res->getString("nick").c_str();
            int sex = res->getInt("sex");
            auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
            applyList.push_back(apply_ptr);
        }
        return true;
    } catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
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
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("UPDATE friend_apply SET status = 1 "
                                        "WHERE from_uid = ? AND to_uid = ?"));

        pstmt->setInt(1, to); // from id
        pstmt->setInt(2, from);

        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0) {
            return false;
        }
        return true;
    } catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }

    return true;
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
        con->_con->setAutoCommit(false);

        // 插入认证方好友数据
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
                                        "VALUES (?, ?, ?) "));
        pstmt->setInt(1, from); // from id
        pstmt->setInt(2, to);
        pstmt->setString(3, back_name);

        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0) {
            con->_con->rollback();
            return false;
        }

        // 插入申请方好友数据
        std::unique_ptr<sql::PreparedStatement> pstmt2(
            con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
                                        "VALUES (?, ?, ?) "));

        pstmt2->setInt(1, to); // from id
        pstmt2->setInt(2, from);
        pstmt2->setString(3, "");

        int rowAffected2 = pstmt2->executeUpdate();
        if (rowAffected2 < 0) {
            con->_con->rollback();
            return false;
        }

        // 提交事务
        con->_con->commit();
        std::cout << "addfriend insert friends success" << std::endl;

        return true;
    } catch (sql::SQLException& e) {
        // 回滚事务
        if (con) {
            con->_con->rollback();
        }
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }

    return true;
}

bool MysqlDao::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info_list)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("select * from friend where self_id = ? "));

        pstmt->setInt(1, self_id);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next()) {
            auto friend_id = res->getInt("friend_id");
            auto back = res->getString("back");

            auto user_info = GetUser(friend_id);
            if (user_info == nullptr) {
                continue;
            }

            user_info->back = user_info->name;
            user_info_list.push_back(user_info);
        }
        return true;
    } catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }

    return true;
}
