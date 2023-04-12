#include<stdio.h>
#include<stdlib.h>
#include<locale.h>
//include<math.h>

typedef struct fat_BS{
    unsigned char bootjmp[3];
    unsigned char oem_name[8];
    unsigned short bytes_per_sector;
    unsigned char sectors_per_cluster;
    unsigned short reserved_sector_count;
    unsigned char table_count;
    unsigned short root_entry_count;
    unsigned short total_sectors_16;
    unsigned char media_type;
    unsigned short table_size_16;
    unsigned short sectors_per_track;
    unsigned short head_side_count;
    unsigned int hidden_sector_count;
    unsigned int total_sectors_32;
    unsigned char extended_section[54];
}__attribute__((packed))fat_BS_t;

typedef struct info_arq{
    unsigned char name[8];
    unsigned char ext[3];
    unsigned char file_type;
    int size;
    int first_cluster;
}__attribute__((packed))info_arq_t;

void print_arq();
int verifica();

int main(){
    //função para printar acento no vscode
    setlocale(LC_ALL, "Portuguese_Brasil");
    system("chcp 65001> nul");

    FILE *fp;
    fat_BS_t boot_record;
    char nome_arq[100], escolha[25];
    unsigned short *fat;
    unsigned char *dir_raiz, current_entry[32];
    int size, cluster, entry_count=0, pos=0, procura=0;

    printf("Digite o nome do arquivo: ");
    scanf("%s", nome_arq);

    if((fp = fopen(nome_arq, "rb")) == NULL){
		  printf("\nERRO! O arquivo não pode ser aberto!");
		  return 0;
	}

    //lê o arquivo para pegar as informações
    fseek(fp, 0, SEEK_SET);
    fread(&boot_record, sizeof(fat_BS_t), 1, fp);

    //cálculo do nº de setores para o diretório raiz
    int root_dir_sectors = boot_record.root_entry_count * 32 / boot_record.bytes_per_sector;
    //cálculo da posição dos setores do diretório raiz
    unsigned int root = boot_record.reserved_sector_count + (boot_record.table_count * boot_record.table_size_16);
    //cálculo dos setores da área de dados
    unsigned int data = root + (boot_record.root_entry_count * 32 / boot_record.bytes_per_sector);

    printf("\n========================FAT 16========================\n");
    printf("\nNúmero de fat's: %4x\n", boot_record.table_count);
    printf("Setores por fat: %4hd\n", boot_record.table_size_16);
    printf("Bytes por setor: %4hd\n", boot_record.bytes_per_sector);
    printf("Setor por cluster: %4x\n", boot_record.sectors_per_cluster);
    printf("Número de setores reservados: %4hd\n", boot_record.reserved_sector_count);
    printf("Número de entradas do diretório raiz: %4hd\n", boot_record.root_entry_count);
    printf("Setores do diretório raiz: %4hd\n\n", root_dir_sectors);
  
    //alocação para a fat
    fat = malloc(boot_record.table_size_16 * boot_record.bytes_per_sector);
    //coloca o ponteiro no inicio da fat
    fseek(fp, boot_record.reserved_sector_count * boot_record.bytes_per_sector, SEEK_SET);
    fread(fat, boot_record.table_size_16 * boot_record.bytes_per_sector, 1, fp); 

    //alocação para o diretório raiz
    dir_raiz = malloc(boot_record.root_entry_count * 32);
    //coloca o ponteiro no inicio do diretorio raiz
    fseek(fp, boot_record.reserved_sector_count * boot_record.bytes_per_sector + (boot_record.table_count * boot_record.table_size_16 * boot_record.bytes_per_sector), SEEK_SET);
    fread(dir_raiz, root_dir_sectors * 32, 1, fp); 

    info_arq_t *arquivos = malloc(sizeof(info_arq_t) * root_dir_sectors);
    info_arq_t file;

    while(entry_count < boot_record.root_entry_count){
        if(dir_raiz[entry_count * 32] == 0x00) break; //verifica se a entrada é vazia

        if(dir_raiz[entry_count * 32] == 0xE5){ //verifica se a entrada ta marcada como excluida
            entry_count++;
            continue;
        }

        if(dir_raiz[(entry_count * 32) + 11] == 0x0F){ //verifica se o valor da entrada 11 é long fitamame
            entry_count++;
            continue;
        }

        for(int j=0; j<32; j++){ //cópia dos 32bytes do diretorio raiz
            current_entry[j] = dir_raiz[j + (entry_count * 32)];
        }

        //Verifica se é um diretório (10) ou arquivo (20)
        if(current_entry[11] == 0x10 || current_entry[11] == 0x20){
            //pega tam do arquivo em bytes
            size = current_entry[31] << 24 | current_entry[30] << 16 | current_entry[29] << 8 | current_entry[28];
            // pega o nº do 1º cluster
            cluster = current_entry[27] << 8 | current_entry[26];
        }

        for(int k=0; k<8; k++){ //pega o nome
            file.name[k] = current_entry[k];
        }

        for(int k=0; k<3; k++){ //pega a extensao
            file.ext[k] = current_entry[k + 8];
        }

        file.file_type = current_entry[11]; //salva o tipo: arq ou dir
        file.size = size; //salva o tamanho
        file.first_cluster = cluster; //salva o 1º cluster
        arquivos[pos] = file; //salva os dados da struct no vetor
        pos++; //prox posição do vetor
        entry_count++; //prox entrada
    }

    printf("==================================================\n\n");
    for(int k=0; k<pos; k++){
        printf("[%d] ", k);
        print_arq(arquivos[k]);
    }

    while(procura == 0){
        printf("\nDigite o Nº do arquivo que deseja abrir: ");
        scanf("%s", escolha);
        int index = atoi(escolha); //converte o input para um número inteiro
        if(index>=0 && index<pos){ //vê se é válido
            //se o input for um índice válido entre 0 e pos-1, seleciona o arquivo pelo índice
            procura = 1;
            pos = index;
        }
        else{
            for(int k=0; k<pos; k++){
                if(verifica(arquivos[k], escolha)){ //verificação do nome do arquivo com o valor da entrada
                    //se o input do usuário for um índice válido entre 0 e pos-1, seleciona o arquivo pelo índice
                    procura = 1;
                    pos = k;
                }
            }
        }
    }
    printf("\n==================================================\n");
    printf("\nSua escolha: ");
    file = arquivos[pos];
    print_arq(file);

    //calculo do Nº de cluster para salvar o arquivo
    int n = (file.size / boot_record.bytes_per_sector) + 1;
    //guarda os números de clustes que o arquivo usa
    int file_clusters[n];
    int clusters = 1;
    file_clusters[0] = file.first_cluster;

    for(int i=0; i<n; i++){
        //mapeia os cluster do arquivo com a tabela fat
        file_clusters[i + 1] = fat[file_clusters[i]];
        //verifica o valor da tabela
        if(file_clusters[i + 1] >= 0xFFFF) break;
        
        clusters++;
    }

    //calculo do tamanho em bytes de um cluster
    int bytes_per_cluster = boot_record.sectors_per_cluster * boot_record.bytes_per_sector;
    if(file.file_type == 0x10){
        unsigned char DATA[bytes_per_cluster];

        //pega o inicio do cluster
        fseek(fp, (data + ((file_clusters[0] - 2) * boot_record.sectors_per_cluster)) * boot_record.bytes_per_sector, SEEK_SET);
        fread(&DATA, bytes_per_cluster, 1, fp);
        
        info_arq_t dir_file;
        entry_count = 0;

        //para acessar o subisiretório
        while(entry_count < bytes_per_cluster / 32){
            if(DATA[entry_count * 32] == 0x00) break; //verifica se é vazio

            if(DATA[entry_count * 32] == 0xE5){ //verifica se ta marcado como excluido
                entry_count++;
                continue;
            }

            if(DATA[(entry_count * 32) + 11] == 0x0F){ //verifica se é long fitamame
                entry_count++;
                continue;
            }

            for(int j=0; j<32; j++){ //armazena os dados da entrada
                current_entry[j] = DATA[j + (entry_count * 32)];
            }

            //Verifica se é um diretório (10) ou arquivo (20)
            if(current_entry[11] == 0x10 || current_entry[11] == 0x20){
                //pega tam do arquivo em bytes
                size = current_entry[31] << 24 | current_entry[30] << 16 | current_entry[29] << 8 | current_entry[28];
                // pega o nº do 1º cluster
                cluster = current_entry[27] << 8 | current_entry[26];
            }

            for(int k=0; k<8; k++){ //pega o nome
                dir_file.name[k] = current_entry[k];
            }

            for(int k=0; k<3; k++){ //pega a extensao
                dir_file.ext[k] = current_entry[k + 8];
            }

            dir_file.file_type = current_entry[11]; //salva o tipo: arq ou dir
            dir_file.size = size; //salva o tamanho
            dir_file.first_cluster = cluster; //salva o 1º cluster
            print_arq(dir_file); //printa os arquivos
            entry_count++; //prox entrada
        }
    } //fim subdiretório

    else if(file.file_type == 0x20) //caso seja um arquivo
    {
        unsigned char DATA[file.size];
        //clusters do arquivo
        for (int i=0; i<clusters; i++){
            //pega o inicio do cluster
            fseek(fp, (data + ((file_clusters[i] - 2) * boot_record.sectors_per_cluster)) * boot_record.bytes_per_sector, SEEK_SET);
            //caluclo do Nº de bytes lidos
            int num_bytes = i * bytes_per_cluster;

            if (i == clusters - 1){ //verifica se é o último cluster
                //lê os dados do arquivo do cluster
                fread(&DATA[num_bytes], file.size - (num_bytes), 1, fp);
            }
            else //se for o último cluster lê o resto dos bytes
                fread(&DATA[num_bytes], bytes_per_cluster, 1, fp);
        }
        printf("\n");
        for(int i=0; i<file.size; i++){
            printf("%c", DATA[i]);
        }
    } 
    free(dir_raiz);
    free(fat);
    fclose(fp);
}

void print_arq(info_arq_t entry){
    for(int i=0; i<8; i++){ //para o nome
        if(entry.name[i] == ' ') break; //se tiver esapaço sai
        printf("%c", entry.name[i]);
    }
    if(entry.file_type == 0x20) printf("."); //se for um arquivo printa um ponto para a extensao

    for(int i=0; i<3; i++){ //para a extensao
        if(entry.ext[i] == ' ') break; //se tiver espaço sai
        printf("%c", entry.ext[i]);
    }
    printf(" - tamanho: %d bytes\n", entry.size);
}

int verifica(info_arq_t entry, char *escolha){ //compara o número de entrada com o nome do arquivo
    char fname[12];
    int tam = 0;

    for(int i=0; i<8; i++){ //para o nome
        if (entry.name[i] == ' ') break; //se tiver espaço sai
        fname[i] = entry.name[i];
        tam++;
    }
    if(entry.file_type == 0x20){ //se for um arquivo adiciona um ponto no vet para a extensao
        fname[tam] = '.';
        tam++;
    }
    for(int i=0; i<3; i++){ //para a extensao
        if (entry.ext[i] == ' ') break;
        fname[tam] = entry.ext[i];
        tam++;
    }
    for(int i=0; i<tam; i++){ //faz a comparação final
        if(fname[i] != escolha[i]) return 0;
    }
    return 1;
}