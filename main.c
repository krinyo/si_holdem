#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//DECK
#define DECK_SIZE       52
#define SUITS_SIZE      4
#define NUMBERS_SIZE    13
#define ONE_NUMBER_SIZE 3
#define UNICODE_EL_SIZE 4
//TABLE
#define MAX_TABLE_CARDS  5
#define MAX_PLAYERS     2
#define MIN_PLAYERS     2
#define TABLE_SIZE_X    16
#define TABLE_SIZE_Y    8
#define START_STATE     1
#define START_SES_MONEY 0
#define START_TAB_CARDS 0
#define PREFLOP_PUT     0
#define FLOP_PUT        3
#define TURN_PUT        1
#define RIVER_PUT       1
//PLAYER
#define PLAYER_CARD_SUM 2
#define MAX_NAME_SIZE   256
#define TEST_PLAYER_MON 1000
//SERVER
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define SERVER_PORT     14888
#define MAX_CONNECTIONS 2
#define RECONNECT_COUNT	5
/*DEBUG*/
#ifdef DEBUG
#define DEBUG_PRINT     1
#else
#define DEBUG_PRINT     0
#endif

const char suits[SUITS_SIZE][UNICODE_EL_SIZE] = {"♠", "♥", "♦", "♣"};
const char numbers[NUMBERS_SIZE][ONE_NUMBER_SIZE] = 
        {"2", "3", "4", "5", "6", "7", "8", 
        "9", "10", "J", "Q", "K", "A"};
//-------------------------------------
struct card{
        char number[ONE_NUMBER_SIZE];
        char suit[UNICODE_EL_SIZE];
        struct card *next_card_ptr;
};
//------------------------------------
struct deck{
	int size;
	struct card first_card[DECK_SIZE];	
};

//------------------------------------

struct player{
	int position;
	unsigned int player_money;
};
//------------------------------------

struct table{
	struct deck *deck_ptr;
	struct player table_players[MAX_PLAYERS];
	struct card table_cards[MAX_TABLE_CARDS];
	unsigned int session_money;
};
//------------------------------------

struct server{
	int server_socket;
	struct sockaddr_in socket_params;
	struct table *main_table;
	int descriptors[MAX_PLAYERS];	
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
        while(-1 == bind(main_server->server_socket,
                                (struct sockaddr*)&(main_server->socket_params),
                                sizeof(main_server->socket_params)))
        {
		if(DEBUG_PRINT){printf("Trying to bind socket\n");}
                sleep(2);
        }
        if(DEBUG_PRINT){printf("SUCCESSFULLY BINDED!\n");}
}
struct server *init_server()
{
        struct server *main_server = malloc(sizeof(struct server));
        
	init_server_socket(main_server);
        try_to_bind_socket(main_server);
        listen(main_server->server_socket, MAX_CONNECTIONS);
	
	if(DEBUG_PRINT){ printf("Listening on: %d:%d\n", INADDR_ANY, SERVER_PORT); }
        
	//main_server->main_table = init_table();
	return main_server;
}
void wait_for_clients(struct server *main_server)
{
	int conn_counter = 0;
	int conn_tryings = 0;
	int client_desc;
	int server_socket = main_server->server_socket;
	while(conn_counter < MIN_PLAYERS && conn_tryings < RECONNECT_COUNT){
        	if((client_desc = accept(server_socket, NULL, NULL)) != -1){
			if(DEBUG_PRINT){printf("ACCEPTED!\n");}
                        conn_counter ++;
                }
		if(DEBUG_PRINT){printf("WAITING FOR PLAYERS!\n");}
		conn_tryings ++;
		sleep(2);
	}
}
//------------------------------------

int main()
{
	struct server *main_server = init_server();
}
