#pragma once
#include <stdexcept>
#include <QString>
#include <QByteArray>
// ============================================================
// 1. 统一异常基类 —— 自动带 文件名 + 行号 + 函数名 + 自定义消息
// ============================================================
class BaseException: public std::runtime_error
{
public:
	explicit BaseException(const QString& message,
		const char* file = __FILE__,
		int         line = __LINE__
		)
		: std::runtime_error("")
		, fullMessage(QString("%1:%2  \n%4")
			.arg(file)                     // 文件名
			.arg(line)                     // 行号
			.arg(message))                 // 你自己的错误描述
	{}

	// 标准 what() —— 给 qCritical()、spdlog、printf 等用
	const char* what() const noexcept override
	{
		cachedUtf8 = fullMessage.toLocal8Bit();          // 只转一次
		return cachedUtf8.constData();
	}

	// Qt 专用 —— 直接返回带行列信息的 QString，最方便弹窗/日志
	QString qwhat() const { return fullMessage; }

private:
	QString fullMessage;
	mutable QByteArray cachedUtf8;   // what() 可能被多次调用，缓存一下
};

// ============================================================
// 2. 常用异常类型（按需增删，直接继承就行）
// ============================================================
class LogicError : public BaseException { public: using BaseException::BaseException; };
class InvalidArgument : public BaseException { public: using BaseException::BaseException; };
class RuntimeError : public BaseException { public: using BaseException::BaseException; };
class FileError : public BaseException { public: using BaseException::BaseException; };
class NetworkError : public BaseException { public: using BaseException::BaseException; };
class ParseError : public BaseException { public: using BaseException::BaseException; };
class DatabaseError : public BaseException { public: using BaseException::BaseException; };
class NotImplemented : public BaseException { public: using BaseException::BaseException; };

// ============================================================
// 3. 超级好用的抛异常宏（可选，但强烈推荐）
// ============================================================
#define THROW_INVALID(msg)     throw InvalidArgument(msg, __FILE__, __LINE__)
#define THROW_RUNTIME(msg)     throw RuntimeError   (msg, __FILE__, __LINE__)
#define THROW_FILE(msg)        throw FileError      (msg, __FILE__, __LINE__)
#define THROW_NETWORK(msg)     throw NetworkError   (msg, __FILE__, __LINE__)
#define THROW_PARSE(msg)       throw ParseError     (msg, __FILE__, __LINE__)
#define THROW_NOT_IMP() throw NotImplemented("功能尚未实现", __FILE__, __LINE__)

//万能宏
#define THROW_EX(msg) throw RuntimeError(msg,__FILE__,__LINE__);