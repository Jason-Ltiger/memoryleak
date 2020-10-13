
/*
 * memory leak :解决内存泄漏,这份代码只是出现在我们调试的时候，在非debug的时候，不去进行调试。
 * 系统长时间运行，内存泄漏，资源被耗尽，会导致系统崩溃https://www.bilibili.com/video/BV1ZW411y7zk?from=search&seid=1635452711301616817
 * 写代码时间是30%，调试的时间是70;
 */



 /*
  * 使用new  、delete 记录类，每new 一次 记录一次，每delete 一次，记录一次
  * 当程序执行完毕的时候，所记录的没删除的就是剩余的内存泄漏
  * 这个全局的对象会维护一张记录表，在两个时机会被触发，一个是new的时候；一个是delete的时候
  */

  //debug 模式问题
#ifndef NDEBUG

#ifndef TRACER_NEW_H
#define TRACER_NEW_H

#include <map>
#include <iostream>

//h文件
class TracerNew
{
    class TracerNewInfo
    {
    public:
        TracerNewInfo(const char* file, long line) :file_(file), line_(line) {};
        ~TracerNewInfo() {};
    public:
        const char* File() const { return file_; };
        long Line() const { return line_; };
    private:
        const char* file_;
        long line_;
    };

    //为解决死锁问题，在这里进行封装内部类，维护++与--!!! 友元类
    class lock
    {
    public:
        lock(TracerNew& tracer) : m_tracer(tracer) { ++m_tracer.m_count; };
        ~lock() { --m_tracer.m_count; };
    private:
        TracerNew& m_tracer;
    };

public:
    TracerNew() { TracerNew::ready = true; m_count = 0; };
    ~TracerNew() {
        TracerNew::ready = false;
        //在全局对象销毁的时候，将信息进行输出
        Dump();
    };
    static bool ready;
public:
    //将信息存入到表当中
    void Add(void* p, const char* file, long line) {
        if (m_count > 0) {
            return;
        }
        //++m_count;
        lock t_lock(*this);
        //这段代码，可能会产生递归,因为又调用了对象方法
        m_tracer_infos.emplace(std::map<void*, TracerNewInfo>::value_type(p, TracerNewInfo(file, line)));
        //--m_count;
        //m_tracer_infos[p] = TracerNewInfo(file, line);
    };

    // 删除信息，remove可能也会导致delete，然而你的remove 是由delete 调用而来的，会导致，反复的remove，反复的delete 产生无限循环。
    void Remove(void* p) {
        if (m_count > 0) {
            return;
        }
        //++m_count;
        lock t_lock(*this);
        //  如何才是标准的删除
        auto iter = m_tracer_infos.find(p);
        if (iter != m_tracer_infos.end()) {
            m_tracer_infos.erase(iter);
        }
        // --m_count;
    };

    // 倾倒垃圾，封装函数，看着很高级，把所需要的代码都进行了封装感觉高大上把。
    void Dump() {
        //打印信息到控制台
        for (auto tracer_info : m_tracer_infos) {
            std::cout << "0x" << tracer_info.first << ":\t" << tracer_info.second.File() << "\tInline: " << tracer_info.second.Line() << std::endl;
        }

    }

private:
    std::map<void*, TracerNewInfo> m_tracer_infos;
    long m_count;

};
bool TracerNew::ready = false;
//cpp文件
TracerNew NewTracer;

#include <cstddef>
#include <cstdlib>
//将这个实现放到这个文件内operator new
void* operator new (size_t size, const char* file, long line) {

    void* p = malloc(size);
    //new 成功之后，需要将此条信息，放入到tracernew当中。
    if (TracerNew::ready) {
        NewTracer.Add(p, file, line);
    }

    return p;
};

// 栈可能爆的代码,这里全局对象的创建会调用这个方法
//void* operator new(size_t size) {
//
//    auto p = malloc(size);
//    if (TracerNew::ready) {
//        NewTracer.Add(p, "unknow", -1);
//    }
//    return p;
//}

// 全局所有的对象销毁都会调用这个delete函数，假如你的newtracer 都销毁了，也就是main函数结束之时，你又调用了delete你怎么还能去delete成功呢
//谁让你重载了全局的delete了。
//从中我们可以看到，局部变量的释放先于全局变量和静态变量，而静态变量之间，全局变量之间的以及静态变量与全局变量之间的内存释放顺序依然遵循后声明先释放的次序
// 你的newtracer 都被销毁了，你还remove什么
void operator delete(void* p) {

    //  加了判断位是对本全局对象的保护判断
    if (TracerNew::ready) {
        //remove你的p指针
        NewTracer.Remove(p);
    }

    //释放p指针
    free(p);
}

#endif //!TRACER_NEW_H



/*
 * 标准写法,新的头文件
 */

#ifndef DEBUG_NEW_H
#define DEBUG_NEW_H

 //#include <cstddef>
 //#inlcude "tracernew.h"
  //全局的new的重载,这个文件只是用来宏定义
 //void* operator new (size_t size, const char* file, long line);

 //C++ new 宏替换
#define new new(__FILE__,__LINE__)


#endif //!DEBUG_NEW_H

//#include debugnew.h

#endif //!NDEBUG

//测试函数，直接执行
int main() {

    int* p = new int;
    // 非常危险的delete ，可能会递归
    //delete p;
    return 0;

}