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
#include <cjson/cJSON.h>
#include "easywsclient.hpp"
#include "md5.h"

#define READ_DATA_SIZE	1024
#define MD5_SIZE		16
#define MD5_STR_LEN		(MD5_SIZE * 2)

using  easywsclient::WebSocket;
static WebSocket::pointer ws = NULL;
static int isExit = 0;

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
    
    /*
    char* pstr = cJSON_Print(root);
    std::cout << pstr << std::endl;
    free(pstr);
    //*/
    
    cJSON* itemName = cJSON_GetObjectItem(root, "data");
    if(itemName)
    {
        printf("[%s] %s\n",
            cJSON_GetObjectItem(itemName, "sendNick")->valuestring,
            cJSON_GetObjectItem(itemName, "content")->valuestring);
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
    std::cout << "Usage: hydm -i [roomid]" << std::endl;
    std::cout << "    Press [Q] to stop download" << std::endl;
    std::cout << "    Version 1.0.0 by NLSoft 2019.08" << std::endl;
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

    while ((option_index = getopt(argc, argv, "i:")) != -1) {
        switch (option_index) {
        case 'i':
            strcpy(roomid, optarg);
            break;
        }
    }

    time_t times = time(NULL);
	sprintf(data, "data={\"roomId\":%s}&key=7ba102eb&timestamp=%ld", roomid, times);
    
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
                pthread_join(thread, &ret);
				break;
			}
		}
	}
	delete ws;
	
    # ifdef _WIN32
	WSACleanup();
	# endif

	return (0);
}