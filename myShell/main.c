#include "log.h"
#include "shell.h"
int main(int argc,char* argv[]) 
{
    setbuf(stdout, NULL);
    freopen(argv[1],"r",stdin);
    log_init(&Log); //��ʼ��Log
    prefix(); //��ʾ·��
    size_t len=0;
    char* buffer=NULL;
    while(getline(&buffer,&len,stdin)!=-1) {//��ȡ����
        buffer[strlen(buffer)-1]='\0';//ĩλ����Ϊ0
        if(!execute(buffer)) {//ִ������
            break;
        }
	
        prefix();
        free(buffer);//�ͷ��ڴ�
        buffer=NULL;
    }

    if(buffer!=NULL){//������ֹ�Ĵ���
        free(buffer);
        buffer=NULL;
    }
    log_destroy(&Log);
    return 0;
}
