#ifdef _WIN32
#include <WinSock2.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <conio.h>
#include <iostream>
#include <getopt.h>
#include <zmq.h>
#include <cjson/cJSON.h>
#include "easywsclient.hpp"
#include "md5.h"
#include <map>

#define READ_DATA_SIZE	1024
#define MD5_SIZE		16
#define MD5_STR_LEN		(MD5_SIZE * 2)

using  easywsclient::WebSocket;
static WebSocket::pointer ws = NULL;
static int isExit = 0;
static double start,end,cost;
static unsigned int mm,ss,ms;
static int isDEBUG = 0;
static int isSAVE = 0;
static int isZMQ = 0;
static char outLRC[512];
void *zmq_ctx= NULL, *g_zmq_socket= NULL;
static FILE *g_out = NULL;
static char zmqport[10];

typedef struct t_emots {
    char idx[10];
    char str[10];
}_emots;
_emots emots[] ={
    {"/{dx", "/大笑"},
    {"/{sh", "/送花"},
    {"/{tx", "/偷笑"},
    {"/{dk", "/大哭"},
    {"/{hh", "/嘿哈"},
    {"/{66", "/666"},
    {"/{gd", "/感动"},
    {"/{yw", "/疑问"},
    {"/{xh", "/喜欢"},
    {"/{jx", "/奸笑"},
    {"/{zan","/赞"},
    {"/{ka", "/可爱"},
    {"/{am", "/傲慢"},
    {"/{kx", "/开心"},
    {"/{88", "/拜拜"},
    {"/{hx" , "/害羞"},
    {"/{zs" , "/衰"},
    {"/{pu" , "/吐血"},
    {"/{zc" , "/嘴馋"},
    {"/{sq" , "/生气"},
    {"/{fe" , "/扶额"},
    {"/{bz" , "/闭嘴"},
    {"/{kw" , "/枯萎"},
    {"/{xu" , "/嘘"},
    {"/{xk" , "/笑哭"},
    {"/{lh" , "/流汗"},
    {"/{bk" , "/不看"},
    {"/{hq" , "/哈欠"},
    {"/{tp" , "/调皮"},
    {"/{gl" , "/鬼脸"},
    {"/{cl" , "/戳脸"},
    {"/{dg" , "/大哥"},
    {"/{kun", "/困"},
    {"/{yb" , "/拥抱"},
    {"/{zt" , "/猪头"},
    {"/{kl" , "/骷髅"},
    {"/{cc" , "/臭臭"},
    {"/{xd" , "/心动"},
    {"/{dao", "/刀"}
};

int initzmq(void)
{
    char bind_address[50];
    sprintf(bind_address, "tcp://localhost:%s", zmqport);
    zmq_ctx = zmq_ctx_new();
    if (!zmq_ctx) {
        printf("Could not create ZMQ context: %s\n", zmq_strerror(errno));
        return 1;
    }

    g_zmq_socket = zmq_socket(zmq_ctx, ZMQ_REQ);
    if (!g_zmq_socket) {
        printf("Could not create ZMQ socket: %s\n", zmq_strerror(errno));
        return 1;
    }

    if (zmq_connect(g_zmq_socket, bind_address) == -1) {
        printf("Could not bind ZMQ responder to address '%s': %s\n",
               bind_address, zmq_strerror(errno));
        return 1;
    }
    return 0;
}

void destroyzmq(void)
{
    if(g_zmq_socket) zmq_close(g_zmq_socket);
    if(zmq_ctx) zmq_ctx_destroy(zmq_ctx);    
}

int zmqsend(char *data)
{
    char src_buf[1024];
    char *recv_buf;
    int recv_buf_size;
    zmq_msg_t msg;
    
    sprintf(src_buf, "Parsed_drawtext_2 reinit text='　%s'", data);

    if (zmq_send(g_zmq_socket, src_buf, strlen(src_buf), 0) == -1) {
        printf("Could not send message: %s\n", zmq_strerror(errno));
        return 1;
    }

    if (zmq_msg_init(&msg) == -1) {
        printf("Could not initialize receiving message: %s\n", zmq_strerror(errno));
        return 1;
    }

    if (zmq_msg_recv(&msg, g_zmq_socket, 0) == -1) {
        printf("Could not receive message: %s\n", zmq_strerror(errno));
        zmq_msg_close(&msg);
        return 1;
    }

    recv_buf_size = zmq_msg_size(&msg) + 1;
    recv_buf = (char*)malloc(recv_buf_size);
    if (!recv_buf) {
        printf("Could not allocate receiving message buffer\n");
        zmq_msg_close(&msg);
        return 1;
    }
    memcpy(recv_buf, zmq_msg_data(&msg), recv_buf_size - 1);
    recv_buf[recv_buf_size-1] = 0;
    //printf("%s\n", recv_buf);
    zmq_msg_close(&msg);
    free(recv_buf);
    return 0;
}

std::string string_To_UTF8(const std::string& str) 
{
    int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
    wchar_t* pwBuf = new wchar_t[nwLen + 1];
    ZeroMemory(pwBuf, nwLen * 2 + 2);
    ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen); 
    int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
    char* pBuf = new char[nLen + 1];
    ZeroMemory(pBuf, nLen + 1);
    ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);
    std::string strRet(pBuf);
    delete []pwBuf;
    delete []pBuf;
    pwBuf = NULL;
    pBuf  = NULL;
    return strRet;
}

void get_str_md5(const char *str, char *md5_str)
{
	MD5_CTX md5;
	unsigned char md5_value[MD5_SIZE];
	int i=0;
	MD5Init(&md5);
	MD5Update(&md5, (unsigned char *)str, strlen(str));
	MD5Final(&md5, md5_value);
	for(i = 0; i < MD5_SIZE; i++)
	{
		snprintf(md5_str + i*2, 2+1, "%02x", md5_value[i]);
	}
	md5_str[MD5_STR_LEN] = '\0';
}

// 0-黑 1-蓝 2-绿 3-浅绿 4-红 5-紫 6-黄 7-白 8-灰 9-淡蓝 
//10-淡绿 11-淡浅绿  12-淡红 13-淡紫 14-淡黄 15-亮白 
void SetColor2(int ForeColor,int BackColor)
{
     WORD wColor;
     HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
     CONSOLE_SCREEN_BUFFER_INFO csbi;
     if(GetConsoleScreenBufferInfo(hStdOut, &csbi))
     {
          //wColor = (csbi.wAttributes & 0xF0) + (ForgC & 0x0F);
          wColor = ForeColor + BackColor * 0x10;
          SetConsoleTextAttribute(hStdOut, wColor);
     }
}

void replace_str(char *str,char *oldstr,char *newstr, char *outstr){
    char bstr[1024];
    memset(bstr,0,1024);
    for(int i = 0;i < strlen(str);i++){
        if(!strncmp(str+i,oldstr,strlen(oldstr))){
            strcat(bstr,newstr);
            i += strlen(oldstr) - 1;
        }else{
        	strncat(bstr,str + i,1);
	    }
    }
    strcpy(outstr,bstr);
}

void emotsStr(char *inStr, char *outStr)
{
    strcpy(outStr, inStr);
    for(int i=0;i<sizeof(emots)/sizeof(_emots);i++)
    {
        replace_str(outStr, emots[i].idx, emots[i].str,outStr);
    }
}

void handle_message(const std::string & message)
{
    /*
    {
        "data": {
                "content":      "66",
                "fansLevel":    13,
                "nobleLevel":   1,
                "roomid":       520187,
                "sendNick":     "安于现状",
                "senderAvatarUrl":      "https://huyaimg.msstatic.com/avatar/1075/92/2efdfff2335438962736a8982931e5_180_135.jpg?1561531615",
                "senderGender": 1,
                "showMode":     2
        },
        "statusCode":   200,
        "statusMsg":    ""
    }
    //*/
    
    //std::cout << message << std::endl;
    //*
    cJSON* root = cJSON_Parse(message.c_str());
    
    //*
    if(isDEBUG)
    {
        char* pstr = cJSON_Print(root);
        std::cout << pstr << std::endl;
        free(pstr);
    }
    //*/
    
    cJSON* itemName = cJSON_GetObjectItem(root, "data");
    if(itemName)
    {
        //printf("[%s] %s\n",
        //    cJSON_GetObjectItem(itemName, "sendNick")->valuestring,
        //    cJSON_GetObjectItem(itemName, "content")->valuestring);
        char newStr[1024];
        //strcpy(newStr, cJSON_GetObjectItem(itemName, "content")->valuestring);
        emotsStr(cJSON_GetObjectItem(itemName, "content")->valuestring, newStr);

        SetColor2(3,0);
        printf("[");
        SetColor2(0,3);
        printf("%s",cJSON_GetObjectItem(itemName, "sendNick")->valuestring);
        SetColor2(3,0);
        printf("] ");
        printf("%s\n",newStr); 
        
        if(isSAVE)
        {
            end=clock();
            cost=(end-start)/CLOCKS_PER_SEC;
            mm = (unsigned long int)cost / 60;
            ss = (unsigned long int)cost % 60;
            ms = (unsigned long int)(cost * 100) % 100;
            //FILE *out = fopen(outLRC,"ab+");
            if(g_out)
            {
                fprintf(g_out,"[%02d:%02d.%02d]　[%s] %s\n",
                    mm,ss,ms,
                    cJSON_GetObjectItem(itemName, "sendNick")->valuestring,
                    newStr);
                //fclose(g_out);
            }
        }
        else
        {
            if(isZMQ && zmq_ctx && g_zmq_socket)
            {
                char zmqMSG[512];
                sprintf(zmqMSG, "%s",
                    //cJSON_GetObjectItem(itemName, "sendNick")->valuestring,
                    newStr);
                zmqsend(zmqMSG);
            }
        }
    }
    if(root) cJSON_Delete(root);
}

static void * sendping(void *arg)
{
    while(!isExit)
    {
        if(ws) ws->send("ping");
        usleep(2 * 1000 * 1000);
    }
    //std::cout << "pthread exit\n" << std::endl;
    pthread_exit(NULL);
    return NULL;
}

void help()
{
    std::cout << "Usage: hydm [options] -i [roomid]" << std::endl;
    std::cout << "    -o [filename] set output LRC filename" << std::endl;
    std::cout << "    -z [port] set zmq port" << std::endl;
    std::cout << "    -d debug mode" << std::endl;
    std::cout << "    Press [Q] to stop" << std::endl;
    std::cout << "    Version 1.0.6 by NLSoft 2019.08" << std::endl;
}

int main(int argc, char** argv) 
{
	char data[512];
	char sign[512];
	char roomid[50];
	char ws_url[512];
	int option_index = 0;

    if(argc == 1)
    {
        help();
        exit(0);
    }

    memset(outLRC, 0, 512);
    memset(zmqport, 0, 10);
    while ((option_index = getopt(argc, argv, "o:i:z:d")) != -1) {
        switch (option_index) {
        case 'i':
            strcpy(roomid, optarg);
            break;
        case 'o':
            strcpy(outLRC, optarg);
            g_out = fopen(outLRC,"wb");
            if(g_out)
            {
                fprintf(g_out,"[00:00.00]　\n");
                //fclose(g_out);
                isSAVE = 1;
            }
            break;
        case 'd':
            isDEBUG = 1;
            break;
        case 'z':
            strcpy(zmqport, optarg);
            isZMQ = 1;
            break;
        }
    }

    start=clock();
    time_t times = time(NULL);
	sprintf(data, "data={\"roomId\":%s}&key=7ba102eb&timestamp=%ld", roomid, times);
    
    if(isZMQ) initzmq();

    std::string sdata = data;
    std::string utf_8data = string_To_UTF8(sdata);    
	get_str_md5(utf_8data.c_str(), sign);
    //printf("%ld\n", times);
	//printf("%s\n", data);
	//printf("%s\n", sign);
	sprintf(ws_url, "ws://openapi.huya.com/index.html?do=getMessageNotice&data={\"roomId\":%s}&appId=155600060237190380&timestamp=%ld&sign=%s",
		roomid, times, sign);
	//printf("%s\n", ws_url);

	# ifdef _WIN32
	INT rc;
	WSADATA wsaData;
	rc = WSAStartup(MAKEWORD(2, 2),  & wsaData);
	if (rc) {
		printf("WSAStartup Failed.\n");
		return 1;
	}
	# endif

	ws = WebSocket::from_url(ws_url);
	assert(ws);
	ws->send("ping");
    
    pthread_t thread;
    void *ret;
    pthread_create( & thread, NULL, sendping, NULL);

	while (ws->getReadyState() != WebSocket::CLOSED) {
		ws->poll();
		ws->dispatch(handle_message);
		char exitflag = '\0';
		if (_kbhit())
        {
			exitflag = _getch();
			if (exitflag == 'q' || exitflag == 'Q') {
                isExit = 1;
                if(g_out) fclose(g_out);
                pthread_join(thread, &ret);
				break;
			}
		}
        //printf("while---------------\n");
        usleep(50 * 1000);
	}
	delete ws;
	if(isZMQ) destroyzmq();

    # ifdef _WIN32
	WSACleanup();
	# endif

	return (0);
}