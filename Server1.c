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

bool Register(cJSON* cjson)
{
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

        printf("sql语句生成\n");

        mysql_query(&mysql,sql);//执行sql语句
        result=mysql_store_result(&mysql);//sql查询结果

        printf("查询完毕\n");

        if(row=mysql_fetch_row(result))//如果查到东西了
        {
		printf("username cant use\n");//用户名已存在
        }
        else
        {
		sprintf(sql,"%s","select count(id) from user");//查找user表中有多少条记录
		mysql_query(&mysql,sql);//执行sql语句
        	result=mysql_store_result(&mysql);//sql查询结果
		row=mysql_fetch_row(result);

		int i=atoi(row[0]);
		i++;
		printf("%d",i);//i为user表中的记录数＋1

		sprintf(sql,"%s(%d,'%s','%s')","insert into user values",i,cJSON_GetObjectItem(cjson,"username")->valuestring,cJSON_GetObjectItem(cjson,"password")->valuestring);//向user表添加一条记录，包括新注册用户的帐号密码以及为他分配的id
		mysql_query(&mysql,sql);//执行sql语句

		printf("注册成功\n");
        }

        mysql_close(&mysql);//关掉sql句柄
        return 0;
}

bool Log_in(cJSON* cjson)
{
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

	printf("sql语句生成\n");

    	mysql_query(&mysql,sql);//执行sql语句
    	result=mysql_store_result(&mysql);//sql查询结果

	printf("查询完毕\n");

    	if(row=mysql_fetch_row(result))//如果查到东西了
    	{
    		if(strcmp(cJSON_GetObjectItem(cjson,"password")->valuestring,row[2])==0)//比较密码是否一样
    		{
    			printf("log success\n");
			int i=atoi(row[0]);
			printf("%d\n",i);
    			online[i].id=i;//将id存进去
	    		online[i].state=1;//更新登录状态
			online[i].confd=cJSON_GetObjectItem(cjson,"confd")->valueint;//将当前TCP套接字存进去
    			strcpy(online[i].username,cJSON_GetObjectItem(cjson,"username")->valuestring);//将帐号存进去
    			printf("%d %d %s\n",online[i].id,online[i].state,online[i].username); 
		}
		else
		{
			printf("log failed\n");//登录失败
		}
	}
	else
	{
		printf("cant find username\n");//没有该用户
	}

	mysql_close(&mysql);//关掉sql句柄
	return 0;
}

bool Send_person(cJSON* cjson)
{
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

	int i=cJSON_GetObjectItem(cjson,"sendid")->valueint;

	if(online[i].state==1)//如果接受者在线
	{
		printf("%s\n",cJSON_GetObjectItem(cjson,"message")->valuestring);
		//发送消息之后将双方的index＋＋？？？？？？？？？？？？？？
	}
	else//如果接受者不在线
	{
		sprintf(sql,"select count(index) from history%d",cJSON_GetObjectItem(cjson,"cnumber")->valueint);//查找会话号对应的历史记录有多少条
                mysql_query(&mysql,sql);//执行sql语句
                result=mysql_store_result(&mysql);//sql查询结果
                row=mysql_fetch_row(result);

		sprintf(sql,"insert into history%d values(%d,'%s',%d,'%s')",cJSON_GetObjectItem(cjson,"cnumber")->valueint,cJSON_GetObjectItem(cjson,"cnumber")->valueint,cJSON_GetObjectItem(cjson,"message")->valuestring,atoi(row[0])+1,);//添加一条新的历史记录，时间的字符串还没有生成？？？？？？？？？？？？
        	mysql_query(&mysql,sql);//执行sql语句
	}
}

void *Classify(void *arg)
{
        cJSON* cjson=(cJSON*)arg;
        int Case = cJSON_GetObjectItem(cjson,"case")->valueint;//得到case码

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
	printf("新线程\n");
        while(1)
        {
                memset(buf[index],0,sizeof(buf[index]));
                if(recv(confd,buf[index],sizeof(buf[index]),0)==0)//接受到的消息临时存放到buf
                {
                        printf("connect stop\n");//如果客户端断开连接
			for(int i=1;i<2000;i++)
			{
				if(online[i].confd==confd)
				{
					online[i].state=0;//下线
					break;
				}
			}
			close(confd);//关闭套接字
                        return 0;//线程结束
                }
                else
                {
			printf("%s收到\n",buf[index]);
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
						printf("1\n");
						strcpy(buf[(index+1)%100],&buf[index][x+1]);
                                        	buf[index][x+1]='\0';
						cJSON* cjson=cJSON_Parse(buf[index]);
						cJSON_AddNumberToObject(cjson,"confd",confd);//将TCP套接字句柄放入cjson消息中
                                        	pthread_create(&tid,NULL,Classify,cjson);//创建线程
                                        	index++;
                                        	index=index%100;
						x=0;
					}
					else if(count==0)//拆完了
					{
						printf("拆完了\n");
						cJSON* cjson=cJSON_Parse(buf[index]);
                                                cJSON_AddNumberToObject(cjson,"confd",confd);//将TCP套接字句柄放入cjson消息中
                                	        pthread_create(&tid,NULL,Classify,cJSON_Parse(buf[index]));//创建线程
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
                printf("socket error\n");//套接字建立失败
                return -1;
        }

        if(bind(lisfd,(struct sockaddr*)&myaddr,sizeof(myaddr))!=0)//用于监听的套接字与本机ip和端口进行绑定
        {
                printf("bind error\n");//绑定失败
                return -1;
        }

 		listen(lisfd,10);//开始监听
        printf("start listen\n");

        while(1)
        {
                confd[index]=accept(lisfd,(struct sockaddr*)&cliaddr,(socklen_t*)&socklen);//与发出请求的客户端建立连接，得到一个与客户端建立连接的套接字,放到临时存放套接字的数组中
                if(confd[index]==-1)
                {
                        printf("TCP accept error\n");//连接失败
                }
                printf("connect success\n");
                printf("client ip=%s\tport=%d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));//输出建立连接的客户端的ip和端口

                pthread_create(&tid,NULL,handClient,&confd[index]);//为客户端与服务器的连接建立一个线程，传入线程的参数为与客户端建立连接的套接字(临时)的指针
                index++;//临时存放套接字数组的下标加一
                index=index%100;//下标不超过99
        }
	close(lisfd);
	return 0;
}
