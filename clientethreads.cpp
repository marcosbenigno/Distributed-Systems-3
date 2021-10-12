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

#include <iomanip>

#include <sstream>

#include <string>

#include <chrono>


#define PORT 4444
#define MAXLINE 1024

using namespace std;

// testado no ubuntu

//struct para passar argumentos para threads criadoras de processos 
struct Argumentos {
        int id;
        int loops;
        int segundos;
};


string time_in_HH_MM_SS_MMM() {

    // fonte da funcao: https://stackoverflow.com/questions/24686846/get-current-time-in-milliseconds-or-hhmmssmmm-format
    
    using namespace std::chrono;

    auto now = system_clock::now();

    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    auto timer = system_clock::to_time_t(now);

    std::tm bt = *std::localtime(&timer);

    std::ostringstream oss;

    oss << std::put_time(&bt, "%H-%M-%S"); // HH:MM:SS
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}


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
                        perror("erro na criacao do socket");
                        exit(1);
                }

                memset( & servaddr, 0, sizeof(servaddr));

                //informacao de socket servidor
                servaddr.sin_family = AF_INET;
                servaddr.sin_port = htons(PORT);
                servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
                

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
                        
                        clock_t quandoFizRequest = clock();
                        
                        char respostaDoServidor[4];
                        
                        //espera msg de grant do servidor

                        while (respostaDoServidor[0] != '3') {
                        recvfrom(socketCliente, respostaDoServidor, 4, SOCK_NONBLOCK, (struct sockaddr * ) & servaddr, & len);
                         clock_t agora = clock();
                        double tempo = (double)(agora - quandoFizRequest) * 1000.0 / CLOCKS_PER_SEC;
                        
                        //se tempo >= 10seg, retira com break
                        	if (tempo >= 10000.0) {
                        		i -= 1;
                        		break;
                        	}
                        
                        }
			
			// se mensagem == 3: grant
                        if (respostaDoServidor[0] == '3') {
                        	
                        	//escreve no arquivo .txt
                                ofstream outfile;

                                outfile.open("log_processos.txt", std::ios_base::app);

                                auto t = std::time(nullptr);
                                auto tm = * std::localtime( & t);

                                std::ostringstream oss;
                                oss << std::put_time( & tm, "%d-%m-%Y ");
                                
                                auto str = oss.str();
                                
				 str += time_in_HH_MM_SS_MMM(); 
                                
                                outfile << meuId << ": " << str << endl;
                                outfile.close();
                               
                                cout <<"Processo "<<meuId<<" escreveu: "<< str << endl;
                                sleep(processoInfo -> segundos);
                                
                        }
                        
                        
                        
                        //mensagem de release: <id>|2
                        string release = meuId + "|2";
                        char * releaseFormatado = (char * ) release.c_str();
                        
                        //envia release
                        sendto(socketCliente, (const char * ) releaseFormatado, strlen(releaseFormatado),
                                MSG_CONFIRM, (const struct sockaddr * ) & servaddr,
                                sizeof(servaddr));
                        respostaDoServidor[0] = '0';
                        
                }               

        }
        if (pid != 0) {
                //processo pai
             //   wait(NULL);

        }

}

int main(int argc, char ** argv) {

	//limpando log de processos
	ofstream file;
	file.open("log_processos.txt", ofstream::out | ofstream::trunc);
	file.close();

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
