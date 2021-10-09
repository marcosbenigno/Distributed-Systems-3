#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <string.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <arpa/inet.h>

#include <netinet/in.h>

#include <string.h>

#include <iostream>

#include <wait.h>

#include <fstream>

#include <vector>

#include <iomanip>

#include <ctime>

#include <sstream>


#define PORT 4444
#define MAXLINE 1024

using namespace std;

//struct para passar argumentos para threads criadoras de processos 
struct Argumentos {
        int id;
        int loops;
        int segundos;
};

void * funcao(void * argumentos) {
	//funcao que cria processo com fork
	
        struct Argumentos * processoInfo = (Argumentos * ) argumentos;

        int pid = fork();
        
        if (pid < 0) {
                cout<<"erro no fork"<<endl;
                exit(1);
                
        } else if (pid == 0) {
        
        	//processo filho
                string meuId = to_string(processoInfo -> id);
		
		
                int socketCliente;

                struct sockaddr_in servaddr;

                //criacao de socket
                if ((socketCliente = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                        perror("socket creation failed");
                        exit(1);
                }

                memset( & servaddr, 0, sizeof(servaddr));

                //informacao de socket servidor
                servaddr.sin_family = AF_INET;
                servaddr.sin_port = htons(PORT);
                servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

                int n;
                socklen_t len;
                len = sizeof(servaddr);

		//mensagem de request: <id>|1
		
                string request = meuId + "|1";
                
                //mensagem convertida para formato correto
                char * requestFormatado = (char * ) request.c_str();
                
                //loop
                for (int i = 0; i < processoInfo -> loops; i++) {
                	//envia msg de request
                        sendto(socketCliente, (const char * ) requestFormatado, strlen(requestFormatado), MSG_CONFIRM, (const struct sockaddr * ) & servaddr, sizeof(servaddr));
                        
                        

                        char respostaDoServidor[4];
                        
                        //espera msg de grant do servidor
                        recvfrom(socketCliente, respostaDoServidor, 4, 0, (struct sockaddr * ) & servaddr, & len);

			// se mensagem == 3: grant
                        if (respostaDoServidor[0] == '3') {
                        	
                        	//escreve no arquivo .txt
                                ofstream outfile;

                                outfile.open("log_processos.txt", std::ios_base::app);

                                auto t = std::time(nullptr);
                                auto tm = * std::localtime( & t);

                                std::ostringstream oss;
                                oss << std::put_time( & tm, "%d-%m-%Y %H-%M-%S");
                                auto str = oss.str();

                                
                                outfile << meuId << ": " << str << endl;
                                
                                cout <<"Processo "<<meuId<<" escreveu: "<< str << endl;
                                
                        }
                        
                        sleep(processoInfo -> segundos);
                        
                        //mensagem de release: <id>|2
                        string release = meuId + "|2";
                        char * releaseFormatado = (char * ) release.c_str();
                        
                        //envia release
                        sendto(socketCliente, (const char * ) releaseFormatado, strlen(releaseFormatado),
                                MSG_CONFIRM, (const struct sockaddr * ) & servaddr,
                                sizeof(servaddr));
                        
                }               

        }
        if (pid != 0) {
                //processo pai

        }

}

int main(int argc, char ** argv) {


        int iteracoes, delay, loop;
        cout << "Entre com o numero de processos " << endl;
        cin >> iteracoes;
        cout << "Entre com o numero de segundos de delay " << endl;
        cin >> delay;
        cout << "Entre com o numero de loops de cada processo " << endl;
        cin >> loop;

        pthread_t threads[iteracoes];

        int i;
        vector < struct Argumentos > argumentos(iteracoes);
        //foram utilizadas threads para criacao dos processos para contornar comportamento serial
        //de criacao execucao dos processos filhos apenas com um loop e um for
        
	//criacao de thread
        for (i = 1; i < iteracoes + 1; i++) {
                argumentos[i - 1].loops = loop;
                argumentos[i - 1].id = i;
                argumentos[i - 1].segundos = delay;
                pthread_create( & (threads[i - 1]), NULL, & funcao, & argumentos[i - 1]);
        }

        for (i = 0; i < iteracoes; i++) {
                pthread_join(threads[i], NULL);
        }
        

        return 0;
}
