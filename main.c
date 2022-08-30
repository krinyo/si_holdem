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
	struct card *first_card[DECK_SIZE];	
};

//------------------------------------

struct player{
	int position;

};
//------------------------------------

struct table{
	struct deck *deck_ptr;
	struct player *table_players[MAX_PLAYERS];
	struct card *table_cards[MAX_TABLE_CARDS];
	
};
//------------------------------------

struct server{
	struct table *main_table;
	int socket;

};
//------------------------------------

int main()
{

}
