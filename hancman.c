#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
typedef uint32_t LSet; // Letter Set
typedef struct Game Game;
Game humanHost();
Game botHost();
void playHuman();
void drawMan(int lives);
bool getL(LSet set, char l);
void toggL(LSet *set, char l);
bool setL(LSet *set, char l, bool val);
bool lSubset(LSet A, LSet B);
LSet strToLSet(char *str);
void printBlanks(char *word, LSet guessed);
void printLSet(LSet s);
struct Game
{
    char *word;
    int lives;
    LSet right, guessed;
    bool done;
};
void drawGame(Game g);
Game newGame(char *word);
bool guess(Game *g, char l);
char getGuessInput(LSet guessed);
Game humanHost()
{
    static char wordInput[16];
    printf("\nPlaying 2-Player!\nEXECUTIONER, input a word: \e[8m");
    scanf("%15s", wordInput);
    printf("\e[2K\e[28mEXECUTIONER entered a word!\n");
    return newGame(wordInput);
}
Game botHost()
{
    srand(clock());
    FILE *f = fopen("dictionary.txt", "r");
    unsigned long words = 1, wordIndex;
    static char word[16];
    while ((fscanf(f, "%*[^\n]"), fscanf(f, "%*c")) != EOF)
        ++words;
    wordIndex = (((long)rand() << 32) | rand()) % words;
    rewind(f);
    for (int i = 0; i < wordIndex; i++)
        fscanf(f, "%15s", word);
    // printf("Out of %ld words, #%ld was picked: %s", words, wordIndex, word);
    return newGame(word);
}
void playHuman(Game g)
{
    /*
    printf("(it was %s)", word);
    printLSet(strToLSet(word));
    printLSet((LSet)0b00000000000000000000000000010101);
    printBlanks(word, guessed);
    printLSet(guessed);
    toggL(&guessed, 'A');
    printBlanks(word, guessed);
    printLSet(guessed);
    toggL(&guessed, 'H');
    printBlanks(word, guessed);
    printLSet(guessed);
    toggL(&guessed, 'Z');
    printBlanks(word, guessed);
    printLSet(guessed);
    toggL(&guessed, 'B');
    printBlanks(word, guessed);
    printLSet(guessed);
    */
    while (!g.done)
    {
        drawGame(g);
        guess(&g, getGuessInput(g.guessed));
    }
    drawGame(g);
    printf("You %s! The word was: %s", g.lives > 0 ? "won" : "lost", g.word);
}
void drawMan(int lives)
{
    char *man[6] = {
        lives < 6 ? "O" : " ",
        lives < 5 ? "|" : " ",
        lives < 4 ? "/" : " ",
        lives < 3 ? "\\" : " ",
        lives < 2 ? "/" : " ",
        lives < 1 ? "\\" : " "};
    printf("\n");
    printf("┌━━┐\n");
    printf("│  ╽\n");
    printf("│  %s\n", man[0]);
    printf("│ %s%s%s\n", man[4], man[1], man[5]);
    printf("┷ %s %s\n", man[2], man[3]);
}
void drawGame(Game g)
{
    drawMan(g.lives);
    printBlanks(g.word, g.guessed);
    printLSet(g.guessed);
}
Game newGame(char *word)
{
    Game g = {word, 6, strToLSet(word), 0, false};
    return g;
}
bool guess(Game *g, char l)
{
    l = toupper(l);
    toggL(&(g->guessed), toupper(l));
    if (!getL(g->right, l))
    {
        if (--g->lives <= 0)
        {
            g->done = true;
        }
    }
    else if (lSubset(g->right, g->guessed))
        g->done = true;
    return getL(g->right, l);
}
char getGuessInput(LSet guessed)
{
    char guess;
    while (!guess)
    {
        printf("RESCUER, guess a letter: ");
        scanf(" %c", &(guess));
        // printf("%c", guess);
        if (!isalpha(guess))
        {
            printf("Not a letter! ");
            guess = 0;
        }
        else if (getL(guessed, guess))
        {
            printf("Already guessed! ");
            guess = 0;
        }
    }
}

bool getL(LSet set, char l)
{
    return (set >> (l - 65)) & 1;
}
void toggL(LSet *set, char l)
{
    *set ^= 1 << (l - 65);
}
bool setL(LSet *set, char l, bool val)
{
    if (val != 0 && val != 1)
        return false;
    if (getL(*set, l) != val)
        toggL(set, l);
    return true;
}
bool lSubset(LSet A, LSet B)
{
    return (A & B) == A;
}
LSet strToLSet(char *str)
{
    LSet s = 0;
    for (char *l = str; *l != '\0'; l++)
        setL(&s, toupper(*l), 1);
    return s;
}
void printBlanks(char *word, LSet guessed)
{
    printf("\n");
    for (int i = 0; i < strlen(word); i++)
    {
        printf("%c ", getL(guessed, word[i]) ? word[i] : '_');
    }
    printf("\n");
}
void printLSet(LSet s)
{
    LSet curr = s;
    char l;
    printf("\n");
    for (int i = 0; i < 26; i++)
    {
        l = i + 65;
        if (getL(s, l))
            printf("\e[2m");
        printf("%c\e[22m ", l);
    }
    printf("\n");
}
int main(int argc, char *argv[])
{
    playHuman(botHost());
    printf("\n");
    return 0;
}