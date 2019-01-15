#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include <string>
#include <time.h>
#include <functional>
#include <pthread.h>
#include <iostream>
#include <deque>
#include <map>
#include <stack>
#include <vector>
using namespace std;

string getTime()
{
    time_t timep;
    time (&timep);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S",localtime(&timep) );
    return tmp;
}

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

struct block_info{
    bool stage1_start;
    bool stage1_finish;
    bool stage2_start;
    bool stage2_finish;
    bool stage3_start;
    bool stage3_finish;
    vector<START_INFO> startInfo_array;
};

class CXml{
public:
    CXml(string file)
    :sfile(file)
    { }

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
                        cout << "Error===> [" << sfile << "]:" << stmp << " in " << end << " invalid match CDSECT_start or CDSECT_start" << endl;
                        exit(-1);
                    }
                    
                    commentAndCdata.pop();
                    continue;
                }

                if(ldata.empty() || (stmp.substr(1) == "?>" && ldata.back().stype != PI_start) || (stmp[1] == '/' && ldata.back().stype != StagorEmptytag_start))
                {
                    cout << "Error===> [" << sfile << "]:'>' " << " in " << end << " without '<'" << endl;
                    exit(-1);
                }
                
                if(stmp.substr(1) != "?>" && stmp[1] != '/')
                {
                    bool invalid = false;
                    if(ldata.back().stype != StagorEmptytag_start && ldata.back().stype != Etag_start){
                        invalid = true;
                    }else{
                        if(0 != ldata.back().endpos)
                            invalid = true;
                    }
                    if(invalid){
                        cout << "Error===> [" << sfile << "]:'>' " << " in " << end << " without '<'" << endl;
                        exit(-1);
                    }
                }
                if(0 != ldata.back().endpos){
                    cout << "Error===> [" << sfile << "]:'>' " << " in " << end << " without '<'" << endl;
                    exit(-1);
                }
                    
                ldata.back().endpos = end;
                //cout << ldata.back().fileoffset << "-" << ldata.back().endpos<< " ";
            }
           
            if('<' == ch)
            {
                int offset = ftell(fpr) - 1;
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
                            cout << "Error===> [" << sfile << "]:Invalid string:" << stmp << " in pos:" << offset << endl;
                            exit(-1);
                        }
                        s_comment = true;
                        commentAndCdata.push(tmp);
                        //fseek(fpr, -5, 1);
                        break;
                    }
                    default:
                        tmp.stype = StagorEmptytag_start;
                        break;
                }
                if(s_comment)
                    continue;
                ldata.push_back(tmp);
            }
        }
        
        if(!commentAndCdata.empty())
        {
            cout << "Error===> [" << sfile << "]:CDSECT_start or CDSECT_start in " << commentAndCdata.top().fileoffset << " invalid match" << endl;
            exit(-1);
        }
        cout << "\t\t\t" << getTime() << endl;
        cout << "########################## [ step 1 ] ##########################" << endl;
        fclose(fpr);
    }

    bool checkValid(int ith = 0)
    {
        cout << "########################## [ step 2 ] ##########################" << endl;
        string step2_start = getTime();
        
        FILE *fpr;
        if((fpr=fopen(sfile.c_str(), "r+")) == NULL)
        { 
            cout << "Error===> open " << sfile << "error!" << endl; 
            exit(-1);
        }
        cout << "check ";
        stack<START_INFO> st;
        for(int i = 0; i < ldata.size(); ++i)
        {
            if(CDSECT_start != ldata[i].stype && COMMENT_start != ldata[i].stype && 0 == ldata[i].endpos)    // 形如 <![CDATA[  < >xx</ >  < />  ]]>    或 <!--- <test pattern="SECAM" /><test pattern="NTSC" /> -->
            {
                cout << "Error===> [" << sfile << "]:Invalid format, '<' in " << ldata[i].fileoffset << " without '>'" << endl;
                return false;
            }
            
            if((CDSECT_start == ldata[i].stype || COMMENT_start == ldata[i].stype) && 0 == ldata[i].endpos)
            {
                if(CDSECT_start == ldata[i].stype){
                    cout << "Info====> push: <![CDATA[\t start: " << ldata[i].fileoffset << endl;
                }else{
                    cout << "Info====> push: <!--\t start: " << ldata[i].fileoffset << endl;
                }
                st.push(ldata[i]);
                continue;
            }

            if(!st.empty() && (CDSECT_start == st.top().stype || COMMENT_start == st.top().stype))
            {
                fseek(fpr, ldata[i].endpos + 1, SEEK_SET);    // 如果栈顶有评论和注释,则需要寻找它们的结束字段
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
                            if(CDSECT_start == ldata[i].stype){
                                cout << "Info====> pop :<![CDATA[\t start:" << ldata[i].fileoffset << " end:" << end << endl;
                            }else{
                                cout << "Info====> pop :<!--\t start:" << ldata[i].fileoffset << " end:" << end << endl;
                            }
                            st.pop();
                            break;
                        }
                        cout << "Error===> [" << sfile << "]:CDSECT_start or COMMENT_start in " << st.top().fileoffset << " not match end: " << end << endl;
                        exit(-1);
                    }
                }
            }

            fseek(fpr, ldata[i].endpos - 2, SEEK_SET);    // 定位至 '>' 左边二位, 读取形如 ]]> -->  ?> 等
            string stmp(3, 0);
            fread((void*)&stmp.c_str()[0], sizeof(char), 3, fpr);
            if(StagorEmptytag_start == ldata[i].stype && '/' == stmp[stmp.size() - 2])    
                continue;    // 形如 <dia:point val="1.95,6.85"/> 为空元素,直接跳过,不用入栈
            if(PI_start == ldata[i].stype && '?' == stmp[stmp.size() - 2])
                continue;    // 形如 <?xml version="1.0" encoding="UTF-8"?>
            if(CDSECT_start == ldata[i].stype && "]]>" == stmp)
                continue;
            if(COMMENT_start == ldata[i].stype && "-->" == stmp)
                continue;
            
            fseek(fpr, ldata[i].fileoffset, SEEK_SET);        // 定位至 '<'
            string att = "";
            char ch = '\0';
            do{
                ch = fgetc(fpr);
                att.append(1, ch);    // 截取属性,形如 <dia:diagramdata>   或  <dia:layer name="Background" 
            }while(' ' != ch && '>' != ch);

            att[att.size() - 1] = '>';
            ldata[i].attr = att;
            if(Etag_start == ldata[i].stype)
            {
                START_INFO tp = st.top();
                string start = tp.attr.substr(1, tp.attr.size() - 1);
                if(st.empty() || (StagorEmptytag_start != tp.stype) || (StagorEmptytag_start == tp.stype && start != att.substr(2, att.size() - 2)))
                {
                    cout << "Error===> [" << sfile << "]:Invalid format, " << att << " in " << ldata[i].fileoffset << " without '<'" << endl;
                    return false;
                }
                cout << "Info====> pop :" << tp.attr << "\t start:" << tp.fileoffset << " end:" << ldata[i].endpos << endl;
                st.pop();
                continue;
            }
            if(st.empty() || StagorEmptytag_start == ldata[i].stype)
            {
                cout << "Info====> push:" << ldata[i].attr << "\t start:" << ldata[i].fileoffset << endl;
                st.push(ldata[i]);
            }else{
                cout << "Debug====> " << ldata[i].stype << endl;
            }
        }
        
        cout << "\t\t\t" << step2_start << endl;
        cout << "\t\t\t" << getTime() << endl;
        cout << "########################## [ step 2 ] ##########################" << endl;
        return st.empty();
    }
    
private:
    string sfile;
    stack<START_INFO> commentAndCdata;
    vector<START_INFO> ldata;
};


int main(int argc, char* argv[])
{
    if(argc < 2)
        cout << "invalid parameter, please input your file: argv[1]" << endl;
    
    CXml myxml(argv[1]);
    myxml.buildData();
    cout << "============== [ step3 " << argv[1] << " valid:[" << myxml.checkValid() << "] step3 ] ===========" << endl << endl;

    return 0;
}