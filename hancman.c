#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "regex.h"
typedef uint32_t LSet; // Letter Set
typedef struct Game Game;
Game humanHost();
Game botHost();
void playHuman(Game g);
void playBotProb(Game g, bool print);
void playBotInfo(Game g, bool print);
double binaryEntropy(double p)
{
    return -p * log2(p) - (1 - p) * log2(1 - p);
}
void writeOccurances(size_t *arr, LSet guessed, int wordLen, char *dict, regex_t *re);
int compPattern(regex_t *dest, Game g);
void drawMan(int lives);
bool getL(LSet set, char l);
void toggL(LSet *set, char l);
bool setL(LSet *set, char l, bool val);
bool lSubset(LSet A, LSet B);
LSet strToLSet(char *str);
void lSetToStr(char *dest, LSet l);
void printBlanks(char *word, LSet guessed);
void printLSet(LSet s);
struct Game
{
    char *word;
    int lives, wordLen, guesses;
    LSet right, guessed, wrongs;
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
    size_t words = 0, wordIndex;
    static char word[20];
    while (fgets(word, 20, f) != NULL)
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
    printf("You %s after %d guesses! The word was: %s", g.lives > 0 ? "won" : "lost", g.guesses, g.word);
}
void playBotProb(Game g, bool print)
{
    FILE *f = fopen("dictionary.txt", "r");
    size_t len = 0, maxN;
    char *dict, maxL;
    regex_t pat;
    getdelim(&dict, &len, '\0', f);
    fclose(f);
    compPattern(&pat, g);
    size_t occurs[26];
    while (!g.done)
    {
        if (print)
            drawGame(g);
        memset(occurs, 0, sizeof(occurs));
        writeOccurances(occurs, g.guessed, g.wordLen, dict, &pat);
        maxL = 'A';
        maxN = occurs[0];
        for (int i = 0; i < 26; i++)
        {
            // printf("%c: %lu | ", 'A' + i, occurs[i]);
            if (occurs[i] > maxN)
            {
                maxL = 'A' + i;
                maxN = occurs[i];
            }
        }
        // printf("Max: %c(%lu)\n", maxL, maxN);
        guess(&g, maxL);
        compPattern(&pat, g);
    }
    if (print)
    {
        drawGame(g);
        printf("You %s! The word was: %s", g.lives > 0 ? "won" : "lost", g.word);
    }
}
void playBotInfo(Game g, bool print)
{
    FILE *f = fopen("dictionary.txt", "r");
    size_t len = 0;
    char *dict, maxL;
    regex_t pat;
    getdelim(&dict, &len, '\0', f);
    fclose(f);
    compPattern(&pat, g);
    size_t occurs[26];
    double currEnt, maxEnt, occursT;
    while (!g.done)
    {
        if (print)
            drawGame(g);
        memset(occurs, 0, sizeof(occurs));
        writeOccurances(occurs, g.guessed, g.wordLen, dict, &pat);
        maxL = 'A';
        maxEnt = 0;
        occursT = 0;
        for (int i = 0; i < 26; i++)
            occursT += occurs[i];
        for (int i = 0; i < 26; i++)
        {
            if (occurs[i] == 0)
                continue;
            currEnt = binaryEntropy(occurs[i] / occursT);
            printf("%c: Occurance(s): %lu  Proability: %f  Entropy: %f\n", 'A' + i, occurs[i], occurs[i] / occursT, currEnt);
            if (currEnt > maxEnt || occurs[i] / occursT == 1)
            {
                maxL = 'A' + i;
                maxEnt = currEnt;
            }
        }
        printf("Max: %c(%f)\n", maxL, maxEnt);
        guess(&g, maxL);
        compPattern(&pat, g);
    }
    if (print)
    {
        drawGame(g);
        printf("You %s after %d guesses! The word was: %s", g.lives > 0 ? "won" : "lost", g.guesses, g.word);
    }
}
void writeOccurances(size_t *arr, LSet guessed, int wordLen, char *dict, regex_t *re)
{
    LSet currSet;
    regmatch_t match = {.rm_so = 0};
    char currWord[wordLen + 1];
    int regErr = 0;
    // printf("First 35(if that) matches:");
    for (/*size_t i=0*/; regErr == 0; /*i++*/)
    {
        if ((regErr = regexec(re, dict, 1, &match, 0)) != 0)
            return;
        strncpy(currWord, dict + match.rm_so, wordLen);
        currWord[wordLen] = '\0';
        currSet = strToLSet(currWord);
        // if(i<35) printf("\n%s\n", currWord);
        // printLSet(currSet);
        for (char i = 'A'; i <= 'Z'; i++)
        {
            arr[i - 'A'] += getL(currSet & (~guessed), i);
        }
        dict += match.rm_so + wordLen;
    }
}
int compPattern(regex_t *dest, Game g)
{
    char negLetters[26], neg[26 + 3];
    int negLen;
    if (g.wrongs == (LSet)0)
        strcpy(neg, ".");
    else
    {
        lSetToStr(negLetters, g.wrongs);
        sprintf(neg, "[^%s]", negLetters);
    }
    negLen = strlen(neg);
    char pat[g.wordLen * negLen + 1 * 2 + 1];
    strcpy(pat, "^");
    for (char *i = g.word; *i != '\0'; i++)
    {
        if (getL(g.guessed, *i))
        {
            strncat(pat, i, 1);
        }
        else
        {
            strncat(pat, neg, negLen);
        }
    };
    strcat(pat, "$\0");
    // printf("\n|%s|\n", pat);
    return regcomp(dest, pat, REG_NEWLINE | REG_ICASE | REG_EXTENDED);
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
    Game g = {.word = word, .lives = 6, .wordLen = strlen(word), .guesses = 0, .right = strToLSet(word), .guessed = (LSet)0, .wrongs = (LSet)0, .done = false};
    return g;
}
bool guess(Game *g, char l)
{
    l = toupper(l);
    if (getL(g->guessed, l))
        return getL(g->right, l);
    toggL(&(g->guessed), toupper(l));
    if (!getL(g->right, l))
    {
        setL(&(g->wrongs), l, 1);
        if (--g->lives <= 0)
        {
            g->done = true;
        }
    }
    else if (lSubset(g->right, g->guessed))
        g->done = true;
    g->guesses++;
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
    return guess;
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
void lSetToStr(char *dest, LSet l)
{
    for (char i = 'A'; i <= 'Z'; i++)
    {
        if (getL(l, i))
            *(dest++) = i;
    }
    *dest = '\0';
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
    // playHuman(botHost());
    playBotInfo(botHost(), true);
    printf("\n");
    return 0;
}