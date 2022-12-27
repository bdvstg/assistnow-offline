/*
compile:
gcc assistnow-offline.cpp -lstdc++ -o assistnow-offline

docs:
'UBX_MAG_ANO' & 'AssistNow Offline' in https://www.u-blox.com/docs/UBX-13003221
*/

#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <string.h>
#include <vector>
#include <cassert>
#include <algorithm>
#include <stdarg.h>

std::string myname = "";
std::string optFilename = "mgaoffline.ubx";
bool optPrintRaw = false;
bool optPrint = true;
bool optPrintData = false;
bool optPrintType = true;
bool optPrintVer = true;
bool optShowRange = false;

std::string optQuery = "";

#define SWAP(A,B) { decltype(A) tmp = A; A = B; B = tmp; }

//from https://shengyu7697.github.io/cpp-string-split/
const std::vector<std::string> split(const std::string& str, const std::string& pattern) {
    std::vector<std::string> result;
    std::string::size_type begin, end;

    end = str.find(pattern);
    begin = 0;

    while (end != std::string::npos) {
        if (end - begin != 0) {
            result.push_back(str.substr(begin, end-begin)); 
        }    
        begin = end + pattern.size();
        end = str.find(pattern, begin);
    }

    if (begin != str.length()) {
        result.push_back(str.substr(begin));
    }
    return result;
}

std::string replace(std::string str, std::string target, std::string to) {
    std::string::size_type begin, end;
    while(true) {
        end = str.find(target);
        if(end == std::string::npos)
            break;
        std::string str_front = str.substr(0,end);
        std::string str_end = str.substr(end+target.size(),str.size());
        str = str_front + to + str_end;
    }
    return str;
}

inline bool startsWith(std::string a, std::string b) {
    return (a.rfind(b,0) == 0);
}

bool match_arg(const char *arg, ...) {
    va_list vl;

    char *opt = nullptr;

    va_start(vl, arg);
    
    while(true) {
        opt = va_arg(vl, char*);
        if(opt == nullptr)
            break;
        if(startsWith(arg, opt))
            return true;
    }

    va_end( vl );
    return false;
}

#define MATCH_TRUE(V) match_arg(V, "true", "True", "TRUE", "1", nullptr)
#define MATCH_FALSE(V) match_arg(V, "false", "False", "FALSE", "0", nullptr)

bool check_bool(std::string a, bool *v) {
    auto cols = split(a, "=");
    if (cols.size() == 2) {
        const char* arg = cols[1].c_str();
        if(MATCH_FALSE(arg)) {
            *v = false;
            return true;
        }
        else if(MATCH_TRUE(arg)) {
            *v = true;
            return true;
        }
        else {
            printf("error: unknow value '%s'\n", cols[1].c_str());
            exit(2);
        }
    } else if (cols.size() == 1) {
        *v = true;
        return true;
    } else if (cols.empty() || cols.size() > 2) {
        printf("error: wrong arg '%s'\n", a.c_str());
        exit(2);
    }
    return false;
}

bool check_string(std::string a, std::string &v) {
    auto cols = split(a, "=");
    if (cols.size() == 2) {
        v = cols[1];
    } else {
        printf("error: wrong arg '%s', need =value\n", a.c_str());
        exit(2);
    }
    return false;
}

void help() {
    std::string s = R"(
Tool for u-blox .ubx file that download from AssistNow Offline service

usages:
    %name% --print=true --data=true --type=true --ver=true
        print all data
    %name% --print=false --range=true
        only show the range of date
    %name% --file=my_mgaoffline.ubx
        use my_mgaoffline.ubx instead mgaoffline.ubx
    %name% --query=2022-12-18,2022-12-25
        only show data that 2022-12-18 to 2022-12-25, both 2022-12-18 and 2022-12-25 included
    %name% --query=now
        only show data of today (use system time, UTC)
    %name% --print-raw=true
        show raw data, --print/--data/--type/--ver/--range will be ignore

options:
    --print, -p
        print datas
    --data, -d
        if --print, print field 'data' or not
    --type, -t
        if --print, print field 'type' or not
    --ver, -v
        if --print, print field 'ver' or not
    --range, -r
        print range or not, the range of date of this file
    --file, -f
        use alternative file instead default mgaoffline.ubx
    --query, -q
        if --print or --print-raw, only show the range of query range, for format pls refer to usages
    --print-raw
        print raw data instead text, for send data to u-blox receiver
        --query will works
        --print/--data/--type/--ver/--range will be ignore
    --help, -h
        hi, it me

exit code:
    0: normally exit
    2: error, syntax error or can not open file
    13: query nothing

)";
    s = replace(s, "%name%", myname);
    printf("%s", s.c_str());
    exit(0);
}

void arg_parse(int argc, const char * argv[]) {
#define MATCH_ARGS(...) match_arg(argv[i], __VA_ARGS__, nullptr)
#define CHECK_BOOL(V) check_bool(argv[i], V)
#define CHECK_STR(V) check_string(argv[i], V)
    myname = std::string(argv[0]);
    for(int i = 1; i < argc; i++) {
        if(MATCH_ARGS("-h", "--help")) {
            help();
        } else if(MATCH_ARGS("--print-raw")) {
            CHECK_BOOL(&optPrintRaw);
        } else if(MATCH_ARGS("-p", "--print")) {
            CHECK_BOOL(&optPrint);
        } else if(MATCH_ARGS("-d", "--data")) {
            CHECK_BOOL(&optPrintData);
        } else if(MATCH_ARGS("-t", "--type")) {
            CHECK_BOOL(&optPrintType);
        } else if(MATCH_ARGS("-v", "--ver")) {
            CHECK_BOOL(&optPrintVer);
        } else if(MATCH_ARGS("-r", "--range")) {
            CHECK_BOOL(&optShowRange);
        } else if(MATCH_ARGS("-f", "--file")) {
            CHECK_STR(optFilename);
        } else if(MATCH_ARGS("-q", "--query")) {
            CHECK_STR(optQuery);
        }
    }
}

uint16_t little_endian16(const uint8_t *v) {
    uint16_t t = (((uint16_t)v[1]) << 8) + v[0];
    return t;
}
void endian_swap16(uint8_t *v) {
    uint8_t t = v[0];
    v[0] = v[1];
    v[1] = t;
}

uint16_t checksum(uint8_t *data, int size)
{// first byte is v_class, last is end of payload, size should be length + 4
    uint8_t ck_a = 0, ck_b = 0;
    for(int i = 0; i < size; i++)
    {
        ck_a = ck_a + data[i];
        ck_b = ck_b + ck_a;
    }
    uint16_t ret = (((uint16_t)ck_a) << 8) + (uint16_t)ck_b;
#if BIG_ENDIAN
    endian_swap16((uint8_t*)&ret);
#endif
    return ret;
}

std::string to_string(uint8_t *v, int size) {
    std::string ret = "";
    for (int i = 0 ; i < size ; i++) {
        char buf[5];
        memset(buf, 0, 5);
        sprintf(buf, "%02x", v[i]);
        ret = ret + std::string(buf);
    }
    return ret;
}



inline void range_error(std::string str) {
    printf("error: %s is not a valid range or date\n", str.c_str());
    exit(2);
}
#define RANGE_ERROR(Y,M,D) range_error(Y + "-" + M + "-" + D)

inline int to_int(std::string str_y, std::string str_m, std::string str_d) {
    int y, m, d;
    try {
        y = std::stoi(str_y);
        m = std::stoi(str_m);
        d = std::stoi(str_d);
    } catch (std::exception &ex) {
        RANGE_ERROR(str_y, str_m, str_d);
    }
    if(y < 2000 || y > 2050 || m < 1 || m > 12 || d < 1 || d > 31) {
        RANGE_ERROR(str_y, str_m, str_d);
    }
    return (y * 10000) + (m*100) + d;
}

typedef struct UBX_MAG_ANO_ {
    uint8_t header[2] = {0xb5, 0x62};
    uint8_t v_class = 0x13;
    uint8_t id = 0x20;
    uint16_t length = 76;

    // payload of size of 76
    uint8_t v_type;
    uint8_t version;
    uint8_t svId;
    uint8_t gnssId;
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t reserved1;
    uint8_t data[64];
    uint8_t reserved2[4];

    uint16_t ck; //ck_a, ck_b
} UBX_MAG_ANO;
static_assert(sizeof(UBX_MAG_ANO) == 84);

inline int to_int(UBX_MAG_ANO v) {
    return ((2000+v.year) * 10000) + (v.month*100) + v.day;
}

std::pair<UBX_MAG_ANO,UBX_MAG_ANO> minmax(std::vector<UBX_MAG_ANO> &datas) {
    auto minmax = std::minmax_element(datas.begin(), datas.end(), [](UBX_MAG_ANO &a, UBX_MAG_ANO &b){
        int v1 = to_int(a);
        int v2 = to_int(b);
        return v1 < v2;
    });
    return {*minmax.first,*minmax.second};
}

std::vector<UBX_MAG_ANO> query(std::vector<UBX_MAG_ANO> data, int beg, int end) {
    std::vector<UBX_MAG_ANO> output;
    std::copy_if(data.begin(), data.end(), std::back_inserter(output), [beg,end](UBX_MAG_ANO u){
        int v = to_int(u);
        return (beg <= v && v <= end);
    });
    return output;
}

bool UBX_MAG_ANO_is_valid(UBX_MAG_ANO *data) {
    uint8_t *ptr = (uint8_t*) data;

#if BIG_ENDIAN
    uint16_t len = little_endian16((uint8_t*)&data->length);
#else
    uint16_t len = data->length;
#endif
    assert((len == 76) && "if len not 76, it must be something wrong");
    uint16_t ck = checksum(&ptr[2], len + 4);

    //printf("0x%04X, 0x%04X\n", ck, data->ck);

    return (ck == data->ck);
}

void print(UBX_MAG_ANO &v) {
    bool valid = UBX_MAG_ANO_is_valid(&v);
    auto data = to_string(v.data, sizeof(v.data));
    printf("20%d-%d-%d : ", v.year, v.month, v.day);
    if(optPrintType)
        printf("type=%d, ", v.v_type);
    if(optPrintVer)
        printf("ver=%d, ", v.version);
    printf("svId=%d, gnssId=%d, ck=%s",
            v.svId, v.gnssId, (valid?"valid":"invalid"));
    if(optPrintData)
        printf(", data=%s\n", data.c_str());
    printf("\n");
}

void print(std::pair<UBX_MAG_ANO,UBX_MAG_ANO> range) {
    printf("range: 20%d-%d-%d, 20%d-%d-%d\n",
            range.first.year, range.first.month, range.first.day,
            range.second.year, range.second.month, range.second.day);
}

std::vector<UBX_MAG_ANO> read_offline_ubx(std::string fn) {
    FILE *f = fopen(fn.c_str(),"rb");
    if ( f == nullptr) {
        printf("can not open %s\n", fn.c_str());
        exit(1);
    }
    std::vector<UBX_MAG_ANO> datas;
    while(1) {
        UBX_MAG_ANO dat;
        int ret = fread(&dat, sizeof(UBX_MAG_ANO), 1, f);
        if (ret != 1)
            break;
        datas.push_back(dat);
    }
    return datas;
}

std::string get_now() {
    time_t curr_time;
    tm * curr_tm;
    char date_string[100];
    char time_string[100];

    time(&curr_time);
    //curr_tm = localtime(&curr_time);
    curr_tm = gmtime(&curr_time); // UTC, gps prefer this

    strftime(date_string, sizeof(date_string), "%Y-%m-%d,%Y-%m-%d", curr_tm);
    //strftime(time_string, sizeof(time_string), "%H:%M:%S", curr_tm);
    //printf("%s\n", time_string);
    //exit(0);
    return std::string(date_string);
}
	


int main(int argc, const char *argv[]) {
    arg_parse(argc,argv);

    auto datas = read_offline_ubx(optFilename);
    auto range = minmax(datas);

    std::string q_template = "yyyy-mm-dd,yyyy-mm-dd";
    if(optQuery != "")
        if(optQuery == "now" || optQuery.size() == q_template.size()) {
            if(optQuery == "now")
                optQuery = get_now();
            auto d = split(optQuery, ",");
            if(d.size() != 2)
                range_error(optQuery);
            auto begs = split(d[0],"-");
            auto ends = split(d[1],"-");
            if(begs.size() != 3 || ends.size() != 3)
                range_error(optQuery);
            int beg = to_int(begs[0], begs[1], begs[2]);
            int end = to_int(ends[0], ends[1], ends[2]);
            if(beg > end)
                SWAP(beg,end);
            datas = query(datas, beg, end);
        } else {
            printf("invalid query string '%s', must be form of%s\n",
                optQuery.c_str(), q_template.c_str());
            exit(2);
        }
    
    if(optPrintRaw || optPrint) {
        if (datas.size() == 0) {
            printf("no data in %s\n", optQuery.c_str());
            printf("this file has "); print(range);
            exit(13);
        }
    }

    if(optPrintRaw) {
        for(auto v:datas) {
            int cnt = fwrite(&v, sizeof(v), 1, stdout);
            if(cnt != 1)
                printf("error, lost data in stdout\n");
        }
        exit(0); // ignore optShowRange/optPrint
    }

    if(optShowRange) {
        print(range);
    }

    if(optPrint)
        for(auto v:datas)
            print(v);

    return 0;
}
