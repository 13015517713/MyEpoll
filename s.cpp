/*该文件实现服务器的监听
 *
 *
 * 
 */
#include<stdio.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<errno.h>
#include<string.h>
#include<iostream>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
#include<list>
using namespace std;
const int maxn = 1e2+10;
int port=80;
char host[maxn]="localhost";
list<int> clientList;
void setAddr(struct sockaddr_in &addr,char *host,int port){
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port); //转换成网络序（大端）
	addr.sin_addr.s_addr=inet_addr("127.0.0.1");
}
void perr(const char *s){
	perror(s);
	exit(-1);
}
void setnonblocking(int sockfd){
	fcntl(sockfd,F_SETFL,fcntl(sockfd,F_GETFD,0)|O_NONBLOCK);
	return;
}
int sendBroadMessage(int nowfd){ //收到信息先接收信息 ，然后再发给list里面的所有
	char message[1024];
	bzero(message,sizeof(message));
	int len=recv(nowfd,message,1024,0);
	if(len==0){ //客户端关闭了链接
		close(nowfd);
		clientList.remove(nowfd);
		printf("关闭了一个链接");
	}
	else{
		list<int>::iterator it;
		int cnt=0;
		for(it=clientList.begin();it!=clientList.end();it++){
			if(send(*it,message,1024,0)<0){
				printf("%d\n",*it);
				perror("send error");
			} 
			else{
				printf("%d\n发送成功",*it);
			}
		}
	}
	return len;
}
void addfd(int epollfd,int fd,bool flag){// flag用来设置水平还是垂直
	struct epoll_event ev;
	ev.data.fd=fd;
	ev.events=EPOLLIN;
	if(flag){ //设置水平触发
		ev.events|=EPOLLET;
	}
	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
	setnonblocking(fd);
	printf("fd added to epoll!\n\n"); 
	return;

}
int main(){
	int sta=0;
	int sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock<0) perr("socket出错");
	struct sockaddr_in serverAddr,clientAddr;
	bzero(&serverAddr,sizeof(serverAddr));
	bzero(&clientAddr,sizeof(clientAddr));
	setAddr(serverAddr,host,port);
	//	sta=listen(sock,5);
	//	if(sta<0) perr("listen");
	if(bind(sock,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0) perr("bind");	
	sta=listen(sock,5);
	if(sta<0) perr("listen");	
	socklen_t addrLength = sizeof(struct sockaddr_in);


	static struct epoll_event events[1024];

	int epfd=epoll_create(1); //这个参数大于0就行，没啥用
	addfd(epfd,sock,true);
	while(true){
		int epollEventCnt=epoll_wait(epfd,events,1024,-1);
		//这里感觉不应该设置成阻塞
		if(epollEventCnt<0) perr("epollWait");
		printf("epollCnt:%d\n",epollEventCnt);
		for(int i=0;i<epollEventCnt;i++){
			int now=events[i].data.fd;
			if(now==sock){ //监听的得到了信息
				struct sockaddr_in cli;
				socklen_t cliLen=sizeof(cli);
				int newfd=accept(sock,(struct sockaddr*)&cli,&cliLen);
				printf("From:%s:%d\n",inet_ntoa(cli.sin_addr),ntohs(cli.sin_port));				
				addfd(epfd,newfd,true); //这里服务器只设置读入权限
				clientList.push_back(newfd);
				printf("%d描述符添加\n",newfd);
				char message[1024]="Welcome to tellHouse!\n";
				sta=send(newfd,message,1024,0);
				if(sta<0) perr("send error");
			}
			else{
				//别的链接来消息了，这个消息要发给所有的链接。
				sta=sendBroadMessage(now);
				if(sta<0) perr("运行出错");
			}
		}
	}	
	close(epfd);
	close(sock);
	return 0;



}

