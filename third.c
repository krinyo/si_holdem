#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <locale.h>

#include <ncurses.h>

/*DEBUG*/
#ifdef DEBUG
#define DEBUG_PRINT     1
#else
#define DEBUG_PRINT     0
#endif

#define MAX_CONNECTIONS 2
#define MIN_CONNECTIONS 2
#define WAIT_TIME       5
#define MAX_ATTEMPTS    5
#define SERVER_PORT     22222
#define CLIENT_BUF_SIZE 4096

#define SUIT_SIZE       8
#define SUITS_COUNT     4
#define NUMBER_SIZE     3
#define NUMBERS_COUNT   13
#define DECK_SIZE       52
#define PL_CDS_COUNT    2
#define NICKNAME_SIZE   25
#define TABLE_CDS_COUNT 5
#define TABLE_PLS_COUNT 2
#define TABLE_MSG_SIZE  25
#define START_MONEY     10000

const char* const FILENAME = "players.txt";

const char suits[SUITS_COUNT][SUIT_SIZE] = 
    { "\u2660", "\u2665", "\u2666", "\u2663" };

const char numbers[NUMBERS_COUNT][NUMBER_SIZE] = 
    { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" };

struct card{
    char suit[SUIT_SIZE];
    char number[NUMBER_SIZE];
    int suit_int;
    int number_int;
};
void show_card(struct card cd)
{
    printf("|%s %s|\n", cd.number, cd.suit);
}

struct deck{
    struct card *first_card;
    int current_size;
    struct card *start_pointer;
};
void fill_deck(struct deck *main_deck)
{
    int i, j, idx;
    struct card *first_card = main_deck->first_card;
    for(i = 0; i < NUMBERS_COUNT; i ++){
        for(j = 0; j < SUITS_COUNT; j ++){
            idx = i + (j * NUMBERS_COUNT);
            sprintf(first_card[idx].number, "%s", numbers[i]);
            sprintf(first_card[idx].suit, "%s", suits[j]);
            first_card[idx].suit_int = j;
            first_card[idx].number_int = i;
        }
    }
}
struct deck *init_deck()
{
    struct deck *main_deck = malloc(sizeof(struct deck));
    main_deck->first_card = malloc(sizeof(struct card) * DECK_SIZE);
    if(main_deck->first_card == NULL){
        exit(1);
    }
    fill_deck(main_deck);
    main_deck->current_size = DECK_SIZE;
    main_deck->start_pointer = main_deck->first_card;
    return main_deck;
}
void show_all_deck_cards(struct deck main_deck){
    int i;
    for(i = 0; i < main_deck.current_size; i ++){
        show_card(main_deck.first_card[i]);
        //printf("%i \n", i);
    }
}
void swap_card_positions(struct deck *main_deck, int idx_fm, int idx_to){
    struct card temp;
    struct card *first_card = main_deck->first_card;

    memcpy(&temp, first_card+idx_fm, sizeof(struct card));
    memcpy(first_card+idx_fm, first_card+idx_to, sizeof(struct card));
    memcpy(first_card+idx_to, &temp, sizeof(struct card));
}
void shuffle_deck(struct deck *main_deck){
    int i, r;
    for(i = 0; i < DECK_SIZE; i ++){
        r = (rand() % (DECK_SIZE - 1)) + 1;
        swap_card_positions(main_deck, i, r);
    }
}
void pop_deck_card(struct deck *main_deck){
    main_deck->first_card ++;
    main_deck->current_size --;
}
void pop_deck_cards(struct deck *main_deck, int count){
    int i;
    for(i = 0; i < count; i ++){
        pop_deck_card(main_deck);
    }
}
void reset_deck(struct deck *main_deck){
    main_deck->first_card = main_deck->start_pointer;
    main_deck->current_size = DECK_SIZE;
}
struct player{
    struct card cards[PL_CDS_COUNT];
    char nickname[NICKNAME_SIZE];
    unsigned int session_money;
    unsigned int total_money;
    int combo_strengh;
    int high_card;
    int session;

    int table_pos;
};
struct table{
    struct card cards[TABLE_CDS_COUNT];
    struct player players[TABLE_PLS_COUNT];
    int players_count;
    unsigned int table_money;
    int table_cards_count;
    int big_blind_pos;
    int big_blind_sum;
    char table_message[TABLE_MSG_SIZE];
};
struct table *init_table()
{
    struct table *main_table = malloc(sizeof(struct table));
    main_table->players_count = 0;
    main_table->table_money = 0;
    main_table->table_cards_count = 0;
    sprintf(main_table->table_message, "%s", "INITED TABLE");
    return main_table;
}
void show_table_info(struct table main_table)
{
    int i, j;
    printf("TABLE INFO------------\n");
    for(i = 0; i < main_table.table_cards_count; i ++){
        show_card(main_table.cards[i]);
    }
    for(i = 0; i < main_table.players_count; i ++){
        printf("NICKNAME: %s \n", main_table.players[i].nickname);
        printf("Session money: %i \n", main_table.players[i].session_money);
        printf("Total money: %i \n", main_table.players[i].total_money);
        for(j = 0; j < main_table.table_cards_count; j ++){
            show_card(main_table.cards[j]);
        }
    }
    printf("TABLE MONEY = %i\n", main_table.table_money);
    printf("TABLE CARDS COUNT = %i\n", main_table.table_cards_count);
    printf("TABLE MESSAGE %s \n", main_table.table_message);

    printf("TABLE INFO END------------\n");
}
/*server*/
struct client{
    struct sockaddr_in address;
    int descriptor;
    unsigned long from_ip;
    unsigned short from_port;
    char buf[CLIENT_BUF_SIZE];
    char nickname[NICKNAME_SIZE];
    int connected;
    FILE *fd;
    SCREEN *scr;
    WINDOW *win;
};
struct server{
    int server_socket;
    struct sockaddr_in socket_params;
	int desc_count;
    struct client clients[MAX_CONNECTIONS];
};
void init_server_socket(struct server *main_server)
{
    main_server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    main_server->socket_params.sin_family = AF_INET;
    main_server->socket_params.sin_port = htons(SERVER_PORT);
    main_server->socket_params.sin_addr.s_addr = htonl(INADDR_ANY);
}
void try_to_bind_socket(struct server *main_server)
{
	int opt = 1;
	setsockopt(main_server->server_socket, SOL_SOCKET,
			SO_REUSEADDR, &opt, sizeof(opt));
    
	while(-1 == bind(main_server->server_socket,
                    (struct sockaddr*)&(main_server->socket_params),
                        sizeof(main_server->socket_params)))
    {
        if(DEBUG_PRINT){printf("Trying to bind server socket\n");}
        sleep(2);
    }
    if(DEBUG_PRINT){printf("Successfully binded!\n");}
}
struct server *init_server()
{
    struct server *main_server = malloc(sizeof(struct server));
    
    init_server_socket(main_server);
    try_to_bind_socket(main_server);
    listen(main_server->server_socket, MAX_CONNECTIONS);

    main_server->desc_count = 0;

    if(DEBUG_PRINT){ printf("Server is up. Listening on: %d:%d\n", INADDR_ANY, SERVER_PORT); }
    return main_server;
}
void show_client_info(struct client curr_client)
{
    int ip = curr_client.from_ip;
    printf("Descriptor = %i\n", curr_client.descriptor);
    printf("From %d.%d.%d.%d:%d\n", (ip>>24 & 0xff), (ip>>16 & 0xff), (ip>>8 & 0xff), (ip & 0xff), curr_client.from_port);
    printf("Nickname is: %s \n", curr_client.nickname);
}
void show_clients_info(struct server main_server)
{
    int i;
    for(i = 0; i < main_server.desc_count; i ++){
        show_client_info(main_server.clients[i]);
    }
}
void get_client_name(struct client *curr_client)
{
    write(curr_client->descriptor, "What is your name?", 19);
    read(curr_client->descriptor, curr_client->nickname, NICKNAME_SIZE);
}
void get_clients_name(struct server *main_server)
{
    int i;
    for(i = 0; i < main_server->desc_count; i ++){
        get_client_name(&(main_server->clients[i]));
    }
}
void get_clients(struct server *main_server)
{
    fd_set readfds;
    struct timeval timeout;
    int attempts = 0;
    int result;
    while(attempts < MAX_ATTEMPTS || main_server->desc_count < MIN_CONNECTIONS){
        FD_ZERO(&readfds);
        FD_SET(main_server->server_socket, &readfds);

        timeout.tv_sec = WAIT_TIME;
        timeout.tv_usec = 0;

        result = select(main_server->server_socket + 1, &readfds, NULL, NULL, &timeout);
        if(-1 == result){
            printf("Error: select() failed!\n");
        }
        else if(0 == result){
            printf("Timeout: No new connections.\n");
            sleep(5);
        }
        else{
            if(main_server->desc_count >= MAX_CONNECTIONS){
                printf("REACHED MAXIMUM PLAYERS");
                break;
            }
            // Accept the incoming connection
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_socket = accept(main_server->server_socket, (struct sockaddr *) &client_addr, &client_addr_len);

            //add client in empty slot of array
            int i;
            for(i = 0; i < MAX_CONNECTIONS; i++) {
                if(main_server->clients[i].connected == 0) {
                    break;
                }
            }
            if(i == MAX_CONNECTIONS) {
                printf("Error: No empty slot found!\n");
                continue;
            }

            // Add the new client descriptor to the main_server structure and increment the descriptor count
            main_server->clients[i].address = client_addr;
            main_server->clients[i].from_ip = ntohl(client_addr.sin_addr.s_addr);
            main_server->clients[i].from_port = ntohs(client_addr.sin_port);
            main_server->clients[i].descriptor = client_socket;
            main_server->clients[i].connected = 1;
            main_server->desc_count++;
            printf("ACCEPTED NEW CLIENT\n");
            show_client_info(main_server->clients[i]);
            sleep(5);
        }
        attempts++;
    }
}
void check_or_create_players_file() {
    char file_path[100];
    sprintf(file_path, "./%s", FILENAME);
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        file = fopen(file_path, "w");
        if (file == NULL) {
            printf("Error: Unable to create file %s\n", file_path);
            exit(1);
        }
        printf("Created file %s\n", file_path);
        fclose(file);
    } else {
        printf("File %s already exists\n", file_path);
        fclose(file);
    }
}
/*common funcs*/
void put_card_to_table(struct table *main_table, struct deck *main_deck)
{
    memcpy(&(main_table->cards[main_table->table_cards_count]),
                main_deck->first_card, sizeof(struct card));
    pop_deck_card(main_deck);
    main_table->table_cards_count ++;
}
void put_cards_to_table(struct table *main_table, struct deck *main_deck, int count)
{
    int i;
    for(i = 0; i < count; i ++){
        put_card_to_table(main_table, main_deck);
    }
}
int main()
{
    srand(time(NULL));
    struct deck *main_deck = init_deck();
    struct table *main_table = init_table();
    struct server *main_server = init_server();

    get_clients(main_server);
    get_clients_name(main_server);
    show_clients_info(*main_server);
    sleep(5);
    //fill_deck(main_deck);
    //shuffle_deck(main_deck);
    //show_all_deck_cards(*main_deck);
    //put_cards_to_table(main_table, main_deck, TABLE_CDS_COUNT);
    //show_table_info(*main_table);

}