#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#define PROJECT_NAME "Xerces Server"
#define MAX_PORT_NUMBER UINT16_MAX

uint16_t ReadPortFromString(char* str);

int main(int argc, char *argv[]) {

    uint16_t PORT = 1959;

    // Obtenido de GNU C Library (glibc) manual https://sourceware.org/glibc/manual/latest/html_node/Example-of-Getopt.html
    int opt = 0;
    while((opt = getopt(argc, argv, "p:")) != -1){
        switch (opt) {
            case 'p':
                PORT = ReadPortFromString(optarg);
                if(PORT == 0){
                    fprintf(stderr, "ERROR: Puerto debe ser un entero de 1-%d\n", MAX_PORT_NUMBER);
                    return 1;
                }
                break;
            case '?':
                if (optopt == 'p')
                    fprintf (stderr, "Opcion -%c require un argumento.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Opcion desconocida '-%c'.\n", optopt);
                else
                    fprintf (stderr, "Caracter de opcion desconocido `\\x%x'.\n", optopt);
                return 1;
        }
    }

    printf("El puerto es: %d\n",PORT);
    
    return 0;
}

uint16_t ReadPortFromString(char* str){
    uint16_t num = 0;
    // Ejemplo #4 de https://www.geeksforgeeks.org/c/convert-string-to-int-in-c/
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] >= 48 && str[i] <= 57) {
            printf("%d\n",num);
            if(num * 10 + (str[i] - 48) > 65535){                    
                return 0;
            }
            num = num * 10 + (str[i] - 48);
        }
        else {
            return 0;
        }
    }
    return num;
}
