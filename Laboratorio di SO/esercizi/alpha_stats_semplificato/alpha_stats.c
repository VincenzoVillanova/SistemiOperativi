#include "../lib-misc.h"
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>

typedef enum
{
    CONTROLLORE,
    AL,
    MZ
} tnames;

typedef struct
{
    char c;
    int alf[26];
    bool done;
    sem_t sem[3];
} contatore;

typedef struct
{
    int numerov;
    pthread_t tid;
    tnames tnames;
    contatore *cont;
} thread_data;

void init_SHARED(contatore *arg)
{
    int err;
    arg->c = '\0';
    memset(arg->alf, 0, sizeof(arg->alf));
    arg->done = 0;
    if ((err = sem_init(&arg->sem[CONTROLLORE], 0, 1)) != 0)
    {
        exit_with_err("sem_init", err);
    }
    if ((err = sem_init(&arg->sem[AL], 0, 0)) != 0)
    {
        exit_with_err("sem_init", err);
    }
    if ((err = sem_init(&arg->sem[MZ], 0, 0)) != 0)
    {
        exit_with_err("sem_init", err);
    }
}

void *controllore(void *arg)
{
    int err;
    int contatore = 0;
    thread_data *td = (thread_data *)(arg);
    while (1)
    {
        if ((err = sem_wait(&td->cont->sem[CONTROLLORE])) != 0)
        {
            exit_with_err("sem_wait", err);
        }

        td->cont->c = rand() % (122 - 97 + 1) + 97;
        contatore++;
        printf("[C] Ho generato la lettera: %c\n", td->cont->c);
        if (td->cont->c < 'm')
        {
            if ((err = sem_post(&td->cont->sem[AL])) != 0)
            {
                exit_with_err("sem_post", err);
            }
        }
        else
        {
            if ((err = sem_post(&td->cont->sem[MZ])) != 0)
            {
                exit_with_err("sem_post", err);
            }
        }

        if (contatore == td->numerov)
        {
            td->cont->done = 1;
            if ((err = sem_post(&td->cont->sem[AL])) != 0)
            {
                exit_with_err("sem_post", err);
            }
            if ((err = sem_post(&td->cont->sem[MZ])) != 0)
            {
                exit_with_err("sem_post", err);
            }
            printf("[C] Statistiche: ");
            for (int i = 0; i < 26; i++)
            {
                if ((i % 4 && i != 0) == 0)
                {
                    printf("\n");
                }
                printf("%c: %d\t", i + 'a', td->cont->alf[i]);
            }
            printf("\n");
            break;
        }
    }

    pthread_exit(NULL);
}

void *al(void *arg)
{
    int err;
    char val;
    thread_data *td = (thread_data *)(arg);
    while (1)
    {
        if ((err = sem_wait(&td->cont->sem[AL])) != 0)
        {
            exit_with_err("sem_wait", err);
        }

        if (td->cont->done)
        {
            break;
        }

        val = td->cont->c - 'a';
        td->cont->alf[(int)val]++;
        printf("[AL] Ho incrementato il contatore della lettera: %c\n", td->cont->c);
        if ((err = sem_post(&td->cont->sem[CONTROLLORE])) != 0)
        {
            exit_with_err("sem_post", err);
        }
    }

    pthread_exit(NULL);
}

void *mz(void *arg)
{
    int err;
    char val;
    thread_data *td = (thread_data *)(arg);
    while (1)
    {
        if ((err = sem_wait(&td->cont->sem[MZ])) != 0)
        {
            exit_with_err("sem_wait", err);
        }
        if (td->cont->done)
        {
            break;
        }

        val = td->cont->c - 'a';
        td->cont->alf[(int)val]++;
        printf("[MZ] Ho incrementato il contatore della lettera: %c\n", td->cont->c);
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
        printf("Usage %s <numero volte (ex 3)>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (atoi(argv[1]) <= 0)
    {
        printf("Numero volte maggiori e diversi da 0 (ex 3)\n");
        exit(EXIT_FAILURE);
    }

    int err;
    thread_data td[3];
    contatore *lettere = malloc(sizeof(contatore));

    init_SHARED(lettere);

    for (int i = 0; i < 3; i++)
    {
        td[i].numerov = atoi(argv[1]);
        td[i].tnames = i;
        td[i].cont = lettere;
    }

    srand(time(NULL));

    if ((err = pthread_create(&td[CONTROLLORE].tid, NULL, (void *)controllore, (void *)&td[CONTROLLORE])) != 0)
    {
        exit_with_err("pthread_create", err);
    }
    if ((err = pthread_create(&td[AL].tid, NULL, (void *)al, (void *)&td[AL])) != 0)
    {
        exit_with_err("pthread_create", err);
    }
    if ((err = pthread_create(&td[MZ].tid, NULL, (void *)mz, (void *)&td[MZ])) != 0)
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
    {
        sem_destroy(&td->cont->sem[i]);
    }

    free(lettere);
    exit(EXIT_SUCCESS);
}
