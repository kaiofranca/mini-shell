//SHELL DE LUCAS LIMA E KAIO FRANÇA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define MAX_LEN 1024
#define MAX_ARGS 100

void processar_comando(char *args[]);
void meu_ls(char *args[]);
void meu_cp(char *origem, char *destino);
void meu_cat(char *arquivo);
void meu_mv(char *origem, char *destino);
void executar_outros(char *args[]);
void imprimir_info_arquivo(char *path, char *nome_arquivo);

int main() {
    char linha[MAX_LEN];
    char *args[MAX_ARGS];

    while (1) {
        printf("KL-shell> ");
        
        if (fgets(linha, MAX_LEN, stdin) == NULL) {
            break;
        }

        linha[strcspn(linha, "\n")] = 0;

        int i = 0;
        char *ptr = linha;
        
        while (*ptr != '\0' && i < MAX_ARGS - 1) {
            while (*ptr == ' ' || *ptr == '\t') ptr++;
            
            if (*ptr == '\0') break;

            if (*ptr == '"') {
                ptr++;
                args[i] = ptr;
                
                while (*ptr != '\0' && *ptr != '"') ptr++;
                
                if (*ptr == '"') {
                    *ptr = '\0';
                    ptr++;
                }
            } else {
                args[i] = ptr;
                while (*ptr != '\0' && *ptr != ' ' && *ptr != '\t') ptr++;
                
                if (*ptr != '\0') {
                    *ptr = '\0';
                    ptr++;
                }
            }
            i++;
        }
        args[i] = NULL;
        
        if (args[0] == NULL) continue;

        if (strcmp(args[0], "exit") == 0) {
            exit(0);
        }
        
        processar_comando(args);
    }
    return 0;
}

void processar_comando(char *args[]) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            chdir(getenv("HOME")); 
        } else {
            if (chdir(args[1]) != 0) {
                perror("Erro ao mudar diretorio"); 
            }
        }
        return;
    }

    if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("Erro no pwd");
        }
        return;
    }

    if (strcmp(args[0], "mkdir") == 0) {
        if (args[1] == NULL) {
            printf("Uso: mkdir <nome_diretorio>\n");
        } else {
            if (mkdir(args[1], 0755) != 0) {
                perror("Erro no mkdir");
            }
        }
        return;
    }

    if (strcmp(args[0], "rmdir") == 0) {
        if (args[1] == NULL) {
            printf("Uso: rmdir <nome_diretorio>\n");
        } else {
            if (rmdir(args[1]) != 0) {
                perror("Erro no rmdir");
            }
        }
        return;
    }

    if (strcmp(args[0], "rm") == 0) {
        if (args[1] == NULL) {
            printf("Uso: rm <arquivo>\n");
        } else {
            if (unlink(args[1]) != 0) {
                perror("Erro no rm");
            }
        }
        return;
    }
    
    if (strcmp(args[0], "mv") == 0) {
        if (args[1] == NULL || args[2] == NULL) {
            printf("Uso: mv <origem> <destino>\n");
        } else {
            meu_mv(args[1], args[2]);
        }
        return;
    }

    executar_outros(args);
}

void meu_ls(char *args[]) {
    const char *caminho = ".";
    int mostrar_ocultos = 0;
    int formato_longo = 0;
    
    int i = 1;
    while (args[i] != NULL) {
        if (strcmp(args[i], "-a") == 0) mostrar_ocultos = 1;
        else if (strcmp(args[i], "-l") == 0) formato_longo = 1;
        else if (strcmp(args[i], "-la") == 0 || strcmp(args[i], "-al") == 0) {
            mostrar_ocultos = 1;
            formato_longo = 1;
        }
        else caminho = args[i];
        i++;
    }

    DIR *d = opendir(caminho);
    struct dirent *dir;

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (!mostrar_ocultos && dir->d_name[0] == '.') continue;

            if (formato_longo) {
                imprimir_info_arquivo((char*)caminho, dir->d_name);
            } else {
                printf("%s  ", dir->d_name);
            }
        }
        if (!formato_longo) printf("\n"); 
        closedir(d);
    } else {
        perror("Erro no ls");
    }
}

void meu_cp(char *origem, char *destino) {
    int fd_orig, fd_dest;
    char buffer[4096];
    ssize_t bytes;
    struct stat st;
    char caminho_final[1024]; 

    if (stat(destino, &st) == 0 && S_ISDIR(st.st_mode)) {
        
        char *nome_arquivo = strrchr(origem, '/');
        if (nome_arquivo != NULL) nome_arquivo++; 
        else nome_arquivo = origem;

        sprintf(caminho_final, "%s/%s", destino, nome_arquivo);
        fd_dest = open(caminho_final, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else {
        fd_dest = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }

    if (fd_dest < 0) { 
        perror("[ERRO] Falha ao criar destino"); 
        return; 
    }

    fd_orig = open(origem, O_RDONLY);
    if (fd_orig < 0) { 
        perror("[ERRO] Falha ao abrir origem"); 
        close(fd_dest); 
        return; 
    }
    
    int total_copiado = 0;
    while ((bytes = read(fd_orig, buffer, sizeof(buffer))) > 0) {
        if (write(fd_dest, buffer, bytes) != bytes) {
            perror("[ERRO] Falha na escrita");
            break;
        }
        total_copiado += bytes;
    }

    close(fd_orig);
    close(fd_dest);
}

void meu_cat(char *arquivo) {
    int fd = open(arquivo, O_RDONLY);
    char buffer[4096];
    ssize_t bytes;

    if (fd < 0) {
        perror("Erro ao abrir arquivo");
        return;
    }

    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, bytes);
    }
    printf("\n");
    close(fd);
}

void meu_mv(char *origem, char *destino) {
    struct stat st;
    char caminho_final[1024];

    if (stat(destino, &st) == 0 && S_ISDIR(st.st_mode)) {
        
        char *nome_arquivo = strrchr(origem, '/');
        if (nome_arquivo != NULL) {
            nome_arquivo++;
        } else {
            nome_arquivo = origem;
        }

        sprintf(caminho_final, "%s/%s", destino, nome_arquivo);
        
        if (rename(origem, caminho_final) != 0) {
            perror("Erro no mv (movendo para pasta)");
        }
    } else {
        if (rename(origem, destino) != 0) {
            perror("Erro no mv");
        }
    }
}

void executar_outros(char *args[]) {
    if (strcmp(args[0], "ls") == 0) {
        meu_ls(args); 
        return;
    }
    if (strcmp(args[0], "cp") == 0) {
        if (args[1] && args[2]) meu_cp(args[1], args[2]);
        else printf("Uso: cp <origem> <destino>\n");
        return;
    }
    if (strcmp(args[0], "cat") == 0) {
        if (args[1]) meu_cat(args[1]);
        else printf("Uso: cat <arquivo>\n");
        return;
    }

    pid_t pid = fork();

    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("Comando não encontrado ou falha na execução");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("Erro ao criar processo");
    } else {
        wait(NULL);
    }
}

void imprimir_info_arquivo(char *path, char *nome_arquivo) {
    struct stat fileStat;
    char caminho_completo[1024];

    sprintf(caminho_completo, "%s/%s", path, nome_arquivo);

    if (stat(caminho_completo, &fileStat) < 0) {
        perror("Erro ao ler stat");
        return;
    }

    printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
    printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
    printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
    printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
    printf(" ");

    printf("%ld ", fileStat.st_nlink);

    struct passwd *pw = getpwuid(fileStat.st_uid);
    printf("%s ", pw ? pw->pw_name : "unknown");

    struct group *gr = getgrgid(fileStat.st_gid);
    printf("%s ", gr ? gr->gr_name : "unknown");

    printf("%5ld ", fileStat.st_size);

    char data[100];
    struct tm *tm_info = localtime(&fileStat.st_mtime);
    strftime(data, sizeof(data), "%b %d %H:%M", tm_info);
    printf("%s ", data);

    printf("%s\n", nome_arquivo);
}