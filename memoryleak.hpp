
/*
 * memory leak :����ڴ�й©,��ݴ���ֻ�ǳ��������ǵ��Ե�ʱ���ڷ�debug��ʱ�򣬲�ȥ���е��ԡ�
 * ϵͳ��ʱ�����У��ڴ�й©����Դ���ľ����ᵼ��ϵͳ����https://www.bilibili.com/video/BV1ZW411y7zk?from=search&seid=1635452711301616817
 * д����ʱ����30%�����Ե�ʱ����70;
 */



 /*
  * ʹ��new  ��delete ��¼�࣬ÿnew һ�� ��¼һ�Σ�ÿdelete һ�Σ���¼һ��
  * ������ִ����ϵ�ʱ������¼��ûɾ���ľ���ʣ����ڴ�й©
  * ���ȫ�ֵĶ����ά��һ�ż�¼��������ʱ���ᱻ������һ����new��ʱ��һ����delete��ʱ��
  */

  //debug ģʽ����
#ifndef NDEBUG

#ifndef TRACER_NEW_H
#define TRACER_NEW_H

#include <map>
#include <iostream>

//h�ļ�
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

    //Ϊ����������⣬��������з�װ�ڲ��࣬ά��++��--!!! ��Ԫ��
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
        //��ȫ�ֶ������ٵ�ʱ�򣬽���Ϣ�������
        Dump();
    };
    static bool ready;
public:
    //����Ϣ���뵽����
    void Add(void* p, const char* file, long line) {
        if (m_count > 0) {
            return;
        }
        //++m_count;
        lock t_lock(*this);
        //��δ��룬���ܻ�����ݹ�,��Ϊ�ֵ����˶��󷽷�
        m_tracer_infos.emplace(std::map<void*, TracerNewInfo>::value_type(p, TracerNewInfo(file, line)));
        //--m_count;
        //m_tracer_infos[p] = TracerNewInfo(file, line);
    };

    // ɾ����Ϣ��remove����Ҳ�ᵼ��delete��Ȼ�����remove ����delete ���ö����ģ��ᵼ�£�������remove��������delete ��������ѭ����
    void Remove(void* p) {
        if (m_count > 0) {
            return;
        }
        //++m_count;
        lock t_lock(*this);
        //  ��β��Ǳ�׼��ɾ��
        auto iter = m_tracer_infos.find(p);
        if (iter != m_tracer_infos.end()) {
            m_tracer_infos.erase(iter);
        }
        // --m_count;
    };

    // �㵹��������װ���������źܸ߼���������Ҫ�Ĵ��붼�����˷�װ�о��ߴ��ϰѡ�
    void Dump() {
        //��ӡ��Ϣ������̨
        for (auto tracer_info : m_tracer_infos) {
            std::cout << "0x" << tracer_info.first << ":\t" << tracer_info.second.File() << "\tInline: " << tracer_info.second.Line() << std::endl;
        }

    }

private:
    std::map<void*, TracerNewInfo> m_tracer_infos;
    long m_count;

};
bool TracerNew::ready = false;
//cpp�ļ�
TracerNew NewTracer;

#include <cstddef>
#include <cstdlib>
//�����ʵ�ַŵ�����ļ���operator new
void* operator new (size_t size, const char* file, long line) {

    void* p = malloc(size);
    //new �ɹ�֮����Ҫ��������Ϣ�����뵽tracernew���С�
    if (TracerNew::ready) {
        NewTracer.Add(p, file, line);
    }

    return p;
};

// ջ���ܱ��Ĵ���,����ȫ�ֶ���Ĵ���������������
//void* operator new(size_t size) {
//
//    auto p = malloc(size);
//    if (TracerNew::ready) {
//        NewTracer.Add(p, "unknow", -1);
//    }
//    return p;
//}

// ȫ�����еĶ������ٶ���������delete�������������newtracer �������ˣ�Ҳ����main��������֮ʱ�����ֵ�����delete����ô����ȥdelete�ɹ���
//˭����������ȫ�ֵ�delete�ˡ�
//�������ǿ��Կ������ֲ��������ͷ�����ȫ�ֱ����;�̬����������̬����֮�䣬ȫ�ֱ���֮����Լ���̬������ȫ�ֱ���֮����ڴ��ͷ�˳����Ȼ��ѭ���������ͷŵĴ���
// ���newtracer ���������ˣ��㻹removeʲô
void operator delete(void* p) {

    //  �����ж�λ�ǶԱ�ȫ�ֶ���ı����ж�
    if (TracerNew::ready) {
        //remove���pָ��
        NewTracer.Remove(p);
    }

    //�ͷ�pָ��
    free(p);
}

#endif //!TRACER_NEW_H



/*
 * ��׼д��,�µ�ͷ�ļ�
 */

#ifndef DEBUG_NEW_H
#define DEBUG_NEW_H

 //#include <cstddef>
 //#inlcude "tracernew.h"
  //ȫ�ֵ�new������,����ļ�ֻ�������궨��
 //void* operator new (size_t size, const char* file, long line);

 //C++ new ���滻
#define new new(__FILE__,__LINE__)


#endif //!DEBUG_NEW_H

//#include debugnew.h

#endif //!NDEBUG

//���Ժ�����ֱ��ִ��
int main() {

    int* p = new int;
    // �ǳ�Σ�յ�delete �����ܻ�ݹ�
    //delete p;
    return 0;

}