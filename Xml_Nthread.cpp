#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <time.h>
#include <functional>
#include <pthread.h>
#include <iostream>
#include <deque>
#include <map>
#include <stack>
#include <vector>
#include <list>
using namespace std;

string getTime()
{
    time_t timep;
    time (&timep);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S",localtime(&timep) );
    return tmp;
}

const int BLOCK_SIZE = 1 << 20;

// 禁止赋值构造与拷贝构造
class Noncopyable{
protected:
    Noncopyable(){ }
    ~Noncopyable(){ }
private:
    Noncopyable(const Noncopyable &);
    Noncopyable & operator=(const Noncopyable &);
};

// 锁
class MutexLock{
public:
    MutexLock()
    :_islocking(false)
    {
        pthread_mutex_init(&_mutex,NULL);
    }
    
    ~MutexLock()
    {    pthread_mutex_destroy(&_mutex);    }
    
    void lock()
    {
        pthread_mutex_lock(&_mutex);
        _islocking=true;
    }
    
    void unlock()
    {
        pthread_mutex_unlock(&_mutex);
        _islocking=false;
    }
    
    bool status()const
    {    return _islocking;    }
    
    pthread_mutex_t * getMutexLockPtr()
    {    return & _mutex;    }
    
private:
    MutexLock(const MutexLock&);
    MutexLock& operator=(const MutexLock&);
private:
    bool _islocking;
    pthread_mutex_t _mutex;
};

// 智能管理锁
class MutexLockGuard{
public:
    MutexLockGuard(MutexLock &mutex)
    :_mutex(mutex)
    {
        _mutex.lock();
    }
    
    ~MutexLockGuard()
    {
        _mutex.unlock();
    }
private:
    MutexLock &_mutex;
};


// 线程类
class Thread:private Noncopyable{
	typedef function<void()> ThreadCallback;
public:
	Thread(ThreadCallback cb)
	:_pthId(0)
	,_isRunning(false)
	,_cb(cb)
	{ }
	
	~Thread()
	{
		cout<<"Thread::~Thread()"<<endl;
		if(_isRunning){
			pthread_detach(_pthId);//资源回收交给主线程进行托管
			_isRunning=false;
		}
	}
	
	void start()
	{
		pthread_create(&_pthId,NULL,threadFunc,this);
		_isRunning = true;
	}
	
	void join()
	{
		if(_isRunning){
			pthread_join(_pthId,NULL);
			_isRunning = false;
		}
	}
	
	static void *threadFunc(void *arg)
	{
		Thread* p=static_cast<Thread*>(arg);
        cout << (bool)p;
		if(p){
            cout << "_cb";
			p->_cb();
        
        }
		return NULL;
	}	
	
	pthread_t getId()
	{
		return pthread_self();
	}
private:
	pthread_t _pthId;
	bool _isRunning;
	ThreadCallback _cb;
};


enum START_TYPE
{
    StagorEmptytag_start = 1,
    Etag_start,
    PI_start,
    CDSECT_start,
    COMMENT_start,
    INVALID              = -1
};

struct START_INFO
{
    START_TYPE stype;
    int fileoffset;
    int blocknum;
    int blockpos;
    int endpos;
    string attr;
    
    //拷贝构造函数
    START_INFO(const START_INFO & c)
    {
        stype=c.stype;
        fileoffset = c.fileoffset;
        blocknum = c.blocknum;
        blockpos = c.blockpos;
        endpos = c.endpos;
        attr = c.attr;        
    }
    
    START_INFO(const START_TYPE type, const int offset = 0, const int bnum = 0, const int bpos = 0, const int end = 0)
    :stype(type)
    ,fileoffset(offset)
    ,blocknum(bnum)
    ,blockpos(bpos)
    ,endpos(end)
    ,attr("")
    { }
};

struct BLOCK_INFO{
    bool stage1_start;
    bool stage1_finish;
    bool stage2_start;
    bool stage2_finish;
    bool stage3_start;
    bool stage3_finish;
    deque<START_INFO> info_list;
    
    BLOCK_INFO(bool s1 = false, bool f1 = false, bool s2 = false, bool f2 = false, bool s3 = false, bool f3 = false)
    :stage1_start(s1)
    ,stage1_finish(f1)
    ,stage2_start(s2)
    ,stage2_finish(f2)
    ,stage3_start(s3)
    ,stage3_finish(f3)
    {}
    
    ~BLOCK_INFO()
    {
        info_list.clear();
    }
    
    void clear()
    {
        stage1_start = false;
        stage1_finish = false;
        stage2_start = false;
        stage2_finish = false;
        stage3_start = false;
        stage3_finish = false;
        info_list.clear();
    }    
};


class CXml: public enable_shared_from_this<CXml>{
public:
    CXml(string file, int th = 2, int blockcap = 1 << 13) // 每块容量限制4096 Bytes
    :sfile(file)
    ,th_num(th)
    ,block_cap(blockcap)
    { }

    void start()
    {
        shared_ptr<Thread> sp(new Thread(bind(&CXml::buildData, this)));
        threads.push_back(sp);
        sp->start();
        for(int i = 0; i < th_num - 1; ++i){
            shared_ptr<Thread> th2(new Thread(bind(&CXml::checkValid, this)));
            threads.push_back(sp);
            th2->start();
        }
    }
    
    void buildData()
    {
        cout << "========================== [ BEGIN  ] ==========================" << endl;
        cout << "########################## [ step 1 ] ##########################" << endl;
        cout << "\t\t\t" << getTime() << endl;
        
        FILE *fpr;
        if((fpr=fopen(sfile.c_str(), "rb+")) == NULL)
        { 
            cout << "open " << sfile << " error!" << endl; 
            exit(-1);
        }
        
        while(1)
        {
            BLOCK_INFO oneblock(true, false, false, false, false, false);
            char ch = '\0';
            while((ch = fgetc(fpr)) != EOF)
            {
                if('>' == ch) // 形如 <!--- <test pattern="SECAM" /><test pattern="NTSC" /> --> 需防止被覆盖
                {
                    int end = ftell(fpr) - 1;
                    fseek(fpr, -3, SEEK_CUR);
                    string stmp(3, 0);
                    fread((void*)&stmp.c_str()[0], sizeof(char), 3, fpr);
                    ch = fgetc(fpr);
                    if(' ' == stmp[1] || (ch >= '0' && ch <= '9')) //内容本身的 >
                        continue;
                    
                    if(stmp == "]]>" || stmp == "-->")
                    {
                        if(commentAndCdata.empty() || (stmp == "]]>" && commentAndCdata.top().stype != CDSECT_start) || (stmp == "-->" && commentAndCdata.top().stype != COMMENT_start))
                        {
                            cout << "Error===> " << stmp << " in " << end << " invalid match CDSECT_start or CDSECT_start" << endl;
                            exit(-1);
                        }
                        
                        commentAndCdata.pop();
                        continue;
                    }

                    if(oneblock.info_list.empty() || (stmp.substr(1) == "?>" && oneblock.info_list.back().stype != PI_start) || (stmp[1] == '/' && oneblock.info_list.back().stype != StagorEmptytag_start))
                    {
                        cout << "Error===> '>' " << " in " << end << " without '<'" << endl;
                        exit(-1);
                    }
                    
                    if(stmp.substr(1) != "?>" && stmp[1] != '/')
                    {
                        bool invalid = false;
                        if(oneblock.info_list.back().stype != StagorEmptytag_start && oneblock.info_list.back().stype != Etag_start){
                            invalid = true;
                        }else{
                            if(0 != oneblock.info_list.back().endpos)
                                invalid = true;
                        }
                        if(invalid){
                            cout << "Error===> '>' " << " in " << end << " without '<'" << endl;
                            exit(-1);
                        }
                    }
                    if(0 != oneblock.info_list.back().endpos){
                        cout << "Error===> '>' " << " in " << end << " without '<'" << endl;
                        exit(-1);
                    }
                        
                    oneblock.info_list.back().endpos = end;
                    cout << oneblock.info_list.back().fileoffset << "-" << oneblock.info_list.back().endpos<< " ";
                }

                if('<' == ch)
                {
                    int offset = ftell(fpr) - 1;
                    if(offset >= block_cap)
                    {
                        oneblock.stage1_finish = true;
                        ldata.push_back(oneblock);
                        oneblock.clear();
                    }
                    
                    if(' ' == (ch = fgetc(fpr)) || (ch >= '0' && ch <= '9'))    //内容本身的 < 
                        continue;
                    
                    START_INFO tmp(START_TYPE::INVALID);
                    tmp.fileoffset = offset;
                    tmp.blocknum = 0;
                    tmp.blockpos = offset;
                    bool s_comment = false;
                    switch(ch)
                    {
                        case '/':
                            tmp.stype = Etag_start;
                            break;
                        case '?':
                            tmp.stype = PI_start;
                            break;
                        case '!':
                        {
                            string stmp(8, 0);
                            int ret = fread((void*)&stmp.c_str()[0], sizeof(char), 8, fpr);
                            if("--" == stmp.substr(0, 2))
                            {
                                tmp.stype = COMMENT_start;
                            }else if("[CDATA[" == stmp.substr(0, 7)){
                                tmp.stype = CDSECT_start;
                            }else{
                                cout << "Error===> Invalid string:" << stmp << " in pos:" << offset << endl;
                                exit(-1);
                            }
                            s_comment = true;
                            
                            commentAndCdata.push(tmp);
                            break;
                        }
                        default:
                            tmp.stype = StagorEmptytag_start;
                            break;
                    }
                    if(s_comment)
                        continue;
                    oneblock.info_list.push_back(tmp);
                }
            }
                //cout << "444";

            if(!commentAndCdata.empty())
            {
                cout << "Error===> CDSECT_start or CDSECT_start in " << commentAndCdata.top().fileoffset << " invalid match" << endl;
                exit(-1);
            }
        }
		cout << "555";

        cout << "\t\t\t" << getTime() << endl;
        cout << "########################## [ step 1 ] ##########################" << endl;
        fclose(fpr);
    }

    bool checkValid()
    {
        cout << "########################## [ step 2 ] ##########################" << endl;
        string step2_start = getTime();
		
		FILE *fpr;
		if((fpr=fopen(sfile.c_str(), "r+")) == NULL)
		{ 
			cout << "Error===> open " << sfile << "error!" << endl; 
			exit(-1);
		}
        stack<START_INFO> st;

        while(1)
        {
#if 1
			int pos = -1;
			{
				MutexLockGuard(MutexLock());
				for(int i = 0; i < ldata.size(); ++i)
				{
					if(ldata[i].stage1_finish && !ldata[i].stage2_start){
						pos = i;
						ldata[pos].stage2_start = true;
					}
				}
			}

			if(-1 == pos){
				sleep(1);
				continue;
			}
				
            for(int i = 0; i < ldata[pos].info_list.size(); ++i)
            {
                if(CDSECT_start != ldata[pos].info_list[i].stype && COMMENT_start != ldata[pos].info_list[i].stype && 0 == ldata[pos].info_list[i].endpos)    // 形如 <![CDATA[  < >xx</ >  < />  ]]>    或 <!--- <test pattern="SECAM" /><test pattern="NTSC" /> -->
                {
                    cout << "Error===> Invalid format, '<' in " << ldata[pos].info_list[i].fileoffset << " without '>'" << endl;
                    return false;
                }
                
				// 可删
                if((CDSECT_start == ldata[pos].info_list[i].stype || COMMENT_start == ldata[pos].info_list[i].stype) && 0 == ldata[pos].info_list[i].endpos)
                {
                    if(CDSECT_start == ldata[pos].info_list[i].stype){
                        cout << "Info====> push: <![CDATA[\t start: " << ldata[pos].info_list[i].fileoffset << endl;
                    }else{
                        cout << "Info====> push: <!--\t start: " << ldata[pos].info_list[i].fileoffset << endl;
                    }
                    st.push(ldata[pos].info_list[i]);
                    continue;
                }

				//可删
                if(!st.empty() && (CDSECT_start == st.top().stype || COMMENT_start == st.top().stype))
                {
                    fseek(fpr, ldata[pos].info_list[i].endpos + 1, SEEK_SET);    // 如果栈顶有评论和注释,则需要寻找它们的结束字段
                    char ch = '\0';
                    while((ch = fgetc(fpr)) != EOF){
                        if('<' == ch){
                            if(' ' == (ch = fgetc(fpr)) || (ch >= '0' && ch <= '9'))    //内容本身的 < 
                                continue;
                            else
                                break;
                        }else if('>' == ch){
                            int end = ftell(fpr) - 1;
                            fseek(fpr, -3, SEEK_CUR);
                            string stmp(4, 0);
                            fread((void*)&stmp.c_str()[0], sizeof(char), 4, fpr);
                            if(' ' == stmp[1] || (stmp[3] >= '0' && stmp[3] <= '9'))    //内容本身的 > 
                                continue;

                            if((CDSECT_start == st.top().stype && "]]>" == stmp.substr(0, stmp.size())) || (COMMENT_start == st.top().stype && "-->" == stmp.substr(0, stmp.size())))
                            {
                                if(CDSECT_start == ldata[pos].info_list[i].stype){
                                    cout << "Info====> pop :<![CDATA[\t start:" << ldata[pos].info_list[i].fileoffset << " end:" << end << endl;
                                }else{
                                    cout << "Info====> pop :<!--\t start:" << ldata[pos].info_list[i].fileoffset << " end:" << end << endl;
                                }
                                st.pop();
                                break;
                            }
                            cout << "Error===> CDSECT_start or COMMENT_start in " << st.top().fileoffset << " not match end: " << end << endl;
                            exit(-1);
                        }
                    }
                }

                fseek(fpr, ldata[pos].info_list[i].endpos - 2, SEEK_SET);    // 定位至 '>' 左边二位, 读取形如 ]]> -->  ?> 等
                string stmp(3, 0);
                fread((void*)&stmp.c_str()[0], sizeof(char), 3, fpr);
                if(StagorEmptytag_start == ldata[pos].info_list[i].stype && '/' == stmp[stmp.size() - 2])    
                    continue;    // 形如 <dia:point val="1.95,6.85"/> 为空元素,直接跳过,不用入栈
                if(PI_start == ldata[pos].info_list[i].stype && '?' == stmp[stmp.size() - 2])
                    continue;    // 形如 <?xml version="1.0" encoding="UTF-8"?>
				
				// 可删
                if(CDSECT_start == ldata[pos].info_list[i].stype && "]]>" == stmp)
                    continue;
                if(COMMENT_start == ldata[pos].info_list[i].stype && "-->" == stmp)
                    continue;
                
                fseek(fpr, ldata[pos].info_list[i].fileoffset, SEEK_SET);        // 定位至 '<'
                string att = "";
                char ch = '\0';
                do{
                    ch = fgetc(fpr);
                    att.append(1, ch);    // 截取属性,形如 <dia:diagramdata>   或  <dia:layer name="Background" 
                }while(' ' != ch && '>' != ch);

                att[att.size() - 1] = '>';
                ldata[pos].info_list[i].attr = att;
                if(Etag_start == ldata[pos].info_list[i].stype)
                {
                    START_INFO tp = st.top();
                    string start = tp.attr.substr(1, tp.attr.size() - 1);
                    if(st.empty() || (StagorEmptytag_start != tp.stype) || (StagorEmptytag_start == tp.stype && start != att.substr(2, att.size() - 2)))
                    {
                        cout << "Error===> Invalid format, " << att << " in " << ldata[pos].info_list[i].fileoffset << " without '<'" << endl;
                        return false;
                    }
                    cout << "Info====> pop :" << tp.attr << "\t start:" << tp.fileoffset << " end:" << ldata[pos].info_list[i].endpos << endl;
                    st.pop();
                    continue;
                }
                if(st.empty() || StagorEmptytag_start == ldata[pos].info_list[i].stype)
                {
                    cout << "Info====> push:" << ldata[pos].info_list[i].attr << "\t start:" << ldata[pos].info_list[i].fileoffset << endl;
                    st.push(ldata[pos].info_list[i]);
                }else{
                    cout << "Debug====> " << ldata[pos].info_list[i].stype << endl;
                }
            }
#endif

        }
        cout << "\t\t\t" << step2_start << endl;
        cout << "\t\t\t" << getTime() << endl;
        cout << "########################## [ step 2 ] ##########################" << endl;
        return st.empty();
	}

private:
    string sfile;
    int th_num;
    int block_cap;
    stack<START_INFO> commentAndCdata;
    vector<BLOCK_INFO> ldata;
    vector<shared_ptr<Thread>> threads;
};

int main(int argc, char* argv[])
{
    if(argc < 2)
        cout << "invalid parameter, please input your file: argv[1]" << endl;
    
    CXml xml(argv[1]);
    xml.start();

    return 0;
}
