#include "global.h"

QString gate_url_prefix = "";

//repolish是一个头文件里定义的函数对象，所以要写一个lamda表达式赋给它
std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w);//把原来的样式去掉
    w->style()->polish(w);//再刷新新的样式
};


std::function<QString(QString)> xorString = [](QString input){
    QString result = input; // 复制原始字符串，以便进行修改
    int length = input.length(); // 获取字符串的长度
    ushort xor_code = length % 255;
    for (int i = 0; i < length; ++i) {
        // 对每个字符进行异或操作
        // 注意：这里假设字符都是ASCII，因此直接转换为QChar
        result[i] = QChar(static_cast<ushort>(input[i].unicode() ^ xor_code));
    }
    return result;
};
