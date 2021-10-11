#include <stdio.h>

#include <pthread.h>

#include <time.h>

#include <stdlib.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <netdb.h>

#include <stdio.h>

#include <unistd.h> 

#include <string.h> 

#include <unistd.h>

#include <signal.h>

#include <iostream>

#include <bits/stdc++.h>

#include <queue>

#include <semaphore.h>

#define PORT 4444


//fila
//array de conectados

using namespace std;

//struct de mensagem recebida
struct mensagem {
        int processo = 0;
        int metodo;
};

//struct representando um processo
struct processo {
        int processo = -1;
        int metodo = -1;
        int ativo = -1;
        sockaddr_in addr;
        int numeroDeAcessos = 0;
        clock_t quandoPegouLock;
        int estaComLock = -1;
};

//semaforo para funcao regiao de exclusao mutua
sem_t semaforo;

//semaforo para fila de processoas a serem inicializados
sem_t semaforoFilaProcessosAInicializar;

//inicialmente, suporte a 200 processos
vector < struct processo > processos(200);

//fila vai ter os ids dos processos
queue < int > fila;

//fila de processos a serem inicializados (p/ comunicao inical entre funcao de exclusao com a de conexao)
queue < int > filaProcessosAInicializar;

//variavel do socket
int socketServidor;

//armazena o id com o socket no momento
int processoComLock = -1;


struct mensagem * processarMensagem(char * msg) {
        //funcao retorna a mensagem processada a partir do char * msg
        int msgProcessada[2];
        char * ptr;
        strtok_r(msg, "|", & ptr);
        struct mensagem * msgAtual = (struct mensagem * ) malloc(sizeof(struct mensagem * ));
        int * result = (int * ) malloc(sizeof(int));
        msgAtual -> processo = atoi(msg);
        msgAtual -> metodo = atoi(ptr);
        return msgAtual;
}

void writeLog(string entrada) {
	//funcao escreve arquivo de log (append string entrada)
        ofstream outfile;
        outfile.open("log_coordenador.txt", std::ios_base::app);
        std::ostringstream oss;
        oss << entrada;
        auto str = oss.str();
        outfile << str << endl;
        outfile.close();
}


void * exclusaoMutua(void * mensagem) {

	//funcao gere a exclusao mutua
        
        sem_wait( & semaforoFilaProcessosAInicializar);
        int nProcesso = filaProcessosAInicializar.front();
        filaProcessosAInicializar.pop();
	sem_post( & semaforoFilaProcessosAInicializar);
	
	//mensagem de grant: "3"
        char mensagemGrant[4];
        *((int * ) mensagemGrant) = '3';
        
        //socklen_t auxiliar para enviar socket
        socklen_t len;
        len = sizeof(processos[(nProcesso)].addr);
        
        
        //while pra evitar a entrada de dois processos de mesmo id na area critica ao msm tempo
        while (processoComLock == nProcesso);

	//while principla
        while (1) {

                sem_wait( & semaforo);
                
                //condicional abaixo checa estados do processo para filizar a thread
                if (processos[(nProcesso)].ativo == -1 && processos[(nProcesso)].metodo == -1 && processos[(nProcesso)].estaComLock == -1) {
                        sem_post( & semaforo);
                        break;
                }

		//entrada na fila quando metodo request: 1
                if (processos[(nProcesso)].metodo == 1 && processos[(nProcesso)].ativo == -1 && processos[(nProcesso)].estaComLock == -1) {
                	//ativa processo e atualiza numero de acessos
                        processos[(nProcesso)].ativo = 1;
                        processos[(nProcesso)].numeroDeAcessos += 1;
                        
                        //escreve log
                        writeLog(to_string((nProcesso)) + ": 1");
                        
                        if (fila.empty()) {
                        
                        	//envia msg de grant se fila estiver vazia
                                sendto(socketServidor, mensagemGrant, 4, MSG_CONFIRM, (struct sockaddr * ) & (processos[(nProcesso)].addr), len);
                                
                                //atualiza estados de processo e escreve log
                                processoComLock = (nProcesso);
                                writeLog(to_string((nProcesso)) + ": 3");
                                processos[(nProcesso)].quandoPegouLock = clock();
                                processos[(nProcesso)].estaComLock = 1;
                        }
                        fila.push((nProcesso));

                }

		//chega metodo release: 2
                if (processos[(nProcesso)].metodo == 2 && processos[(nProcesso)].ativo == 1 && processos[(nProcesso)].estaComLock == 1) {
                
                	//remove da fila, atualiza estados e escreve log
                        fila.pop();
                        processos[(nProcesso)].metodo = -1;
                        processos[(nProcesso)].ativo = -1;
                        processos[(nProcesso)].estaComLock = -1;
                        writeLog(to_string((nProcesso)) + ": 2");
                        
			
                        if (!fila.empty()) {
				//concede grant ao proximo da fila
                                sendto(socketServidor, mensagemGrant, 4, MSG_CONFIRM, (const struct sockaddr * ) & (processos[fila.front()].addr), sizeof(processos[fila.front()].addr));
                                
                                processos[fila.front()].quandoPegouLock = clock();
                                processos[fila.front()].estaComLock = 1;
                                writeLog(to_string(fila.front()) + ": 3");
                                processoComLock = fila.front();
                        } else {
                        	//atualiza quem esta com lock
                                processoComLock = -1;
                        }
                        sem_post( & semaforo);
                        //finaliza loop
                        break;
                }

		//condicional abaixo checa tempo consumido por quem esta com lock
                if (!fila.empty()) {
                
                        clock_t agora = clock();
                        double tempo = (double)(agora - processos[fila.front()].quandoPegouLock) * 1000.0 / CLOCKS_PER_SEC;
                        //se tempo >= 10seg, retira da fila por inatividade
                        if (tempo >= 10000.0) {
                                processos[fila.front()].estaComLock = -1;
                                processos[fila.front()].metodo = -1;
                                processos[fila.front()].ativo = -1;
                               
                                fila.pop();

                                //concede o lock pro proximo da fila
                                if (!fila.empty()) {
                                
                                        sendto(socketServidor, mensagemGrant, 4, MSG_CONFIRM, (const struct sockaddr * ) & (processos[fila.front()].addr), sizeof(processos[fila.front()].addr));
                                        
                                        processos[fila.front()].quandoPegouLock = clock();
                                        processos[fila.front()].estaComLock = 1;
                                        processoComLock = fila.front();
                                        writeLog(to_string(fila.front()) + ": 3");
                                        
                                } else {
                                        processoComLock = -1;
                                }
                                sem_post( & semaforo);
                                break;
                        }
                }

                sem_post( & semaforo);
        }

}

void * conexao(void * args) {
	//funcao de conexao. recebe as msgs e mantem o socket
        int n;

        char buffer[1024];
        sockaddr_in enderecoServidor, enderecoCliente;
        socklen_t tamanhoDeEndereco, cliLen;
        memset( & enderecoServidor, 0, sizeof(enderecoServidor));
        memset( & enderecoCliente, 0, sizeof(enderecoCliente));
        
        //criacao do socket
        socketServidor = socket(AF_INET, SOCK_DGRAM, 0);
        
        if (socketServidor < 0) {
                cout << "Erro na criacao o socket.A\n";
                exit(1);
        }
        
        //setando endereco do servidor
        enderecoServidor.sin_family = AF_INET;
        enderecoServidor.sin_port = htons(PORT);
        enderecoServidor.sin_addr.s_addr = inet_addr("127.0.0.1");

        int rc = bind(socketServidor, (struct sockaddr * ) & enderecoServidor, sizeof(enderecoServidor));
        if (rc < 0) {
                cout << "Erro no bind" << PORT << endl;
                exit(1);
        }

	//inicializa vetor de processos com struct processo
        for (int i = 0; i < 200; i++) {     
                struct processo processoAtual;
                processos[i] = processoAtual;
        }

	
        vector < pthread_t > threadsExclusao(1000);
        int contador = 0;
        
        //vetor para armazenar ids dos processos e passar para threads de exclusao
        vector < int > ids;
        
        
        while (1) {
                
                memset(buffer, '\0', sizeof(buffer));
                cliLen = sizeof(enderecoCliente);
                
                //esperando msg
                n = recvfrom(socketServidor, buffer, sizeof(buffer), 0, (sockaddr * ) & enderecoCliente, & cliLen);
                
                if (n < 0) {
                        cout << "Nao consegue receber dados \n";
                        continue;
                }

		//processando mensagem recebida 
                struct mensagem * msgAtual = (processarMensagem(buffer));
                int processo = msgAtual -> processo;
                ids.push_back(processo);
                
                //while pra evitar a entrada de dois processos de mesmo id na area critica ao msm tempo
                while (processoComLock == processo && msgAtual -> metodo == 1);

		//condicional para metodo request: 1
                if (processos[processo].ativo == -1 && msgAtual -> metodo == 1) {
			//seta estados
			 sem_wait( & semaforoFilaProcessosAInicializar);
			filaProcessosAInicializar.push(processo);
			 sem_post( & semaforoFilaProcessosAInicializar);
                        processos[processo].processo = processo;
                        processos[processo].metodo = 1;
                        processos[processo].addr = enderecoCliente;
			
			//cria thread de exclusao
                        pthread_create( & (threadsExclusao[contador]), NULL, & exclusaoMutua, NULL);
                        
                        contador++;

		//else if para metodo release: 2
                } else if (processos[processo].ativo == 1 && msgAtual -> metodo == 2) {
                        processos[processo].metodo = 2;
                }

        }
}

void primeiroHandler(int sig) {
        //exibe fila com recebimento de sinal SIGUSR1
        queue < int > copiaFila = fila;
        cout << "Fila"<<endl<<"Inicio: ";
        while (!copiaFila.empty()) {
                cout << " " << copiaFila.front();
                copiaFila.pop();
        }
	cout<<endl;
	cout<<"Emita um signal SIGUSR1 para exibir a fila de pedidos atual\nEmita um sinal SIGUSR2 para exibir quantas vezes um pedido foi atendido\nEmita um sinal SIGCHLD para terminar a execucao."<<endl;
}

void segundoHandler(int sig) {
	//exibe numero de acessos com sinal sinal SIGUSR2
        cout << "(processo, acessos)" << endl;
        for (int i = 1; i < processos.size(); i++) {
                if (processos[i].processo == -1) {
                        break;
                }
                cout << "(" << processos[i].processo << ", " << processos[i].numeroDeAcessos << ")" << endl;

        }
        cout<<endl;
        cout<<"Emita um signal SIGUSR1 para exibir a fila de pedidos atual\nEmita um sinal SIGUSR2 para exibir quantas vezes um pedido foi atendido\nEmita um sinal SIGCHLD para terminar a execucao."<<endl;
}

void terceiroHandler(int sig) {
	//finaliza coordenador com recebimento de sinal SIGUSR
        cout<<"finalizando......."<<endl;
        exit(0);
}

void * interface(void * intervalo) {

        //funcao de interface
        
        //setando funcoes handler
        signal(SIGUSR1, primeiroHandler);
        signal(SIGUSR2, segundoHandler);
        signal(SIGCHLD, terceiroHandler);
        
        cout<<"Emita um signal SIGUSR1 para exibir a fila de pedidos atual\nEmita um sinal SIGUSR2 para exibir quantas vezes um pedido foi atendido\nEmita um sinal SIGCHLD para terminar a execucao."<<endl;
        
        while (1) {
                pause();
        }

}

int main() {

	//limpando log de coordenador
	ofstream file;
	file.open("log_coordenador.txt", ofstream::out | ofstream::trunc);
	file.close();
	
        //inicializacao de semaforo
        sem_init( & semaforo, 0, 1);
        sem_init( & semaforoFilaProcessosAInicializar, 0, 1);
        //criacao de threads
        pthread_t threads[2];
        //interface
        pthread_create( & (threads[1]), NULL, & interface, NULL);
        //conexao
        pthread_create( & (threads[0]), NULL, & conexao, NULL);

        pthread_join(threads[1], NULL);
        pthread_join(threads[0], NULL);

        return 0;
}
