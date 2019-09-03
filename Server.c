#include<cstring>
#include<cstdio>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<mysql/mysql.h>
#include <unistd.h>
#include<stdlib.h>
#include"cJSON.h"
#include"cJSON.c"
#include "time.h"
#include<iostream>
#include<string>
using namespace std;

#define MESSAGE_LEN 1024
#define MAX_ONLINE 2000
#define ITEM_LEN 100
#define SQL_LEN 1000
#define SQLNAME "chatroom"
#define SQLUSER "root"
#define SQLPASSWD "123456"

class Client_link
{
	public:
		char username[ITEM_LEN];//用户名
		int id;//用户id
		int state;//在线状态
		int confd;//套接字句柄
		Client_link()
		{
			this->state=0;
		}
};

Client_link online[MAX_ONLINE];//记录在线状态

bool searchPerson(cJSON* cjson)
{
	printf("查询好友\n");
	char sql[SQL_LEN];//存放sql语句
	MYSQL mysql;//sql句柄
	MYSQL_RES *result;//sql查询结果
	MYSQL_ROW row;//sql查询结果的一行

	mysql_init(&mysql);//句柄初始化

	if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
	{
		printf("%s\n",mysql_error(&mysql));
		return -1;
	}

	printf("数据库连接\n");


	sprintf(sql,"select id,username,usernickname,userintro,userbirthday,usergender,userage,user_selfiefilename from user where username='%s'",cJSON_GetObjectItem(cjson,"searchID")->valuestring);
	mysql_query(&mysql,sql);//执行sql语句
	result = mysql_store_result(&mysql);
	if(row = mysql_fetch_row(result))//如果查到东西了
	{
		printf("开始生成\n");
		cJSON* back=cJSON_CreateObject();
		cJSON_AddNumberToObject(back,"state",1);
        	cJSON_AddNumberToObject(back,"case",7);
        	cJSON_AddNumberToObject(back,"id",atoi(row[0]));
        	cJSON_AddStringToObject(back,"username",row[1]);
        	cJSON_AddStringToObject(back,"usernickname",row[2]);
        	cJSON_AddNumberToObject(back,"userage",atoi(row[6]));
        	cJSON_AddStringToObject(back,"usergender",row[5]);
        	cJSON_AddStringToObject(back,"userintro",row[3]);
        	cJSON_AddStringToObject(back,"user_selfiefilename",row[7]);
        	cJSON_AddStringToObject(back,"userbirthday",row[4]);
        	char* p=cJSON_Print(back);
		printf("生成%s",p);
        	send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
        	free(p);
        	cJSON_Delete(back);
	}
	else
	{
		cJSON* back=cJSON_CreateObject();
		cJSON_AddNumberToObject(back,"state",2);
        	cJSON_AddNumberToObject(back,"case",7);
        	char* p=cJSON_Print(back);
        	send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
        	free(p);
        	cJSON_Delete(back);
	}


	mysql_free_result(result);
	mysql_close(&mysql);//关掉sql句柄
        return 0;
}


bool searchGroup(cJSON* cjson)//通过群号（会话号）查找群聊
{
	printf("查询群聊\n");
    	char sql[SQL_LEN];//存放sql语句
    	MYSQL mysql;//sql句柄
    	MYSQL_RES *result;//sql查询结果
    	MYSQL_ROW row;//sql查询结果的一行

    	mysql_init(&mysql);//句柄初始化

    	if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
    	{
		printf("%s\n",mysql_error(&mysql));
        	return -1;
    	}

    	printf("数据库连接\n");

    	sprintf(sql,"select groupname, ownerID from chatgroup where cnumber='%d'",cJSON_GetObjectItem(cjson,"searchID")->valueint);
    	mysql_query(&mysql,sql);//执行sql语句
    	result = mysql_store_result(&mysql);
    	if(row = mysql_fetch_row(result))//如果查到了
	{
		cJSON* back=cJSON_CreateObject();//生成查到了的返回消息
                cJSON_AddNumberToObject(back,"state",1);
                cJSON_AddNumberToObject(back,"case",8);
                cJSON_AddNumberToObject(back,"cnumber",cJSON_GetObjectItem(cjson,"searchID")->valueint);
                cJSON_AddStringToObject(back,"groupname",row[0]);
                cJSON_AddNumberToObject(back,"ownerID",atoi(row[1]));
                char* p=cJSON_Print(back);
                send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);//返回消息
                free(p);
                cJSON_Delete(back);

	}
	else
	{
		cJSON* back=cJSON_CreateObject();//生成没查到的返回消息
                cJSON_AddNumberToObject(back,"state",2);
                cJSON_AddNumberToObject(back,"case",8);
                char* p=cJSON_Print(back);
                send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);//返回消息
                free(p);
                cJSON_Delete(back);

	}
    	mysql_free_result(result);
	mysql_close(&mysql);//关掉sql句柄
        return 0;
}


//个人资料更新 从界面接收信息
bool Update_User_Info(cJSON* msg)
{
	printf("Goto Update_User_Info\n");
	MYSQL mysql;//sql句柄
        mysql_init(&mysql);//句柄初始化

        if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
        {
            printf("%s\n",mysql_error(&mysql));
            return -1;
        }

        printf("数据库连接\n");

	cJSON *item;
	int id, userage;
	char usergender[20];
	char userintro[50], username[20], usernickname[20];
	char userbirthday[15], user_selfiefilename[200];
	item = cJSON_GetObjectItem(msg, "id");//用户id
	id = item->valueint;
	item = cJSON_GetObjectItem(msg, "usernickname");
	strcpy(usernickname, item->valuestring);
	item = cJSON_GetObjectItem(msg, "userintro");//用户个人签名
	strcpy(userintro, item->valuestring);
	item = cJSON_GetObjectItem(msg, "userbirthday");//格式:2019.01.01 后面可改
	strcpy(userbirthday, item->valuestring);
	item = cJSON_GetObjectItem(msg, "usergender");//0男1女
	strcpy(usergender, item->valuestring);
	item = cJSON_GetObjectItem(msg, "userage");//用户年龄
	userage = item->valueint;
	item = cJSON_GetObjectItem(msg, "user_selfiefilename");//用户头像文件名
	strcpy(user_selfiefilename, item->valuestring);
	
	//存入数据库
	char sqlstring[1024];
	sprintf(sqlstring, "update user set usernickname='%s', usergender='%s', userintro='%s', user_selfiefilename='%s', userbirthday='%s', userage=%d  where id=%d", usernickname, usergender, userintro, user_selfiefilename, userbirthday, userage, id);//对数据库中的个人信息进行更新
	printf("sql:%s\n", sqlstring);
	mysql_query(&mysql, sqlstring);

	cJSON* back=cJSON_CreateObject();//生成个人信息已更新的消息
	cJSON_AddNumberToObject(back,"case",6);
	char* p=cJSON_Print(back);
	send(cJSON_GetObjectItem(msg,"confd")->valueint,p,strlen(p),0);//返回
	free(p);
	cJSON_Delete(back);


	printf("Exit Update_User_Info\n");
        mysql_close(&mysql);//关掉sql句柄
        return 0;
}



//由客户端发来的cJSON:"id":...和"case":5
//用用户id获取用户资料并发送回去 成功返回1，若id不存在则返回0 
bool Send_User_Info(cJSON *msg)
{
	printf("Goto Send_User_Info\n");
	cJSON *item;
	int id,userage;
	char usergender[20];
	char userintro[50], usernickname[20], username[20];
	char userbirthday[15], user_selfiefilename[200];
	item = cJSON_GetObjectItem(msg, "id");//用户id
	id = item->valueint;
	
	//从数据库获取用户资料，
	char sqlstring[1024];
	int ret, numRows;

	MYSQL mysql;//sql句柄
	MYSQL_ROW row;
        MYSQL_RES * result;
        mysql_init(&mysql);//句柄初始化

        if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
        {
            printf("%s\n",mysql_error(&mysql));
            return -1;
        }

        printf("数据库连接\n");

	char tmp[] = "id,username,usernickname,userage,usergender,userintro,user_selfiefilename,userbirthday";
	sprintf(sqlstring, "select %s from user where id=%d", tmp, id);
	mysql_query(&mysql, sqlstring);
	result = mysql_store_result(&mysql);
	if(result)
	{
		numRows = mysql_num_rows(result);
		if(numRows > 0)
		{
			ret = 1;
			char temp[100];
			row = mysql_fetch_row(result);
			//从数据库读取信息
			sscanf(row[1], "%s", username);
			sscanf(row[2], "%s", usernickname);
			sscanf(row[5], "%s", userintro);
			sscanf(row[7], "%s", userbirthday);
			sscanf(row[4], "%s", usergender);
			sscanf(row[3], "%s", temp);
			userage = atoi(temp);			
			sscanf(row[6], "%s", user_selfiefilename);
		}
		else
		{
			ret = 0;
			printf("没取到行，可能数数据库为空？ : ret=%d\n", ret);
		}
		if (ret)
		{
			char message[1024];
			memset(message, 0, sizeof(message));
			sprintf(message, "{\"case\":%d,\"id\":%d,\"username\":\"%s\",\"usernickname\":\"%s\",\"userage\":%d,\"usergender\":%s,\"userintro\":\"%s\",\"user_selfiefilename\":\"%s\",\"userbirthday\":\"%s\"}",5,id, username, usernickname, userage, usergender, userintro, user_selfiefilename, userbirthday);
			cJSON *new_message = cJSON_Parse(message);
			printf("%s\n",cJSON_Print(new_message));
			send(online[id].confd, message, 1024, 0);
		}
		else
		{
			printf("未取得信息!\n");
		}
	}
	printf("Exit Send_User_Info\n");
	mysql_free_result(result);
        mysql_close(&mysql);//关掉sql句柄
        return 0;
}

bool Register(cJSON* cjson)
{
	printf("注册\n");
        char sql[SQL_LEN];//存放sql语句
        MYSQL mysql;//sql句柄
        MYSQL_RES *result;//sql查询结果
        MYSQL_ROW row;//sql查询结果的一行

        mysql_init(&mysql);//句柄初始化

        if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
        {
            printf("%s\n",mysql_error(&mysql));
            return -1;
        }

        printf("数据库连接\n");

        sprintf(sql,"%s'%s'","select id,username,password from user where username=",cJSON_GetObjectItem(cjson,"username")->valuestring);//查找帐号

        printf("查找帐号sql语句生成\n");

        mysql_query(&mysql,sql);//执行sql语句
        result=mysql_store_result(&mysql);//sql查询结果

        printf("查找帐号sql语句查询完毕\n");

        if(row=mysql_fetch_row(result))//如果查到东西了
        {
		cJSON* back=cJSON_CreateObject();//生成用户名已存在的消息并返回
		cJSON_AddNumberToObject(back,"case",1);
		cJSON_AddNumberToObject(back,"state",2);
		char* p=cJSON_Print(back);
		send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
		free(p);
		cJSON_Delete(back);
        }
        else
        {
		sprintf(sql,"%s","select count(id) from user");//查找user表中有多少条记录
		mysql_query(&mysql,sql);//执行sql语句
        	result=mysql_store_result(&mysql);//sql查询结果
		row=mysql_fetch_row(result);

		int i=atoi(row[0]);
		i++;

		sprintf(sql,"insert into user values(%d,'%s','%s','%s','%s','%s','%s','%s','%s',%d,'%s')",i,cJSON_GetObjectItem(cjson,"username")->valuestring,cJSON_GetObjectItem(cjson,"password")->valuestring," "," "," "," "," "," ",0," ");//向user表添加一条记录，包括新注册用户的帐号密码以及为他分配的id
		mysql_query(&mysql,sql);//执行sql语句

		printf("向user数据库添加%s",sql);

		cJSON* back=cJSON_CreateObject();//生成注册成功的消息并返回
                cJSON_AddNumberToObject(back,"case",1);
                cJSON_AddNumberToObject(back,"state",1);
                char* p=cJSON_Print(back);
                send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
                free(p);
                cJSON_Delete(back);
        }

	mysql_free_result(result);
        mysql_close(&mysql);//关掉sql句柄
        return 0;
}

bool Log_in(cJSON* cjson)
{
	printf("登录\n");
	char sql[SQL_LEN];//存放sql语句
	MYSQL mysql;//sql句柄
    	MYSQL_RES *result;//sql查询结果
    	MYSQL_ROW row;//sql查询结果的一行

    	mysql_init(&mysql);//句柄初始化

    	if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
    	{
            printf("%s\n",mysql_error(&mysql));
            return -1;
    	}

	printf("数据库连接\n");

	sprintf(sql,"%s'%s'","select id,username,password from user where username=",cJSON_GetObjectItem(cjson,"username")->valuestring);//查找帐号对应的id，帐号和密码

	printf("查找帐号id密码sql语句生成\n");

    	mysql_query(&mysql,sql);//执行sql语句
    	result=mysql_store_result(&mysql);//sql查询结果

	printf("查找帐号id密码sql语句查询完毕\n");

    	if(row=mysql_fetch_row(result))//如果查到东西了
    	{
    		if(strcmp(cJSON_GetObjectItem(cjson,"password")->valuestring,row[2])==0)//比较密码是否一样
    		{
    			printf("登录成功\n");
			int i=atoi(row[0]);
			printf("id为%d的帐号上线\n",i);
    			online[i].id=i;//将id存进去
	    		online[i].state=1;//更新登录状态
			online[i].confd=cJSON_GetObjectItem(cjson,"confd")->valueint;//将当前TCP套接字存进去
    			strcpy(online[i].username,cJSON_GetObjectItem(cjson,"username")->valuestring);//将帐号存进去
		}
		else
		{
			printf("登录失败\n");//登录失败
		}
	}
	else
	{
		printf("帐号错误\n");//没有该用户
	}

	mysql_free_result(result);
	mysql_close(&mysql);//关掉sql句柄
	return 0;
}

bool Send_person(cJSON* cjson)
{
	printf("发送私聊\n");
	char sql[SQL_LEN];//存放sql语句
        MYSQL mysql;//sql句柄
        MYSQL_RES *result;//sql查询结果
        MYSQL_ROW row;//sql查询结果的一行
	cJSON* back;
	char* p;

	time_t t;
        struct tm * lt;
        time(&t);
        lt = localtime (&t);//转为时间结构。
        char time[64];
	
	sprintf(time, "%ld %ld %ld %ld %ld %ld", lt->tm_year+1900,lt->tm_mon+1,lt->tm_mday,lt->tm_hour,lt->tm_min,lt->tm_sec);


        mysql_init(&mysql);//句柄初始化

        if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
        {
            printf("%s\n",mysql_error(&mysql));
            return -1;
        }

        printf("数据库连接\n");

	int i=cJSON_GetObjectItem(cjson,"recvid")->valueint;

	if(online[i].state==1)//如果接受者在线
	{
		sprintf(sql,"update conversation set cindex=cindex+1 where cnumber=%d",cJSON_GetObjectItem(cjson,"cnumber")->valueint);//会话双方index＋1
                mysql_query(&mysql,sql);//执行sql语句

        	cJSON_AddNumberToObject(cjson,"state",2);
		cJSON_AddStringToObject(cjson,"time",time);		
        	p=cJSON_Print(cjson);
        	send(online[i].confd,p,strlen(p),0);
        	free(p);

	}
	else
	{
		sprintf(sql,"update conversation set cindex=cindex+1 where cnumber=%d and id1=%d",cJSON_GetObjectItem(cjson,"cnumber")->valueint,cJSON_GetObjectItem(cjson,"sendid")->valueint);//发送方index＋1
                mysql_query(&mysql,sql);//执行sql语句
                printf("接受者不在线，存储%s，发送方index＋1\n",cJSON_GetObjectItem(cjson,"message")->valuestring);
	}
	sprintf(sql,"select cnumber from history%d where cindex=0",cJSON_GetObjectItem(cjson,"cnumber")->valueint);//查找会话号对应的历史记录有多少条
	mysql_query(&mysql,sql);//执行sql语句
	result=mysql_store_result(&mysql);//sql查询结果
	row=mysql_fetch_row(result);


	sprintf(sql,"insert into history%d (cnumber,id,message,cindex,time) values(%d,%d,'%s',%d,'%s')",cJSON_GetObjectItem(cjson,"cnumber")->valueint,cJSON_GetObjectItem(cjson,"cnumber")->valueint,cJSON_GetObjectItem(cjson,"sendid")->valueint,cJSON_GetObjectItem(cjson,"message")->valuestring,atoi(row[0])+1,time);//添加一条新的历史记录
	mysql_query(&mysql,sql);//执行sql语句
	printf("添加历史记录%s\n",sql);

	sprintf(sql,"update history%d set cnumber=cnumber+1 where cindex=0",cJSON_GetObjectItem(cjson,"cnumber")->valueint);//历史记录条数＋1
	mysql_query(&mysql,sql);//执行sql语句
	printf("历史记录条数＋1\n");

	back=cJSON_CreateObject();
	cJSON_AddNumberToObject(back,"case",3);
	cJSON_AddNumberToObject(back,"state",1);
	p=cJSON_Print(back);
	send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
	free(p);
	cJSON_Delete(back);

	mysql_free_result(result);
	mysql_close(&mysql);//关掉sql句柄
        return 0;
}

bool Send_group(cJSON* cjson)
{
	char sql[SQL_LEN];//存放sql语句
        MYSQL mysql;//sql句柄
        MYSQL_RES *result;//sql查询结果
        MYSQL_ROW row;//sql查询结果的一行
	cJSON* back;
	char* p;

	time_t t;
        struct tm * lt;
        time(&t);
        lt = localtime (&t);//转为时间结构。
        char time[64];

        sprintf(time, "%ld %ld %ld %ld %ld %ld", lt->tm_year+1900,lt->tm_mon+1,lt->tm_mday,lt->tm_hour,lt->tm_min,lt->tm_sec);


        mysql_init(&mysql);//句柄初始化

        if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
        {
            printf("%s\n",mysql_error(&mysql));
            return -1;
        }

        printf("数据库连接\n");

        int i=cJSON_GetObjectItem(cjson,"sendid")->valueint;

	sprintf(sql,"select id1 from conversation where cnumber=%d",cJSON_GetObjectItem(cjson,"cnumber")->valueint);//查找群聊成员
	mysql_query(&mysql,sql);//执行sql语句
	result=mysql_store_result(&mysql);//sql查询结果
	printf("查找%d群的群成员\n",cJSON_GetObjectItem(cjson,"cnumber")->valueint);
        while(row=mysql_fetch_row(result))
	{
		printf("群成员%d\n",atoi(row[0]));
		if(online[atoi(row[0])].state==1)//如果该群成员在线
		{
			sprintf(sql,"update conversation set cindex=cindex+1 where cnumber=%d and id1=%d",cJSON_GetObjectItem(cjson,"cnumber")->valueint,atoi(row[0]));//该群成员index＋1
                	mysql_query(&mysql,sql);//执行sql语句

			cJSON_AddNumberToObject(cjson,"state",2);
			cJSON_AddStringToObject(cjson,"time",time);
                	p=cJSON_Print(cjson);
                	send(online[atoi(row[0])].confd,p,strlen(p),0);
                	free(p);	

		}
	}
        sprintf(sql,"select cnumber from history%d where cindex=0",-cJSON_GetObjectItem(cjson,"cnumber")->valueint);//查找会话号对应的历史记录有多少条
        mysql_query(&mysql,sql);//执行sql语句
        result=mysql_store_result(&mysql);//sql查询结果
        row=mysql_fetch_row(result);

        sprintf(sql,"insert into history%d (cnumber,id,message,cindex,time) values(%d,%d,'%s',%d,'%s')",-cJSON_GetObjectItem(cjson,"cnumber")->valueint,cJSON_GetObjectItem(cjson,"cnumber")->valueint,cJSON_GetObjectItem(cjson,"sendid")->valueint,cJSON_GetObjectItem(cjson,"message")->valuestring,atoi(row[0])+1,time);//添加一条新的历史记录
        mysql_query(&mysql,sql);//执行sql语句
	printf("添加历史记录\n");

        sprintf(sql,"update history%d set cnumber=cnumber+1 where cindex=0",-cJSON_GetObjectItem(cjson,"cnumber")->valueint);//历史记录条数＋1
	mysql_query(&mysql,sql);//执行sql语句
	printf("历史记录条数＋1\n");

	back=cJSON_CreateObject();
        cJSON_AddNumberToObject(back,"case",4);
	cJSON_AddNumberToObject(back,"state",1);
        p=cJSON_Print(back);
        send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
        free(p);
        cJSON_Delete(back);

	mysql_free_result(result);
	mysql_close(&mysql);//关掉sql句柄
        return 0;
}

bool addFriend(cJSON* cjson)
{
	printf("添加好友\n");
	char sql[SQL_LEN];//存放sql语句
        MYSQL mysql;//sql句柄
        MYSQL_RES *result;//sql查询结果
        MYSQL_ROW row;//sql查询结果的一行
        char* p;
        cJSON* back;

        mysql_init(&mysql);//句柄初始化

        if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
        {
            printf("%s\n",mysql_error(&mysql));
            return -1;
        }

        printf("数据库连接\n");

	int i=cJSON_GetObjectItem(cjson,"recvid")->valueint;

	if(online[i].state==1)//如果接受者在线
	{
        	p=cJSON_Print(cjson);//将好友申请发给接受者
        	send(online[i].confd,p,strlen(p),0);
        	free(p);
	}
	else
	{
		sprintf(sql,"insert into conversation (id1,id2,cindex) values(%d,%d,-3)",cJSON_GetObjectItem(cjson,"recvid")->valueint,cJSON_GetObjectItem(cjson,"sendid")->valueint);//存储好友申请
                mysql_query(&mysql,sql);//执行sql语句
                printf("接受者不在线，好友申请已存储\n");
	}
	
	back=cJSON_CreateObject();//返回发送着申请已收到
    	cJSON_AddNumberToObject(back,"case",9);
    	p=cJSON_Print(back);
    	send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
    	free(p);
    	cJSON_Delete(back);

	mysql_close(&mysql);//关掉sql句柄
        return 0;
}

bool acceptFriend(cJSON* cjson)
{
	printf("同意好友申请\n");
	char sql[SQL_LEN];//存放sql语句
        MYSQL mysql;//sql句柄
        MYSQL_RES *result;//sql查询结果
        MYSQL_ROW row;//sql查询结果的一行
        cJSON* back;
        char* p;

        mysql_init(&mysql);//句柄初始化

        if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
        {
            printf("%s\n",mysql_error(&mysql));
            return -1;
        }

        printf("数据库连接\n");

	sprintf(sql,"select * from conversation where id1=%d and id2=%d",cJSON_GetObjectItem(cjson,"sendid")->valueint,cJSON_GetObjectItem(cjson,"recvid")->valueint);//看看两个人是不是已经是好友了
        mysql_query(&mysql,sql);//执行sql语句
        result=mysql_store_result(&mysql);//sql查询结果
        if(row=mysql_fetch_row(result))//如果已经是好友了
	{
		back=cJSON_CreateObject();
	        cJSON_AddNumberToObject(back,"case",10);
		cJSON_AddNumberToObject(back,"state",1);
	        p=cJSON_Print(back);
	        send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
	        free(p);
	        cJSON_Delete(back);
		return 0;
	}

	int i=cJSON_GetObjectItem(cjson,"sendid")->valueint;

	if(online[i].state==1)//如果发送者在线
	{
		printf("发送者在线，更新发送者好友列表\n");
	}
	 
	sprintf(sql,"select cindex from conversation where cnumber=0");//查找当前有多少会话
	mysql_query(&mysql,sql);//执行sql语句
	result=mysql_store_result(&mysql);//sql查询结果
	row=mysql_fetch_row(result);
	
	sprintf(sql,"insert into conversation (id1,id2,cnumber,cindex) values(%d,%d,%d,0)",cJSON_GetObjectItem(cjson,"sendid")->valueint,cJSON_GetObjectItem(cjson,"recvid")->valueint,atoi(row[0])+1);//更新会话表
	mysql_query(&mysql,sql);//执行sql语句
	
	sprintf(sql,"insert into conversation (id1,id2,cnumber,cindex) values(%d,%d,%d,0)",cJSON_GetObjectItem(cjson,"recvid")->valueint,cJSON_GetObjectItem(cjson,"sendid")->valueint,atoi(row[0])+1);//更新会话表
	mysql_query(&mysql,sql);//执行sql语句
	
	sprintf(sql,"create table history%d(cnumber int,id int,message varchar(1024),cindex int,time char(32))",atoi(row[0])+1);//创建该会话的历史记录表
	mysql_query(&mysql,sql);//执行sql语句
	
	sprintf(sql,"insert into history%d (cnumber,cindex) values(0,0)",atoi(row[0])+1);//初始化历史记录表
	mysql_query(&mysql,sql);//执行sql语句
	
	sprintf(sql,"update conversation set cindex=cindex+1 where cnumber=0");//会话数＋1
	mysql_query(&mysql,sql);//执行sql语句

	back=cJSON_CreateObject();
        cJSON_AddNumberToObject(back,"case",10);
        cJSON_AddNumberToObject(back,"state",2);
        cJSON_AddNumberToObject(back,"cnumber",atoi(row[0])+1);
        p=cJSON_Print(back);
        send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
        free(p);
        cJSON_Delete(back);

	mysql_free_result(result);
	mysql_close(&mysql);//关掉sql句柄
        return 0;
}

bool deleteFriend(cJSON* cjson)
{
	printf("删除好友\n");
	char sql[SQL_LEN];//存放sql语句
        MYSQL mysql;//sql句柄
        MYSQL_RES *result;//sql查询结果
        MYSQL_ROW row;//sql查询结果的一行
	char* p;
	cJSON* back;

        mysql_init(&mysql);//句柄初始化

        if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
        {
            printf("%s\n",mysql_error(&mysql));
            return -1;
        }

        printf("数据库连接\n");

	int i=cJSON_GetObjectItem(cjson,"recvid")->valueint;

	if(online[i].state==1)//如果接受者在线
	{
		printf("接受者在线，更新接受者好友列表\n");
	}

	back=cJSON_CreateObject();
    	cJSON_AddNumberToObject(back,"case",11);
    	p=cJSON_Print(back);
    	send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
    	free(p);
   	cJSON_Delete(back);
	
	sprintf(sql,"delete from conversation where cnumber=%d",cJSON_GetObjectItem(cjson,"cnumber")->valueint);//查找会话号对应的历史记录有多少条
	mysql_query(&mysql,sql);//执行sql语句
	
	sprintf(sql,"drop table history%d",cJSON_GetObjectItem(cjson,"cnumber")->valueint);//查找会话号对应的历史记录有多少条
	mysql_query(&mysql,sql);//执行sql语句

	mysql_close(&mysql);//关掉sql句柄
        return 0;
}


bool createGroup(cJSON* cjson)//创建群聊
{
    	char sql[SQL_LEN];//存放sql语句
    	MYSQL mysql;//sql句柄
    	MYSQL_RES *result;//sql查询结果
    	MYSQL_ROW row;//sql查询结果的一行
    	int newnum;
    	time_t t;
    	struct tm * lt;
    	time(&t);
	lt = localtime (&t);//转为时间结构。
    	char time[64];

    	mysql_init(&mysql);//句柄初始化

    	if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
    	{
        	printf("%s\n",mysql_error(&mysql));
        	return -1;
    	}

    	printf("数据库连接\n");

    	sprintf(sql,"select cindex from conversation where cnumber=%d",0);
    	mysql_query(&mysql,sql);//执行sql语句
    	result = mysql_store_result(&mysql);
	row = mysql_fetch_row(result);
	printf("%s\n",row[0]);
    	newnum = atoi(row[0])+1;

    	sprintf(sql,"update conversation set cindex = %d where cnumber = %d",newnum,0);
	mysql_query(&mysql,sql);//执行sql语句
    	printf("cindex修改为%d\n",newnum);

	sprintf(sql,"insert into chatgroup(cnumber, groupname, ownerID, adminID) values (%d,' %s', %d, %d)",-newnum,cJSON_GetObjectItem(cjson,"groupname")->valuestring,cJSON_GetObjectItem(cjson,"ownerID")->valueint,0);
    	mysql_query(&mysql,sql);//执行sql语句


    	printf("信息插入chatgroup\n");

    	sprintf(sql,"insert into conversation(id1, cnumber, cindex, status) values (%d, %d, %d, %d)",cJSON_GetObjectItem(cjson,"ownerID")->valueint, -newnum, 0, 2);
    	mysql_query(&mysql,sql);//执行sql语句
    	printf("信息插入conversation\n");

    	sprintf(sql,"create table history%d(cnumber int, id int, message varchar(1024), cindex int, time char(64))", newnum);
    	mysql_query(&mysql,sql);
    	printf("创建表history%d\n",newnum);

    
    	sprintf(time, "%ld %ld %ld %ld %ld %ld", lt->tm_year+1900,lt->tm_mon+1,lt->tm_mday,lt->tm_hour,lt->tm_min,lt->tm_sec);

    	sprintf(sql,"insert into history%d(cnumber, cindex, time) values (%d, %d, '%s')",newnum,-newnum, 0, time);
    	mysql_query(&mysql,sql);//执行sql语句
    	printf("插入信息，时间:%s\n",time);

	sprintf(sql,"select usernickname from user where id=%d",cJSON_GetObjectItem(cjson,"ownerID")->valueint);
    	mysql_query(&mysql,sql);//执行sql语句
    	result = mysql_store_result(&mysql);
	row = mysql_fetch_row(result);
    	cJSON* back=cJSON_CreateObject();
	cJSON_AddNumberToObject(back,"ownerID",cJSON_GetObjectItem(cjson,"ownerID")->valueint);
    	cJSON_AddStringToObject(back,"ownernickname",row[0]);
    	cJSON_AddStringToObject(back,"groupname",cJSON_GetObjectItem(cjson,"groupname")->valuestring);
    	cJSON_AddNumberToObject(back,"case",cJSON_GetObjectItem(cjson, "case")->valueint);
    	char* p=cJSON_Print(back);
    	send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
    	free(p);
    	cJSON_Delete(back);


	mysql_free_result(result);
	mysql_close(&mysql);//关掉sql句柄
    	return 0;
}

bool sendGroupInv(cJSON* cjson)//申请&邀请
{
    char sql[SQL_LEN];//存放sql语句
    MYSQL mysql;//sql句柄
    MYSQL_RES *result;//sql查询结果
    MYSQL_ROW row;//sql查询结果的一行

    char sql2[SQL_LEN];//存放sql语句
    MYSQL mysql2;//sql句柄
    MYSQL_RES *result2;//sql查询结果
    MYSQL_ROW row2;//sql查询结果的一行

    int check = cJSON_GetObjectItem(cjson,"oper")->valueint;
    int flag = 1;

    mysql_init(&mysql);//句柄初始化

    if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
    {
        printf("%s\n",mysql_error(&mysql));
        return -1;
    }

    printf("数据库连接\n");

    int i = cJSON_GetObjectItem(cjson,"recvID")->valueint;

    sprintf(sql,"select id1 from conversation where cnumber=%d",cJSON_GetObjectItem(cjson,"cnumber")->valueint);
    mysql_query(&mysql,sql);//执行sql语句
    result = mysql_store_result(&mysql);

    while(row = mysql_fetch_row(result))
    {
        if(i == atoi(row[0]))
        {
            flag = 0;
            break;
        }
    }
    if(flag == 0 )
    {
        printf("already in the group\n");
        return -1;
    }
    else
    {
        if(check)//invite
        {
            if(online[i].state==1)
            {
                //回传
                sprintf(sql,"select groupname from chatgroup where cnumber=%d",cJSON_GetObjectItem(cjson,"cnumber")->valueint);
                mysql_query(&mysql,sql);//执行sql语句
                result = mysql_store_result(&mysql);
                row = mysql_fetch_row(result);

                cJSON* back=cJSON_CreateObject();
                cJSON_AddNumberToObject(back,"sendID", cJSON_GetObjectItem(cjson,"sendID")->valueint);
                cJSON_AddNumberToObject(back,"cnumber", cJSON_GetObjectItem(cjson,"cnumber")->valueint);
                cJSON_AddStringToObject(back,"groupname",row[0]);

                sprintf(sql,"select usernickname from user where id=%d",cJSON_GetObjectItem(cjson,"sendID")->valueint);
                mysql_query(&mysql,sql);//执行sql语句
                result = mysql_store_result(&mysql);
                row = mysql_fetch_row(result);

                cJSON_AddNumberToObject(back,"oper",cJSON_GetObjectItem(cjson,"oper")->valueint);
                cJSON_AddStringToObject(back,"sendname",row[0]);
                cJSON_AddNumberToObject(back,"case",cJSON_GetObjectItem(cjson, "case")->valueint);
                char* p=cJSON_Print(back);
                // send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
                send(online[i].confd,p,strlen(p),0);
                free(p);
                cJSON_Delete(back);
                printf("接收者在线，发送给接收者邀请信息，数据库不变\n");
            }
            else//接收者不在线
            {
                sprintf(sql,"insert into conversation(id1, cnumber, cindex) values (%d, %d, %d)", i, cJSON_GetObjectItem(cjson,"cnumber")->valueint, -2);
                mysql_query(&mysql,sql);//执行sql语句
                printf("邀请已储存\n");
            }
        }
        else//apply
        {
            sprintf(sql, "select ownerID,adminID from chatgroup where cnumber = %d",cJSON_GetObjectItem(cjson,"cnumber")->valueint);
            mysql_query(&mysql,sql);//执行sql语句
            result = mysql_store_result(&mysql);
            row = mysql_fetch_row(result);
            for(int cnt = 0;cnt < 2; cnt++)
            {
                if(row[cnt]==0)
                     continue;
                if(online[atoi(row[cnt])].state==1)
                {
                    sprintf(sql2,"select groupname from chatgroup where cnumber=%d",cJSON_GetObjectItem(cjson,"cnumber")->valueint);
                    mysql_query(&mysql,sql2);//执行sql语句
                    result2 = mysql_store_result(&mysql);
                    row2 = mysql_fetch_row(result2);

                    cJSON* back=cJSON_CreateObject();
                    cJSON_AddNumberToObject(back,"sendID", atoi(row[cnt]));
                    cJSON_AddNumberToObject(back,"cnumber", cJSON_GetObjectItem(cjson,"cnumber")->valueint);
                    cJSON_AddStringToObject(back,"groupname",row2[0]);

                    sprintf(sql2,"select usernickname from user where id=%d", atoi(row[cnt]));
                    mysql_query(&mysql,sql2);//执行sql语句
                    result2 = mysql_store_result(&mysql);
                    row2 = mysql_fetch_row(result2);
                    
                    cJSON_AddNumberToObject(back,"oper",cJSON_GetObjectItem(cjson,"oper")->valueint);
                    cJSON_AddStringToObject(back,"sendname",row2[0]);
                    cJSON_AddNumberToObject(back,"case",cJSON_GetObjectItem(cjson, "case")->valueint);
                    char* p=cJSON_Print(back);
                    // send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
                    send(online[atoi(row[cnt])].confd,p,strlen(p),0);
                    free(p);
                    cJSON_Delete(back);
                    printf("接收者在线，发送给管理者申请信息，数据库不变\n");
                }
                else//接收者不在线
                {
                    sprintf(sql,"insert into conversation(id1, id2, cnumber, cindex) values (%d, %d, %d, %d)", atoi(row[cnt]),i, cJSON_GetObjectItem(cjson,"cnumber")->valueint, -1);
                    mysql_query(&mysql,sql);//执行sql语句
                    printf("申请已储存\n");
                }
            }
        }
    }
    mysql_free_result(result2);
    mysql_free_result(result);
	mysql_close(&mysql);//关掉sql句柄
    return 0;
}


bool operGroupInv(cJSON* cjson)//申请&邀请，同意&拒绝
{
    char sql[SQL_LEN];//存放sql语句
    MYSQL mysql;//sql句柄
    MYSQL_RES *result;//sql查询结果
    MYSQL_ROW row;//sql查询结果的一行
    int check = cJSON_GetObjectItem(cjson,"reply")->valueint;

    mysql_init(&mysql);//句柄初始化

    if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
    {
        printf("%s\n",mysql_error(&mysql));
        return -1;
    }

    printf("数据库连接\n");

    int i = cJSON_GetObjectItem(cjson,"sendID")->valueint;

    if(check)//agree
    {
        sprintf(sql,"select max(cindex) from history%d", -cJSON_GetObjectItem(cjson,"cnumber")->valueint);
        mysql_query(&mysql,sql);
        result = mysql_store_result(&mysql);
        row = mysql_fetch_row(result);
       
        if(cJSON_GetObjectItem(cjson,"oper")->valueint == 0)
            sprintf(sql,"insert into conversation(id1, cnumber, cindex, status) values (%d, %d, %d, %d)", cJSON_GetObjectItem(cjson,"sendID")->valueint, cJSON_GetObjectItem(cjson,"cnumber")->valueint, atoi(row[0]), 0);
        else
            sprintf(sql,"insert into conversation(id1, cnumber, cindex, status) values (%d, %d, %d, %d)", cJSON_GetObjectItem(cjson,"recvID")->valueint, cJSON_GetObjectItem(cjson,"cnumber")->valueint, row[0], 0);
        mysql_query(&mysql,sql);//执行sql语句
        printf("已插入conversation\n");

        //返回给sendID
        if(online[i].state==1)
        {
            sprintf(sql,"select groupname from chatgroup where cnumber=%d",cJSON_GetObjectItem(cjson,"cnumber")->valueint);
            mysql_query(&mysql,sql);//执行sql语句
            result = mysql_store_result(&mysql);
            row = mysql_fetch_row(result);

            cJSON* back=cJSON_CreateObject();
            cJSON_AddNumberToObject(back,"recvID", cJSON_GetObjectItem(cjson,"recvID")->valueint);
            cJSON_AddNumberToObject(back,"cnumber", cJSON_GetObjectItem(cjson,"cnumber")->valueint);
            cJSON_AddStringToObject(back,"groupname", row[0]);
            cJSON_AddNumberToObject(back,"reply",1);
            cJSON_AddNumberToObject(back,"oper",cJSON_GetObjectItem(cjson,"oper")->valueint);

            sprintf(sql,"select usernickname from user where id=%d",cJSON_GetObjectItem(cjson,"recvID")->valueint);
            mysql_query(&mysql,sql);//执行sql语句
            result = mysql_store_result(&mysql);
            row = mysql_fetch_row(result);

            cJSON_AddStringToObject(back,"recvname",row[0]);
            cJSON_AddNumberToObject(back,"case",cJSON_GetObjectItem(cjson, "case")->valueint);
            char* p=cJSON_Print(back);
            // send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
            send(online[i].confd,p,strlen(p),0);
            free(p);
            cJSON_Delete(back);
        }
    }
    else//disagree
    {
        //返回给sendID
        if(online[i].state==1)
        {
            sprintf(sql,"select groupname from chatgroup where cnumber=%d",cJSON_GetObjectItem(cjson,"cnumber")->valueint);
            mysql_query(&mysql,sql);//执行sql语句
            result = mysql_store_result(&mysql);
            row = mysql_fetch_row(result);
		printf("groupname\n");
            cJSON* back=cJSON_CreateObject();
            cJSON_AddNumberToObject(back,"recvID", cJSON_GetObjectItem(cjson,"recvID")->valueint);
            cJSON_AddNumberToObject(back,"cnumber", cJSON_GetObjectItem(cjson,"cnumber")->valueint);
            cJSON_AddNumberToObject(back,"reply",0);
            cJSON_AddStringToObject(back,"groupname", row[0]);
            cJSON_AddNumberToObject(back,"oper",cJSON_GetObjectItem(cjson,"oper")->valueint);
		printf("cjson\n");
            sprintf(sql,"select usernickname from user where id=%d",cJSON_GetObjectItem(cjson,"recvID")->valueint);
            mysql_query(&mysql,sql);//执行sql语句
            result = mysql_store_result(&mysql);
            row = mysql_fetch_row(result);
		printf("usernickname\n");
            cJSON_AddStringToObject(back,"recvname",row[0]);
            cJSON_AddNumberToObject(back,"case",cJSON_GetObjectItem(cjson, "case")->valueint);
            char* p=cJSON_Print(back);
            // send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
            send(online[i].confd,p,strlen(p),0);
            free(p);
            cJSON_Delete(back);
        }
    }
	mysql_close(&mysql);//关掉sql句柄
    return 0;
}

bool quitGroup(cJSON* cjson)
{
    char sql[SQL_LEN];//存放sql语句
    MYSQL mysql;//sql句柄
    MYSQL_RES *result;//sql查询结果
    MYSQL_ROW row;//sql查询结果的一行
    int check = -1;
    cJSON* back=cJSON_CreateObject();

    mysql_init(&mysql);//句柄初始化

    if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
    {
        printf("%s\n",mysql_error(&mysql));
        return -1;
    }
    printf("数据库连接\n");
    
    sprintf(sql,"select status from conversation where cnumber=%d and id1=%d", cJSON_GetObjectItem(cjson,"cnumber")->valueint, cJSON_GetObjectItem(cjson,"ID")->valueint);
    mysql_query(&mysql,sql);//执行sql语句
    result = mysql_store_result(&mysql);
    row = mysql_fetch_row(result);

    check = atoi(row[0]);

    if(check == 0)//群员
    {
        sprintf(sql,"delete from conversation where id1=%d and cnumber=%d", cJSON_GetObjectItem(cjson,"ID")->valueint, cJSON_GetObjectItem(cjson,"cnumber")->valueint);
        mysql_query(&mysql,sql);//执行sql语句
        printf("(群员)已删除conversation\n");
        
        cJSON_AddStringToObject(back,"message","您已退出群聊");
    }
    else if(check == 1)//管理员
    {
        sprintf(sql,"delete from conversation where id1=%d and cnumber=%d", cJSON_GetObjectItem(cjson,"ID")->valueint, cJSON_GetObjectItem(cjson,"cnumber")->valueint);
        mysql_query(&mysql,sql);//执行sql语句
        printf("(管理员)已删除conversation\n");

        sprintf(sql,"update chatgroup set adminID=%d where cnumber=%d", 0, cJSON_GetObjectItem(cjson,"cnumber")->valueint);
        mysql_query(&mysql,sql);//执行sql语句
        printf("(管理员)信息已删除chatgroup\n");

        cJSON_AddStringToObject(back,"message","您卸任管理员并已退出群聊");
    }
    else
    {
        sprintf(sql,"delete from conversation where cnumber=%d", cJSON_GetObjectItem(cjson,"cnumber")->valueint);
        mysql_query(&mysql,sql);//执行sql语句
        printf("(群主)已删除所有conversation\n");

        sprintf(sql,"delete from chatgroup where cnumber=%d", cJSON_GetObjectItem(cjson,"cnumber")->valueint);
        mysql_query(&mysql,sql);//执行sql语句
        printf("(群主)所有信息已删除chatgroup\n");

        sprintf(sql,"drop table history%d", -cJSON_GetObjectItem(cjson,"cnumber")->valueint);
        mysql_query(&mysql,sql);//执行sql语句
        printf("(群主)history 已删除\n");

        cJSON_AddStringToObject(back,"message","您已解散群聊");
    }
    char* p=cJSON_Print(back);
    send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
    free(p);
    cJSON_Delete(back);
    mysql_free_result(result);
	mysql_close(&mysql);//关掉sql句柄
    return 0;
}


bool kickMember(cJSON* cjson)//踢人
{
    char sql[SQL_LEN];//存放sql语句
    MYSQL mysql;//sql句柄
    MYSQL_RES *result;//sql查询结果
    MYSQL_ROW row;//sql查询结果的一行
    int check = -2;
    cJSON* back=cJSON_CreateObject();

    mysql_init(&mysql);//句柄初始化

    if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
    {
        printf("%s\n",mysql_error(&mysql));
        return -1;
    }

    printf("数据库连接\n");

    sprintf(sql,"select ownerID,adminID from chatgroup where cnumber=%d", cJSON_GetObjectItem(cjson,"cnumber")->valueint);
    mysql_query(&mysql,sql);//执行sql语句
    result = mysql_store_result(&mysql);
    row = mysql_fetch_row(result);

    if(cJSON_GetObjectItem(cjson,"sendID")->valueint == atoi(row[0])||cJSON_GetObjectItem(cjson,"sendID")->valueint == atoi(row[1]))
    {
        if(cJSON_GetObjectItem(cjson,"recvID")->valueint == atoi(row[0])||cJSON_GetObjectItem(cjson,"recvID")->valueint == atoi(row[1]))
            check = -1;
        else
            check = 1;
    }
    else
        check = 0;
    printf("%d\n",check);
    if(check)
    {
        sprintf(sql,"delete from conversation where cnumber=%d and id1=%d", cJSON_GetObjectItem(cjson,"cnumber")->valueint, cJSON_GetObjectItem(cjson,"recvID")->valueint);
        mysql_query(&mysql,sql);//执行sql语句
        printf("request done\n");
        cJSON_AddStringToObject(back,"message","移除成员成功");
    }
    else if(check == -1)
    {
        cJSON_AddStringToObject(back,"message","不能移除群主或管理员");
    }
    else
    {
        cJSON_AddStringToObject(back,"message","权限不足");
    }
    char* p=cJSON_Print(back);
    send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
    free(p);
    cJSON_Delete(back);
    mysql_free_result(result);
	mysql_close(&mysql);//关掉sql句柄
    return 0;
}


bool setAdmin(cJSON* cjson)//设置admin
{
    char sql[SQL_LEN];//存放sql语句
    MYSQL mysql;//sql句柄
    MYSQL_RES *result;//sql查询结果
    MYSQL_ROW row;//sql查询结果的一行
    int check = cJSON_GetObjectItem(cjson, "oper")->valueint;

    cJSON* back=cJSON_CreateObject();

    mysql_init(&mysql);//句柄初始化

    if(mysql_real_connect(&mysql,"localhost",SQLUSER,SQLPASSWD,SQLNAME,0,NULL,0)==NULL)//连接数据库
    {
        printf("%s\n",mysql_error(&mysql));
        return -1;
    }

    printf("数据库连接\n");

    sprintf(sql,"select ownerID from chatgroup where cnumber=%d", cJSON_GetObjectItem(cjson,"cnumber")->valueint);
    mysql_query(&mysql,sql);//执行sql语句
    result = mysql_store_result(&mysql);
    row = mysql_fetch_row(result);
    
    if(cJSON_GetObjectItem(cjson, "sendID")->valueint!=atoi(row[0]))
    {
        cJSON_AddStringToObject(back,"message","权限不足");
    }
    else if(check)
    {
        sprintf(sql,"select adminID from chatgroup where cnumber=%d", cJSON_GetObjectItem(cjson,"cnumber")->valueint);
        mysql_query(&mysql,sql);//执行sql语句
        result = mysql_store_result(&mysql);
        row = mysql_fetch_row(result);
        if(atoi(row[0])!=0)
        {
            //返回已有管理员
            cJSON_AddStringToObject(back,"message","已有管理员");
        }
        else
        {
            sprintf(sql,"update chatgroup set adminID=%d where cnumber=%d", cJSON_GetObjectItem(cjson,"recvID")->valueint, cJSON_GetObjectItem(cjson,"cnumber")->valueint);
            mysql_query(&mysql,sql);//执行sql语句
            sprintf(sql,"update conversation set status=%d where id1=%d and cnumber=%d", 1, cJSON_GetObjectItem(cjson,"recvID")->valueint, cJSON_GetObjectItem(cjson,"cnumber")->valueint);
            mysql_query(&mysql,sql);//执行sql语句
            cJSON_AddStringToObject(back,"message","设置成功");
        }
    }
    else//remove
    {
        sprintf(sql,"select adminID from chatgroup where cnumber=%d", cJSON_GetObjectItem(cjson,"cnumber")->valueint);
        mysql_query(&mysql,sql);//执行sql语句
        result = mysql_store_result(&mysql);
        row = mysql_fetch_row(result);
        if(atoi(row[0])!=cJSON_GetObjectItem(cjson,"recvID")->valueint)
        {
            cJSON_AddStringToObject(back,"message","该成员不是管理员");
        }
        else
        {
            sprintf(sql,"update chatgroup set adminID=%d where cnumber=%d", 0, cJSON_GetObjectItem(cjson,"cnumber")->valueint);
            mysql_query(&mysql,sql);//执行sql语句
            sprintf(sql,"update conversation set status=%d where id1=%d and cnumber=%d", 0, cJSON_GetObjectItem(cjson,"recvID")->valueint, cJSON_GetObjectItem(cjson,"cnumber")->valueint);
            mysql_query(&mysql,sql);//执行sql语句
            cJSON_AddStringToObject(back,"message","成功解除管理员");
        }
    }
    char* p=cJSON_Print(back);
    send(cJSON_GetObjectItem(cjson,"confd")->valueint,p,strlen(p),0);
    free(p);
    cJSON_Delete(back);
    mysql_free_result(result);
	mysql_close(&mysql);//关掉sql句柄
    return 0;
}

void *Classify(void *arg)
{
        cJSON* cjson=(cJSON*)arg;
        int Case = cJSON_GetObjectItem(cjson,"case")->valueint;//得到case码
	printf("处理case为%d的消息",Case);

        switch(Case)//分类讨论
	{
                case 1:
			Register(cjson);
                    	break;
                case 2:
                	Log_in(cjson);
                	break;
		case 3:
			Send_person(cjson);
			break;
		case 4:
			Send_group(cjson);
			break;
		case 5:
			Send_User_Info(cjson);
			break;
		case 6:
			Update_User_Info(cjson);
			break;
		case 7:
			searchPerson(cjson);
			break;
		case 8:
			searchGroup(cjson);
			break;
		case 9:
			addFriend(cjson);
			break;
		case 10:
			acceptFriend(cjson);
			break;
		case 11:
			deleteFriend(cjson);
			break;
		case 12:
			createGroup(cjson);
			break;
		case 13:
			sendGroupInv(cjson);
			break;
		case 19:
			sendGroupInv(cjson);
			break;
		case 14:
			operGroupInv(cjson);
			break;
		case 20:
			operGroupInv(cjson);
			break;
		case 15:
			quitGroup(cjson);
			break;
		case 16:
			kickMember(cjson);
			break;
		case 18:
			kickMember(cjson);
			break;
		case 17:
			setAdmin(cjson);
			break;
		case 25:
			setAdmin(cjson);
			break;
	}
	cJSON_Delete(cjson);//释放cJSON
}

void *handClient(void *arg)
{
        char buf[ITEM_LEN][MESSAGE_LEN]={0};//临时存放接受的消息
        int *p=(int*)arg;
        int confd=*p;//将临时的套接字存放到线程中
        int index=0;//临时存放消息的下标
        int x=0;
	int count=0;//为了拆分消息，记录括号个数
        pthread_t tid;//存放线程编号
        while(1)
        {
		printf("已进入新线程，开始接受消息\n");
                memset(buf[index],0,sizeof(buf[index]));
                if(recv(confd,buf[index],sizeof(buf[index]),0)==0)//接受到的消息临时存放到buf
                {
			for(int i=1;i<2000;i++)
			{
				if(online[i].confd==confd)
				{
					online[i].state=0;//下线
					printf("id为%d的客户端下线\n",i);
					break;
				}
			}
			close(confd);//关闭套接字
                        return 0;//线程结束
                }
                else
                {
			printf("从客户端收到%s\n",buf[index]);
                        while(1)//将接受到的客户端消息拆分，依次处理，每处理一个消息，创建一个线程
                        {
				if(buf[index][x]=='{'||buf[index][x]=='[')
				{
					count++;//左括号加
				}
				else if(buf[index][x]=='}'||buf[index][x]==']')
				{
					count--;//右括号减
					if(count==0&&buf[index][x+1]!='\0')//没拆完
					{
						strcpy(buf[(index+1)%100],&buf[index][x+1]);
                                        	buf[index][x+1]='\0';
						cJSON* cjson=cJSON_Parse(buf[index]);
						cJSON_AddNumberToObject(cjson,"confd",confd);//将TCP套接字句柄放入cjson消息中
						printf("拆分得到%s\n",cJSON_Print(cjson));
                                        	pthread_create(&tid,NULL,Classify,cjson);//创建线程
                                        	index++;
                                        	index=index%100;
						x=0;
					}
					else if(count==0)//拆完了
					{
						cJSON* cjson=cJSON_Parse(buf[index]);
                                                cJSON_AddNumberToObject(cjson,"confd",confd);//将TCP套接字句柄放入cjson消息中
						printf("拆分完毕得到%s\n",cJSON_Print(cjson));
                                	        pthread_create(&tid,NULL,Classify,cjson);//创建线程
                        	                index++;
                	                        index=index%100;
        	                                x=0;
	                                        break;
					}
				}
				x++;
                        }
                }
        }
	return 0;
}

int main()
{
        struct sockaddr_in myaddr;//存放本机ip和端口
        struct sockaddr_in cliaddr;//存放客户端ip和端口
        int socklen=sizeof(cliaddr);//cliaddr字节长度
        int confd[100]={0};//临时存放与客户端建立连接的套接字
        int index=0;//临时存放套接字时的下标

        pthread_t tid;//存放多线程编号

        memset(&myaddr,0,sizeof(myaddr));

        myaddr.sin_family=AF_INET;
        myaddr.sin_port=htons(8888);
        myaddr.sin_addr.s_addr=inet_addr("192.168.72.128");//输入本机ip和端口

        int lisfd=socket(AF_INET,SOCK_STREAM,0);//建立一个用于监听的套接字
        if(lisfd==0)
        {
                printf("套接字创建失败\n");//套接字建立失败
                return -1;
        }

        if(bind(lisfd,(struct sockaddr*)&myaddr,sizeof(myaddr))!=0)//用于监听的套接字与本机ip和端口进行绑定
        {
                printf("套接字绑定失败\n");//绑定失败
                return -1;
        }

 	listen(lisfd,10);//开始监听
        printf("开始监听\n");

        while(1)
        {
                confd[index]=accept(lisfd,(struct sockaddr*)&cliaddr,(socklen_t*)&socklen);//与发出请求的客户端建立连接，得到一个与客户端建立连接的套接字,放到临时存放套接字的数组中
                if(confd[index]==-1)
                {
                        printf("建立TCP连接失败\n");//连接失败
                }
                printf("建立TCP连接成功\n");
                printf("ip=%s\tport=%d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));//输出建立连接的客户端的ip和端口

                pthread_create(&tid,NULL,handClient,&confd[index]);//为客户端与服务器的连接建立一个线程，传入线程的参数为与客户端建立连接的套接字(临时)的指针
                index++;//临时存放套接字数组的下标加一
                index=index%100;//下标不超过99
        }
	close(lisfd);
	return 0;
}
