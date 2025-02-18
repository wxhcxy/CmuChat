#pragma once

#include "const.h"
#include "MysqlDao.h"

class MysqlMgr : public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string& email);	//检验邮箱
	bool UpdatePwd(const std::string& name, const std::string& email);	//更新密码
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	bool TestProcedure(const std::string& email, int& uid, std::string& name);
private:
	MysqlMgr();
	MysqlDao  _dao;	//操作mysql这一层
};
