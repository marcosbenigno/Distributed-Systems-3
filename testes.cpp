#include <stdio.h>

#include <time.h>

#include <iostream>

#include <fstream>

#include <vector>

#include <chrono>


using namespace std;

int compararTempos(time_t tempo1, time_t tempo2, int mili1, int mili2) {
        //diferenca entre tempo1 e tempo 2
        
        int diferencaMili = (mili1 - mili2);
        double diferenca = difftime(tempo1, tempo2);
        if (diferenca != 0.0 ) {
        	return diferenca > 0.0 ? 1 : -1;
        } else {
        	if (diferencaMili > 0) {
        		return 1;
        	} else {
        		return -1;
        	}
        } 
        
}

int main(void) {

        int processos, loops;
        cout << "Quantos processos foram utilizados? " << endl;
        cin >> processos;
        cout << "Quantos loops foram utilizados?" << endl;
        cin >> loops;

        int linhas = 0;
        //log gerado por processos
        ifstream file("log_processos.txt");
        string str;
        vector < int > numeroDeEscritas(processos + 1, 0);
        size_t pos = 0;
        int i = 0;

	//variaveis para analisar tempos
        time_t t1, t2;
        struct tm * timeptr, tm1, tm2;
        string lastTime;
        string currentTime;
        int lastMili, currentMili;
        vector < int > comparacaoTempos;
	//--------------Analise de log dos processos----------------//
	
	cout<<"--Log de processos--"<<endl;
        while (std::getline(file, str)) {
        	//iterando linhas de log de processos
                if (i > 0) {
                        lastTime = currentTime;
                        lastMili = currentMili;
                }
                
                linhas += 1;
                
                //split de string de linha
                string linha = str;
                string delimiter = ":";
                string id = linha.substr(0, linha.find(delimiter));
                numeroDeEscritas[stoi(id)] += 1;
                pos = linha.find(delimiter);
                linha.erase(0, pos + delimiter.length() + 1);
                
                //tratando miliseconds
                string copiaLinha = linha;
                currentMili = stoi(copiaLinha.substr(copiaLinha.find(".") + 1, copiaLinha.length() + 1));
                
                currentTime = linha.substr(0, linha.find("."));
                
                
                
                if (i > 0) {

                        //comparacao de tempos de linhas consecutivas
                        strptime((char * ) lastTime.c_str(), "%d-%m-%Y %H-%M-%S", & tm1);
                        strptime((char * ) currentTime.c_str(), "%d-%m-%Y %H-%M-%S", & tm2);

                        t1 = mktime( & tm1);
                        t2 = mktime( & tm2);
                        

			//resultado de compararTempos em vetor
                        comparacaoTempos.push_back(compararTempos(t2, t1, currentMili, lastMili));

                }

                i += 1;
        }

        for (int k = 0; k < comparacaoTempos.size(); k++) {
		//Checagem de comparacao de horarios
                if (comparacaoTempos[k] != 1) {
                        cout << "Ordem de horarios esta incorreta - nao 'e crescente." << endl;
                        break;
                }

                if (k == comparacaoTempos.size() - 1) {
                        cout << "Ordem de horarios esta correta - 'e crescente" << endl;
                }
        }
        
        
	//checagem numero de linhas no log dos processos
        if (linhas == (processos * loops)) {
                cout << "Numero de linhas esta correto." << endl;
        } else {
                cout << "Numero de linhas esta incorreto." << endl;
        }
        
	//exibicao de quantas vezes cada processo escreveu
        for (int k = 1; k < numeroDeEscritas.size(); k++) {
                cout << k << " escreveu " << numeroDeEscritas[k] << " vezes" << endl;
        }

        //--------------Analise de log do coordenador----------------//

        cout << endl << "--Log do coordenador--" << endl;
        ifstream fileCoord("log_coordenador.txt");
        string line;
        
        //vetor armazena um vetor para cada processo. nele, sao armazenados os metodos executados ordenados
        vector < vector < int >> processosEMetodos(processos);
        
        //vetores armazenam ordem de msgs de requests e releases
        vector < int > requests;
        vector < int > releases;

	//inicializacao de vetores
        for (int k = 0; k < processosEMetodos.size(); k++) {
                vector < int > vetor;
                processosEMetodos[k] = vetor;
        }


        while (std::getline(fileCoord, line)) {
        	//tratamento da linha
                string linha = line;
                std::string delimiter = ":";
                std::string metodo = linha.substr(linha.find(delimiter) + 2, linha.length());
                std::string processo = line.substr(0, line.find(delimiter));

		//armazenamento de metodo no vetor do processo
                processosEMetodos[stoi(processo) - 1].push_back(stoi(metodo));

		//armazenamento de id de processos que executaram request e release, em ordem
                if (stoi(metodo) == 1) {
                        requests.push_back(stoi(processo));
                } else if (stoi(metodo) == 2) {
                        releases.push_back(stoi(processo));
                }

        }


	//checagem de ordem de metodos em cada processo
        for (int k = 0; k < processosEMetodos.size(); k++) {
        	if (processosEMetodos[k].size() > 0) {
        		for (int j = 0; j < processosEMetodos[k].size() - 2; j+=3) {
        			if (!(processosEMetodos[k][j] == 1 && processosEMetodos[k][j+1] == 3 && processosEMetodos[k][j+2] == 2)) {
					cout << "Ordem de metodos esta incorreta." << endl;
                                	break;        			
        			}
        		}
        	}

                if (k == processosEMetodos.size() - 1) {
                        cout << "Ordem de metodos correta." << endl;
                }
        }
        
        //checagem na ordem de execucao de request e release de processos
        if (requests.size() == releases.size()) {
                for (int k = 0; k < requests.size(); k++) {
                        if (requests[k] != releases[k]) {
                                cout << "Ordem de release/request incorreta por processos." << endl;
                                break;
                        }

                        if (k == requests.size() - 1) {
                                cout << "Ordem de release/request correta por processos." << endl;
                        }
                }
        } else {
                cout << "Ordem de release/request incorreta por processos." << endl;
        }

        return 1;
}
