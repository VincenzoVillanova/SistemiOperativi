#include "../lib-misc.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

typedef enum
{
    CONTROLLORE,
    PARI,
    DISPARI
} thread_n;

typedef struct
{
    int numero;
    int verifica[2];
    bool done;
    sem_t sem[3];
} contatore;

typedef struct
{
    char *pathname;
    pthread_t tid;
    thread_n thread_n;
    contatore *cont;
} thread_data;

void init_SHARED(contatore *sh)
{
    int err;
    memset(sh->verifica, 0, sizeof(sh->verifica));
    sh->done = 0;
    sh->numero = 0;
    if ((err = sem_init(&sh->sem[CONTROLLORE], 0, 1)) != 0)
    {
        exit_with_err("sem_init", err);
    }
    if ((err = sem_init(&sh->sem[PARI], 0, 0)) != 0)
    {
        exit_with_err("sem_init", err);
    }
    if ((err = sem_init(&sh->sem[DISPARI], 0, 0)) != 0)
    {
        exit_with_err("sem_init", err);
    }
}

void controllore(void *arg)
{
    int err;
    thread_data *td = (thread_data *)(arg);
    FILE *file = fopen(td->pathname, "r");
    int num = 0;
    if (!file)
    {
        exit_with_sys_err("fopen");
    }

    while (fscanf(file, "%d", &num) == 1)
    {
        if ((err = sem_wait(&td->cont->sem[CONTROLLORE])) != 0)
        {
            exit_with_err("sem_wait", err);
        }

        td->cont->numero = num;

        printf("[C] Generato il numero: %d\n", td->cont->numero);

        if ((td->cont->numero % 2) == 0)
        {
            if ((err = sem_post(&td->cont->sem[PARI])) != 0)
            {
                exit_with_err("sem_post", err);
            }
        }
        else
        {
            if ((err = sem_post(&td->cont->sem[DISPARI])) != 0)
            {
                exit_with_err("sem_post", err);
            }
        }
    }
    td->cont->done = 1;
    if ((err = sem_post(&td->cont->sem[PARI])) != 0)
    {
        exit_with_err("sem_post", err);
    }
    if ((err = sem_post(&td->cont->sem[DISPARI])) != 0)
    {
        exit_with_err("sem_post", err);
    }
    printf("[C] I risultati sono P = %d, D = %d \n", td->cont->verifica[0], td->cont->verifica[1]);
    fclose(file);
    pthread_exit(NULL);
}

void pari(void *arg)
{
    int err;
    thread_data *td = (thread_data *)(arg);

    while (1)
    {
        if ((err = sem_wait(&td->cont->sem[PARI])) != 0)
        {
            exit_with_err("sem_wait", err);
        }

        if (td->cont->done)
        {
            break;
        }

        td->cont->verifica[0]++;
        printf("[P] Incrementato il contatore dei numeri pari\n");
        if ((err = sem_post(&td->cont->sem[CONTROLLORE])) != 0)
        {
            exit_with_err("sem_post", err);
        }
    }

    pthread_exit(NULL);
}

void dispari(void *arg)
{
    int err;
    thread_data *td = (thread_data *)(arg);

    while (1)
    {
        if ((err = sem_wait(&td->cont->sem[DISPARI])) != 0)
        {
            exit_with_err("sem_wait", err);
        }

        if (td->cont->done)
        {
            break;
        }

        td->cont->verifica[1]++;
        printf("[D] Incrementato il contatore dei numeri dispari\n");
        if ((err = sem_post(&td->cont->sem[CONTROLLORE])) != 0)
        {
            exit_with_err("sem_post", err);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage %s <nomefile.txt>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int err;

    thread_data td[3];
    contatore *numeri = malloc(sizeof(contatore));

    init_SHARED(numeri);

    for (int i = 0; i < 3; i++)
    {
        td[i].pathname = argv[1];
        td[i].cont = numeri;
        td[i].thread_n = i;
    }

    srand(time(NULL));

    err = pthread_create(&td[CONTROLLORE].tid, NULL, (void *)controllore, (void *)&td[CONTROLLORE]);
    if (err)
    {
        exit_with_err("pthread_create", err);
    }

    err = pthread_create(&td[PARI].tid, NULL, (void *)pari, (void *)&td[PARI]);
    if (err)
    {
        exit_with_err("pthread_create", err);
    }

    err = pthread_create(&td[DISPARI].tid, NULL, (void *)dispari, (void *)&td[DISPARI]);
    if (err)
    {
        exit_with_err("pthread_create", err);
    }

    for (int i = 0; i < 3; i++)
    {
        if ((err = pthread_join(td[i].tid, NULL)) != 0)
        {
            exit_with_err("pthread_join", err);
        }
    }

    for (int i = 0; i < 3; i++)
        sem_destroy(&numeri->sem[i]);

    free(numeri);
    exit(EXIT_SUCCESS);
}